#!/usr/bin/env python3
"""Run the real Windows control/relay and generated-title stats clients locally."""
import argparse
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import time
import urllib.request

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('build',type=Path);p.add_argument('--wine',type=Path);p.add_argument('--game-root',type=Path,required=True)
p.add_argument('--server-python',type=Path,help='Optional alternate Python, including a Windows python.exe with --wine')
a=p.parse_args();build=a.build.resolve(strict=True);repo=Path(__file__).resolve().parent.parent
runner=[str(a.wine.resolve(strict=True))] if a.wine else []
def guest(path):return 'Z:'+str(path).replace('/','\\') if a.wine else str(path)
def port(kind):
    with socket.socket(socket.AF_INET,kind) as s:s.bind(('127.0.0.1',0));return s.getsockname()[1]
http_port,udp_port=port(socket.SOCK_STREAM),port(socket.SOCK_DGRAM);url=f'http://127.0.0.1:{http_port}'
server_python=str(a.server_python.resolve(strict=True)) if a.server_python else sys.executable
server_runner=runner if a.server_python and a.server_python.suffix.lower()=='.exe' and a.wine else []
def server_path(path):return guest(path) if server_runner else str(path)
with tempfile.TemporaryDirectory(prefix='motogp-online-') as directory:
    root=Path(directory).resolve()
    for iteration in range(2):
        with (root/f'server-{iteration}.log').open('w+') as log:
            server=subprocess.Popen(server_runner+[server_python,server_path(repo/'server/community_server.py'),'--http-port',str(http_port),'--udp-port',str(udp_port),'--database',server_path(root/'server.sqlite3')],stdout=log,stderr=subprocess.STDOUT,cwd=root)
            try:
                deadline=time.monotonic()+10
                while True:
                    try:
                        with urllib.request.urlopen(url+'/health',timeout=1) as response:
                            if response.status==200:break
                    except OSError:
                        if server.poll() is not None or time.monotonic()>=deadline:raise RuntimeError('Local community service failed to start')
                        time.sleep(0.05)
                subprocess.run(runner+[str(build/'online_service_probe.exe'),url,guest(root/'profiles')],check=True,timeout=40,cwd=root)
                if iteration==0:subprocess.run(runner+[str(build/'online_guest_probe.exe'),guest(a.game_root.resolve(strict=True)),url],check=True,timeout=40,cwd=root)
                print('Local service restart/persistent results: passed' if iteration else 'Real Windows clients and original native stats ABI: passed',flush=True)
            finally:
                server.terminate()
                try:server.wait(timeout=5)
                except subprocess.TimeoutExpired:server.kill();server.wait()
                log.seek(0);print(log.read(),end='')
print('These checks do not replace an internet race or native Windows GPU/UI validation.')
