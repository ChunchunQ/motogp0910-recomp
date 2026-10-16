import asyncio
import hashlib
import hmac
import json
from pathlib import Path
import secrets
import sqlite3
import struct
import tempfile
import time
import unittest
from collections import defaultdict
from community_server import *

SCHEMAS=load_schemas(Path(__file__).with_name('leaderboards.json'))

def score(value):return {'3':[2,value.to_bytes(8,'big',signed=True).hex()],'2':[1,(abs(value)//100).to_bytes(8,'big').hex()]}
def advert(peer,session=42,capacity=2):return struct.pack('>QQIIIIII',session,peer,0x21,capacity,1,0,0,0)+b'K'*16+text('Qatar')+b'\0\0\0\0'

class StoreTests(unittest.TestCase):
    def test_persistent_identity_best_signed_scores_and_paging(self):
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/'db.sqlite3';store=Store(path,SCHEMAS);token=secrets.token_bytes(32)
            a=store.connect(token,'A');b=store.connect(secrets.token_bytes(32),'B');c=store.connect(secrets.token_bytes(32),'C')
            store.write(a&0xFFFFFF,'a'*32,[(1,score(-12000000))]);store.write(b&0xFFFFFF,'b'*32,[(1,score(-10000000))]);store.write(c&0xFFFFFF,'c'*32,[(1,score(-11000000))])
            store.write(a&0xFFFFFF,'d'*32,[(1,score(-15000000))])
            total,rows=store.read(1,1,100);self.assertEqual(total,3);self.assertEqual([r[0] for r in rows],[b,c,a])
            self.assertEqual(int(rows[2][2]['3'][1],16),int.from_bytes((-12000000).to_bytes(8,'big',signed=True),'big'))
            store.write(a&0xFFFFFF,'a'*32,[(1,score(-8000000))]);self.assertEqual(store.read(1,1,1)[1][0][0],b)
            self.assertEqual(store.read(1,1,100,[a])[1][0][3],3)
            self.assertEqual(store.read(1,1,1,around=c)[1][0][0],c)
            self.assertEqual(store.read(1,1,1,around=PEER_BASE|999999),(3,[]))
            store.db.close();store=Store(path,SCHEMAS);self.assertEqual(store.connect(token,'Renamed'),a);self.assertEqual(store.read(1,3,1)[1][0][1],'Renamed');store.db.close()
    def test_invalid_batch_rolls_back_and_rejects_unknown_schema(self):
        store=Store(':memory:',SCHEMAS);peer=store.connect(b'A'*32,'A')&0xFFFFFF
        with self.assertRaises(Invalid):store.write(peer,'a'*32,[(1,score(10)),(99999,score(10))])
        self.assertEqual(store.read(1,1,10),(0,[]))
        with self.assertRaises(Invalid):store.write(peer,'b'*32,[(1,{'2':[1,'000000000000000a']})])
        self.assertEqual(store.db.execute('SELECT COUNT(*) FROM writes').fetchone()[0],0)
    def test_native_schema_uses_rating_and_best_for_all_378_views(self):
        self.assertEqual(set(map(int,SCHEMAS)),set(range(1,379)))
        for v in SCHEMAS.values():self.assertEqual((v['primary_column'],v['order'],v['aggregation']),(3,'desc','best'))

class Capture:
    def __init__(self):self.sent=[]
    def sendto(self,data,address):self.sent.append((data,address))

class RelayTests(unittest.TestCase):
    def setUp(self):
        self.service=Service(Store(':memory:',SCHEMAS),30821);self.transport=Capture();self.service.transport=self.transport
        self.credentials=[secrets.token_bytes(32) for _ in range(3)];self.clients=[]
        for n,key in enumerate(self.credentials):
            body=self.service.connect(key,f'Player{n}');peer,ticket,secret,port=struct.unpack('>Q16s32sH',body);self.clients.append(self.service.clients[peer])
        self.seq=defaultdict(int)
        self.addCleanup(self.service.store.db.close)
        for client in self.clients:
            self.send(client,HEARTBEAT)
            challenge=self.transport.sent[-1][0][36:-32]
            self.seq[client.peer]+=1
            data=ENVELOPE.pack(ONLINE_MAGIC,client.ticket,self.seq[client.peer],0)+challenge
            data+=hmac.digest(client.key,b'C'+data,'sha256')
            self.service.datagram_received(data,('127.0.0.1',31000+(client.peer&0xFFFFFF)))
        self.transport.sent.clear()
    def send(self,client,kind,target=0,request=1,session=0,body=b'',peer=None):
        self.seq[client.peer]+=1
        data=ENVELOPE.pack(ONLINE_MAGIC,client.ticket,self.seq[client.peer],target)+packet(kind,request,peer or client.peer,session,body)
        data+=hmac.digest(client.key,b'C'+data,'sha256')
        self.service.datagram_received(data,('127.0.0.1',31000+(client.peer&0xFFFFFF)));return data
    def test_integrity_replay_and_spoofing(self):
        a,b,c=self.clients
        self.send(a,ADVERT,session=42,body=advert(a.peer))
        data=self.send(b,DISCOVER);self.assertEqual(len(self.transport.sent),1)
        self.assertEqual(decode(self.transport.sent[0][0][36:-32])[0],DISCOVER)
        self.service.datagram_received(data,('127.0.0.1',33333));self.assertEqual(len(self.transport.sent),1)
        corrupt=bytearray(data);corrupt[-1]^=1;self.service.datagram_received(corrupt,('127.0.0.1',33333));self.assertEqual(len(self.transport.sent),1)
        self.send(c,DISCOVER,peer=b.peer);self.assertEqual(len(self.transport.sent),1)
        self.send(a,ADVERT,target=b.peer,session=42,body=advert(a.peer));self.assertEqual(len(self.transport.sent),2)
        envelope,address=self.transport.sent[-1];self.assertTrue(hmac.compare_digest(envelope[-32:],hmac.digest(b.key,b'S'+envelope[:-32],'sha256')))
        self.assertFalse(hmac.compare_digest(envelope[-32:],hmac.digest(b.key,b'C'+envelope[:-32],'sha256')))
        self.assertEqual(b.endpoint,('127.0.0.1',31000+(b.peer&0xFFFFFF)))
    def test_admission_cross_room_routing_and_close(self):
        a,b,c=self.clients
        self.send(a,ADVERT,session=42,body=advert(a.peer));self.send(b,HEARTBEAT)
        self.send(c,DATA,target=a.peer,session=42,body=struct.pack('>HHH',1000,1001,4)+b'abcd');self.assertEqual(len(self.transport.sent),0)
        self.send(b,JOIN,target=a.peer,session=42,body=b'K'*16);self.assertEqual(len(self.transport.sent),1)
        self.send(a,JOINED,target=b.peer,session=42,body=advert(a.peer)+b'\0');self.assertEqual(b.room,a.peer)
        self.send(b,DATA,target=a.peer,session=42,body=struct.pack('>HHH',1000,1001,4)+b'abcd');self.assertEqual(len(self.transport.sent),3)
        self.send(c,JOIN,target=a.peer,session=42,body=b'K'*16);self.assertEqual(decode(self.transport.sent[-1][0][36:-32])[0],REJECT)
        self.send(a,LEFT,session=42);self.assertFalse(self.service.rooms);self.assertEqual(b.room,0)
    def test_expiry_disconnect_and_ticket_ownership(self):
        a,b,c=self.clients
        self.send(a,ADVERT,session=42,body=advert(a.peer));self.service.rooms[a.peer].seen-=16;self.service.expire();self.assertFalse(self.service.rooms)
        self.service.handle('/v2/disconnect',self.credentials[0],b.ticket);self.assertIn(a.peer,self.service.clients)
        self.service.handle('/v2/disconnect',self.credentials[0],a.ticket);self.assertNotIn(a.peer,self.service.clients)
        again=self.service.connect(self.credentials[0],'NewName');self.assertEqual(struct.unpack_from('>Q',again)[0],a.peer)
    def test_leave_and_authoritative_roster_release_stale_members(self):
        a,b,c=self.clients
        self.send(a,ADVERT,session=42,body=advert(a.peer));self.send(b,JOIN,target=a.peer,session=42,body=b'K'*16)
        self.send(a,JOINED,target=b.peer,session=42,body=advert(a.peer)+b'\0')
        self.send(b,LEAVE,target=a.peer,session=42)
        self.assertEqual(b.room,0);self.assertNotIn(b.peer,self.service.rooms[a.peer].members)
        self.send(b,JOIN,target=a.peer,session=42,body=b'K'*16);self.send(a,JOINED,target=b.peer,session=42,body=advert(a.peer)+b'\0')
        roster=struct.pack('>IIIIIIBQIHB',0x21,2,1,0,0,0,1,a.peer,0x0A000000|(a.peer&0xFFFFFF),30810,0)
        self.send(a,ROSTER,session=42,body=roster)
        self.assertEqual(self.service.rooms[a.peer].members,{a.peer});self.assertEqual(b.room,0)
    def test_replay_window_boundaries_and_reordering(self):
        w=ReplayWindow()
        self.assertTrue(w.accept(129));self.assertTrue(w.accept(2));self.assertFalse(w.accept(1));self.assertFalse(w.accept(2));self.assertTrue(w.accept(128));self.assertTrue(w.accept(1000));self.assertFalse(w.accept(129))

class HttpTests(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        self.service=Service(Store(':memory:',SCHEMAS),30821);self.service.transport=Capture()
        self.server=await asyncio.start_server(self.service.http,'127.0.0.1',0,limit=8192)
        self.port=self.server.sockets[0].getsockname()[1]
        self.addCleanup(self.service.store.db.close)
    async def asyncTearDown(self):self.server.close();await self.server.wait_closed()
    async def request(self,data):
        r,w=await asyncio.open_connection('127.0.0.1',self.port);w.write(data);await w.drain();answer=await r.read();w.close();await w.wait_closed();return answer
    async def test_actual_http_handshake_and_validation(self):
        key=secrets.token_bytes(32);body=struct.pack('>Q',COMPAT)+text('Test')
        request=b'POST /v2/connect HTTP/1.1\r\nHost: localhost\r\nAuthorization: Bearer '+key.hex().encode()+b'\r\nContent-Length: '+str(len(body)).encode()+b'\r\n\r\n'+body
        answer=await self.request(request);self.assertTrue(answer.startswith(b'HTTP/1.1 200'));self.assertEqual(len(answer.split(b'\r\n\r\n',1)[1]),58)
        self.assertTrue((await self.request(b'GET /health HTTP/1.1\r\nHost: localhost\r\n\r\n')).startswith(b'HTTP/1.1 200'))
        self.assertTrue((await self.request(request.replace(b'Content-Length:',b'Transfer-Encoding: chunked\r\nContent-Length:'))).startswith(b'HTTP/1.1 400'))
        self.assertTrue((await self.request(request.replace(b'Host: localhost',b'Host: localhost\r\nHost: another'))).startswith(b'HTTP/1.1 400'))
        self.assertTrue((await self.request(request[:-len(body)]+bytes([body[0]^1])+body[1:])).startswith(b'HTTP/1.1 400'))

if __name__=='__main__':unittest.main()
