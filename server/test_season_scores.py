import asyncio
import json
import secrets
import sqlite3
import struct
import tempfile
import unittest
from pathlib import Path
from community_server import *

SCHEMAS = load_schemas(Path(__file__).with_name('leaderboards.json'))

def cols(value, season=None):
    out = {'3': [2, value.to_bytes(8, 'big', signed=True).hex()], '2': [1, (abs(value)//100).to_bytes(8, 'big').hex()]}
    if season is not None: out['13'] = [1, season.to_bytes(8, 'big').hex()]
    return out

VIEW_MID = 1 + 1*21 + 4   # group 0, middle class slot (250cc / Moto2), Mugello
VIEW_TOP = 1 + 2*21 + 4   # MotoGP, Mugello

class SeasonStoreTests(unittest.TestCase):
    def setUp(self):
        self.store = Store(':memory:', SCHEMAS)
        self.a = self.store.connect(b'A'*32, 'A') & 0xFFFFFF
        self.b = self.store.connect(b'B'*32, 'B') & 0xFFFFFF
    def test_250cc_2009_and_moto2_2010_coexist_for_the_same_rider_and_view(self):
        self.store.write_scoped(self.a, 'a'*32, 3, 0, [(VIEW_MID, cols(-670000, 0))])
        self.store.write_scoped(self.a, 'a'*32, 2, 1, [(VIEW_MID, cols(-650000, 1))])   # same nonce, other scope group
        t09, r09 = self.store.read(VIEW_MID, 1, 10, season=0)
        t10, r10 = self.store.read(VIEW_MID, 1, 10, season=1)
        self.assertEqual((t09, t10), (1, 1))
        self.assertEqual(int(r09[0][2]['3'][1], 16), (-670000) & (2**64-1))
        self.assertEqual(int(r10[0][2]['3'][1], 16), (-650000) & (2**64-1))
    def test_best_kept_per_scope_and_replay_is_per_scope(self):
        self.store.write_scoped(self.a, 'a'*32, 1, 1, [(VIEW_TOP, cols(-600000, 1))])
        self.store.write_scoped(self.a, 'b'*32, 1, 1, [(VIEW_TOP, cols(-700000, 1))])   # slower: kept best
        self.store.write_scoped(self.a, 'b'*32, 1, 1, [(VIEW_TOP, cols(-500000, 1))])   # replayed nonce: ignored
        self.assertEqual(self.store.db.execute('SELECT value FROM season_scores').fetchall(), [(-600000,)])
        self.store.write_scoped(self.b, 'c'*32, 1, 0, [(VIEW_TOP, cols(-500000, 0))])
        self.assertEqual(self.store.read(VIEW_TOP, 1, 10, season=1)[0], 1)
    def test_rejects_forged_or_mismatched_scope(self):
        with self.assertRaises(Invalid): self.store.write_scoped(self.a, 'a'*32, 2, 0, [(VIEW_MID, cols(-1, 0))])   # Moto2 2009
        with self.assertRaises(Invalid): self.store.write_scoped(self.a, 'a'*32, 3, 1, [(VIEW_MID, cols(-1, 1))])   # 250cc 2010
        with self.assertRaises(Invalid): self.store.write_scoped(self.a, 'a'*32, 1, 0, [(VIEW_MID, cols(-1, 0))])   # wrong slot
        with self.assertRaises(Invalid): self.store.write_scoped(self.a, 'a'*32, 3, 0, [(VIEW_MID, cols(-1, 1))])   # property 13 differs
        with self.assertRaises(Invalid): self.store.write_scoped(self.a, 'a'*32, 3, 0, [(VIEW_MID, cols(-1))])      # no property 13
        self.assertEqual(self.store.db.execute('SELECT COUNT(*) FROM season_scores').fetchone()[0], 0)
    def test_legacy_writes_stay_legacy_and_mirror_known_seasons(self):
        self.store.write(self.a, 'a'*32, [(VIEW_MID, cols(-660000, 1)), (VIEW_TOP, cols(-610000))])
        self.assertEqual(self.store.read(VIEW_MID, 1, 10)[0], 1)              # legacy board
        self.assertEqual(self.store.read(VIEW_MID, 1, 10, season=1)[0], 1)    # mirrored (2010 Moto2)
        self.assertEqual(self.store.read(VIEW_TOP, 1, 10, season=0)[0], 0)    # no season: legacy only
    def test_migration_copies_known_seasons_once_and_keeps_legacy(self):
        with tempfile.TemporaryDirectory() as folder:
            path = str(Path(folder)/'db.sqlite3'); store = Store(path, SCHEMAS)
            a = store.connect(b'A'*32, 'A') & 0xFFFFFF
            store.db.execute("DROP TABLE season_migrations"); store.db.execute("DELETE FROM season_scores")
            with store.db:
                store.db.execute('INSERT INTO scores VALUES(?,?,?,?,?)', (a, VIEW_MID, -1, json.dumps(cols(-1, 0)), 100))
                store.db.execute('INSERT INTO scores VALUES(?,?,?,?,?)', (a, VIEW_TOP, -2, json.dumps(cols(-2)), 101))
            store.db.close()
            store = Store(path, SCHEMAS)
            self.assertEqual(store.db.execute('SELECT player,view,season,value,updated,source FROM season_scores').fetchall(),
                             [(a, VIEW_MID, 0, -1, 100, 'legacy-v1')])
            self.assertEqual(store.db.execute('SELECT copied,legacy_only FROM season_migrations').fetchone(), (1, 1))
            self.assertEqual(store.db.execute('SELECT COUNT(*) FROM scores').fetchone()[0], 2)
            store.db.close(); store = Store(path, SCHEMAS)      # repeatable: nothing copied twice
            self.assertEqual(store.db.execute('SELECT COUNT(*) FROM season_scores').fetchone()[0], 1); store.db.close()

class SeasonHttpTests(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        self.service = Service(Store(':memory:', SCHEMAS), 30821)
        self.server = await asyncio.start_server(self.service.http, '127.0.0.1', 0, limit=8192)
        self.port = self.server.sockets[0].getsockname()[1]
        self.addCleanup(self.service.store.db.close)
    async def asyncTearDown(self): self.server.close(); await self.server.wait_closed()
    async def post(self, path, key, body):
        r, w = await asyncio.open_connection('127.0.0.1', self.port)
        w.write(f'POST {path} HTTP/1.1\r\nHost: x\r\nAuthorization: Bearer {key.hex()}\r\nContent-Length: {len(body)}\r\n\r\n'.encode() + body); await w.drain()
        answer = await r.read(); w.close(); await w.wait_closed(); head, _, rest = answer.partition(b'\r\n\r\n'); return int(head.split()[1]), rest
    async def test_capabilities_scoped_write_and_read(self):
        key = secrets.token_bytes(32); self.service.store.connect(key, 'A')
        self.assertEqual(await self.post('/v2/stats/scoped/capabilities', key, b''), (200, b'{"supported": true, "protocol": 1}'))
        def write_body(prefix, season, value):
            body = bytes(prefix) + text('e'*32) + struct.pack('>I', 1) + struct.pack('>II', VIEW_MID, 3)
            for column, kind, bits in ((3, 2, value & (2**64-1)), (2, 1, abs(value)//100), (13, 1, season)):
                body += struct.pack('>HBQ', column, kind, bits)
            return body
        self.assertEqual((await self.post('/v2/stats/scoped/write', key, write_body([1, 2, 1, 0], 1, -650000)))[0], 200)
        self.assertEqual((await self.post('/v2/stats/scoped/write', key, write_body([1, 3, 0, 0], 0, -670000)))[0], 200)
        self.assertEqual((await self.post('/v2/stats/scoped/write', key, write_body([1, 2, 0, 0], 0, -1)))[0], 400)
        self.assertEqual((await self.post('/v2/stats/scoped/write', key, write_body([2, 2, 1, 0], 1, -1)))[0], 400)
        read = struct.pack('>IIIQI', VIEW_MID, 1, 10, 0, 0)
        status, page = await self.post('/v2/stats/scoped/read', key, bytes([1, 2, 1, 0]) + read)
        self.assertEqual(status, 200); self.assertEqual(struct.unpack('>II', page[:8]), (1, 1))
        status, page = await self.post('/v2/stats/scoped/read', key, bytes([1, 1, 1, 0]) + read)   # MotoGP scope, middle-slot view
        self.assertEqual(status, 400)

if __name__ == '__main__': unittest.main()
