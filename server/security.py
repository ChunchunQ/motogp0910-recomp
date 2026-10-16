"""Bounded admission and strict HTTP framing for the community service."""
from __future__ import annotations
from collections import OrderedDict
import ipaddress
import re
import time

class Invalid(ValueError): pass
class Limited(Invalid): pass

class Bucket:
    def __init__(self, rate, burst):
        self.rate, self.burst, self.tokens, self.at = rate, burst, float(burst), time.monotonic()
    def take(self, amount=1):
        now = time.monotonic()
        self.tokens = min(self.burst, self.tokens + max(0, now-self.at)*self.rate)
        self.at = now
        if self.tokens < amount: return False
        self.tokens -= amount
        return True

class Limits:
    """Do not evict live entries: address churn must not reset active quotas."""
    def __init__(self, capacity=4096): self.entries, self.capacity = OrderedDict(), capacity
    def prune(self):
        now = time.monotonic()
        while self.entries and now-next(iter(self.entries.values())).at > 3600:
            self.entries.popitem(last=False)
    def take(self, key, rate, burst):
        entry = self.entries.get(key)
        if entry is None:
            self.prune()
            if len(self.entries) >= self.capacity: return False
            entry = Bucket(rate, burst); self.entries[key] = entry
        self.entries.move_to_end(key)
        return entry.take()

_TOKEN = re.compile(r"[!#$%&'*+.^_`|~0-9A-Za-z-]+\Z")
_DIGITS = re.compile(r'[0-9]+\Z')
_HEX = re.compile(r'[0-9a-fA-F]{64}\Z')

def parse_headers(data):
    if len(data) > 8192 or not data.endswith(b'\r\n\r\n'): raise Invalid('invalid headers')
    lines = data[:-4].decode('ascii').split('\r\n')
    if len(lines) > 65: raise Invalid('too many headers')
    parts = lines[0].split(' ')
    if len(parts) != 3 or parts[2] != 'HTTP/1.1': raise Invalid('HTTP/1.1 required')
    method, path, _ = parts
    headers = {}
    for line in lines[1:]:
        key, separator, value = line.partition(':')
        if not separator or not _TOKEN.fullmatch(key) or any(ord(c)<32 or ord(c)==127 for c in value):
            raise Invalid('invalid header field')
        key = key.lower()
        if key in headers: raise Invalid('duplicate header')
        headers[key] = value.strip(' ')
    if not headers.get('host') or 'transfer-encoding' in headers or 'expect' in headers:
        raise Invalid('invalid request framing')
    if method == 'GET' and path == '/health':
        if headers.get('content-length', '0') != '0': raise Invalid('unexpected request body')
        return method, path, headers, 0
    if method != 'POST': raise Invalid('POST required')
    if path.startswith('/v1/'): raise Invalid('Update the game and server together: Online protocol v2 required')
    if path not in ('/v2/connect','/v2/disconnect','/v2/stats/read','/v2/stats/write'): raise Invalid('unknown endpoint')
    length = headers.get('content-length', '')
    if not _DIGITS.fullmatch(length) or len(length)>5: raise Invalid('Content-Length required')
    size = int(length)
    # Native stats: at most sixteen views, nine columns per view.
    maximum = {'/v2/connect':25, '/v2/disconnect':16, '/v2/stats/read':824, '/v2/stats/write':1750}[path]
    if size > maximum: raise Invalid('request too large')
    if headers.get('content-type','application/octet-stream').lower() != 'application/octet-stream':
        raise Invalid('binary content required')
    return method, path, headers, size

def credential_header(value):
    if not value.startswith('Bearer ') or not _HEX.fullmatch(value[7:]): raise Invalid('invalid credential')
    return bytes.fromhex(value[7:])

def client_ip(value):
    try: return str(ipaddress.ip_address(value))
    except ValueError: raise Invalid('invalid client address') from None

def read_proxy_secret(path):
    with open(path, 'rb') as source: raw = source.read(67)
    if len(raw)>66: raise Invalid('proxy key file is too large')
    value=raw.strip()
    if not _HEX.fullmatch(value.decode('ascii')): raise Invalid('proxy key must contain exactly 64 hexadecimal characters')
    return value.decode('ascii').lower()
