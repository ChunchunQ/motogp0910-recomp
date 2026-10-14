#!/usr/bin/env python3
"""Stage the pinned custom runtime after CMake copies SDK DLLs."""
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import shutil
import zipfile

ROOT = Path(__file__).resolve().parent.parent

def stage(destination):
    folder = ROOT / 'third_party/rexglue-runtime'
    manifest = json.loads((folder / 'manifest.json').read_text())
    sdk=json.loads((ROOT/'sdk-version.json').read_text())
    if manifest['sdk_version'] != sdk['version'] or manifest['source_commit'] != sdk['source_commit']:
        raise ValueError('Runtime must match the SDK version and source pin in sdk-version.json')
    archive = folder / manifest['archive']
    if hashlib.sha256(archive.read_bytes()).hexdigest() != manifest['sha256']:
        raise ValueError('Spatial runtime archive SHA-256 mismatch')
    destination = Path(destination)
    destination.mkdir(parents=True, exist_ok=True)
    # Read and validate everything before replacing either member of the pair.
    with zipfile.ZipFile(archive) as package:
        contents = {name: package.read(name) for name in manifest['dll_sha256']}
    for name, data in contents.items():
        if Path(name).name != name or hashlib.sha256(data).hexdigest() != manifest['dll_sha256'][name]:
            raise ValueError('Spatial runtime DLL checksum/name mismatch')
    for name, data in contents.items():
        with tempfile.NamedTemporaryFile(dir=destination, prefix=name+'.', suffix='.stage-tmp', delete=False) as output:
            temporary = Path(output.name)
            output.write(data)
        try:
            temporary.replace(destination / name)
        finally:
            temporary.unlink(missing_ok=True)
    shutil.copytree(folder/'licenses', destination/'licenses', dirs_exist_ok=True)
    print('Staged verified matching ReXGlue runtime:', destination)

if __name__ == '__main__':
    stage(sys.argv[1] if len(sys.argv)>1 else ROOT / 'out/build/windows-cross-release')
