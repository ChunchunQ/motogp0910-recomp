#!/usr/bin/env python3
"""Exercise real native faults in isolated Windows fixture processes, without game data."""
import argparse
import json
import shutil
import struct
import subprocess
import tempfile
import time
from pathlib import Path


def dump_exception(path):
    data = path.read_bytes()
    assert data[:4] == b'MDMP', 'Invalid minidump signature'
    count, directory = struct.unpack_from('<II', data, 8)
    assert 0 < count < 100 and directory + count * 12 <= len(data)
    for i in range(count):
        kind, size, offset = struct.unpack_from('<III', data, directory + i * 12)
        assert offset + size <= len(data), 'Minidump directory range'
        if kind == 6:
            return struct.unpack_from('<I', data, offset + 8)[0]
    raise AssertionError('Actual exception stream missing from minidump')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--wine', type=Path)
    parser.add_argument('--output-dir', type=Path, default=Path('out/crash-report-validation'))
    args = parser.parse_args()
    build = args.build_dir.resolve(strict=True)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix='run-', dir=args.output_dir.resolve()))
    results = []
    cases = [('clean', True, None), ('handled', True, None), ('av', True, 0xC0000005),
             ('fastfail', True, 0xC0000409), ('cpp', True, 0xE06D7363),
             ('nonzero', True, None), ('parent', False, None), ('av', False, 0xC0000005), ('cpp', False, 0xE06D7363)]
    for mode, monitored, expected in cases:
        folder = root / (mode + ('-monitor' if monitored else '-direct'))
        folder.mkdir()
        for dll in build.glob('*.dll'):
            if dll.name.lower().startswith(('msvcp', 'vcruntime')):
                shutil.copy2(dll, folder / dll.name)
        shutil.copy2(build / 'crash_report_probe.exe', folder / 'motogp0910.exe')
        shutil.copy2(build / 'motogp0910-crash-monitor.exe', folder / 'motogp0910-crash-monitor.exe')
        (folder / 'motogp0910.toml').write_text('lan_enabled = true\nlan_bind_ip = ""\n')
        target = folder / ('motogp0910-crash-monitor.exe' if monitored else 'motogp0910.exe')
        command = ([str(args.wine.resolve())] if args.wine else []) + [str(target), mode]
        with (folder / 'process-output.txt').open('w') as output:
            process = subprocess.run(command, cwd=folder, stdout=output, stderr=subprocess.STDOUT, timeout=45)
        reports = list((folder / 'logs').glob('crash-*/report.txt'))
        if mode == 'parent':
            deadline = time.monotonic() + 15
            while not (folder / 'logs/delay-complete.txt').exists() and time.monotonic() < deadline:
                time.sleep(0.05)
            assert (folder / 'logs/delay-complete.txt').read_text() == 'game survived parent exit\n'
            assert process.returncode == 0 and not reports
        elif mode in ('clean', 'handled'):
            assert process.returncode == 0 and not reports, 'Handled/clean exits must not be reported as crashes'
        else:
            assert process.returncode != 0 and len(reports) == 1, f'{mode}: exactly one crash report required'
            report = reports[0].read_text()
            assert (reports[0].parent / 'game.log').read_text() == 'Diagnostic fixture evidence\n'
            assert (reports[0].parent / 'motogp0910.toml').is_file()
            if expected is not None:
                assert f'Exception: 0x{expected:08x}' in report, report
                assert dump_exception(reports[0].parent / 'crash.dmp') == expected
                assert 'Registers:' in report and 'Native stack' in report and 'Modules' in report
                assert 'Image SHA-256:' in report
                if mode == 'cpp':
                    assert 'Crash fixture: uncaught C++ failure' in report
            else:
                assert 'No native exception event' in report and '17' in report
            if mode == 'fastfail':
                assert 'Parameter[0]: 0x7' in report
        results.append({'case': folder.name, 'returncode': process.returncode, 'reports': len(reports)})
        print(f'{folder.name}: passed', flush=True)
    (root / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    print(f'{len(cases)} native crash-report scenarios passed. Evidence: {root}')


if __name__ == '__main__':
    main()
