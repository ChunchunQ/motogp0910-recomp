#!/usr/bin/env python3
"""Owned loopback HTTPS/proxy checks; requires local Caddy and OpenSSL, no VDS."""
import argparse
import json
import os
from pathlib import Path
import secrets
import socket
import ssl
import struct
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--caddy',type=Path,required=True);p.add_argument('--openssl',default='openssl')
p.add_argument('--server-python',type=Path);p.add_argument('--wine',type=Path)
a=p.parse_args();root=Path(__file__).resolve().parent.parent
runner=[str(a.wine.resolve(strict=True))] if a.wine else []
server_python=str(a.server_python.resolve(strict=True)) if a.server_python else sys.executable
server_runner=runner if a.server_python and a.server_python.suffix.lower()=='.exe' and a.wine else []
def server_path(path):return 'Z:'+str(path).replace('/','\\') if server_runner else str(path)
def port():
    with socket.socket() as s:s.bind(('127.0.0.1',0));return s.getsockname()[1]
def request(url,context=None,body=None,headers=None):
    opener=urllib.request.build_opener(urllib.request.ProxyHandler({}),urllib.request.HTTPSHandler(context=context))
    try:
        with opener.open(urllib.request.Request(url,data=body,headers=headers or {}),timeout=2) as r:return r.status,r.read()
    except urllib.error.HTTPError as error:return error.code,error.read()
def ready(url,context,process):
    deadline=time.monotonic()+10;last='no response'
    while time.monotonic()<deadline:
        if process.poll() is not None:raise RuntimeError('Test service exited; inspect the local test logs')
        try:
            status,body=request(url,context);last=f'HTTP {status}: '+body[:128].decode('utf-8','replace')
            if status==200:return
        except (OSError,urllib.error.URLError) as error:last=type(error).__name__+': '+str(error)
        time.sleep(0.05)
    raise RuntimeError('Local test service did not become ready: '+last)
checks=0
def check(value,message):
    global checks
    if not value:raise AssertionError(message)
    checks+=1;print(message,flush=True)
with tempfile.TemporaryDirectory(prefix='motogp-security-') as folder:
    directory=Path(folder);processes=[];logs=[]
    try:
        cert,key=directory/'localhost.crt',directory/'localhost.key'
        subprocess.run([a.openssl,'req','-x509','-newkey','rsa:2048','-nodes','-days','1','-subj','/CN=localhost','-addext','subjectAltName=DNS:localhost,IP:127.0.0.1','-keyout',str(key),'-out',str(cert)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
        trusted=ssl.create_default_context(cafile=str(cert))
        proxy_key=directory/'proxy.key';proxy_key.write_text(secrets.token_hex(32)+'\n');proxy_key.chmod(0o600)
        backend,frontend,direct=port(),port(),port()
        def start(command,name,env=None):
            log=(directory/name).open('w+');logs.append(log)
            process=subprocess.Popen(command,cwd=directory,env=env,stdout=log,stderr=subprocess.STDOUT);processes.append(process);return process
        command=server_runner+[server_python,server_path(root/'server/community_server.py')]
        proxy_server=start(command+['--http-port',str(backend),'--udp-bind','127.0.0.1','--udp-port','0','--database',server_path(directory/'proxy.sqlite3'),'--proxy-secret-file',server_path(proxy_key)],'backend.log')
        # Derive the tested configuration from the actual shipped template.
        config=(root/'server/Caddyfile.example').read_text()
        config=config.replace('play.example.com',f'https://localhost:{frontend}').replace('127.0.0.1:30820',f'127.0.0.1:{backend}')
        config=config.replace('admin off','admin off\n    auto_https disable_redirects')
        config=config.replace(f'https://localhost:{frontend} {{',f'https://localhost:{frontend} {{\n    tls {json.dumps(str(cert))} {json.dumps(str(key))}')
        config_path=directory/'Caddyfile';config_path.write_text(config)
        environment=os.environ.copy();environment['MOTOGP_PROXY_SECRET']=proxy_key.read_text().strip()
        environment['XDG_CONFIG_HOME']=str(directory/'config');environment['XDG_DATA_HOME']=str(directory/'cache')
        caddy=start([str(a.caddy.resolve(strict=True)),'run','--config',str(config_path),'--adapter','caddyfile'],'caddy.log',environment)
        url=f'https://localhost:{frontend}';ready(url+'/health',trusted,caddy)
        status,body=request(url+'/health',trusted)
        check(status==200 and json.loads(body)['protocol']=='MGO2','HTTPS through shipped Caddy template reaches authenticated MGO2 backend')
        try:request(url+'/health');raise AssertionError('Untrusted TLS certificate was accepted')
        except urllib.error.URLError as error:check(isinstance(error.reason,ssl.SSLCertVerificationError),'Untrusted certificate is rejected without disabling TLS verification')
        binary=struct.pack('>QH',0x4D4750394C414E02,4)+b'Test'
        headers={'Authorization':'Bearer '+secrets.token_hex(32),'Content-Type':'application/octet-stream','X-MotoGP-Proxy-Key':'attacker','X-MotoGP-Client-IP':'203.0.113.9'}
        status,body=request(url+'/v2/connect',trusted,binary,headers)
        check(status==200 and len(body)==58,'Caddy overwrites forged proxy key and client IP before forwarding')
        check(request(f'http://127.0.0.1:{backend}/health')[0]==400,'Direct backend request without proxy key is rejected')
        check(request(f'http://127.0.0.1:{backend}/health',headers={'X-MotoGP-Proxy-Key':'0'*64,'X-MotoGP-Client-IP':'127.0.0.1'})[0]==400,'Wrong proxy key is rejected')
        check(request(url+'/../../etc/passwd',trusted)[0]==404,'Unexpected paths are rejected by proxy allowlist')
        check(request(url+'/v1/connect',trusted,binary,headers)[0]==404,'Old Online API cannot bypass v2 protections')
        check(request(url+'/v2/stats/write',trusted,b'X'*2049,headers)[0] in (400,413),'Oversized request is rejected by the shipped front/back limits')
        direct_server=start(command+['--http-port',str(direct),'--udp-bind','127.0.0.1','--udp-port','0','--database',server_path(directory/'direct.sqlite3'),'--cert',server_path(cert),'--key',server_path(key)],'tls.log')
        direct_url=f'https://localhost:{direct}';ready(direct_url+'/health',trusted,direct_server)
        check(request(direct_url+'/health',trusted)[0]==200,'Direct native Python TLS listener works with pre-handshake socket admission')
        check(request(direct_url+'/v2/connect',trusted,binary,{'Authorization':'Bearer '+secrets.token_hex(32),'Content-Type':'application/octet-stream'})[0]==200,'Native TLS listener completes real v2 handshake')
        check(not (directory/'config'/'caddy'/'autosave.json').exists(),'Caddy does not persist the expanded private proxy key in autosave JSON')
        check(all(process.poll() is None for process in processes),'Both server processes and Caddy survive invalid requests')
    except BaseException:
        # Logs contain no request authorization/proxy values. Never print the
        # adapted Caddy JSON or the environment containing the private key.
        for log in logs:log.flush();log.seek(0);print(log.read(),file=sys.stderr)
        raise
    finally:
        for process in reversed(processes):
            process.terminate()
            try:process.wait(timeout=5)
            except subprocess.TimeoutExpired:process.kill();process.wait()
        for log in logs:log.close()
print(f'{checks} real HTTPS/proxy/deployment checks passed; public VDS not exercised.')
