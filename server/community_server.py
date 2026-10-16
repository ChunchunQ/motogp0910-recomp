#!/usr/bin/env python3
"""MotoGP 09/10 community directory, authenticated UDP relay and SQLite stats.

Python 3.11+ standard library only. Identical service on Windows and Linux.
The original game remains the simulation host; this is not an Xbox Live service.
"""
from __future__ import annotations
import argparse
import asyncio
from dataclasses import dataclass, field
import hashlib
import hmac
import json
import logging
from pathlib import Path
import secrets
import sqlite3
import ssl
import struct
import time

LOG = logging.getLogger('motogp.online')
TITLE, GAME, COMPAT = 0x434307F0, 0x101, 0x4D4750394C414E02
PEER_BASE = 0x0009000000000000
WIRE = struct.Struct('>IHHIIQ IQQI')
ENVELOPE = struct.Struct('>4s16sQQ')
DISCOVER, ADVERT, JOIN, JOINED, LEAVE, LEFT, HEARTBEAT, REJECT, DATA, ROSTER = range(1, 11)
MAX_BODY = 65536

class Invalid(ValueError): pass

class Reader:
    def __init__(self, data): self.data, self.pos = data, 0
    def raw(self, size):
        if size < 0 or self.pos + size > len(self.data): raise Invalid('truncated request')
        value = self.data[self.pos:self.pos+size]; self.pos += size; return value
    def u8(self): return self.raw(1)[0]
    def u16(self): return int.from_bytes(self.raw(2), 'big')
    def u32(self): return int.from_bytes(self.raw(4), 'big')
    def u64(self): return int.from_bytes(self.raw(8), 'big')
    def blob(self, maximum):
        size = self.u16()
        if size > maximum: raise Invalid('field too large')
        return self.raw(size)
    def text(self, maximum):
        value = self.blob(maximum).decode('utf-8')
        if '\0' in value: raise Invalid('NUL in text')
        return value
    def end(self):
        if self.pos != len(self.data): raise Invalid('trailing request bytes')

def text(value):
    data = value.encode('utf-8'); return struct.pack('>H', len(data)) + data

def load_schemas(path):
    document=json.loads(Path(path).read_text('utf-8'))
    if document.get('schema_version')!=1:raise Invalid('unsupported leaderboard schema version')
    schemas={}
    for group in document['ranges']:
        first,last=group['first'],group['last']
        if not isinstance(first,int) or not isinstance(last,int) or not 1<=first<=last<=4096:raise Invalid('invalid leaderboard range')
        for view in range(first,last+1):
            if str(view) in schemas:raise Invalid('overlapping leaderboard ranges')
            schemas[str(view)]={k:v for k,v in group.items() if k not in ('first','last')}
    for schema in schemas.values():
        if schema['order'] not in ('asc','desc') or schema.get('aggregation','best') not in ('best','sum','last') or not 0<=schema['primary_column']<=65535:raise Invalid('invalid leaderboard configuration')
    return schemas

def packet(kind, request, peer, session, body=b''):
    if len(body) > 1352: raise Invalid('packet too large')
    return WIRE.pack(0x4D475039, 1, kind, TITLE, GAME, COMPAT, request, peer, session, len(body)) + body

def decode(data):
    if not 48 <= len(data) <= 1400: raise Invalid('packet length')
    magic, version, kind, title, game, compat, request, peer, session, size = WIRE.unpack(data[:48])
    if (magic, version, title, game, compat) != (0x4D475039, 1, TITLE, GAME, COMPAT): raise Invalid('incompatible packet')
    if not 1 <= kind <= 10 or not peer or size != len(data)-48: raise Invalid('packet header')
    return kind, request, peer, session, data[48:]

def room_details(body):
    r = Reader(body)
    room, host = r.u64(), r.u64()
    flags, capacity, members, state, private, private_members = (r.u32() for _ in range(6))
    key, name = r.raw(16), r.text(31)
    r.blob(768); r.blob(256); r.end()
    if not room or not host or not name or not 1 <= members <= capacity <= 20 or state > 1 or private > capacity or private_members > private or private_members > members or members-private_members > capacity-private:
        raise Invalid('invalid room')
    return room, host, key, capacity, members, state

class ReplayWindow:
    def __init__(self): self.top, self.bits = 0, 0
    def accept(self, value):
        if not 0 < value < 2**64: return False
        if value > self.top:
            shift = value-self.top
            self.bits = 0 if shift >= 128 else (self.bits << shift) & ((1 << 128)-1)
            self.top = value; self.bits |= 1; return True
        offset = self.top-value
        if offset >= 128 or self.bits & (1 << offset): return False
        self.bits |= 1 << offset; return True

@dataclass
class Client:
    peer: int
    ticket: bytes = field(default_factory=lambda: secrets.token_bytes(16))
    key: bytes = field(default_factory=lambda: secrets.token_bytes(32))
    endpoint: tuple | None = None
    seen: float = field(default_factory=time.monotonic)
    created: float = field(default_factory=time.monotonic)
    replay: ReplayWindow = field(default_factory=ReplayWindow)
    sequence: int = 0
    room: int = 0  # host identity of room membership
    allowance: float = 256000
    budget_at: float = field(default_factory=time.monotonic)
    search_request: int = 0
    search_at: float = 0
    control_sequence: int = 0

@dataclass
class Room:
    session: int
    host: int
    body: bytes
    seen: float = field(default_factory=time.monotonic)
    members: set = field(default_factory=set)
    pending: dict = field(default_factory=dict)
    host_sequence: int = 0

class Store:
    def __init__(self, path, schemas):
        self.db = sqlite3.connect(path)
        self.db.execute('PRAGMA journal_mode=WAL')
        self.db.execute('PRAGMA foreign_keys=ON')
        self.db.executescript('''
          CREATE TABLE IF NOT EXISTS players (id INTEGER PRIMARY KEY AUTOINCREMENT, credential TEXT UNIQUE NOT NULL, name TEXT NOT NULL);
          CREATE TABLE IF NOT EXISTS scores (player INTEGER NOT NULL REFERENCES players(id), view INTEGER NOT NULL, value INTEGER NOT NULL, columns TEXT NOT NULL, updated INTEGER NOT NULL, PRIMARY KEY(player,view));
          CREATE TABLE IF NOT EXISTS writes (player INTEGER NOT NULL, nonce TEXT NOT NULL, created INTEGER NOT NULL, PRIMARY KEY(player,nonce));
          CREATE INDEX IF NOT EXISTS scores_view_rating ON scores(view,value DESC,updated,player);
          CREATE INDEX IF NOT EXISTS writes_created ON writes(created);
        ''')
        self.schemas = schemas
    def connect(self, credential, name):
        if not 1 <= len(name.encode('utf-8')) <= 15 or '\0' in name: raise Invalid('player name must be 1-15 UTF-8 bytes')
        digest = hashlib.sha256(credential).hexdigest()
        row = self.db.execute('SELECT id FROM players WHERE credential=?', (digest,)).fetchone()
        with self.db:
            if row: self.db.execute('UPDATE players SET name=? WHERE id=?', (name, row[0])); number = row[0]
            else: number = self.db.execute('INSERT INTO players(credential,name) VALUES(?,?)', (digest, name)).lastrowid
        if not 0 < number <= 0xFFFFFF: raise Invalid('player limit reached')
        if name=='Player':
            with self.db:self.db.execute('UPDATE players SET name=? WHERE id=?',('Player '+f'{number:04X}'[-4:],number))
        return PEER_BASE | number
    def owner(self, credential):
        row = self.db.execute('SELECT id FROM players WHERE credential=?', (hashlib.sha256(credential).hexdigest(),)).fetchone()
        if not row: raise Invalid('connect before using statistics')
        return row[0]
    def write(self, player, nonce, views):
        # Only native views with an explicitly researched ordering/aggregation are
        # eligible. Never guess that higher race times or repeated totals win.
        validated=[]
        for view, columns in views:
            schema = self.schemas.get(str(view))
            if not schema: raise Invalid('leaderboard view has not been configured')
            primary = str(schema['primary_column'])
            if primary not in columns: raise Invalid('missing primary column')
            kind, bits = columns[primary]
            if kind not in (1, 2): raise Invalid('primary score must be an integer')
            value = int.from_bytes(bytes.fromhex(bits), 'big', signed=True)
            if not schema.get('minimum', 0) <= value <= schema.get('maximum', 2**53): raise Invalid('score out of range')
            validated.append((view, columns, schema, value))
        with self.db:
            if self.db.execute('SELECT 1 FROM writes WHERE player=? AND nonce=?', (player, nonce)).fetchone(): return
            for view, columns, schema, value in validated:
                old = self.db.execute('SELECT value FROM scores WHERE player=? AND view=?', (player, view)).fetchone()
                mode = schema.get('aggregation', 'best')
                if old and mode == 'best' and ((value >= old[0]) if schema['order'] == 'asc' else (value <= old[0])): continue
                if old and mode == 'sum':
                    value += old[0]
                    columns[str(schema['primary_column'])]=[columns[str(schema['primary_column'])][0],int(value).to_bytes(8,'big',signed=True).hex()]
                self.db.execute('INSERT INTO scores VALUES(?,?,?,?,?) ON CONFLICT(player,view) DO UPDATE SET value=excluded.value, columns=excluded.columns, updated=excluded.updated', (player, view, value, json.dumps(columns, separators=(',', ':')), int(time.time())))
            self.db.execute('INSERT INTO writes VALUES(?,?,?)', (player, nonce, int(time.time())))
            self.db.execute('DELETE FROM writes WHERE created<?', (int(time.time())-30*86400,))
    def read(self, view, start, count, users=(), around=0):
        schema = self.schemas.get(str(view))
        if not schema: raise Invalid('leaderboard view has not been configured')
        direction = 'ASC' if schema['order'] == 'asc' else 'DESC'
        query = f'''WITH ranked AS (SELECT s.player,p.name,s.columns,ROW_NUMBER() OVER (ORDER BY s.value {direction},s.updated ASC,s.player ASC) rank FROM scores s JOIN players p ON p.id=s.player WHERE s.view=?) SELECT * FROM ranked'''
        params=[view]
        total=self.db.execute('SELECT COUNT(*) FROM scores WHERE view=?',(view,)).fetchone()[0]
        if users:
            query += ' WHERE player IN (' + ','.join('?' for _ in users) + ')'; params.extend(x & 0xFFFFFF for x in users)
        else:
            if around:
                row=self.db.execute(query+' WHERE player=?', (view, around & 0xFFFFFF)).fetchone()
                if not row:return total,[]
                start=max(1, row[3]-count//2)
            query += ' WHERE rank>=?'; params.append(max(1,start))
        query += ' ORDER BY rank LIMIT ?'; params.append(count)
        return total, [(PEER_BASE | p, name, json.loads(cols), rank) for p,name,cols,rank in self.db.execute(query, params)]

class Service(asyncio.DatagramProtocol):
    def __init__(self, store, udp_port, max_clients=256, max_rooms=128):
        self.store, self.udp_port = store, udp_port
        self.max_clients, self.max_rooms = max_clients, max_rooms
        self.clients, self.tickets, self.rooms = {}, {}, {}
        self.http_budget = {}
        self.transport = None
    def connection_made(self, transport):
        self.transport=transport
        self.udp_port=transport.get_extra_info('sockname')[1]
    def connect(self, credential, name):
        self.expire()
        if len(self.clients) >= self.max_clients: raise Invalid('server is full')
        peer = self.store.connect(credential, name)
        old=self.clients.get(peer)
        if old:
            if time.monotonic()-old.seen < 15 and old.endpoint: raise Invalid('this online profile is already connected')
            self.remove(peer)
        client=Client(peer); self.clients[peer]=client; self.tickets[client.ticket]=client
        LOG.info('Connected player %016x', peer)
        return struct.pack('>Q16s32sH', peer, client.ticket, client.key, self.udp_port)
    def forward(self, client, data):
        if not client.endpoint: return
        client.sequence += 1
        body=ENVELOPE.pack(b'MGO1', client.ticket, client.sequence, client.peer)+data
        self.transport.sendto(body+hmac.digest(client.key, b'S'+body, 'sha256'), client.endpoint)
    def datagram_received(self, data, address):
        if not 68+48 <= len(data) <= 1468: return
        magic, ticket, seq, target = ENVELOPE.unpack(data[:36])
        client=self.tickets.get(ticket)
        if magic != b'MGO1' or not client: return
        if not hmac.compare_digest(data[-32:], hmac.digest(client.key,b'C'+data[:-32],'sha256')) or not client.replay.accept(seq): return
        now=time.monotonic()
        client.allowance=min(256000, client.allowance+(now-client.budget_at)*128000);client.budget_at=now
        if client.allowance < len(data): return
        client.allowance -= len(data)
        client.endpoint=address;client.seen=now
        try:
            kind, request, peer, session, body=decode(data[36:-32])
            if peer != client.peer: return
            if not target and kind in (ADVERT,LEFT):
                if seq<=client.control_sequence:return
                client.control_sequence=seq
            self.process(client, target, kind, request, session, body, data[36:-32],seq)
        except (Invalid, UnicodeError, struct.error): return
    def process(self, client, target, kind, request, session, body, raw, sequence):
        peer=client.peer
        if not target:
            if kind == HEARTBEAT and not session and not body: return
            if kind == ADVERT:
                room_id, host, key, cap, members, state=room_details(body)
                if host != peer or room_id != session: return
                previous=self.rooms.get(peer)
                if not previous:
                    if len(self.rooms) >= self.max_rooms or client.room: return
                    previous=Room(session,peer,body,members={peer});self.rooms[peer]=previous;client.room=peer
                if previous.session != session: return
                previous.body=body;previous.seen=time.monotonic();return
            if kind == LEFT and peer in self.rooms and self.rooms[peer].session == session:
                self.close_room(peer);return
            if kind == ROSTER:
                room=self.rooms.get(peer)
                if not room or room.session!=session or sequence<room.host_sequence:return
                r=Reader(body)
                flags,capacity,members,state,private,private_members=(r.u32() for _ in range(6));count=r.u8()
                if not 1<=count<=20 or count!=members:return
                present=set()
                for _ in range(count):
                    member,ip,port,priv=r.u64(),r.u32(),r.u16(),r.u8()
                    if member not in room.members or member in present or ip!=(0x0A000000|(member&0xFFFFFF)) or port!=30810 or priv>1:return
                    present.add(member)
                r.end()
                if peer not in present:return
                for removed in room.members-present:
                    previous=self.clients.get(removed)
                    if previous:
                        previous.room=0;self.forward(previous,packet(LEFT,0,peer,session))
                    room.pending.pop(removed,None)
                room.members=present;room.host_sequence=sequence;return
            if kind == DISCOVER and not body and request:
                self.expire()
                now=time.monotonic()
                if now-client.search_at < 0.2:return
                client.search_request=request;client.search_at=now
                for room in self.rooms.values():
                    host=self.clients.get(room.host)
                    if host and host.peer != peer:
                        # Query the simulation host's actual current capacity.
                        # Cached counts can be stale just after leave/rejoin.
                        self.forward(host,raw)
                return
            return
        destination=self.clients.get(target)
        if not destination or target == peer: return
        if kind == ADVERT:
            room=self.rooms.get(peer)
            if not room or room.session!=session or request!=destination.search_request or time.monotonic()-destination.search_at>5:return
            rid,host,key,cap,members,state=room_details(body)
            if rid!=session or host!=peer:return
            room.body=body;room.seen=time.monotonic();self.forward(destination,raw);return
        if kind == JOIN:
            room=self.rooms.get(target)
            if not room or room.session != session or client.room not in (0,target): return
            rid,host,key,cap,members,state=room_details(room.body)
            if body != key or state or (len(room.members) >= cap and peer not in room.members):
                self.forward(client,packet(REJECT,request,target,session));return
            if len(room.pending)>=40 and peer not in room.pending: return
            room.pending[peer]=(request,time.monotonic())
            self.forward(destination,raw);return
        room=self.rooms.get(client.room)
        if not room or room.session != session: return
        if kind in (JOINED, REJECT) and peer == room.host:
            pending=room.pending.get(target)
            if not pending or pending[0] != request: return
            if kind == JOINED:
                if not body or body[-1] > 1: return
                rid,host,key,cap,members,state=room_details(body[:-1])
                if rid != session or host != peer: return
                destination.room=peer;room.members.add(target)
                room.host_sequence=max(room.host_sequence,sequence)
            self.forward(destination,raw)
            if kind==REJECT: room.pending.pop(target,None)
            return
        if peer not in room.members or target not in room.members or destination.room != room.host: return
        if kind not in (LEAVE, LEFT, HEARTBEAT, DATA, ROSTER): return
        if kind in (LEFT, ROSTER) and peer != room.host: return
        if kind==LEFT and sequence<room.host_sequence:return
        if kind == ROSTER:
            # The game authenticates host roster data; the relay additionally
            # bounds routing to admitted members, never the embedded IP values.
            if not 25 <= len(body) <= 325: return
        self.forward(destination,raw)
        if kind == LEAVE and target==room.host and peer!=room.host:
            # Release the directory slot even if the host's UDP acknowledgment
            # is lost. The leaving client has already cleared its local room.
            room.members.discard(peer);client.room=0
            room.pending.pop(peer,None)
        if kind == LEFT:
            room.members.discard(target);destination.room=0
            room.pending.pop(target,None);room.host_sequence=sequence
    def close_room(self, host):
        room=self.rooms.pop(host,None)
        if not room: return
        for peer in room.members:
            client=self.clients.get(peer)
            if client:
                if peer!=host:self.forward(client,packet(LEFT,0,host,room.session))
                client.room=0
        LOG.info('Closed room %016x', room.session)
    def remove(self, peer):
        client=self.clients.get(peer)
        if not client:return
        self.close_room(peer)
        if client.room in self.rooms:self.rooms[client.room].members.discard(peer)
        self.tickets.pop(client.ticket,None);self.clients.pop(peer,None)
    def expire(self):
        now=time.monotonic()
        for host,room in list(self.rooms.items()):
            if now-room.seen>15:self.close_room(host)
            else:room.pending={p:v for p,v in room.pending.items() if now-v[1]<10}
        for peer,c in list(self.clients.items()):
            if now-c.seen>45:self.remove(peer)
        self.http_budget={ip:v for ip,v in self.http_budget.items() if now-v[0]<60}
    async def housekeeping(self):
        while True:await asyncio.sleep(1);self.expire()
    def handle(self, path, credential, body):
        if path == '/v1/connect':
            r=Reader(body)
            if r.u64()!=COMPAT:raise Invalid('client build/protocol is incompatible')
            name=r.text(15);r.end();return self.connect(credential,name)
        owner=self.store.owner(credential)
        if path == '/v1/disconnect':
            current=self.clients.get(PEER_BASE | owner)
            if len(body)!=16:raise Invalid('invalid ticket identifier')
            if current and hmac.compare_digest(current.ticket,body):self.remove(current.peer)
            return b''
        if path == '/v1/stats/write':
            r=Reader(body);nonce=r.text(32)
            if len(nonce)!=32 or any(c not in '0123456789abcdef' for c in nonce):raise Invalid('invalid write id')
            count=r.u32()
            if not 1<=count<=16:raise Invalid('invalid view count')
            views=[]
            for _ in range(count):
                view=r.u32();amount=r.u32()
                if not 1<=amount<=64:raise Invalid('invalid column count')
                columns={}
                for _ in range(amount):
                    column=r.u16();kind=r.u8();bits=r.raw(8)
                    if str(column) in columns or kind not in (1,2,3,5):raise Invalid('invalid column')
                    columns[str(column)]=[kind,bits.hex()]
                views.append((view,columns))
            r.end();self.store.write(owner,nonce,views);return b''
        if path == '/v1/stats/read':
            r=Reader(body);view,start,count,around=r.u32(),r.u32(),r.u32(),r.u64();amount=r.u32()
            if not 1<=count<=100 or amount>100:raise Invalid('invalid query range')
            users=[r.u64() for _ in range(amount)];r.end()
            total,rows=self.store.read(view,start,count,users,around);out=struct.pack('>II',total,len(rows))
            for peer,name,cols,rank in rows:
                out+=struct.pack('>QI',peer,rank)+text(name)+struct.pack('>I',len(cols))
                for column,(kind,bits) in cols.items():out+=struct.pack('>HB',int(column),kind)+bytes.fromhex(bits)
            return out
        raise Invalid('unknown endpoint')
    async def http(self, reader, writer):
        status,response=400,b'Invalid request'
        try:
            address=writer.get_extra_info('peername')[0]
            start,used=self.http_budget.get(address,(time.monotonic(),0))
            if time.monotonic()-start>=60:start,used=time.monotonic(),0
            self.http_budget[address]=(start,used+1)
            if used>=300:status,response=429,b'Too many requests'
            else:
                header=await asyncio.wait_for(reader.readuntil(b'\r\n\r\n'),5)
                if len(header)>8192:raise Invalid('headers too large')
                lines=header.decode('ascii').split('\r\n');method,path,version=lines[0].split(' ')
                if version!='HTTP/1.1':raise Invalid('HTTP version')
                headers={}
                for line in lines[1:]:
                    if not line:continue
                    key,value=line.split(':',1);key=key.lower()
                    if key in headers:raise Invalid('duplicate header')
                    headers[key]=value.strip()
                if method=='GET' and path=='/health':
                    status,response=200,json.dumps({'service':'motogp0910','protocol':'MGO1','players':len(self.clients),'rooms':len(self.rooms)}).encode()
                else:
                    if method!='POST' or 'transfer-encoding' in headers:raise Invalid('POST with Content-Length required')
                    size=int(headers.get('content-length','-1'))
                    if not 0<=size<=MAX_BODY:raise Invalid('request too large')
                    auth=headers.get('authorization','')
                    if not auth.startswith('Bearer ') or len(auth)!=71:raise Invalid('missing credential')
                    credential=bytes.fromhex(auth[7:])
                    if len(credential)!=32:raise Invalid('invalid credential')
                    body=await asyncio.wait_for(reader.readexactly(size),5)
                    response=self.handle(path,credential,body);status=200
        except (ValueError,UnicodeError,asyncio.IncompleteReadError,asyncio.LimitOverrunError,TimeoutError) as error:
            response=str(error).encode('utf-8')[:256]
        except Exception:
            LOG.exception('Service request failed');status,response=500,b'Service request failed'
        try:
            names={200:'OK',400:'Bad Request',429:'Too Many Requests',500:'Internal Server Error'}
            writer.write(f'HTTP/1.1 {status} {names[status]}\r\nContent-Type: application/octet-stream\r\nContent-Length: {len(response)}\r\nConnection: close\r\n\r\n'.encode()+response)
            await writer.drain()
        except (ConnectionError,OSError):pass
        finally:
            writer.close()
            try:await writer.wait_closed()
            except (ConnectionError,OSError):pass

async def run(args):
    db=Path(args.database);db.parent.mkdir(parents=True,exist_ok=True)
    schemas=load_schemas(args.leaderboards)
    service=Service(Store(db,schemas),args.udp_port,args.max_clients,args.max_rooms)
    ssl_context=None
    if args.cert and args.key:
        ssl_context=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);ssl_context.minimum_version=ssl.TLSVersion.TLSv1_2
        ssl_context.load_cert_chain(args.cert,args.key)
    elif args.http_bind not in ('127.0.0.1','::1'):
        raise Invalid('Public control API requires --cert and --key; use loopback behind an HTTPS reverse proxy')
    loop=asyncio.get_running_loop()
    transport,_=await loop.create_datagram_endpoint(lambda:service,local_addr=(args.udp_bind,args.udp_port))
    server=await asyncio.start_server(service.http,args.http_bind,args.http_port,ssl=ssl_context,limit=8192)
    task=asyncio.create_task(service.housekeeping())
    LOG.info('Control API %s:%s (%s), UDP relay %s:%s',args.http_bind,args.http_port,'TLS' if ssl_context else 'loopback HTTP',args.udp_bind,service.udp_port)
    try:
        async with server:await server.serve_forever()
    finally:
        task.cancel();transport.close();service.store.db.close()

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--http-bind',default='127.0.0.1');p.add_argument('--http-port',type=int,default=30820)
    p.add_argument('--udp-bind',default='0.0.0.0');p.add_argument('--udp-port',type=int,default=30821)
    p.add_argument('--database',default='data/community.sqlite3')
    p.add_argument('--leaderboards',default=str(Path(__file__).with_name('leaderboards.json')))
    p.add_argument('--cert');p.add_argument('--key');p.add_argument('--max-clients',type=int,default=256);p.add_argument('--max-rooms',type=int,default=128)
    args=p.parse_args()
    if not 1<=args.max_clients<=4096 or not 1<=args.max_rooms<=128:p.error('invalid service capacity')
    logging.basicConfig(level=logging.INFO,format='%(asctime)s %(levelname)s %(message)s')
    try:asyncio.run(run(args))
    except KeyboardInterrupt:pass

if __name__=='__main__':main()
