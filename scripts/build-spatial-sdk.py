#!/usr/bin/env python3
"""Reproduce the FSR/shared-SDL DLL pair from the pinned SDK source + project patches."""
import argparse
import json
import hashlib
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parent.parent

def run(*args, cwd=None):
    subprocess.run([str(a) for a in args], cwd=cwd, check=True)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk-source', type=Path, required=True)
    parser.add_argument('--build-dir', type=Path, default=ROOT/'out/spatial-sdk-build')
    parser.add_argument('--toolchain', type=Path)
    parser.add_argument('--compiler', default='clang++')
    parser.add_argument('--cmake', default='cmake')
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--vulkan', action='store_true', help='Build the Linux Vulkan plugin instead of Windows D3D12')
    args = parser.parse_args()
    if not 1<=args.jobs<=16:
        parser.error('--jobs must be 1..16')
    vendor=ROOT/'third_party/fidelityfx'
    for name,digest in json.loads((vendor/'manifest.json').read_text())['sha256'].items():
        if hashlib.sha256((vendor/name).read_bytes()).hexdigest()!=digest:
            raise SystemExit(f'FidelityFX provider checksum mismatch: {name}')
    source = args.sdk_source.resolve()
    manifest = json.loads((ROOT/'sdk-version.json').read_text())
    if not source.exists():
        source.parent.mkdir(parents=True, exist_ok=True)
        run('git', 'clone', '--branch', manifest['release_tag'], '--recursive',
            'https://github.com/rexglue/rexglue-sdk.git', source)
    revision = subprocess.check_output(['git', '-C', str(source), 'rev-parse', 'HEAD'], text=True).strip()
    if revision != manifest['source_commit']:
        raise SystemExit('SDK source must match the exact pinned commit in sdk-version.json.')
    for name in ['sdk-nightly-explicit-video-mode.patch','sdk-nightly-symmetric-deadzone.patch','sdk-livery-readback.patch','sdk-experimental-temporal-fsr.patch','sdk-ultrawide-presentation.patch']:
        patch = ROOT/'cmake'/name
        reversed_check = subprocess.run(['git','apply','--reverse','--check',str(patch)], cwd=source,
                                        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if reversed_check.returncode:
            run('git','apply','--check',patch,cwd=source)
            run('git','apply',patch,cwd=source)
    run('git','submodule','update','--init','--recursive',cwd=source)
    command = [args.cmake, '-S', source, '-B', args.build_dir.resolve(), '-G', 'Ninja',
               '-DCMAKE_BUILD_TYPE=Release', f'-DREXGLUE_USE_VULKAN={"ON" if args.vulkan else "OFF"}',
               f'-DREXGLUE_USE_D3D12={"OFF" if args.vulkan else "ON"}',
               '-DREXGLUE_ENABLE_FIDELITYFX=OFF', '-DREXGLUE_ENABLE_TRACY=ON',
               '-DREXGLUE_BUILD_TESTS=OFF', f'-DCMAKE_PROJECT_INCLUDE={ROOT / "cmake/sdk-temporal-fsr.cmake"}']
    if args.toolchain:
        command.append(f'-DCMAKE_TOOLCHAIN_FILE={args.toolchain.resolve()}')
    else:
        command.append(f'-DCMAKE_CXX_COMPILER={args.compiler}')
        command.append(f'-DCMAKE_C_COMPILER={args.compiler.replace("clang++", "clang")}')
    run(*command)
    run(args.cmake, '--build', args.build_dir.resolve(), '--target', 'rexruntime',
        'rexgpu-xenos', '--parallel', args.jobs)
    print('Built matching runtime:', source/('out/linux-amd64' if args.vulkan else 'out/win-amd64'))
    print('These local DLLs have compiler-specific checksums; the packaged pair remains pinned separately.')

if __name__ == '__main__':
    main()
