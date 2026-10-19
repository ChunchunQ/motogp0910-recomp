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
import os
from pathlib import Path
import secrets
import sys
import sqlite3
import socket
import ssl
import struct
import time
import urllib.error
import urllib.request

LOG = logging.getLogger('motogp.online')
TITLE, GAME, COMPAT = 0x434307F0, 0x101, 0x4D4750394C414E02
PEER_BASE = 0x0009000000000000
NOBK_XUID_BASE = 0x100000  # NOBK Online: xuid = 0x100000 + site account id
LINK_TOKEN_CHARS = frozenset('ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_')
WIRE = struct.Struct('>IHHIIQ IQQI')
ENVELOPE = struct.Struct('>4s16sQQ')
DISCOVER, ADVERT, JOIN, JOINED, LEAVE, LEFT, HEARTBEAT, REJECT, DATA, ROSTER = range(1, 11)
MAX_BODY = 65536
ONLINE_MAGIC = b'MGO2'
NATIVE_COLUMNS = {1:1, 2:1, 3:2, 4:1, 5:1, 6:1, 7:1, 13:1, 0x8011:1}
# Also support the isolated python.org Windows embeddable runtime.
sys.path.insert(0,str(Path(__file__).resolve().parent))
from security import Invalid, Limited, Bucket, Limits, parse_headers, credential_header, client_ip, read_proxy_secret, network_key

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
    candidate: tuple | None = None
    challenge: bytes = b''
    challenge_at: float = 0
    challenge_sent: float = 0
    deferred: tuple | None = None
    packets: Bucket = field(default_factory=lambda: Bucket(1000,2000))
    outgoing: Bucket = field(default_factory=lambda: Bucket(256000,512000))
    adverts: set = field(default_factory=set)
    queries: dict = field(default_factory=dict)
    joins: Bucket = field(default_factory=lambda: Bucket(10,20))
    dropped: dict = field(default_factory=dict)
    relayed: dict = field(default_factory=dict)  # kind -> [last monotonic time, count] of room packets this peer sent

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
    def __init__(self, path, schemas, max_players=10000, max_db_mib=256):
        self.max_players = max_players
        self.db = sqlite3.connect(path, timeout=0.1)
        self.deadline = 0.0
        self.db.set_progress_handler(lambda: int(bool(self.deadline) and time.monotonic()>self.deadline), 1000)
        self.db.execute('PRAGMA trusted_schema=OFF')
        self.db.execute('PRAGMA journal_size_limit=8388608')
        page_size = self.db.execute('PRAGMA page_size').fetchone()[0]
        maximum_pages=max_db_mib*1024*1024//page_size
        actual=self.db.execute(f'PRAGMA max_page_count={maximum_pages}').fetchone()[0]
        if actual>maximum_pages:
            self.db.close()
            raise Invalid('Existing database exceeds quota; back up and increase --max-db-mib')
        self.db.execute('PRAGMA journal_mode=WAL')
        self.db.execute('PRAGMA foreign_keys=ON')
        self.db.executescript('''
          CREATE TABLE IF NOT EXISTS players (id INTEGER PRIMARY KEY AUTOINCREMENT, credential TEXT UNIQUE NOT NULL, name TEXT NOT NULL);
          CREATE TABLE IF NOT EXISTS scores (player INTEGER NOT NULL REFERENCES players(id), view INTEGER NOT NULL, value INTEGER NOT NULL, columns TEXT NOT NULL, updated INTEGER NOT NULL, PRIMARY KEY(player,view));
          CREATE TABLE IF NOT EXISTS writes (player INTEGER NOT NULL, nonce TEXT NOT NULL, created INTEGER NOT NULL, PRIMARY KEY(player,nonce));
          CREATE INDEX IF NOT EXISTS scores_view_rating ON scores(view,value DESC,updated,player);
          CREATE INDEX IF NOT EXISTS writes_created ON writes(created);
        ''')
        self.db.execute('CREATE INDEX IF NOT EXISTS writes_player_created ON writes(player,created DESC,nonce)')
        # NOBK Online site accounts: a linked player keeps one identity (records,
        # name) on every PC; extra PCs' profile credentials point at it.
        if 'account' not in {row[1] for row in self.db.execute('PRAGMA table_info(players)')}:
            with self.db:self.db.execute('ALTER TABLE players ADD COLUMN account INTEGER')
        self.db.executescript('''
          CREATE UNIQUE INDEX IF NOT EXISTS players_account ON players(account) WHERE account IS NOT NULL;
          CREATE TABLE IF NOT EXISTS credential_alias (credential TEXT PRIMARY KEY, player INTEGER NOT NULL REFERENCES players(id));
        ''')
        self.player_count = self.db.execute('SELECT COUNT(*) FROM players').fetchone()[0]
        self.schemas = schemas
    def player_for(self, digest):
        """(player id, linked account or None) for a profile credential digest."""
        row = self.db.execute('SELECT p.id, p.account FROM credential_alias a JOIN players p ON p.id=a.player WHERE a.credential=?', (digest,)).fetchone()
        return row or self.db.execute('SELECT id, account FROM players WHERE credential=?', (digest,)).fetchone()
    def connect(self, credential, name, require_account=False):
        if not 1 <= len(name.encode('utf-8')) <= 15 or any(ord(c)<32 or ord(c)==127 for c in name): raise Invalid('player name must be 1-15 UTF-8 bytes')
        digest = hashlib.sha256(credential).hexdigest()
        row = self.player_for(digest)
        if require_account and not (row and row[1]):
            raise Invalid('sign in with your NOBK Online account in the launcher')
        with self.db:
            # A linked player's name is the site account name; the game cannot change it.
            if row and row[1]: number = row[0]; name = None
            elif row: self.db.execute('UPDATE players SET name=? WHERE id=?', (name, row[0])); number = row[0]
            else:
                if self.player_count >= self.max_players: raise Limited('registered player capacity reached')
                number = self.db.execute('INSERT INTO players(credential,name) VALUES(?,?)', (digest, name)).lastrowid
                if not 0 < number <= 0xFFFFFF: raise Limited('player limit reached')
        if not row: self.player_count += 1
        if name=='Player':
            with self.db:self.db.execute('UPDATE players SET name=? WHERE id=?',('Player '+f'{number:04X}'[-4:],number))
        return PEER_BASE | number
    def owner(self, credential):
        row = self.player_for(hashlib.sha256(credential).hexdigest())
        if not row: raise Invalid('connect before using statistics')
        return row[0]
    def link(self, credential, account, username):
        """Bind this PC's profile credential to a verified NOBK Online account.
        First link keeps the profile's existing records; another PC of the same
        account then shares that player instead of starting a new one."""
        if not 0 < account < 2**31 or not 1 <= len(username.encode('utf-8')) <= 15 or any(ord(c)<32 or ord(c)==127 for c in username):
            raise Invalid('invalid account')
        digest = hashlib.sha256(credential).hexdigest()
        own = self.player_for(digest)
        linked = self.db.execute('SELECT id FROM players WHERE account=?', (account,)).fetchone()
        with self.db:
            if linked:
                number = linked[0]
                if not own or own[0] != number:
                    if own and own[1]: raise Invalid('this PC is linked to another NOBK Online account')
                    self.db.execute('INSERT OR REPLACE INTO credential_alias(credential,player) VALUES(?,?)', (digest, number))
            elif own:
                if own[1]: raise Invalid('this PC is linked to another NOBK Online account')
                number = own[0]
                self.db.execute('UPDATE players SET account=? WHERE id=?', (account, number))
            else:
                if self.player_count >= self.max_players: raise Limited('registered player capacity reached')
                number = self.db.execute('INSERT INTO players(credential,name,account) VALUES(?,?,?)', (digest, username, account)).lastrowid
                if not 0 < number <= 0xFFFFFF: raise Limited('player limit reached')
                self.player_count += 1
            self.db.execute('UPDATE players SET name=? WHERE id=?', (username, number))
        return number
    def write(self, player, nonce, views):
        # Only native views with an explicitly researched ordering/aggregation are
        # eligible. Never guess that higher race times or repeated totals win.
        validated=[]
        if not 1<=len(views)<=16 or len({v for v,_ in views})!=len(views): raise Invalid('invalid view batch')
        if len(nonce)!=32 or any(c not in '0123456789abcdef' for c in nonce): raise Invalid('invalid write id')
        for view, columns in views:
            if not 1<=len(columns)<=9: raise Invalid('invalid native columns')
            for column,(kind,bits) in columns.items():
                if not column.isdecimal() or str(int(column))!=column or NATIVE_COLUMNS.get(int(column))!=kind or len(bits)!=16:
                    raise Invalid('unsupported native column')
                try: raw = bytes.fromhex(bits)
                except ValueError: raise Invalid('invalid column value') from None
                if len(raw)!=8: raise Invalid('invalid column value')
                value = int.from_bytes(raw,'big',signed=True)
                if kind==1 and not -(2**31)<=value<2**31: raise Invalid('noncanonical int32 column')
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
                    if not schema.get('minimum',0)<=value<=schema.get('maximum',2**53): raise Invalid('aggregate score out of range')
                    columns[str(schema['primary_column'])]=[columns[str(schema['primary_column'])][0],int(value).to_bytes(8,'big',signed=True).hex()]
                self.db.execute('INSERT INTO scores VALUES(?,?,?,?,?) ON CONFLICT(player,view) DO UPDATE SET value=excluded.value, columns=excluded.columns, updated=excluded.updated', (player, view, value, json.dumps(columns, separators=(',', ':')), int(time.time())))
            self.db.execute('INSERT INTO writes VALUES(?,?,?)', (player, nonce, int(time.time())))
            # Bound replay history even when every submission is a no-op.
            self.db.execute('DELETE FROM writes WHERE player=? AND nonce IN (SELECT nonce FROM writes WHERE player=? ORDER BY rowid DESC LIMIT -1 OFFSET 256)', (player,player))
    def read(self, view, start, count, users=(), around=0):
        schema = self.schemas.get(str(view))
        if not schema: raise Invalid('leaderboard view has not been configured')
        if not 1<=count<=100 or len(users)>100 or not 0<=start<=0xFFFFFFFF: raise Invalid('invalid query range')
        if any((x & 0xFFFFFFFFFF000000)!=PEER_BASE or not (x & 0xFFFFFF) for x in (*users, *((around,) if around else ()))):
            raise Invalid('invalid online identity')
        direction = 'ASC' if schema['order'] == 'asc' else 'DESC'
        before = '<' if direction=='ASC' else '>'
        total=self.db.execute('SELECT COUNT(*) FROM scores WHERE view=?',(view,)).fetchone()[0]
        def rank(player,value,updated):
            return 1+self.db.execute(f'SELECT COUNT(*) FROM scores WHERE view=? AND (value {before} ? OR (value=? AND (updated<? OR (updated=? AND player<?))))', (view,value,value,updated,updated,player)).fetchone()[0]
        base='SELECT s.player,p.name,s.columns,s.value,s.updated FROM scores s JOIN players p ON p.id=s.player WHERE s.view=?'
        if users:
            query=base+' AND s.player IN ('+','.join('?' for _ in users)+')'
            found=self.db.execute(query,(view,*(x&0xFFFFFF for x in users))).fetchall()
            rows=sorted([(p,name,cols,rank(p,value,updated)) for p,name,cols,value,updated in found],key=lambda row:row[3])[:count]
        else:
            if around:
                row=self.db.execute('SELECT value,updated FROM scores WHERE view=? AND player=?',(view,around&0xFFFFFF)).fetchone()
                if not row:return total,[]
                start=max(1,rank(around&0xFFFFFF,*row)-count//2)
            start=max(1,start)
            found=self.db.execute(base+f' ORDER BY s.value {direction},s.updated ASC,s.player ASC LIMIT ? OFFSET ?',(view,count,start-1))
            rows=[(p,name,cols,start+i) for i,(p,name,cols,_,_) in enumerate(found)]
        # Old v1 databases remain usable, but never pass arbitrary historic
        # columns through to the guest. New writes are strictly validated.
        result=[]
        for p,name,cols,position in rows:
            columns={k:v for k,v in json.loads(cols).items() if k.isdecimal() and NATIVE_COLUMNS.get(int(k))==v[0]}
            result.append((PEER_BASE|p,name,columns,position))
        return total,result

class Service(asyncio.DatagramProtocol):
    def __init__(self, store, udp_port, max_clients=256, max_rooms=128, proxy_secret=None):
        self.store, self.udp_port = store, udp_port
        self.max_clients, self.max_rooms = max_clients, max_rooms
        self.clients, self.tickets, self.rooms = {}, {}, {}
        self.limits = Limits()
        # Only existing authenticated profiles can allocate this separate pool.
        # Four endpoint budgets per persistent player fit even at capacity.
        self.profile_limits = Limits(4*max(store.max_players,store.player_count))
        self.registrations = Bucket(1,200)
        self.http_connections = 0
        self.http_ips = {}
        # A full 20-player room may share one public NAT address. Keep
        # headroom for concurrent reconnect/health traffic from that network.
        self.max_http_connections, self.max_http_per_ip = 128, 32
        self.header_timeout, self.body_timeout, self.write_timeout = 5, 5, 3
        self.proxy_secret = proxy_secret
        self.http_global = Bucket(100,250)
        self.udp_checks = Bucket(40000,80000)
        self.udp_packets = Bucket(20000,40000)
        self.udp_incoming = Bucket(16000000,32000000)
        self.udp_outgoing = Bucket(32000000,64000000)
        self.discovery = Bucket(64,128)
        self.db_cpu = Bucket(0.25,0.25)
        self.transport = None
        self.live_snapshot = None
        self.drops_logged = 0.0
        # NOBK Online accounts: launcher tokens are verified by the site's own
        # account API on this machine (loopback); None disables /v2/link.
        self.account_api = None
        self.require_account = False
        self.account_checks = Bucket(5,20)
    def connection_made(self, transport):
        self.transport=transport
        self.udp_port=transport.get_extra_info('sockname')[1]
    def connect(self, credential, name):
        self.expire()
        row=self.store.player_for(hashlib.sha256(credential).hexdigest())
        peer=(PEER_BASE | row[0]) if row else 0
        old=self.clients.get(peer)
        if len(self.clients)>=self.max_clients and not old:raise Limited('server is full')
        if old:
            if time.monotonic()-old.seen < 15 and old.endpoint: raise Invalid('this online profile is already connected')
        peer=self.store.connect(credential,name,self.require_account)
        if old:self.remove(peer)
        client=Client(peer); self.clients[peer]=client; self.tickets[client.ticket]=client
        LOG.info('Connected player %016x', peer)
        return struct.pack('>Q16s32sH', peer, client.ticket, client.key, self.udp_port)
    def envelope(self, client, data, address):
        size = len(data)+68
        if not client.outgoing.take(size) or not self.udp_outgoing.take(size):
            client.dropped['out-limit']=client.dropped.get('out-limit',0)+1;return
        client.sequence += 1
        body=ENVELOPE.pack(ONLINE_MAGIC, client.ticket, client.sequence, client.peer)+data
        self.transport.sendto(body+hmac.digest(client.key, b'S'+body, 'sha256'), address)
    def forward(self, client, data):
        if client.endpoint: self.envelope(client,data,client.endpoint)
    def receive_packet(self, client, target, raw, sequence):
        kind, request, peer, session, body=decode(raw)
        if peer != client.peer: return
        if not target and kind in (ADVERT,LEFT):
            if sequence<=client.control_sequence:return
            client.control_sequence=sequence
        self.process(client,target,kind,request,session,body,raw,sequence)
    def datagram_received(self, data, address):
        # Apply cheap size/global admission before any cryptography. Unknown
        # tickets are silent; malformed traffic never generates a reply.
        if not 84 <= len(data) <= 1468: return
        magic, ticket, seq, target = ENVELOPE.unpack(data[:36])
        client=self.tickets.get(ticket)
        if magic != ONLINE_MAGIC or not client or not self.udp_checks.take(): return
        if not hmac.compare_digest(data[-32:], hmac.digest(client.key,b'C'+data[:-32],'sha256')) or not client.replay.accept(seq): return
        # Unauthenticated traffic/replays must never spend another player's
        # quota, or the verified global gameplay budget. Crypto work has its
        # own bounded gate so failure-path CPU is still capped.
        if not client.packets.take() or not self.udp_packets.take() or not self.udp_incoming.take(len(data)):
            client.dropped['in-limit']=client.dropped.get('in-limit',0)+1;return
        now=time.monotonic()
        client.allowance=min(256000, client.allowance+(now-client.budget_at)*128000);client.budget_at=now
        if client.allowance < len(data):
            client.dropped['in-bytes']=client.dropped.get('in-bytes',0)+1;return
        client.allowance -= len(data)
        raw=data[36:-32]
        try:
            if address != client.endpoint:
                if not target and client.candidate==address and now-client.challenge_at<=5 and len(raw)==16 and hmac.compare_digest(raw,client.challenge):
                    # Possession of the HTTPS key alone does not prove that a
                    # claimed UDP source address can receive our packets.
                    client.endpoint=address;client.seen=now
                    deferred=client.deferred
                    client.candidate=None;client.challenge=b'';client.deferred=None
                    if deferred:self.receive_packet(client,*deferred)
                    return
                if len(raw)<48:return
                kind,request,peer,session,body=decode(raw)
                if peer!=client.peer:return
                if client.candidate is None or now-client.challenge_at>5:
                    client.candidate=address;client.challenge=secrets.token_bytes(16)
                    client.challenge_at=now;client.challenge_sent=0
                    client.deferred=(target,raw,seq)
                if client.candidate==address and now-client.challenge_sent>=0.5:
                    # 84 bytes out, at least 116 in: no amplification to an
                    # unverified victim, including NAT endpoint changes.
                    self.envelope(client,client.challenge,address);client.challenge_sent=now
                return
            if len(raw)==16:return  # harmless repeated challenge acknowledgement
            self.receive_packet(client,target,raw,seq)
            client.seen=now
        except (Invalid, UnicodeError, struct.error): return
    def clear_pending(self, peer, except_host=0):
        for host,room in self.rooms.items():
            if host!=except_host:room.pending.pop(peer,None)
    def roster(self, room, body):
        r=Reader(body)
        flags,capacity,members,state,private,private_members=(r.u32() for _ in range(6));count=r.u8()
        if not 1<=count==members<=capacity<=20 or state>1 or not private_members<=private<=capacity or private_members>members or members-private_members>capacity-private:
            raise Invalid('invalid roster counts')
        present=set();private_count=0
        for _ in range(count):
            member,ip,port,priv=r.u64(),r.u32(),r.u16(),r.u8()
            current=self.clients.get(member)
            if member not in room.members or member in present or not current or current.room!=room.host or ip!=(0x0A000000|(member&0xFFFFFF)) or port!=30810 or priv>1:
                raise Invalid('invalid roster member')
            present.add(member);private_count+=priv
        r.end()
        if room.host not in present or private_count!=private_members:raise Invalid('invalid roster host/private slots')
        return present
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
                    self.clear_pending(peer)
                if previous.session != session: return
                previous.body=body;previous.seen=time.monotonic();return
            if kind == LEFT and peer in self.rooms and self.rooms[peer].session == session:
                self.close_room(peer,'host left');return
            if kind == ROSTER:
                room=self.rooms.get(peer)
                if not room or room.session!=session or sequence<room.host_sequence:return
                present=self.roster(room,body)
                for removed in room.members-present:
                    LOG.info('Room %016x: host %d removed player %d',room.session,peer&0xFFFFFF,removed&0xFFFFFF)
                    previous=self.clients.get(removed)
                    if previous:
                        if previous.room==peer:previous.room=0
                        self.forward(previous,packet(LEFT,0,peer,session))
                    room.pending.pop(removed,None)
                room.members=present;room.host_sequence=sequence;return
            if kind == DISCOVER and not body and request:
                self.expire()
                now=time.monotonic()
                if now-client.search_at < 0.2 or not self.discovery.take():return
                client.search_request=request;client.search_at=now;client.adverts.clear()
                for room in self.rooms.values():
                    host=self.clients.get(room.host)
                    if host and host.peer != peer:
                        # Query the simulation host's actual current capacity.
                        # Cached counts can be stale just after leave/rejoin.
                        host.queries[peer]=(request,now)
                        self.forward(host,raw)
                return
            return
        destination=self.clients.get(target)
        if not destination or target == peer: return
        if kind == ADVERT:
            room=self.rooms.get(peer)
            if not room or room.session!=session or request!=destination.search_request or time.monotonic()-destination.search_at>5:return
            query=client.queries.get(target)
            if not query or query[0]!=request or time.monotonic()-query[1]>5 or peer in destination.adverts:return
            rid,host,key,cap,members,state=room_details(body)
            if rid!=session or host!=peer:return
            destination.adverts.add(peer)
            room.body=body;room.seen=time.monotonic();self.forward(destination,raw);return
        if kind == JOIN:
            if not request or not client.joins.take():return
            room=self.rooms.get(target)
            if not room or room.session != session or client.room not in (0,target): return
            rid,host,key,cap,members,state=room_details(room.body)
            if body != key or state or (len(room.members) >= cap and peer not in room.members):
                self.forward(client,packet(REJECT,request,target,session));return
            if len(room.pending)>=40 and peer not in room.pending: return
            room.pending[peer]=(request,time.monotonic(),client.ticket,key)
            self.forward(destination,raw);return
        room=self.rooms.get(client.room)
        if not room or room.session != session: return
        if kind in (JOINED, REJECT) and peer == room.host:
            pending=room.pending.get(target)
            if not pending or pending[0] != request or time.monotonic()-pending[1]>=10 or pending[2]!=destination.ticket or destination.room not in (0,peer): return
            if kind==REJECT and body:return
            if kind == JOINED:
                if not body or body[-1] > 1: return
                rid,host,key,cap,members,state=room_details(body[:-1])
                current_key=room_details(room.body)[2]
                if rid != session or host != peer or key!=pending[3] or key!=current_key or (target not in room.members and len(room.members)>=min(cap,room_details(room.body)[3])): return
                self.clear_pending(target,except_host=peer)
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
            if sequence<room.host_sequence or target not in self.roster(room,body): return
        if kind in (LEAVE,LEFT) and body:return
        if kind==HEARTBEAT and (len(body)>1 or (body and body[0]>1)):return
        if kind==DATA:
            r=Reader(body);source_port,dest_port=r.u16(),r.u16();r.blob(1344);r.end()
            if not source_port or not dest_port:return
        self.forward(destination,raw)
        mark=client.relayed.setdefault(kind,[0.0,0]);mark[0]=time.monotonic();mark[1]+=1
        if kind == LEAVE and target==room.host and peer!=room.host:
            # Release the directory slot even if the host's UDP acknowledgment
            # is lost. The leaving client has already cleared its local room.
            LOG.info('Room %016x: player %d left; traffic %s; host traffic %s',room.session,peer&0xFFFFFF,self.traffic(peer),self.traffic(room.host))
            room.members.discard(peer);client.room=0
            room.pending.pop(peer,None)
        if kind == LEFT:
            room.members.discard(target);destination.room=0
            room.pending.pop(target,None);room.host_sequence=sequence
    def traffic(self, peer):
        """Seconds since this peer last sent each room packet kind through the relay (diagnostics)."""
        client=self.clients.get(peer)
        if not client:return 'gone'
        now=time.monotonic();names={DATA:'data',HEARTBEAT:'hb',ROSTER:'roster'}
        parts=[f'{names[k]} {now-client.relayed[k][0]:.1f}s/{client.relayed[k][1]}' for k in names if k in client.relayed]
        return ', '.join(parts) or 'none'
    def close_room(self, host, reason=''):
        room=self.rooms.pop(host,None)
        if not room: return
        for peer in room.members:
            client=self.clients.get(peer)
            if client and client.room==host:
                if peer!=host:self.forward(client,packet(LEFT,0,host,room.session))
                client.room=0
        LOG.info('Closed room %016x host %d members %s: %s; traffic %s', room.session, host&0xFFFFFF, sorted(m&0xFFFFFF for m in room.members), reason or 'closed',
                 '; '.join(f'{m&0xFFFFFF}: {self.traffic(m)}' for m in sorted(room.members|{host})))
    def remove(self, peer):
        client=self.clients.get(peer)
        if not client:return
        self.close_room(peer,'host disconnected')
        if client.room in self.rooms:self.rooms[client.room].members.discard(peer)
        self.clear_pending(peer)
        self.tickets.pop(client.ticket,None);self.clients.pop(peer,None)
    def expire(self):
        now=time.monotonic()
        for host,room in list(self.rooms.items()):
            if now-room.seen>15:self.close_room(host,'no host advert for 15 s')
            else:room.pending={p:v for p,v in room.pending.items() if now-v[1]<10}
        for peer,c in list(self.clients.items()):
            if now-c.seen>45:LOG.info('Player %d timed out (no UDP for 45 s)',peer&0xFFFFFF);self.remove(peer)
        if now-self.drops_logged>=10:
            for c in self.clients.values():
                if c.dropped:LOG.info('Player %d: %s',c.peer&0xFFFFFF,', '.join(f'{k} {v}' for k,v in sorted(c.dropped.items())));c.dropped.clear()
            self.drops_logged=now
        self.limits.prune();self.profile_limits.prune()
        for client in self.clients.values():
            client.queries={p:v for p,v in client.queries.items() if now-v[1]<5}
            if client.candidate and now-client.challenge_at>5:
                client.candidate=None;client.challenge=b'';client.deferred=None
    async def housekeeping(self):
        tick=0
        while True:
            await asyncio.sleep(1);self.expire();tick+=1
            if self.live_snapshot and tick%5==0:
                try:self.write_snapshot()
                except Exception:LOG.exception('live snapshot failed')
    def write_snapshot(self):
        """Write a public, read-only summary (names, rooms) for the website API.
        No tickets, keys, endpoints or IP addresses are included."""
        ids=sorted({c.peer&0xFFFFFF for c in self.clients.values() if c.endpoint})
        names={}
        for i in range(0,len(ids),200):
            chunk=ids[i:i+200]
            names.update(self.store.db.execute('SELECT id,name FROM players WHERE id IN ('+','.join('?'*len(chunk))+')',chunk).fetchall())
        rooms=[]
        for host,room in self.rooms.items():
            try:
                r=Reader(room.body);r.u64();r.u64()
                flags,capacity,members,state,private,private_members=(r.u32() for _ in range(6))
                r.raw(16);title=r.text(31)
            except (Invalid,UnicodeError,struct.error):continue
            rooms.append({'host':host&0xFFFFFF,'host_name':names.get(host&0xFFFFFF,''),'name':title,
                'capacity':capacity,'members':[{'id':m&0xFFFFFF,'name':names.get(m&0xFFFFFF,'')} for m in sorted(room.members)],
                'in_race':bool(state),'age':int(time.monotonic()-room.seen)})
        data={'updated':int(time.time()),'online':[{'id':i,'name':names.get(i,'')} for i in ids],'rooms':rooms}
        path=Path(self.live_snapshot);temporary=path.with_name(path.name+'.tmp')
        temporary.write_text(json.dumps(data,ensure_ascii=False,separators=(',',':')),encoding='utf-8')
        os.replace(temporary,path)
    def connect_name(self, body):
        r=Reader(body)
        if r.u64()!=COMPAT:raise Invalid('client build/protocol is incompatible')
        name=r.text(15);r.end()
        if not name or any(ord(c)<32 or ord(c)==127 for c in name):raise Invalid('invalid player name')
        return name
    def verify_account(self, token):
        """Blocking (run in a thread): launcher token -> (account id, username)."""
        request=urllib.request.Request(self.account_api,data=json.dumps({'token':token}).encode(),method='POST',
            headers={'Content-Type':'application/json','X-Requested-With':'nobk','Accept-Language':'tr'})
        try:
            with urllib.request.build_opener(urllib.request.ProxyHandler({})).open(request,timeout=4) as reply:
                data=json.loads(reply.read(8192))
        except urllib.error.HTTPError as error:
            try:message=json.loads(error.read(2048)).get('error','')
            except ValueError:message=''
            raise Invalid(str(message or 'NOBK Online sign-in was rejected')[:120]) from None
        except (OSError,ValueError):raise Limited('NOBK Online account service unavailable') from None
        try:
            account=int(str(data['xuid']),16)-NOBK_XUID_BASE;name=str(data['gamertag'])
        except (KeyError,TypeError,ValueError):raise Limited('NOBK Online account service returned an invalid answer') from None
        if not 0<account<2**31:raise Limited('NOBK Online account service returned an invalid answer')
        return account,name
    @staticmethod
    def link_token(body):
        token=body.decode('ascii')
        if not 16<=len(token)<=128 or any(c not in LINK_TOKEN_CHARS for c in token):raise Invalid('invalid NOBK Online sign-in token')
        return token
    def handle(self, path, credential, body):
        if path == '/v2/connect':return self.connect(credential,self.connect_name(body))
        owner=self.store.owner(credential)
        if path == '/v2/disconnect':
            current=self.clients.get(PEER_BASE | owner)
            if len(body)!=16:raise Invalid('invalid ticket identifier')
            if current and hmac.compare_digest(current.ticket,body):self.remove(current.peer)
            return b''
        if path == '/v2/stats/write':
            r=Reader(body);nonce=r.text(32)
            if len(nonce)!=32 or any(c not in '0123456789abcdef' for c in nonce):raise Invalid('invalid write id')
            count=r.u32()
            if not 1<=count<=16:raise Invalid('invalid view count')
            views=[]
            for _ in range(count):
                view=r.u32();amount=r.u32()
                if not 1<=amount<=9:raise Invalid('invalid column count')
                columns={}
                for _ in range(amount):
                    column=r.u16();kind=r.u8();bits=r.raw(8)
                    if str(column) in columns or kind not in (1,2,3,5):raise Invalid('invalid column')
                    columns[str(column)]=[kind,bits.hex()]
                views.append((view,columns))
            r.end();self.store.write(owner,nonce,views);return b''
        if path == '/v2/stats/read':
            r=Reader(body);view,start,count,around=r.u32(),r.u32(),r.u32(),r.u64();amount=r.u32()
            if not 1<=count<=100 or amount>100:raise Invalid('invalid query range')
            users=[r.u64() for _ in range(amount)];r.end()
            total,rows=self.store.read(view,start,count,users,around);out=struct.pack('>II',total,len(rows))
            for peer,name,cols,rank in rows:
                out+=struct.pack('>QI',peer,rank)+text(name)+struct.pack('>I',len(cols))
                for column,(kind,bits) in cols.items():out+=struct.pack('>HB',int(column),kind)+bytes.fromhex(bits)
            if len(out)>MAX_BODY:raise Limited('response too large')
            return out
        raise Invalid('unknown endpoint')
    async def http(self, reader, writer):
        # Admission precedes header/body reads. Close overloaded sockets without
        # allocating another long-lived write or TLS shutdown coroutine.
        socket_ip=writer.get_extra_info('peername')[0]
        if self.http_connections>=self.max_http_connections:
            writer.transport.abort();return
        self.http_connections+=1
        address=None;admitted=False
        status,response=400,b'Invalid request'
        try:
            if not self.proxy_secret:
                address=network_key(socket_ip)
                if self.http_ips.get(address,0)>=self.max_http_per_ip:raise Limited('connection limit')
                self.http_ips[address]=self.http_ips.get(address,0)+1;admitted=True
            if not self.http_global.take():raise Limited('service request limit')
            header=await asyncio.wait_for(reader.readuntil(b'\r\n\r\n'),self.header_timeout)
            method,path,headers,size=parse_headers(header)
            if not self.proxy_secret and 'x-motogp-proxy-key' in headers:raise Invalid('configure --proxy-secret-file for this proxy')
            if self.proxy_secret:
                # Only a configured local reverse proxy may assert client IPs.
                if socket_ip not in ('127.0.0.1','::1') or not hmac.compare_digest(headers.get('x-motogp-proxy-key',''),self.proxy_secret):
                    raise Invalid('untrusted proxy')
                address=network_key(client_ip(headers.get('x-motogp-client-ip','')))
                if self.http_ips.get(address,0)>=self.max_http_per_ip:raise Limited('connection limit')
                self.http_ips[address]=self.http_ips.get(address,0)+1;admitted=True
            if not self.limits.take(('http',address),5,300):raise Limited('request limit')
            if method=='GET':
                status,response=200,json.dumps({'service':'motogp0910','protocol':'MGO2','players':len(self.clients),'rooms':len(self.rooms)}).encode()
            else:
                credential=credential_header(headers.get('authorization',''))
                owner_key=hashlib.sha256(credential).digest()
                if path=='/v2/link':
                    if not self.account_api:raise Invalid('NOBK Online accounts are not enabled on this server')
                    if not self.limits.take(('link',address),1/10,10) or not self.account_checks.take():raise Limited('sign-in request limit')
                elif path!='/v2/connect':
                    self.store.owner(credential)  # invented profiles never allocate state
                    rate,burst=(1,64) if path.endswith('/write') else (5,64)
                    if not self.profile_limits.take((path,owner_key),rate,burst):raise Limited('profile request limit')
                body=await asyncio.wait_for(reader.readexactly(size),self.body_timeout)
                if path=='/v2/connect':
                    self.connect_name(body)
                    known=self.store.player_for(owner_key.hex())
                    if known:
                        # Existing players behind one NAT must not spend the
                        # same scarce new-profile quota when reconnecting.
                        if not self.profile_limits.take(('connect-owner',owner_key),1,20):raise Limited('profile connection request limit')
                    elif not self.limits.take(('register',address),1/60,20) or not self.registrations.take():
                        raise Limited('registration limit')
                if path=='/v2/link':
                    # The site account answer arrives first, off the event loop, so
                    # a slow account service never delays the UDP relay.
                    account,username=await asyncio.to_thread(self.verify_account,self.link_token(body))
                if not self.db_cpu.take(0.001):raise Limited('storage work limit')
                began=time.monotonic();self.store.deadline=began+0.05
                try:
                    if path=='/v2/link':
                        self.store.link(credential,account,username);response=username.encode('utf-8')
                        LOG.info('Linked NOBK Online account %d',account)
                    else:response=self.handle(path,credential,body)
                finally:
                    self.store.deadline=0
                    self.db_cpu.tokens=max(0,self.db_cpu.tokens-(time.monotonic()-began))
                status=200
        except Limited as error:status,response=429,str(error).encode()[:128]
        except (ValueError,UnicodeError,asyncio.IncompleteReadError,asyncio.LimitOverrunError,TimeoutError) as error:
            response=(str(error) if isinstance(error,Invalid) else 'Invalid or incomplete request').encode('utf-8')[:128]
        except sqlite3.Error:
            # Disk quota/lock/query deadlines are overload, not an invitation to
            # expose internal SQL/path information or flood logs with traces.
            status,response=503,b'Storage busy or capacity reached'
        except (ConnectionError,OSError):return
        except Exception:
            LOG.exception('Unexpected service request failure');status,response=500,b'Service request failed'
        finally:
            try:
                names={200:'OK',400:'Bad Request',429:'Too Many Requests',500:'Internal Server Error',503:'Service Unavailable'}
                writer.write(f'HTTP/1.1 {status} {names[status]}\r\nContent-Type: application/octet-stream\r\nContent-Length: {len(response)}\r\nConnection: close\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\n\r\n'.encode()+response)
                await asyncio.wait_for(writer.drain(),self.write_timeout)
            except (ConnectionError,OSError,TimeoutError):pass
            finally:
                writer.close()
                try:await asyncio.wait_for(writer.wait_closed(),self.write_timeout)
                except (ConnectionError,OSError,TimeoutError):writer.transport.abort()
                self.http_connections-=1
                if admitted:
                    self.http_ips[address]-=1
                    if not self.http_ips[address]:self.http_ips.pop(address)

class ControlServer:
    """Reserve sockets before TLS, including clients that never send ClientHello.

    asyncio.start_server's application callback runs *after* TLS negotiation, so
    an application-only connection counter does not bound pending handshakes.
    """
    def __init__(self, service, host, port, ssl_context=None):
        self.service,self.ssl_context=service,ssl_context
        family,kind,protocol,_,address=socket.getaddrinfo(host,port,type=socket.SOCK_STREAM,flags=socket.AI_PASSIVE)[0]
        self.socket=socket.socket(family,kind,protocol)
        if os.name=='nt':self.socket.setsockopt(socket.SOL_SOCKET,socket.SO_EXCLUSIVEADDRUSE,1)
        else:self.socket.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
        self.socket.bind(address);self.socket.listen(128);self.socket.setblocking(False)
        self.tasks,self.addresses=set(),{}
    async def connection(self, connection, address):
        transport=None
        try:
            loop=asyncio.get_running_loop();reader=asyncio.StreamReader(limit=8192)
            protocol=asyncio.StreamReaderProtocol(reader)
            options={'ssl_handshake_timeout':5,'ssl_shutdown_timeout':3} if self.ssl_context else {}
            transport,_=await loop.connect_accepted_socket(lambda:protocol,connection,ssl=self.ssl_context,**options)
            writer=asyncio.StreamWriter(transport,protocol,reader,loop)
            await self.service.http(reader,writer)
        except (ConnectionError,OSError,TimeoutError):pass
        finally:
            if transport:transport.abort()
            else:connection.close()
            self.addresses[address]-=1
            if not self.addresses[address]:self.addresses.pop(address)
    async def serve_forever(self):
        loop=asyncio.get_running_loop()
        try:
            while True:
                connection,endpoint=await loop.sock_accept(self.socket);address=network_key(endpoint[0])
                per_ip=self.service.max_http_connections if self.service.proxy_secret else self.service.max_http_per_ip
                if len(self.tasks)>=self.service.max_http_connections or self.addresses.get(address,0)>=per_ip:
                    connection.close();await asyncio.sleep(0);continue
                self.addresses[address]=self.addresses.get(address,0)+1
                task=asyncio.create_task(self.connection(connection,address));self.tasks.add(task)
                task.add_done_callback(self.tasks.discard)
                await asyncio.sleep(0)
        finally:
            self.socket.close()
            for task in self.tasks:task.cancel()
            await asyncio.gather(*self.tasks,return_exceptions=True)

async def run(args):
    db=Path(args.database);db.parent.mkdir(parents=True,exist_ok=True)
    schemas=load_schemas(args.leaderboards)
    proxy_secret=read_proxy_secret(args.proxy_secret_file) if args.proxy_secret_file else None
    if proxy_secret and args.http_bind not in ('127.0.0.1','::1'):raise Invalid('proxy mode must bind to loopback')
    service=Service(Store(db,schemas,args.max_players,args.max_db_mib),args.udp_port,args.max_clients,args.max_rooms,proxy_secret)
    ssl_context=None
    if bool(args.cert)!=bool(args.key):raise Invalid('provide both --cert and --key')
    if args.cert and args.key:
        ssl_context=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);ssl_context.minimum_version=ssl.TLSVersion.TLSv1_2
        ssl_context.load_cert_chain(args.cert,args.key)
    elif args.http_bind not in ('127.0.0.1','::1'):
        raise Invalid('Public control API requires --cert and --key; use loopback behind an HTTPS reverse proxy')
    loop=asyncio.get_running_loop()
    transport,_=await loop.create_datagram_endpoint(lambda:service,local_addr=(args.udp_bind,args.udp_port))
    # A bounded queue absorbs short bursts without changing the protocol or
    # rejecting slow clients. OS policy may cap it: keep its working defaults
    # if tuning is unsupported rather than failing to start on Windows.
    udp_socket=transport.get_extra_info('socket')
    for option in (socket.SO_RCVBUF,socket.SO_SNDBUF):
        try:udp_socket.setsockopt(socket.SOL_SOCKET,option,1024*1024)
        except OSError:LOG.debug('OS kept the default UDP socket buffer')
    service.live_snapshot=args.live_snapshot
    service.account_api=args.account_api;service.require_account=args.require_account
    if args.require_account and not args.account_api:raise Invalid('--require-account needs --account-api')
    server=ControlServer(service,args.http_bind,args.http_port,ssl_context)
    task=asyncio.create_task(service.housekeeping())
    LOG.info('Control API %s:%s (%s), UDP relay %s:%s',args.http_bind,args.http_port,'TLS' if ssl_context else 'loopback HTTP',args.udp_bind,service.udp_port)
    try:
        await server.serve_forever()
    finally:
        task.cancel();transport.close();service.store.db.close()

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--http-bind',default='127.0.0.1');p.add_argument('--http-port',type=int,default=30820)
    p.add_argument('--udp-bind',default='0.0.0.0');p.add_argument('--udp-port',type=int,default=30821)
    p.add_argument('--database',default='data/community.sqlite3')
    p.add_argument('--leaderboards',default=str(Path(__file__).with_name('leaderboards.json')))
    p.add_argument('--account-api',help='NOBK Online launcher sign-in API on this machine, e.g. http://127.0.0.1:8090/api/account/launcher')
    p.add_argument('--require-account',action='store_true',help='Online play only for profiles linked to a NOBK Online account')
    p.add_argument('--live-snapshot',help='Write a public JSON summary of online players/rooms here every 5 s (for the website)')
    p.add_argument('--proxy-secret-file',help='Shared private key file; requires authenticated local Caddy proxy headers')
    p.add_argument('--max-players',type=int,default=10000);p.add_argument('--max-db-mib',type=int,default=256)
    p.add_argument('--cert');p.add_argument('--key');p.add_argument('--max-clients',type=int,default=256);p.add_argument('--max-rooms',type=int,default=128)
    args=p.parse_args()
    if not 1<=args.max_clients<=4096 or not 1<=args.max_rooms<=128:p.error('invalid service capacity')
    if not 1<=args.max_players<=100000 or not 16<=args.max_db_mib<=4096:p.error('invalid storage capacity')
    if not 0<=args.http_port<=65535 or not 0<=args.udp_port<=65535:p.error('invalid port')
    if os.name!='nt':os.umask(0o077)
    logging.basicConfig(level=logging.INFO,format='%(asctime)s %(levelname)s %(message)s')
    try:asyncio.run(run(args))
    except KeyboardInterrupt:pass

if __name__=='__main__':main()
