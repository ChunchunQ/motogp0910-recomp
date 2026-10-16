#!/usr/bin/env python3
"""Model-based protocol checks on owned in-memory services; QA dependency only."""
import argparse
from collections import Counter
from pathlib import Path
import sys
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--python-tools',type=Path,help='Optional local QA-only Hypothesis installation')
p.add_argument('--examples',type=int,default=300);p.add_argument('--steps',type=int,default=150)
a=p.parse_args()
if not 1<=a.examples<=2000 or not 1<=a.steps<=1000:p.error('invalid bounded test size')
if a.python_tools:sys.path.insert(0,str(a.python_tools.resolve()))
sys.path.insert(0,str(Path(__file__).resolve().parent.parent/'server'))
from hypothesis import given, settings, strategies as st, HealthCheck
from hypothesis.stateful import RuleBasedStateMachine, rule, invariant, run_state_machine_as_test
from community_server import *
from test_community_server import RelayTests, advert, SCHEMAS
from security import parse_headers

counts=Counter()
class ProtocolModel(RuleBasedStateMachine):
    def __init__(self):
        super().__init__();self.fixture=RelayTests();self.fixture.setUp()
        self.service=self.fixture.service;self.service.max_rooms=4
        self.history=[];self.clients=self.fixture.clients
        for n in range(3,12):
            response=self.service.connect(bytes([n])*32,f'Peer{n}')
            peer=struct.unpack_from('>Q',response)[0];client=self.service.clients[peer]
            self.clients.append(client);self.prove(client)
        counts['models']+=1
    def teardown(self):self.fixture.doCleanups()
    def address(self,client):return ('127.0.0.1',31000+(client.peer&0xffffff))
    def wire(self,client,raw,target=0):
        self.fixture.seq[client.peer]+=1
        body=ENVELOPE.pack(ONLINE_MAGIC,client.ticket,self.fixture.seq[client.peer],target)+raw
        data=body+hmac.digest(client.key,b'C'+body,'sha256')
        self.service.datagram_received(data,self.address(client));return data
    def prove(self,client):
        self.fixture.send(client,HEARTBEAT)
        nonce=self.fixture.transport.sent[-1][0][36:-32]
        self.wire(client,nonce)
    def send(self,client,kind,target=0,session=0,request=1,body=b''):
        data=self.wire(client,packet(kind,request,client.peer,session,body),target)
        self.history.append((data,self.address(client)));self.history=self.history[-32:]
        counts['actions']+=1
    @rule(index=st.integers(0,11),session=st.integers(1,6),capacity=st.integers(1,5))
    def host(self,index,session,capacity):
        client=self.clients[index];self.send(client,ADVERT,session=session,body=advert(client.peer,session,capacity))
    @rule(index=st.integers(0,11),host=st.integers(0,11),request=st.integers(1,4))
    def join(self,index,host,request):
        client,target=self.clients[index],self.clients[host];room=self.service.rooms.get(target.peer)
        self.send(client,JOIN,target=target.peer,session=room.session if room else 1,request=request,body=b'K'*16)
    @rule(host=st.integers(0,11),guest=st.integers(0,11),request=st.integers(1,4),bad_key=st.booleans())
    def ack(self,host,guest,request,bad_key):
        client,target=self.clients[host],self.clients[guest];room=self.service.rooms.get(client.peer)
        body=bytearray(room.body if room else advert(client.peer))
        if bad_key:body[40]^=1
        self.send(client,JOINED,target=target.peer,session=room.session if room else 42,request=request,body=bytes(body)+b'\0')
    @rule(index=st.integers(0,11))
    def leave(self,index):
        client=self.clients[index];room=self.service.rooms.get(client.room)
        self.send(client,LEAVE,target=room.host if room else self.clients[0].peer,session=room.session if room else 1)
    @rule(index=st.integers(0,11))
    def close(self,index):
        client=self.clients[index];room=self.service.rooms.get(client.peer)
        self.send(client,LEFT,session=room.session if room else 1)
    @rule(host=st.integers(0,11),mask=st.integers(0,4095),poison=st.booleans())
    def roster(self,host,mask,poison):
        client=self.clients[host];room=self.service.rooms.get(client.peer)
        present=[p for i,p in enumerate(sorted(room.members)) if p==client.peer or mask&(1<<i)] if room else [client.peer]
        raw=struct.pack('>IIIIIIB',0x21,max(1,len(present)),len(present),0,0,0,len(present))
        for peer in present:raw+=struct.pack('>QIHB',peer,0x7f000001 if poison else 0x0a000000|(peer&0xffffff),30810,0)
        self.send(client,ROSTER,session=room.session if room else 1,body=raw)
    @rule(index=st.integers(0,11),other=st.integers(0,11))
    def data(self,index,other):
        client,destination=self.clients[index],self.clients[other];room=self.service.rooms.get(client.room)
        self.send(client,DATA,target=destination.peer,session=room.session if room else 1,body=struct.pack('>HHH',1000,1001,4)+b'data')
    @rule(index=st.integers(0,11))
    def discover(self,index):self.send(self.clients[index],DISCOVER)
    @rule(index=st.integers(0,11),kind=st.integers(1,10),raw=st.binary(max_size=128))
    def malformed(self,index,kind,raw):self.send(self.clients[index],kind,body=raw)
    @rule(index=st.integers(0,31))
    def replay(self,index):
        if self.history:
            raw,address=self.history[index%len(self.history)]
            self.service.datagram_received(raw,address);counts['replays']+=1
    @rule(index=st.integers(0,11))
    def reconnect(self,index):
        old=self.clients[index];credential=self.fixture.credentials[index] if index<3 else bytes([index])*32
        self.service.remove(old.peer);response=self.service.connect(credential,f'Peer{index}')
        client=self.service.clients[struct.unpack_from('>Q',response)[0]];self.clients[index]=client;self.prove(client)
        counts['reconnections']+=1
    @rule(index=st.integers(0,11))
    def expire_room(self,index):
        room=self.service.rooms.get(self.clients[index].peer)
        if room:room.seen-=16
        self.service.expire()
    @invariant()
    def isolation(self):
        service=self.service
        assert len(service.rooms)<=service.max_rooms
        assert len(service.clients)==len(service.tickets)==12
        assigned={}
        for host,room in service.rooms.items():
            assert host==room.host and host in room.members and host in service.clients
            assert 1<=len(room.members)<=20 and len(room.pending)<=40
            assert service.clients[host].room==host
            for member in room.members:
                assert member not in assigned and member in service.clients
                assigned[member]=host;assert service.clients[member].room==host
            for member,pending in room.pending.items():
                assert member in service.clients and service.clients[member].ticket==pending[2]
                assert service.clients[member].room in (0,host)
        for peer,client in service.clients.items():
            assert service.tickets[client.ticket] is client
            assert client.room==assigned.get(peer,0)
        counts['invariant_checks']+=1

options=settings(max_examples=a.examples,stateful_step_count=a.steps,deadline=None,derandomize=True,database=None,suppress_health_check=[HealthCheck.too_slow])
run_state_machine_as_test(ProtocolModel,settings=options)

@settings(max_examples=a.examples*4,deadline=None,derandomize=True,database=None)
@given(path=st.sampled_from(['/v2/connect','/v2/disconnect','/v2/stats/read','/v2/stats/write','/v1/connect','/../secret']),name=st.sampled_from(['Content-Length','Content-Length ','Transfer-Encoding','Host','X-Test','Authorization','\tFolded']),value=st.one_of(st.integers(-1,100000).map(str),st.text(max_size=50)),suffix=st.binary(max_size=25))
def framing(path,name,value,suffix):
    raw=f'POST {path} HTTP/1.1\r\nHost: localhost\r\n{name}: {value}\r\n\r\n'.encode('utf-8')+suffix
    try:
        method,route,headers,size=parse_headers(raw)
        assert method=='POST' and route.startswith('/v2/')
        assert headers['content-length'].isascii() and headers['content-length'].isdecimal()
        assert 'transfer-encoding' not in headers and size<=1750
    except (Invalid,UnicodeError):pass
    counts['structured_headers']+=1
framing()
print('Bounded in-memory property checks passed: '+', '.join(f'{k}={v}' for k,v in sorted(counts.items())))
