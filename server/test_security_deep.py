"""Second-pass adversarial regressions; only owned in-memory/loopback targets."""
import asyncio
import hmac
import secrets
import struct
import unittest
from unittest.mock import patch
import test_community_server as fixtures
from test_community_server import SCHEMAS, advert, score
from community_server import *
import security

class QuotaIntegrityTests(unittest.TestCase):
    setUp=fixtures.RelayTests.setUp
    send=fixtures.RelayTests.send
    def test_invalid_mac_cannot_spend_another_players_authenticated_budget(self):
        a=self.clients[0];a.packets=Bucket(0,2)
        data=ENVELOPE.pack(ONLINE_MAGIC,a.ticket,500,0)+packet(HEARTBEAT,1,a.peer,0)+b'X'*32
        for _ in range(2):self.service.datagram_received(data,a.endpoint)
        self.send(a,ADVERT,session=42,body=advert(a.peer))
        self.assertIn(a.peer,self.service.rooms)
    def test_replays_cannot_spend_authenticated_packet_budget(self):
        a=self.clients[0];a.packets=Bucket(0,3)
        data=self.send(a,HEARTBEAT)
        for _ in range(10):self.service.datagram_received(data,a.endpoint)
        self.send(a,ADVERT,session=42,body=advert(a.peer))
        self.assertIn(a.peer,self.service.rooms)
    def test_unknown_tickets_do_not_exhaust_authenticated_global_quota(self):
        a=self.clients[0];self.service.udp_packets=Bucket(0,2)
        data=ENVELOPE.pack(ONLINE_MAGIC,b'Z'*16,500,0)+packet(HEARTBEAT,1,a.peer,0)+b'X'*32
        for _ in range(2):self.service.datagram_received(data,a.endpoint)
        self.send(a,ADVERT,session=42,body=advert(a.peer))
        self.assertIn(a.peer,self.service.rooms)

class HttpQuotaIsolationTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp=fixtures.HttpTests.asyncSetUp
    asyncTearDown=fixtures.HttpTests.asyncTearDown
    request=fixtures.HttpTests.request
    def frame(self,path,credential,body):
        return b'POST '+path.encode()+b' HTTP/1.1\r\nHost: localhost\r\nAuthorization: Bearer '+credential.hex().encode()+b'\r\nContent-Length: '+str(len(body)).encode()+b'\r\n\r\n'+body
    async def test_failed_credential_churn_cannot_fill_profile_quota_table(self):
        credential=b'L'*32;self.service.connect(credential,'Legitimate')
        self.service.limits=security.Limits(16)
        # The previous implementation allocated a quota entry for every unknown
        # credential before validating this deliberately incompatible body.
        for _ in range(15):
            raw=self.frame('/v2/connect',secrets.token_bytes(32),struct.pack('>QH',0,4)+b'Test')
            await self.request(raw)
        answer=await self.request(self.frame('/v2/stats/read',credential,struct.pack('>IIIQI',1,1,1,0,0)))
        self.assertTrue(answer.startswith(b'HTTP/1.1 200'),answer[:100])
        self.assertEqual(self.service.store.player_count,1)
    async def test_invalid_connect_body_does_not_spend_registration_allowance(self):
        self.service.limits.take(('register','127.0.0.1'),0,1)
        self.service.limits.entries[('register','127.0.0.1')].tokens=1
        await self.request(self.frame('/v2/connect',b'X'*32,struct.pack('>QH',0,4)+b'Test'))
        answer=await self.request(self.frame('/v2/connect',b'Y'*32,struct.pack('>QH',COMPAT,4)+b'Test'))
        self.assertTrue(answer.startswith(b'HTTP/1.1 200'),answer[:100])
    async def test_known_profile_limits_are_isolated_from_network_table_capacity(self):
        credential=b'L'*32;self.service.connect(credential,'Legitimate')
        # Keep the source's HTTP quota available, exhaust every other network
        # entry, and ensure it cannot prevent an existing profile's stats read.
        self.service.limits=security.Limits(2)
        self.service.limits.take(('http','127.0.0.1'),5,300)
        self.service.limits.take(('blocked-ip','192.0.2.1'),0,1)
        answer=await self.request(self.frame('/v2/stats/read',credential,struct.pack('>IIIQI',1,1,1,0,0)))
        self.assertTrue(answer.startswith(b'HTTP/1.1 200'),answer[:100])
    async def test_twenty_players_on_one_router_can_reconnect_repeatedly(self):
        credentials=[bytes([n])*32 for n in range(20)]
        body=struct.pack('>Q',COMPAT)+text('SharedRouter')
        for credential in credentials:
            answer=await self.request(self.frame('/v2/connect',credential,body))
            self.assertTrue(answer.startswith(b'HTTP/1.1 200'),answer[:100])
        for _ in range(3):
            for credential in credentials:
                active=self.service.clients[PEER_BASE|self.service.store.owner(credential)]
                answer=await self.request(self.frame('/v2/disconnect',credential,active.ticket))
                self.assertTrue(answer.startswith(b'HTTP/1.1 200'),answer[:100])
                answer=await self.request(self.frame('/v2/connect',credential,body))
                self.assertTrue(answer.startswith(b'HTTP/1.1 200'),answer[:100])
        self.assertEqual(len(self.service.clients),20)

class NetworkGroupingTests(unittest.TestCase):
    def test_ipv6_aliases_and_mapped_ipv4_share_source_budget(self):
        self.assertEqual(security.network_key('2001:db8:abcd:1234::1'),security.network_key('2001:db8:abcd:1234:ffff::99'))
        self.assertNotEqual(security.network_key('2001:db8:abcd:1234::1'),security.network_key('2001:db8:abcd:1235::1'))
        self.assertEqual(security.network_key('::ffff:192.0.2.1'),security.network_key('192.0.2.1'))
    def test_only_fully_replenished_quotas_can_be_discarded_early(self):
        with patch('security.time.monotonic',return_value=100):
            limits=security.Limits(2);limits.take('a',1,1);limits.take('b',1,10)
            limits.entries['b'].tokens=0
        with patch('security.time.monotonic',return_value=102):
            limits.prune()
            self.assertNotIn('a',limits.entries);self.assertIn('b',limits.entries)
            self.assertTrue(limits.take('c',1,1))
        self.assertLessEqual(len(limits.entries),2)

class SharedRouterConnectionsTests(unittest.IsolatedAsyncioTestCase):
    async def test_twenty_simultaneous_players_share_one_public_address(self):
        service=Service(Store(':memory:',SCHEMAS),30821)
        server=ControlServer(service,'127.0.0.1',0)
        task=asyncio.create_task(server.serve_forever());connections=[]
        try:
            for _ in range(20):
                reader,writer=await asyncio.open_connection('127.0.0.1',server.socket.getsockname()[1])
                connections.append((reader,writer))
                writer.write(b'GET /health HTTP/1.1\r\nHost: localhost\r\n');await writer.drain()
            await asyncio.sleep(0.03)
            async def finish(reader,writer):
                writer.write(b'\r\n');await writer.drain()
                return await reader.read()
            answers=await asyncio.gather(*(finish(r,w) for r,w in connections),return_exceptions=True)
            self.assertEqual(sum(isinstance(a,bytes) and a.startswith(b'HTTP/1.1 200') for a in answers),20)
        finally:
            for _,writer in connections:
                writer.close()
                try:await writer.wait_closed()
                except ConnectionError:pass
            task.cancel()
            try:await task
            except asyncio.CancelledError:pass
            service.store.db.close()

if __name__=='__main__':unittest.main()
