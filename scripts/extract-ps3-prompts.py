#!/usr/bin/env python3
"""Extract only the controller textures from the user's PS3 sharedfrontend bank.

The BC3 blocks remain unchanged. No recompression, redrawing, or screen captures.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

NAMES = [
    'xbox_a', 'xbox_b', 'xbox_y', 'xbox_x',
    'ps_cross', 'ps_circle', 'ps_triangle', 'ps_square',
    'ps_left_stick', 'ps_left_stick_alt', 'ps_right_stick',
    'ps_r1', 'ps_l1', 'ps_r2', 'ps_l2', 'ps_select',
    'xbox_left_stick', 'xbox_left_stick_alt', 'xbox_right_stick',
    'xbox_rb', 'xbox_lb', 'xbox_rt', 'xbox_lt', 'xbox_back',
    'xbox_dpad', 'xbox_dpad_up', 'xbox_dpad_down', 'xbox_dpad_right', 'xbox_dpad_left',
    'ps_left_stick_up', 'ps_left_stick_down', 'ps_left_stick_left', 'ps_left_stick_right',
    'ps_dpad', 'ps_dpad_up', 'ps_dpad_down', 'ps_dpad_left', 'ps_dpad_right',
    'xbox_left_stick_up', 'xbox_left_stick_down', 'xbox_left_stick_left', 'xbox_left_stick_right',
]

def extract(bank):
    count = struct.unpack_from('>I', bank)[0]
    if count != 253:
        raise ValueError('Expected the 253-entry MotoGP 09/10 PS3 sharedfrontend bank')
    sizes = struct.unpack_from('>253I', bank, 4)
    offset = 4 + 4 * count
    if offset + sum(sizes) != len(bank):
        raise ValueError('Truncated or unsupported bank')
    records, catalog = [], []
    for index, size in enumerate(sizes):
        entry = bank[offset:offset + size]
        offset += size
        if not 102 <= index <= 143:
            continue
        resource_hash, payload_size = struct.unpack_from('>II', entry)
        width, height = struct.unpack_from('>HH', entry, 12)
        if width != 64 or height != 64 or entry[10] != 2 or payload_size != 4104 or size != 4112:
            raise ValueError(f'Unsupported controller texture {index}')
        pixels = entry[16:]
        records.append(struct.pack('>IIHHI', index, resource_hash, width, height, len(pixels)) + pixels)
        catalog.append(dict(index=index, name=NAMES[index - 102], width=width, height=height,
                            resource_hash=f'{resource_hash:08X}', bc3_sha256=hashlib.sha256(pixels).hexdigest()))
    assert len(records) == len(NAMES) == 42
    return b'MGPS3BTN' + struct.pack('>I', len(records)) + b''.join(records), catalog

def extract_diagram(bank):
    count = struct.unpack_from('>I', bank)[0]
    if count != 253:
        raise ValueError('Expected the original PS3 sharedfrontend bank')
    sizes = struct.unpack_from('>253I', bank, 4)
    start = 4 + 4 * count + sum(sizes[:251])
    record = bank[start:start + sizes[251]]
    resource_hash, payload_size = struct.unpack_from('>II', record)
    width, height = struct.unpack_from('>HH', record, 12)
    if (resource_hash, payload_size, width, height, record[10], len(record)) != (0x14D50C26, 524296, 1024, 512, 2, 524304):
        raise ValueError('Unsupported original DualShock 3 control diagram')
    pixels = record[16:]
    pack = b'MGPS3CTL' + struct.pack('>I', 1) + struct.pack('>IIHHI', 251, resource_hash, width, height, len(pixels)) + pixels
    metadata = dict(format='MotoGP PS3 control diagram BC3 v1', source_bank_sha256=hashlib.sha256(bank).hexdigest(),
                    index=251, resource_hash=f'{resource_hash:08X}', width=width, height=height,
                    bc3_sha256=hashlib.sha256(pixels).hexdigest(), pack_sha256=hashlib.sha256(pack).hexdigest())
    return pack, metadata

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('bank', type=Path)
    parser.add_argument('--output', type=Path, default=Path('resources/controller/ps3-prompts.bin'))
    args = parser.parse_args()
    payload, catalog = extract(args.bank.read_bytes())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(payload)
    args.output.with_suffix('.json').write_text(json.dumps(dict(
        format='MotoGP PS3 controller BC3 textures v1',
        source_bank_sha256=hashlib.sha256(args.bank.read_bytes()).hexdigest(),
        pack_sha256=hashlib.sha256(payload).hexdigest(), textures=catalog), indent=2) + '\n')
    diagram, metadata = extract_diagram(args.bank.read_bytes())
    diagram_file = args.output.with_name('ps3-controls.bin')
    diagram_file.write_bytes(diagram)
    diagram_file.with_suffix('.json').write_text(json.dumps(metadata, indent=2) + '\n')
    print(f'Extracted original DualShock 3 diagram: {diagram_file} ({len(diagram)} bytes)')
    print(f'Extracted {len(catalog)} original controller textures: {args.output} ({len(payload)} bytes)')

if __name__ == '__main__':
    main()
