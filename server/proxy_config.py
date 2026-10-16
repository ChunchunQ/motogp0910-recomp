#!/usr/bin/env python3
"""Create a private proxy key and run Caddy with it; never print the key."""
import argparse
import os
from pathlib import Path
import secrets
import sys
import subprocess
# Also support the isolated python.org Windows embeddable runtime.
sys.path.insert(0,str(Path(__file__).resolve().parent))
from security import read_proxy_secret


def create_key(path):
    path=Path(path);path.parent.mkdir(parents=True,exist_ok=True,mode=0o700)
    # Exclusive creation protects existing deployments from accidental rotation.
    with open(os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600),'w') as output:
        output.write(secrets.token_hex(32)+'\n')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action',choices=('init','run'))
    parser.add_argument('--secret-file',type=Path,default=Path(__file__).with_name('data')/'proxy.key')
    parser.add_argument('--config',type=Path,default=Path(__file__).with_name('Caddyfile'))
    args=parser.parse_args()
    if args.action=='init':
        create_key(args.secret_file)
        print('Private proxy key created. Keep it on the server; do not share it with players.')
    else:
        environment=os.environ.copy();environment['MOTOGP_PROXY_SECRET']=read_proxy_secret(args.secret_file)
        # No shell and no credential on the command line or process arguments.
        result=subprocess.run(['caddy','run','--config',str(args.config.resolve())],env=environment,check=False)
        raise SystemExit(result.returncode)

if __name__=='__main__':main()
