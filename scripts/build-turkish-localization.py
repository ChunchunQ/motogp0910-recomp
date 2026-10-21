#!/usr/bin/env python3
"""Build the Turkish overlay from the owner's decoded game assets.

The input directory must contain english.msg and unpacked/<name>.english.lua.
Decoding is a separate, local extraction step; no console decompressor is shipped.
Only whitelisted display fields are replaced. Physics, IDs and paths stay byte exact.
"""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RESOURCES = ROOT / 'resources/localization/tr'
FIELDS = {'bikes': ('tech_info',), 'tracks': ('location_name', 'gp_name'),
          'upgrades': ('name', 'description'), 'unlockables': ('description',)}
TOKEN = re.compile(r'%(?:\d+\$)?[-+#0 ]*(?:\d+|\*)?(?:\.(?:\d+|\*))?(?:ll|l|h|z)?[diuoxXfFeEgGaAcCsSpn%]|%\d+|<[0-9A-B]>|[\u0700-\u07ff]|\\n')

def load_messages():
    rows = json.loads((RESOURCES / 'messages.json').read_text(encoding='utf-8'))
    assert [r['id'] for r in rows] == list(range(2102)), 'Missing/duplicate message ID'
    for r in rows:
        assert r['tr'] and '\0' not in r['tr'], r
        assert all(ord(c) <= 399 or 0x700 <= ord(c) <= 0x7ff for c in r['tr']), f"Unsupported native glyph: {r}"
        assert TOKEN.findall(r['en']) == TOKEN.findall(r['tr']), f"Changed tokens: {r}"
        assert r['en'] != r['tr'] or r.get('unchanged'), f"Unreviewed message: {r}"
    return rows

def parse_msg(data):
    count, = struct.unpack_from('>I', data)
    assert count == 2102
    result = []
    for i in range(count):
        start = struct.unpack_from('>I', data, 4 + 4 * i)[0] + 4
        assert 4 + 4 * count <= start < len(data)
        end = data.index(b'\0', start)
        result.append(data[start:end].decode('utf-8'))
    return result

def msg_bytes(rows):
    result = bytearray(4 + 4 * len(rows))
    struct.pack_into('>I', result, 0, len(rows))
    for r in rows:
        struct.pack_into('>I', result, 4 + 4 * r['id'], len(result) - 4)
        result.extend(r['tr'].encode('utf-8') + b'\0\0')
    assert parse_msg(result) == [r['tr'] for r in rows]
    return bytes(result)

def translate_script(data, fields, lookup):
    pattern = rb'(\b(?:' + b'|'.join(f.encode() for f in fields) + rb')\s*=\s*\[\[)(.*?)(\]\])'
    def replace(m):
        en = m[2].decode('utf-8')
        assert en in lookup, f'Unreviewed Lua display text: {en}'
        return m[1] + lookup[en].encode('utf-8') + m[3]
    return re.sub(pattern, replace, data, flags=re.S)

def translate_credits(data):
    lookup = json.loads((RESOURCES / 'credits.json').read_text(encoding='utf-8'))
    def replace(m):
        # The original has a few CP1252 copyright/song marks. Preserve their
        # exact bytes unless the entire display field has a reviewed translation.
        source = m[2].decode('cp1252')
        target = lookup.get(source)
        value = target if target else source.translate(str.maketrans({'‘': chr(39), '’': chr(39), '\u00a0': ' '}))
        return m[1] + value.encode('utf-8') + m[3]
    return re.sub(rb'(\btext\s*=\s*")(.*?)(")', replace, data)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--decoded', type=Path)
    parser.add_argument('--check', action='store_true', help='Validate committed catalog and overlay')
    args = parser.parse_args()
    rows = load_messages()
    lookup = {r['en']: r['tr'] for r in rows}
    output = RESOURCES / 'overlay'
    if args.check:
        assert parse_msg((output / 'text/english.msg').read_bytes()) == [r['tr'] for r in rows]
        manifest = json.loads((RESOURCES / 'manifest.json').read_text())
        expected = {'text/english.msg', *(f'scripts/{name}.english.lua' for name in (*FIELDS, 'credits')),
                    *(f'fonts/{name}.font' for name in ('impact_17pt', 'impact_small', 'arial_bold_17pt', 'arial_bold_small'))}
        assert {r['path'] for r in manifest['files']} == expected and len(manifest['files']) == 10
        assert manifest['message_count'] == len(rows)
        assert manifest['translated_messages'] == sum(r['en'] != r['tr'] for r in rows)
        for r in manifest['files']:
            data = (output / r['path']).read_bytes()
            assert len(data) == r['size']
            assert hashlib.sha256(data).hexdigest() == r['sha256']
        print(f"Validated {len(rows)} messages and {len(manifest['files'])} overlay files")
        return
    assert args.decoded, '--decoded is required to rebuild'
    source = (args.decoded / 'english.msg').read_bytes()
    assert parse_msg(source) == [r['en'] for r in rows], 'Different source message bank'
    generated = {'text/english.msg': (source, msg_bytes(rows))}
    for name, fields in FIELDS.items():
        data = (args.decoded / f'unpacked/{name}.english.lua').read_bytes()
        generated[f'scripts/{name}.english.lua'] = data, translate_script(data, fields, lookup)
    data = (args.decoded / 'unpacked/credits.english.lua').read_bytes()
    generated['scripts/credits.english.lua'] = data, translate_credits(data)
    # Text revisions do not require rebuilding the separately generated fonts.
    # Retain only verified, known font records from an existing complete overlay.
    font_paths = {f'fonts/{name}.font' for name in ('impact_17pt', 'impact_small', 'arial_bold_17pt', 'arial_bold_small')}
    font_records = []
    old_manifest = RESOURCES / 'manifest.json'
    if old_manifest.exists():
        for record in json.loads(old_manifest.read_text())['files']:
            if record['path'] in font_paths:
                font = (output / record['path']).read_bytes()
                assert len(font) == record['size'] and hashlib.sha256(font).hexdigest() == record['sha256'], 'Rebuild or restore the existing font overlay first'
                font_records.append(record)
    manifest = {'schema': 1, 'language': 'tr', 'voice_language': 'english', 'message_count': len(rows),
                'translated_messages': sum(r['en'] != r['tr'] for r in rows), 'files': []}
    for relative, (data, translated) in generated.items():
        path = output / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(translated)
        manifest['files'].append({'path': relative, 'source_decoded_sha256': hashlib.sha256(data).hexdigest(),
                                  'sha256': hashlib.sha256(translated).hexdigest(), 'size': len(translated)})
    manifest['files'].extend(font_records)
    (RESOURCES / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(f"Built {len(rows)} messages, {manifest['translated_messages']} translated; {len(generated)} files")

if __name__ == '__main__':
    main()
