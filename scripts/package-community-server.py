#!/usr/bin/env python3
"""Package only public server sources/templates; never its database or secrets."""
from pathlib import Path
import hashlib
import zipfile
root=Path(__file__).resolve().parent.parent
files=('community_server.py','leaderboards.json','start-server.sh','start-server.ps1','Caddyfile.example','motogp-online.service.example')
archive=root/'downloads/MotoGP0910-Community-Server.zip'
archive.parent.mkdir(exist_ok=True)
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
    for name in files:z.write(root/'server'/name,name)
    z.write(root/'docs/community-online.md','README.md')
digest=hashlib.sha256(archive.read_bytes()).hexdigest()
archive.with_suffix('.zip.sha256').write_text(f'{digest}  {archive.name}\n')
print(f'{archive}: {digest}')
