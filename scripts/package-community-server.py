#!/usr/bin/env python3
"""Package only public server sources/templates; never its database or secrets."""
from pathlib import Path
import argparse
import hashlib
import subprocess
import sys
import zipfile
root=Path(__file__).resolve().parent.parent
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--archive', type=Path, default=root/'downloads/MotoGP0910-Community-Server.zip')
parser.add_argument('--include-readmes', action='store_true', help='Include optional deployment instructions; disabled for development builds')
args=parser.parse_args()
# The distributable stays stdlib-only; optional fuzzing tools are never needed
# for this release gate and are never copied into the public server ZIP.
subprocess.run([sys.executable,'-m','unittest','discover','-s','server','-p','test_*.py'],cwd=root,check=True)
files=('community_server.py','security.py','proxy_config.py','leaderboards.json','start-server.sh','start-server.ps1','Caddyfile.example','motogp-online.service.example','motogp-caddy.service.example')
archive=args.archive.resolve()
archive.parent.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
    for name in files:z.write(root/'server'/name,name)
    if args.include_readmes:
        z.writestr('README.md',(root/'docs/community-online.md').read_text().replace('(community-security.md)','(SECURITY.md)'))
        z.write(root/'docs/community-security.md','SECURITY.md')
digest=hashlib.sha256(archive.read_bytes()).hexdigest()
archive.with_suffix('.zip.sha256').write_text(f'{digest}  {archive.name}\n')
print(f'{archive}: {digest}')
