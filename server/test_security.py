"""Adversarial regression tests against owned in-memory/loopback services only."""
import asyncio
import hashlib
import hmac
import random
import secrets
import sqlite3
import struct
import time
import unittest
from security import Bucket, Limits, Invalid, Limited, parse_headers
import test_community_server as fixtures
from test_community_server import SCHEMAS, score, advert
from community_server import *

class RelaySecurityTests(unittest.TestCase):
    setUp=fixtures.RelayTests.setUp
    send=fixtures.RelayTests.send
    def envelope(self, client, raw, address, target=0):
        self.seq[client.peer]+=1
        data=ENVELOPE.pack(ONLINE_MAGIC,client.ticket,self.seq[client.peer],target)+raw
        data+=hmac.digest(client.key,b'C'+data,'sha256')
        self.service.datagram_received(data,address)
        return data
    def join(self, host, guest, session=42):
        self.send(guest,JOIN,target=host.peer,session=session,body=b'K'*16)
        self.send(host,JOINED,target=guest.peer,session=session,body=advert(host.peer,session)+b'\0')
    def test_unproved_address_never_binds_or_receives_amplified_reply(self):
        a,b,_=self.clients
        self.send(a,ADVERT,session=42,body=advert(a.peer))
        old=b.endpoint;victim=('192.0.2.17',1234)
        before=len(self.transport.sent)
        data=self.envelope(b,packet(DISCOVER,50,b.peer,0),victim)
        self.assertEqual(b.endpoint,old)
        replies=self.transport.sent[before:]
        self.assertEqual(len(replies),1)
        self.assertEqual(replies[0][1],victim)
        self.assertLessEqual(len(replies[0][0]),len(data))
        self.assertEqual(len(replies[0][0][36:-32]),16)
        self.assertEqual(b.search_request,0)
    def test_endpoint_rebind_requires_current_nonce_at_exact_address(self):
        a,_,_=self.clients;old=a.endpoint;new=('127.0.0.1',44444)
        self.envelope(a,packet(HEARTBEAT,0,a.peer,0),new)
        challenge=self.transport.sent[-1][0][36:-32]
        self.envelope(a,b'X'*16,new);self.assertEqual(a.endpoint,old)
        self.envelope(a,challenge,('127.0.0.1',44445));self.assertEqual(a.endpoint,old)
        self.envelope(a,challenge,new);self.assertEqual(a.endpoint,new)
        self.assertIsNone(a.candidate)
        self.envelope(a,challenge,old);self.assertEqual(a.endpoint,new)
    def test_expired_nonce_and_invalid_mac_cannot_bind(self):
        a,_,_=self.clients;old=a.endpoint;new=('127.0.0.1',44444)
        self.envelope(a,packet(HEARTBEAT,0,a.peer,0),new)
        challenge=self.transport.sent[-1][0][36:-32];a.challenge_at-=6
        self.envelope(a,challenge,new);self.assertEqual(a.endpoint,old)
        old_top=a.replay.top
        bad=ENVELOPE.pack(ONLINE_MAGIC,a.ticket,2**63,0)+b'X'*16+b'0'*32
        self.service.datagram_received(bad,new)
        self.assertEqual(a.replay.top,old_top);self.assertEqual(a.endpoint,old)
    def test_new_client_does_not_enter_directory_until_address_proof(self):
        a,_,_=self.clients;a.endpoint=None
        self.send(a,ADVERT,session=42,body=advert(a.peer))
        self.assertNotIn(a.peer,self.service.rooms)
        challenge=self.transport.sent[-1][0][36:-32]
        self.envelope(a,challenge,('127.0.0.1',31000+(a.peer&0xFFFFFF)))
        self.assertIn(a.peer,self.service.rooms)
    def test_delayed_join_ack_cannot_steal_another_room(self):
        a,b,c=self.clients
        self.send(a,ADVERT,session=42,body=advert(a.peer))
        self.send(c,ADVERT,session=43,body=advert(c.peer,43))
        self.send(b,JOIN,target=a.peer,session=42,body=b'K'*16)
        self.join(c,b,43)
        self.send(a,JOINED,target=b.peer,session=42,body=advert(a.peer)+b'\0')
        self.assertEqual(b.room,c.peer);self.assertNotIn(b.peer,self.service.rooms[a.peer].members)
        self.service.close_room(a.peer);self.assertEqual(b.room,c.peer)
    def test_capacity_is_rechecked_when_join_ack_arrives(self):
        a,b,c=self.clients;self.send(a,ADVERT,session=42,body=advert(a.peer))
        for guest in (b,c):self.send(guest,JOIN,target=a.peer,session=42,body=b'K'*16)
        for guest in (b,c):self.send(a,JOINED,target=guest.peer,session=42,body=advert(a.peer)+b'\0')
        self.assertEqual(len(self.service.rooms[a.peer].members),2);self.assertEqual(c.room,0)
    def test_expired_ack_wrong_key_and_reconnected_identity_are_rejected(self):
        a,b,_=self.clients;self.send(a,ADVERT,session=42,body=advert(a.peer))
        self.send(b,JOIN,target=a.peer,session=42,body=b'K'*16)
        room=self.service.rooms[a.peer];pending=room.pending[b.peer]
        room.pending[b.peer]=(pending[0],pending[1]-11,*pending[2:])
        self.send(a,JOINED,target=b.peer,session=42,body=advert(a.peer)+b'\0');self.assertEqual(b.room,0)
        self.send(b,JOIN,target=a.peer,session=42,body=b'K'*16)
        wrong=bytearray(advert(a.peer));wrong[40]^=1
        self.send(a,JOINED,target=b.peer,session=42,body=bytes(wrong)+b'\0');self.assertEqual(b.room,0)
        self.service.remove(b.peer);self.service.connect(self.credentials[1],'Again')
        self.assertNotIn(b.peer,room.pending)
    def test_poisoned_rosters_and_unrequested_adverts_are_not_forwarded(self):
        a,b,c=self.clients;self.send(a,ADVERT,session=42,body=advert(a.peer));self.join(a,b)
        before=len(self.transport.sent)
        roster=struct.pack('>IIIIIIB',0x21,2,2,0,0,0,2)
        for peer,ip in ((a.peer,0x0A000000|(a.peer&0xFFFFFF)),(b.peer,0x7f000001)):
            roster+=struct.pack('>QIHB',peer,ip,30810,0)
        self.send(a,ROSTER,target=b.peer,session=42,body=roster)
        self.send(a,ADVERT,target=c.peer,session=42,body=advert(a.peer))
        self.assertEqual(len(self.transport.sent),before)
    def test_search_reply_flood_is_deduplicated(self):
        a,b,_=self.clients;self.send(a,ADVERT,session=42,body=advert(a.peer));self.send(b,DISCOVER)
        before=len(self.transport.sent)
        for _ in range(100):self.send(a,ADVERT,target=b.peer,session=42,body=advert(a.peer))
        self.assertEqual(len(self.transport.sent),before+1)
    def test_duplicate_live_login_does_not_rename_profile(self):
        a,_,_=self.clients
        with self.assertRaises(Invalid):self.service.connect(self.credentials[0],'AttackerName')
        self.assertEqual(self.service.store.db.execute('SELECT name FROM players WHERE id=?',(a.peer&0xFFFFFF,)).fetchone()[0],'Player0')
    def test_malformed_udp_fuzz_never_routes_or_raises(self):
        rng=random.Random(1910);a,_,_=self.clients
        for n in range(2000):
            raw=rng.randbytes(rng.randrange(0,1600))
            self.service.datagram_received(raw,('127.0.0.1',43210))
            if n<900:self.envelope(a,raw[:1400],a.endpoint)
        self.assertFalse(self.service.rooms);self.assertFalse(self.transport.sent)
    def test_packet_and_output_quotas_bound_bursts(self):
        a,_,_=self.clients
        a.packets.tokens=0;a.packets.rate=0
        self.send(a,ADVERT,session=42,body=advert(a.peer));self.assertFalse(self.service.rooms)
        a.outgoing.tokens=0;a.outgoing.rate=0
        for _ in range(100):self.service.forward(a,packet(HEARTBEAT,0,a.peer,0))
        self.assertFalse(self.transport.sent)
    def test_old_online_magic_is_rejected_without_changing_lan_compatibility(self):
        a,_,_=self.clients
        data=ENVELOPE.pack(b'MGO1',a.ticket,500,0)+packet(HEARTBEAT,0,a.peer,0)
        data+=hmac.digest(a.key,b'C'+data,'sha256');top=a.replay.top
        self.service.datagram_received(data,a.endpoint);self.assertEqual(a.replay.top,top)
        self.assertEqual(COMPAT,0x4D4750394C414E02)

class StorageSecurityTests(unittest.TestCase):
    def setUp(self):
        self.store=Store(':memory:',SCHEMAS,max_players=3)
        self.addCleanup(self.store.db.close)
        self.peer=self.store.connect(b'A'*32,'A')&0xFFFFFF
    def test_account_limit_is_atomic_and_existing_profiles_still_connect(self):
        self.store.connect(b'B'*32,'B');self.store.connect(b'C'*32,'C')
        with self.assertRaises(Limited):self.store.connect(b'D'*32,'D')
        self.assertEqual(self.store.player_count,3)
        self.assertEqual(self.store.db.execute('SELECT COUNT(*) FROM players').fetchone()[0],3)
        self.assertEqual(self.store.connect(b'A'*32,'Renamed'),PEER_BASE|self.peer)
    def test_no_op_nonce_history_is_bounded_and_latest_replay_idempotent(self):
        for i in range(1100):self.store.write(self.peer,f'{i:032x}',[(1,score(10))])
        self.assertEqual(self.store.db.execute('SELECT COUNT(*) FROM writes').fetchone()[0],256)
        self.store.write(self.peer,f'{1099:032x}',[(1,score(11))])
        self.assertEqual(self.store.read(1,1,1)[1][0][2]['3'],score(10)['3'])
    def test_invalid_column_types_ids_and_int32_bits_are_atomic(self):
        for id,kind,bits in ((65535,1,'0'*16),(2,5,'ffffffffffffffff'),(2,1,'0000000100000000'),(3,1,'0'*16)):
            bad=score(10);bad[str(id)]=[kind,bits]
            with self.subTest(id=id,kind=kind),self.assertRaises(Invalid):self.store.write(self.peer,'a'*32,[(1,bad)])
        self.assertEqual(self.store.read(1,1,10),(0,[]))
    def test_duplicate_views_out_of_range_rating_and_forged_xuid_rejected(self):
        for views in ([(1,score(1)),(1,score(2))],[(1,score(2**60))]):
            with self.assertRaises(Invalid):self.store.write(self.peer,'a'*32,views)
        with self.assertRaises(Invalid):self.store.read(1,1,1,[0xDEAD000000000001])
    def test_storage_quota_and_query_deadline_are_enforced(self):
        self.assertEqual(self.store.db.execute('PRAGMA max_page_count').fetchone()[0]*self.store.db.execute('PRAGMA page_size').fetchone()[0],256*1024*1024)
        self.store.deadline=time.monotonic()-1
        with self.assertRaises(sqlite3.OperationalError):
            self.store.db.execute('WITH RECURSIVE x(n) AS (VALUES(0) UNION ALL SELECT n+1 FROM x WHERE n<100000) SELECT SUM(n) FROM x').fetchone()
        self.store.deadline=0
        self.assertEqual(self.store.owner(b'A'*32),self.peer)
    def test_existing_database_cannot_silently_exceed_configured_quota(self):
        import tempfile
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'large.sqlite3'
            existing=Store(path,SCHEMAS)
            try:
                with existing.db:
                    existing.db.execute('CREATE TABLE filler (data BLOB)')
                    existing.db.execute('INSERT INTO filler VALUES(zeroblob(?))',(17*1024*1024,))
            finally:existing.db.close()
            with self.assertRaises(Invalid):Store(path,SCHEMAS,max_db_mib=16)
            restored=Store(path,SCHEMAS,max_db_mib=32)
            try:self.assertEqual(restored.db.execute('SELECT length(data) FROM filler').fetchone()[0],17*1024*1024)
            finally:restored.db.close()
    def test_actual_sqlite_full_rolls_back_the_entire_score_batch(self):
        pages=self.store.db.execute('PRAGMA page_count').fetchone()[0]
        self.store.db.execute(f'PRAGMA max_page_count={pages}')
        full={str(k):[v,'0000000000000001'] for k,v in NATIVE_COLUMNS.items()}
        with self.assertRaises(sqlite3.DatabaseError):
            self.store.write(self.peer,'a'*32,[(i,dict(full)) for i in range(1,17)])
        self.assertEqual(self.store.db.execute('SELECT COUNT(*) FROM scores').fetchone()[0],0)
        self.assertEqual(self.store.db.execute('SELECT COUNT(*) FROM writes').fetchone()[0],0)
        self.assertEqual(self.store.owner(b'A'*32),self.peer)
    def test_names_are_data_not_sql_or_log_control_sequences(self):
        self.store.connect(b'B'*32,"' OR '1'='1")
        self.assertEqual(self.store.player_count,2)
        for name in ('A\nB','A\x1bB','A\0B'):
            with self.assertRaises(Invalid):self.store.connect(b'C'*32,name)
    def test_native_nine_column_response_is_below_client_capacity(self):
        full={str(k):[v,('0000000000000001')] for k,v in NATIVE_COLUMNS.items()}
        self.store.write(self.peer,'a'*32,[(1,full)])
        self.assertEqual(len(self.store.read(1,1,1)[1][0][2]),9)

class HttpSecurityTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp=fixtures.HttpTests.asyncSetUp
    asyncTearDown=fixtures.HttpTests.asyncTearDown
    request=fixtures.HttpTests.request
    def connect_request(self,key=None,extra=b''):
        key=key or secrets.token_bytes(32);body=struct.pack('>Q',COMPAT)+text('Player')
        return b'POST /v2/connect HTTP/1.1\r\nHost: localhost\r\nAuthorization: Bearer '+key.hex().encode()+b'\r\n'+extra+b'Content-Length: '+str(len(body)).encode()+b'\r\n\r\n'+body
    async def test_proxy_key_required_and_client_addresses_cannot_be_forged(self):
        self.service.proxy_secret='a'*64
        bad=await self.request(self.connect_request(extra=b'X-MotoGP-Client-IP: 192.0.2.1\r\n'))
        self.assertTrue(bad.startswith(b'HTTP/1.1 400'));self.assertEqual(self.service.store.player_count,0)
        good=b'X-MotoGP-Proxy-Key: '+b'a'*64+b'\r\nX-MotoGP-Client-IP: 192.0.2.1\r\n'
        self.assertTrue((await self.request(self.connect_request(extra=good))).startswith(b'HTTP/1.1 200'))
        self.assertIn(('http','192.0.2.1'),self.service.limits.entries)
        self.assertNotIn(('http','127.0.0.1'),self.service.limits.entries)
    async def test_direct_client_forwarding_headers_do_not_change_quotas(self):
        answer=await self.request(self.connect_request(extra=b'X-Forwarded-For: 192.0.2.1\r\nX-MotoGP-Client-IP: 192.0.2.2\r\n'))
        self.assertTrue(answer.startswith(b'HTTP/1.1 200'))
        self.assertIn(('http','127.0.0.1'),self.service.limits.entries)
        self.assertNotIn(('http','192.0.2.2'),self.service.limits.entries)
    async def test_proxy_players_have_independent_rate_budgets(self):
        self.service.proxy_secret='a'*64
        self.service.limits.take(('http','192.0.2.1'),0,1)
        self.service.limits.entries[('http','192.0.2.1')].rate=0
        def headers(ip):return b'X-MotoGP-Proxy-Key: '+b'a'*64+b'\r\nX-MotoGP-Client-IP: '+ip+b'\r\n'
        self.assertTrue((await self.request(self.connect_request(extra=headers(b'192.0.2.1')))).startswith(b'HTTP/1.1 429'))
        self.assertTrue((await self.request(self.connect_request(extra=headers(b'192.0.2.2')))).startswith(b'HTTP/1.1 200'))
    async def test_smuggling_variants_and_traversal_do_not_reach_database(self):
        request=self.connect_request()
        variants=[request.replace(b'Content-Length:',v) for v in (b'Content-Length :',b'content-length: 0\r\nContent-Length:',b'Transfer-Encoding : chunked\r\nContent-Length:',b'Transfer-Encoding: chunked\r\nContent-Length:',b'Expect: 100-continue\r\nContent-Length:')]
        variants.extend((request.replace(b'Host: localhost\r\n',b''),request.replace(b'Host: localhost',b'Host: local\x00host'),request.replace(b' /v2/connect ',b' /../../etc/passwd '),request.replace(b' /v2/connect ',b' /v1/connect ')))
        for raw in variants:
            with self.subTest(raw=raw[:30]):self.assertTrue((await self.request(raw)).startswith(b'HTTP/1.1 400'))
        self.assertEqual(self.service.store.player_count,0)
    async def test_oversized_payload_is_rejected_before_body_read(self):
        header=self.connect_request().split(b'Content-Length:',1)[0]+b'Content-Length: 65536\r\n\r\n'
        self.assertTrue((await asyncio.wait_for(self.request(header),1)).startswith(b'HTTP/1.1 400'))
    async def test_slow_header_timeout_and_connection_slots_are_released(self):
        self.service.header_timeout=0.05;self.service.max_http_per_ip=2
        r1,w1=await asyncio.open_connection('127.0.0.1',self.port)
        r2,w2=await asyncio.open_connection('127.0.0.1',self.port)
        await asyncio.sleep(0.01)
        r3,w3=await asyncio.open_connection('127.0.0.1',self.port)
        answer=await r3.read();w3.close();await w3.wait_closed()
        self.assertTrue(answer.startswith(b'HTTP/1.1 429'))
        await asyncio.wait_for(r1.read(),1);await asyncio.wait_for(r2.read(),1)
        for w in (w1,w2):w.close();await w.wait_closed()
        await asyncio.sleep(0.01)
        self.assertEqual(self.service.http_connections,0);self.assertFalse(self.service.http_ips)
        self.assertTrue((await self.request(b'GET /health HTTP/1.1\r\nHost: localhost\r\n\r\n')).startswith(b'HTTP/1.1 200'))
    async def test_registration_rate_and_unknown_profile_does_not_allocate_owner_buckets(self):
        self.service.limits.take(('register','127.0.0.1'),0,1)
        self.service.limits.entries[('register','127.0.0.1')].rate=0
        self.assertTrue((await self.request(self.connect_request())).startswith(b'HTTP/1.1 429'))
        raw=self.connect_request().replace(b'/v2/connect',b'/v2/stats/write')
        self.assertTrue((await self.request(raw)).startswith(b'HTTP/1.1 400'))
        self.assertFalse(any(k[0]=='/v2/stats/write' for k in self.service.profile_limits.entries))
    async def test_database_failure_is_generic_and_service_recovers(self):
        self.service.store.db.execute('PRAGMA query_only=ON')
        reply=await self.request(self.connect_request())
        self.assertTrue(reply.startswith(b'HTTP/1.1 503'))
        self.assertNotIn(b'SELECT',reply);self.assertNotIn(b'Traceback',reply)
        self.service.store.db.execute('PRAGMA query_only=OFF')
        self.assertTrue((await self.request(self.connect_request())).startswith(b'HTTP/1.1 200'))

class LimitSecurityTests(unittest.TestCase):
    def test_rate_table_has_hard_capacity_and_does_not_evict_live_victims(self):
        from unittest.mock import patch
        with patch('security.time.monotonic',return_value=100) as clock:
            limits=Limits(4)
            for n in range(4):self.assertTrue(limits.take(n,1,1))
            for n in range(4,1000):self.assertFalse(limits.take(n,1,1))
            self.assertFalse(limits.take(0,1,1));self.assertEqual(len(limits.entries),4)
            clock.return_value=3702
            self.assertTrue(limits.take(9999,1,1));self.assertEqual(len(limits.entries),1)
    def test_token_bucket_hard_burst(self):
        b=Bucket(0,3)
        self.assertTrue(b.take(3));self.assertFalse(b.take());self.assertFalse(b.take(1000))
    def test_http_parser_fuzz_is_bounded(self):
        rng=random.Random(20261016)
        for _ in range(2000):
            with self.assertRaises((Invalid,UnicodeError,ValueError)):
                parse_headers(rng.randbytes(rng.randrange(0,1000))+b'\r\n\r\n')

class ControlAdmissionTests(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        self.service=Service(Store(':memory:',SCHEMAS),30821)
        self.service.max_http_connections=3;self.service.max_http_per_ip=2
        self.addCleanup(self.service.store.db.close)
    async def stop(self, task):
        task.cancel()
        try:await task
        except asyncio.CancelledError:pass
    async def test_plaintext_acceptance_and_shutdown_release_all_sockets(self):
        server=ControlServer(self.service,'127.0.0.1',0)
        task=asyncio.create_task(server.serve_forever())
        try:
            r,w=await asyncio.open_connection('127.0.0.1',server.socket.getsockname()[1])
            w.write(b'GET /health HTTP/1.1\r\nHost: localhost\r\n\r\n');await w.drain()
            self.assertTrue((await r.read()).startswith(b'HTTP/1.1 200'))
            w.close();await w.wait_closed()
        finally:await self.stop(task)
        self.assertFalse(server.addresses);self.assertFalse(server.tasks)
    async def test_pending_tls_is_limited_before_application_callback(self):
        context=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        server=ControlServer(self.service,'127.0.0.1',0,context)
        task=asyncio.create_task(server.serve_forever());writers=[]
        try:
            for _ in range(12):
                _,w=await asyncio.open_connection('127.0.0.1',server.socket.getsockname()[1]);writers.append(w)
            await asyncio.sleep(0.02)
            self.assertLessEqual(len(server.tasks),2)
            self.assertEqual(self.service.http_connections,0)
            self.assertLessEqual(sum(server.addresses.values()),2)
        finally:
            await self.stop(task)
            for w in writers:
                w.close()
                try:await w.wait_closed()
                except ConnectionError:pass
        self.assertFalse(server.addresses);self.assertFalse(server.tasks)

class DeploymentSecurityTests(unittest.TestCase):
    def test_proxy_key_created_exclusively_and_not_printed(self):
        import tempfile,os
        from proxy_config import create_key
        from security import read_proxy_secret
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'private'/'proxy.key'
            create_key(path);first=read_proxy_secret(path)
            self.assertEqual(len(first),64)
            with self.assertRaises(FileExistsError):create_key(path)
            self.assertEqual(read_proxy_secret(path),first)
            if os.name!='nt':self.assertEqual(path.stat().st_mode & 0o777,0o600)
            path.write_text('A'*100000)
            with self.assertRaises(Invalid):read_proxy_secret(path)
    def test_100_native_rows_stay_below_response_limit(self):
        store=Store(':memory:',SCHEMAS,max_players=100)
        self.addCleanup(store.db.close)
        credential=None
        for i in range(100):
            credential=i.to_bytes(32,'big');peer=store.connect(credential,f'Player{i}')
            columns={str(k):[v,'0000000000000001'] for k,v in NATIVE_COLUMNS.items()}
            store.write(peer&0xffffff,'a'*32,[(1,columns)])
        service=Service(store,30821)
        response=service.handle('/v2/stats/read',credential,struct.pack('>IIIQI',1,1,100,0,0))
        self.assertEqual(struct.unpack_from('>II',response),(100,100));self.assertLess(len(response),65536)

if __name__=='__main__':unittest.main()
