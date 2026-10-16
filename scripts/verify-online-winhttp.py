#!/usr/bin/env python3
"""Actual Windows WinHTTP negative checks; disposable CURRENT_USER test CA only.

The probe removes its CA in RAII cleanup. No production trust, game settings,
certificate verification flags or external network targets are changed.
"""
import argparse
import http.server
import json
import os
from pathlib import Path
import secrets
import socket
import ssl
import subprocess
import sys
import tempfile
import threading
import time
import urllib.request
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('build',type=Path);p.add_argument('--wine',type=Path);p.add_argument('--server-python',type=Path)
a=p.parse_args();repo=Path(__file__).resolve().parent.parent;build=a.build.resolve()
runner=[str(a.wine.resolve())] if a.wine else []
def guest(path):return 'Z:'+str(path).replace('/','\\') if a.wine else str(path)
server_python=str(a.server_python.resolve()) if a.server_python else sys.executable
server_runner=runner if a.server_python and a.server_python.suffix.lower()=='.exe' and a.wine else []
def server_path(path):return guest(path) if server_runner else str(path)
def port():
    with socket.socket() as s:s.bind(('127.0.0.1',0));return s.getsockname()[1]
with tempfile.TemporaryDirectory(prefix='motogp-winhttp-') as directory:
    root=Path(directory);process=None;servers=[];threads=[];captured=[]
    ca,ca_key,der=root/'ca.crt',root/'ca.key',root/'ca.der'
    cert,key,csr=root/'localhost.crt',root/'localhost.key',root/'localhost.csr'
    subprocess.run(['openssl','req','-x509','-newkey','rsa:2048','-nodes','-days','1','-subj','/CN=MotoGP-QA-'+secrets.token_hex(8),'-addext','basicConstraints=critical,CA:TRUE','-keyout',str(ca_key),'-out',str(ca)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    subprocess.run(['openssl','x509','-in',str(ca),'-outform','DER','-out',str(der)],check=True)
    subprocess.run(['openssl','req','-new','-newkey','rsa:2048','-nodes','-subj','/CN=localhost','-keyout',str(key),'-out',str(csr)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    extensions=root/'leaf.ext';extensions.write_text('basicConstraints=critical,CA:FALSE\nkeyUsage=critical,digitalSignature,keyEncipherment\nextendedKeyUsage=serverAuth\nsubjectAltName=DNS:localhost\n')
    subprocess.run(['openssl','x509','-req','-in',str(csr),'-CA',str(ca),'-CAkey',str(ca_key),'-CAcreateserial','-days','1','-extfile',str(extensions),'-out',str(cert)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    tls=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);tls.minimum_version=ssl.TLSVersion.TLSv1_2;tls.load_cert_chain(cert,key)
    class Capture(http.server.BaseHTTPRequestHandler):
        def do_POST(self):
            captured.append(True);self.send_response(200);self.send_header('Content-Length','0');self.end_headers()
        # A followed 302 can turn POST into GET; count either method so the
        # negative redirect assertion cannot pass by overlooking that request.
        do_GET=do_POST
        def log_message(self,*args):pass
    def start(handler):
        server=http.server.ThreadingHTTPServer(('127.0.0.1',0),handler)
        server.socket=tls.wrap_socket(server.socket,server_side=True)
        thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
        servers.append(server);threads.append(thread);return server.server_port
    capture_port=start(Capture)
    class Redirect(http.server.BaseHTTPRequestHandler):
        def do_POST(self):
            self.send_response(302);self.send_header('Location',f'https://localhost:{capture_port}/capture');self.send_header('Content-Length','0');self.end_headers()
        def log_message(self,*args):pass
    class Broken(http.server.BaseHTTPRequestHandler):
        def do_POST(self):
            self.send_response(200);self.send_header('Content-Length','3');self.end_headers();self.wfile.write(b'bad')
        def log_message(self,*args):pass
    redirect,broken=start(Redirect),start(Broken);good=port()
    log=(root/'backend.log').open('w+')
    try:
        process=subprocess.Popen(server_runner+[server_python,server_path(repo/'server/community_server.py'),'--http-bind','127.0.0.1','--http-port',str(good),'--udp-bind','127.0.0.1','--udp-port','0','--database',server_path(root/'db.sqlite3'),'--cert',server_path(cert),'--key',server_path(key)],stdout=log,stderr=subprocess.STDOUT,cwd=root)
        trusted=ssl.create_default_context(cafile=str(ca));opener=urllib.request.build_opener(urllib.request.ProxyHandler({}),urllib.request.HTTPSHandler(context=trusted))
        deadline=time.monotonic()+10
        while True:
            try:
                with opener.open(f'https://localhost:{good}/health',timeout=1) as response:assert json.load(response)['protocol']=='MGO2'
                break
            except OSError:
                if process.poll() is not None or time.monotonic()>deadline:raise RuntimeError('Owned TLS backend failed to start')
                time.sleep(0.05)
        command=runner+[str(build/'online_https_probe.exe'),f'https://localhost:{good}',f'https://127.0.0.1:{good}',f'https://localhost:{redirect}',guest(der),guest(root/'profiles'),f'https://localhost:{broken}']
        subprocess.run(command+['untrusted-only'],timeout=40,check=True,cwd=root)
        subprocess.run(command,timeout=40,check=True,cwd=root)
        if captured:raise AssertionError('Windows client followed cross-origin redirect')
        print('Redirect capture endpoint received zero requests; no credential forwarding occurred.')
    except BaseException:
        log.flush();log.seek(0);print(log.read(),file=sys.stderr);raise
    finally:
        if process:
            process.terminate()
            try:process.wait(timeout=5)
            except subprocess.TimeoutExpired:process.kill();process.wait()
        log.close()
        for server in servers:server.shutdown();server.server_close()
        for thread in threads:thread.join(timeout=2)
