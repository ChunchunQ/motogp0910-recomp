#!/usr/bin/env python3
"""Bounded abuse/liveness experiment against a temporary owned loopback server."""
import argparse,asyncio,hmac,json,os
import http.client as http_client
from pathlib import Path
import secrets,socket,struct,subprocess,sys,tempfile,time
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--seconds',type=int,default=12);p.add_argument('--packets-per-second',type=int,default=4000)
p.add_argument('--server-python',type=Path);p.add_argument('--wine',type=Path)
a=p.parse_args()
if not 2<=a.seconds<=60 or not 100<=a.packets_per_second<=10000:p.error('bounded local test size required')
repo=Path(__file__).resolve().parent.parent;sys.path.insert(0,str(repo/'server'))
from community_server import ENVELOPE,ONLINE_MAGIC,COMPAT,PEER_BASE,packet,decode,DISCOVER,ADVERT,JOIN,JOINED,HEARTBEAT,DATA,text
from test_community_server import advert
runner=[str(a.wine.resolve())] if a.wine else []
server_runner=runner if a.server_python and a.server_python.suffix.lower()=='.exe' and a.wine else []
server_python=str(a.server_python.resolve()) if a.server_python else sys.executable
def guest(path):return 'Z:'+str(path).replace('/','\\') if server_runner else str(path)
def port():
    with socket.socket() as s:s.bind(('127.0.0.1',0));return s.getsockname()[1]
async def experiment(http_port,udp_port,process):
    metrics={'sent':0,'echoed':0,'alien':0,'bad_packets':0,'health':0,'slow_connections':0,'rss_peak_kib':0};latencies=[]
    def http(path,credential=None,body=None):
        connection=http_client.HTTPConnection('127.0.0.1',http_port,timeout=2)
        headers={'Authorization':'Bearer '+credential.hex(),'Content-Type':'application/octet-stream'} if credential else {}
        try:
            connection.request('POST' if credential else 'GET',path,body,headers)
            response=connection.getresponse();data=response.read()
            if response.status!=200:raise AssertionError(f'Healthy control request rejected: {response.status}')
            return data
        finally:connection.close()
    clients=[]
    loop=asyncio.get_running_loop();slow=[]
    for i in range(3):
        credential=secrets.token_bytes(32);reply=http('/v2/connect',credential,struct.pack('>Q',COMPAT)+text(f'Load{i}'))
        peer,ticket,key,remote_port=struct.unpack('>Q16s32sH',reply)
        sock=socket.socket(socket.AF_INET,socket.SOCK_DGRAM);sock.bind(('127.0.0.1',0));sock.setblocking(False)
        clients.append({'peer':peer,'ticket':ticket,'key':key,'socket':sock,'sequence':0,'credential':credential})
    host,client,attacker=clients
    def seal(c,raw,target=0):
        c['sequence']+=1;body=ENVELOPE.pack(ONLINE_MAGIC,c['ticket'],c['sequence'],target)+raw
        return body+hmac.digest(c['key'],b'C'+body,'sha256')
    async def send(c,kind,session=0,body=b'',target=0):
        data=seal(c,packet(kind,1,c['peer'],session,body),target)
        await loop.sock_sendto(c['socket'],data,('127.0.0.1',udp_port));return data
    async def receive(c,timeout=2):
        data=await asyncio.wait_for(loop.sock_recv(c['socket'],1600),timeout)
        assert hmac.compare_digest(data[-32:],hmac.digest(c['key'],b'S'+data[:-32],'sha256'))
        magic,ticket,_,target=ENVELOPE.unpack(data[:36]);assert magic==ONLINE_MAGIC and ticket==c['ticket'] and target==c['peer']
        return data[36:-32]
    try:
        for c in clients:
            await send(c,HEARTBEAT);nonce=await receive(c);assert len(nonce)==16
            await loop.sock_sendto(c['socket'],seal(c,nonce),('127.0.0.1',udp_port))
        await send(host,ADVERT,session=42,body=advert(host['peer']))
        await asyncio.sleep(0.02)
        await send(client,JOIN,session=42,body=b'K'*16,target=host['peer'])
        assert decode(await receive(host))[0]==JOIN
        await send(host,JOINED,session=42,body=advert(host['peer'])+b'\0',target=client['peer'])
        assert decode(await receive(client))[0]==JOINED
        replay=await send(host,HEARTBEAT)
        # One attacking source must not take the healthy source's TCP slots.
        for _ in range(24):
            try:
                r,w=await asyncio.open_connection('127.0.0.1',http_port,local_addr=('127.0.0.2',0))
                w.write(b'POST ');await w.drain();slow.append(w)
            except ConnectionError:pass
        metrics['slow_connections']=len(slow)
        until=time.monotonic()+a.seconds;pending={};tasks=[]
        async def host_receive():
            while time.monotonic()<until+0.5:
                try:raw=await receive(host,0.3)
                except TimeoutError:continue
                kind,_,peer,session,body=decode(raw)
                if kind==DATA:
                    if peer!=client['peer'] or session!=42:metrics['alien']+=1;continue
                    await send(host,DATA,session=42,body=body,target=client['peer'])
        async def client_receive():
            while time.monotonic()<until+0.5:
                try:raw=await receive(client,0.3)
                except TimeoutError:continue
                kind,_,peer,session,body=decode(raw)
                if kind==DATA:
                    assert peer==host['peer'] and session==42
                    number=struct.unpack_from('>I',body,6)[0]
                    if number in pending:latencies.append(time.monotonic()-pending.pop(number));metrics['echoed']+=1
        async def healthy():
            number=0;next_keepalive=0
            while time.monotonic()<until:
                now=time.monotonic()
                if now>=next_keepalive:
                    await send(host,ADVERT,session=42,body=advert(host['peer']))
                    await send(client,HEARTBEAT);next_keepalive=now+1
                    metadata=json.loads(await asyncio.to_thread(http,'/health'));assert metadata['rooms']==1;metrics['health']+=1
                    try:
                        status=Path(f'/proc/{process.pid}/status').read_text()
                        metrics['rss_peak_kib']=max(metrics['rss_peak_kib'],int(next(s for s in status.splitlines() if s.startswith('VmRSS:')).split()[1]))
                    except (OSError,StopIteration):pass
                number+=1;pending[number]=time.monotonic()
                payload=struct.pack('>I',number)+b'P'*1340
                await send(client,DATA,session=42,body=struct.pack('>HHH',1000,1001,len(payload))+payload,target=host['peer'])
                metrics['sent']+=1;await asyncio.sleep(0.02)
        async def flood():
            sock=attacker['socket'];batch=64;next_batch=time.monotonic()
            invalid=ENVELOPE.pack(ONLINE_MAGIC,host['ticket'],2**63,0)+packet(HEARTBEAT,1,host['peer'],0)+b'X'*32
            unknown=ENVELOPE.pack(ONLINE_MAGIC,b'Z'*16,2**63,0)+packet(HEARTBEAT,1,host['peer'],0)+b'X'*32
            counter=0
            while time.monotonic()<until:
                for _ in range(batch):
                    mode=counter%4
                    if mode==0:data=invalid
                    elif mode==1:data=unknown
                    elif mode==2:data=replay
                    else:data=seal(attacker,packet(DATA,1,attacker['peer'],42,struct.pack('>HHH',1000,1001,6)+b'ATTACK'),host['peer'])
                    await loop.sock_sendto(sock,data,('127.0.0.1',udp_port));counter+=1
                metrics['bad_packets']=counter;next_batch+=batch/a.packets_per_second
                await asyncio.sleep(max(0,next_batch-time.monotonic()))
        await asyncio.gather(host_receive(),client_receive(),healthy(),flood())
        assert metrics['alien']==0 and metrics['echoed']>=metrics['sent']*0.9,metrics
        credential=client['credential'];nonce=secrets.token_hex(16)
        body=text(nonce)+struct.pack('>IIIHBq',1,1,1,3,2,-9000000)
        await asyncio.to_thread(http,'/v2/stats/write',credential,body)
        reply=await asyncio.to_thread(http,'/v2/stats/read',credential,struct.pack('>IIIQI',1,1,100,0,0))
        assert struct.unpack_from('>II',reply)==(1,1)
        metrics['max_echo_ms']=round(max(latencies,default=0)*1000,2)
        metrics['p99_echo_ms']=round(sorted(latencies)[min(len(latencies)-1,int(len(latencies)*0.99))]*1000,2)
        return metrics
    finally:
        for c in clients:c['socket'].close()
        for writer in slow:
            writer.close()
            try:await writer.wait_closed()
            except ConnectionError:pass
with tempfile.TemporaryDirectory(prefix='motogp-owned-load-') as directory:
    directory=Path(directory);hp,up=port(),port()
    with (directory/'server.log').open('w+') as log:
        process=subprocess.Popen(server_runner+[server_python,guest(repo/'server/community_server.py'),'--http-port',str(hp),'--udp-port',str(up),'--udp-bind','127.0.0.1','--database',guest(directory/'db.sqlite3')],stdout=log,stderr=subprocess.STDOUT,cwd=directory)
        try:
            deadline=time.monotonic()+10
            while True:
                try:
                    with socket.create_connection(('127.0.0.1',hp),timeout=1):break
                except OSError:
                    if process.poll() is not None or time.monotonic()>deadline:raise RuntimeError('Owned test server failed to start')
                    time.sleep(0.05)
            metrics=asyncio.run(experiment(hp,up,process))
            assert process.poll() is None
            log.flush();log.seek(0);contents=log.read()
            assert not any(s in contents for s in ('Traceback','Unexpected service request failure','Task exception was never retrieved')),contents
            print(json.dumps(metrics,sort_keys=True))
        finally:
            process.terminate()
            try:process.wait(timeout=5)
            except subprocess.TimeoutExpired:process.kill();process.wait()
