#!/usr/bin/env python3
"""Read-only public JSON API for the NOBK Online website (MotoGP 09/10).

Separate from community_server.py on purpose: it opens the SQLite database
read-only, reads the live snapshot the game server writes, binds to loopback
and answers GET only. It never sees credentials, tickets, keys or addresses.
"""
from __future__ import annotations
import argparse
import json
import logging
import sqlite3
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlsplit

LOG = logging.getLogger('motogp-api')
PREFIX = '/api/motogp/'
STALE = 30  # seconds before a live snapshot counts as offline


# Native race boards store the rider's best lap in 1/60 s ticks, scaled by 100 and
# negated: rating = -(ticks * 100); column 2 holds the unscaled tick count.
# Checked against laps shown in game: Losail time trial 1:45.20 -> 6312,
# Mugello 250cc 1:50.87 -> 6652, Jerez 125cc 1:51.70 -> 6702.
def race_ms(value):
    return round(abs(int(value)) / 6)


def fmt_lap(value):
    centis = round(abs(int(value)) / 60)  # ticks*100/60 hundredths
    minutes, rest = divmod(centis, 6000)
    seconds, hundredths = divmod(rest, 100)
    return f'{minutes}:{seconds:02d}.{hundredths:02d}' if minutes else f'{seconds}.{hundredths:02d}'


def fmt_time(ms):
    ms = int(ms)
    minutes, rest = divmod(ms, 60000)
    seconds, millis = divmod(rest, 1000)
    return f'{minutes}:{seconds:02d}.{millis:03d}' if minutes else f'{seconds}.{millis:03d}'


class Api:
    def __init__(self, database, live, views):
        self.database, self.live, self.views_path = Path(database), Path(live), Path(views) if views else None
        self._views, self._views_mtime = {}, None
        self._seasons = None
        self.lock = threading.Lock()

    # -- helpers ---------------------------------------------------------
    def db(self):
        con = sqlite3.connect(f'file:{self.database}?mode=ro', uri=True, timeout=0.5)
        deadline = time.monotonic() + 0.5
        con.set_progress_handler(lambda: int(time.monotonic() > deadline), 2000)
        con.execute('PRAGMA trusted_schema=OFF')
        return con

    def view_meta(self):
        """Optional views.json: {"12": {"track": "Mugello", "class": "MotoGP", "mode": "Time Trial", "format": "time_ms"}}"""
        if not self.views_path or not self.views_path.exists():
            return {}
        mtime = self.views_path.stat().st_mtime
        with self.lock:
            if mtime != self._views_mtime:
                try:
                    data = json.loads(self.views_path.read_text(encoding='utf-8'))
                    self._views = {int(k): v for k, v in data.items() if str(k).isdecimal() and isinstance(v, dict)}
                except (OSError, ValueError):
                    LOG.exception('views.json unreadable; keeping previous labels')
                self._views_mtime = mtime
            return self._views

    def meta(self, view):
        m = dict(self.view_meta().get(view, {}))
        m['view'] = view
        return m

    def display(self, view, value):
        fmt = self.view_meta().get(view, {}).get('format')
        if fmt is None:
            fmt = 'time_ms' if value < 0 else 'score'
        if fmt == 'time_ms':
            return {'kind': 'time', 'text': fmt_lap(value), 'ms': race_ms(value)}
        return {'kind': 'score', 'text': f'{int(value):,}'.replace(',', '.')}

    @staticmethod
    def columns(raw):
        out = {}
        try:
            for key, (kind, bits) in json.loads(raw).items():
                if key.isdecimal() and len(bits) == 16:
                    out[key] = int.from_bytes(bytes.fromhex(bits), 'big', signed=True)
        except (ValueError, TypeError):
            pass
        return out

    def row(self, view, rank, pid, name, value, cols, updated, total=None, season=None):
        r = {'view': view, 'rank': rank, 'id': pid, 'name': name, 'value': value,
             'display': self.display(view, value), 'columns': self.columns(cols), 'updated': updated}
        if total is not None:
            r['total'] = total
        if season is not None:
            r.update(self.scope(view, season))
        return r

    # Season/class identity (launcher 1.2.0). Native class slot = ((view-1) % 63) // 21:
    # 2 MotoGP, 1 = 250cc in 2009 / Moto2 in 2010, 0 = 125cc. Season 0 = 2009, 1 = 2010.
    CATEGORIES = {1: 'MotoGP', 2: 'Moto2', 3: '250cc', 4: '125cc'}

    @classmethod
    def scope(cls, view, season):
        slot = ((view - 1) % 63) // 21
        category = 1 if slot == 2 else 4 if slot == 0 else (2 if season else 3)
        return {'season': 2009 + season, 'category': category, 'class': cls.CATEGORIES[category]}

    @staticmethod
    def season_mode(q):
        """'combined' (no parameter: one best row per rider and view, the old
        view-only identity for older consumers), 'all' (each season its own board)
        or 0 / 1 for season=2009 / season=2010."""
        if 'season' not in q:
            return 'combined'
        value = q['season'][0]
        if value == 'all':
            return 'all'
        if value in ('2009', '2010'):
            return int(value) - 2009
        raise ValueError('invalid season')

    def source(self, con, mode):
        """SQL rows (view, player, value, columns, updated, season) for a season mode."""
        if self._seasons is None or not self._seasons:
            self._seasons = bool(con.execute("SELECT 1 FROM sqlite_master WHERE type='table' AND name='season_scores'").fetchone())
        if not self._seasons:   # game server not upgraded yet: legacy store
            return 'SELECT view, player, value, columns, updated, NULL AS season FROM scores', 'view'
        base = 'SELECT view, player, value, columns, updated, season FROM season_scores'
        if mode == 'all':
            return base, 'view, season'
        if mode == 'combined':
            return ('SELECT view, player, value, columns, updated, season FROM (SELECT *, ROW_NUMBER() OVER '
                    '(PARTITION BY player, view ORDER BY value DESC, updated ASC) AS pick FROM season_scores) WHERE pick = 1'), 'view'
        return f'{base} WHERE season = {int(mode)}', 'view'

    # One finished race is written to several native groups (0, 2, 3, 5) of the same
    # track/class; the site lists only group 0 (view = 1 + group*63 + class*21 + track).
    MAIN = '(s.view - 1) / 63 = 0'

    def ranked(self, con, mode):
        src, part = self.source(con, mode)
        part = ', '.join('s.' + x.strip() for x in part.split(','))
        return f'''SELECT s.view, s.player, p.name, s.value, s.columns, s.updated,
        ROW_NUMBER() OVER (PARTITION BY {part} ORDER BY s.value DESC, s.updated ASC, s.player ASC) AS rank,
        COUNT(*) OVER (PARTITION BY {part}) AS total, s.season
        FROM ({src}) s JOIN players p ON p.id = s.player WHERE (s.view - 1) / 63 = 0'''

    def board_meta(self, view, season):
        m = self.meta(view)
        if season is not None:
            m.update(self.scope(view, season))
        return m

    def snapshot(self):
        try:
            data = json.loads(self.live.read_text(encoding='utf-8'))
        except (OSError, ValueError):
            return {'updated': 0, 'online': [], 'rooms': [], 'stale': True}
        data['stale'] = time.time() - data.get('updated', 0) > STALE
        if data['stale']:
            data['online'], data['rooms'] = [], []
        return data

    # -- endpoints -------------------------------------------------------
    def status(self, q):
        live = self.snapshot()
        mode = self.season_mode(q)
        with self.db() as con:
            src, part = self.source(con, 'all' if mode == 'combined' else mode)
            players = con.execute('SELECT COUNT(*) FROM players').fetchone()[0]
            records, last = con.execute(f'SELECT COUNT(*), MAX(updated) FROM ({src}) s WHERE {self.MAIN}').fetchone()
            racers = con.execute(f'SELECT COUNT(DISTINCT player) FROM ({src})').fetchone()[0]
            views = con.execute(f'SELECT COUNT(*) FROM (SELECT DISTINCT {part} FROM ({src}) s WHERE {self.MAIN})').fetchone()[0]
        return {'online': len(live['online']), 'rooms': len(live['rooms']),
                'in_race': sum(1 for r in live['rooms'] if r.get('in_race')),
                'players': players, 'racers': racers, 'records': records, 'boards': views,
                'last_record': last or 0, 'server_up': not live['stale'], 'updated': int(time.time())}

    def live_ep(self, q):
        return self.snapshot()

    def views(self, q):
        mode = self.season_mode(q)
        with self.db() as con:
            rows = con.execute(f'SELECT * FROM ({self.ranked(con, mode)}) WHERE rank = 1 ORDER BY view, season').fetchall()
        keep = mode != 'combined'
        return {'views': [{**self.board_meta(v, season if keep else None), 'records': total,
                           'best': self.row(v, rank, pid, name, value, cols, upd, season=season)}
                          for v, pid, name, value, cols, upd, rank, total, season in rows]}

    def leaderboard(self, q):
        view = self.int(q, 'view', 1, 1, 0xFFFFFFFF)
        limit = self.int(q, 'limit', 50, 1, 100)
        offset = self.int(q, 'offset', 0, 0, 1_000_000)
        mode = self.season_mode(q)
        if mode == 'all':
            raise ValueError('choose season=2009 or season=2010 for one board')
        with self.db() as con:
            src, _ = self.source(con, mode)
            total = con.execute(f'SELECT COUNT(*) FROM ({src}) WHERE view=?', (view,)).fetchone()[0]
            rows = con.execute(f'''SELECT s.player, p.name, s.value, s.columns, s.updated, s.season FROM ({src}) s
                JOIN players p ON p.id = s.player WHERE s.view=?
                ORDER BY s.value DESC, s.updated ASC, s.player ASC LIMIT ? OFFSET ?''', (view, limit, offset)).fetchall()
        best = rows[0][2] if rows and offset == 0 else None
        out = []
        for i, (pid, name, value, cols, upd, season) in enumerate(rows):
            r = self.row(view, offset + i + 1, pid, name, value, cols, upd, season=season)
            if best is not None and r['display']['kind'] == 'time' and i:
                r['gap_ms'] = race_ms(value - best)
            out.append(r)
        return {**self.board_meta(view, mode if isinstance(mode, int) else None), 'total': total, 'offset': offset, 'rows': out}

    def player(self, q):
        mode = self.season_mode(q)
        with self.db() as con:
            if 'id' in q:
                found = con.execute('SELECT id, name FROM players WHERE id=?', (self.int(q, 'id', 0, 1, 0xFFFFFF),)).fetchone()
            else:
                name = self.text(q, 'name')
                src, _ = self.source(con, 'combined')
                found = con.execute(f'''SELECT p.id, p.name FROM players p LEFT JOIN ({src}) s ON s.player=p.id
                    WHERE p.name=? GROUP BY p.id ORDER BY COUNT(s.view) DESC, MAX(s.updated) DESC LIMIT 1''', (name,)).fetchone()
            if not found:
                raise LookupError('player not found')
            pid, name = found
            rows = con.execute(f'SELECT * FROM ({self.ranked(con, mode)}) WHERE player=? ORDER BY rank, updated DESC', (pid,)).fetchall()
        records = [{**self.meta(v), **self.row(v, rank, p, n, value, cols, upd, total, season)} for v, p, n, value, cols, upd, rank, total, season in rows]
        online = any(o['id'] == pid for o in self.snapshot()['online'])
        return {'id': pid, 'name': name, 'online': online, 'records': records,
                'firsts': sum(r['rank'] == 1 for r in records), 'podiums': sum(r['rank'] <= 3 for r in records),
                'last': max((r['updated'] for r in records), default=0)}

    def players(self, q):
        term = self.text(q, 'q', required=False)
        like = '%' + term.replace('\\', '\\\\').replace('%', '\\%').replace('_', '\\_') + '%'
        with self.db() as con:
            src, _ = self.source(con, 'combined')
            rows = con.execute(f'''SELECT p.id, p.name, COUNT(s.view) FROM players p LEFT JOIN ({src}) s ON s.player=p.id
                WHERE p.name LIKE ? ESCAPE '\\' GROUP BY p.id ORDER BY COUNT(s.view) DESC, p.name LIMIT 20''', (like,)).fetchall()
        return {'players': [{'id': i, 'name': n, 'records': c} for i, n, c in rows]}

    def recent(self, q):
        limit = self.int(q, 'limit', 20, 1, 50)
        mode = self.season_mode(q)
        with self.db() as con:
            rows = con.execute(f'SELECT * FROM ({self.ranked(con, mode)}) ORDER BY updated DESC, player LIMIT ?', (limit,)).fetchall()
        return {'records': [{**self.meta(v), **self.row(v, rank, p, n, value, cols, upd, total, season)} for v, p, n, value, cols, upd, rank, total, season in rows]}

    def ranking(self, q):
        mode = self.season_mode(q)
        with self.db() as con:
            rows = con.execute(f'''SELECT player, name, COUNT(*), SUM(rank=1), SUM(rank<=3), MAX(updated),
                SUM(total - rank + 1) FROM ({self.ranked(con, mode)}) GROUP BY player
                ORDER BY SUM(rank=1) DESC, SUM(rank<=3) DESC, SUM(total - rank + 1) DESC, COUNT(*) DESC, MIN(updated) LIMIT 100''').fetchall()
        online = {o['id'] for o in self.snapshot()['online']}
        return {'players': [{'pos': i + 1, 'id': pid, 'name': n, 'records': c, 'firsts': f, 'podiums': pd,
                             'points': pts, 'last': last, 'online': pid in online}
                            for i, (pid, n, c, f, pd, last, pts) in enumerate(rows)]}

    ROUTES = {'status': status, 'live': live_ep, 'views': views, 'leaderboard': leaderboard,
              'player': player, 'players': players, 'recent': recent, 'ranking': ranking}

    @staticmethod
    def int(q, key, default, low, high):
        if key not in q:
            return default
        v = q[key][0]
        if not v.isdecimal() or len(v) > 10 or not low <= int(v) <= high:
            raise ValueError(f'invalid {key}')
        return int(v)

    @staticmethod
    def text(q, key, required=True):
        v = q.get(key, [''])[0]
        if (required and not v) or len(v.encode('utf-8')) > 32 or any(ord(c) < 32 for c in v):
            raise ValueError(f'invalid {key}')
        return v


def handler_for(api):
    class Handler(BaseHTTPRequestHandler):
        server_version = 'motogp-api'
        sys_version = ''
        timeout = 5

        def log_message(self, *args):
            pass

        def send(self, status, data, cache=5):
            body = json.dumps(data, ensure_ascii=False, separators=(',', ':')).encode('utf-8')
            self.send_response(status)
            self.send_header('Content-Type', 'application/json; charset=utf-8')
            self.send_header('Content-Length', str(len(body)))
            self.send_header('Cache-Control', f'public, max-age={cache}' if status == 200 else 'no-store')
            self.send_header('X-Content-Type-Options', 'nosniff')
            self.end_headers()
            if self.command != 'HEAD':
                self.wfile.write(body)

        def do_GET(self):
            parts = urlsplit(self.path)
            if not parts.path.startswith(PREFIX) or len(self.path) > 512:
                return self.send(404, {'error': 'not found'})
            route = Api.ROUTES.get(parts.path[len(PREFIX):].strip('/'))
            if not route:
                return self.send(404, {'error': 'not found'})
            try:
                q = parse_qs(parts.query, keep_blank_values=True, max_num_fields=10)
                self.send(200, route(api, q))
            except LookupError as e:
                self.send(404, {'error': str(e)})
            except ValueError as e:
                self.send(400, {'error': str(e)})
            except sqlite3.Error:
                LOG.exception('database read failed')
                self.send(503, {'error': 'temporarily unavailable'})

        do_HEAD = do_GET

        def do_POST(self):
            self.send(405, {'error': 'read only'})
        do_PUT = do_DELETE = do_PATCH = do_POST
    return Handler


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--database', default='/var/lib/motogp-online/community.sqlite3')
    p.add_argument('--live', default='/var/lib/motogp-online/live.json')
    p.add_argument('--views', default=str(Path(__file__).with_name('views.json')))
    p.add_argument('--bind', default='127.0.0.1')
    p.add_argument('--port', type=int, default=8095)
    args = p.parse_args()
    if args.bind not in ('127.0.0.1', '::1'):
        raise SystemExit('bind to loopback; publish through Caddy')
    logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
    server = ThreadingHTTPServer((args.bind, args.port), handler_for(Api(args.database, args.live, args.views)))
    server.daemon_threads = True
    LOG.info('MotoGP read-only API on %s:%s', args.bind, args.port)
    server.serve_forever()


if __name__ == '__main__':
    main()
