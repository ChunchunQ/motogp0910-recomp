#!/usr/bin/env python3
"""Build the embedded bio emoji atlas from a local Noto Color Emoji font.

Requires fontTools and Pillow. Input font is not distributed with the game;
the rendered atlas retains its SIL OFL attribution in the embedded notices.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path
from fontTools.ttLib import TTFont
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('font', type=Path)
    args = parser.parse_args()
    font = TTFont(args.font)
    cmap = font.getBestCmap()
    glyph_sequences = {glyph: chr(cp) for cp, glyph in cmap.items()}
    sequences = {chr(cp): glyph for cp, glyph in cmap.items() if cp >= 0x2000}
    ligatures = []
    for lookup in font['GSUB'].table.LookupList.Lookup:
        for table in lookup.SubTable:
            for first, values in getattr(table, 'ligatures', {}).items():
                ligatures.extend(([first, *item.Component], item.LigGlyph) for item in values)
    for _ in range(8):
        changed = False
        for components, glyph in ligatures:
            if not all(part in glyph_sequences for part in components):
                continue
            sequence = ''.join(glyph_sequences[part] for part in components)
            sequences[sequence] = glyph
            if glyph not in glyph_sequences:
                glyph_sequences[glyph] = sequence
                changed = True
        if not changed:
            break
    bitmaps = font['CBDT'].strikeData[0]
    sequences = {s: g for s, g in sequences.items() if g in bitmaps and s not in ('\u200d', '\ufe0f')}
    glyphs = sorted(set(sequences.values()))
    if len(glyphs) > 0x1900:
        raise SystemExit('Emoji atlas exceeds the reserved BMP private-use range')
    columns, cell = 64, 32
    atlas = Image.new('RGBA', (columns * cell, ((len(glyphs) + columns - 1) // columns) * cell))
    for index, glyph in enumerate(glyphs):
        bitmap = bitmaps[glyph]
        bitmap.decompile()
        picture = Image.open(io.BytesIO(bitmap.imageData)).convert('RGBA')
        picture.thumbnail((cell - 2, cell - 2), Image.Resampling.LANCZOS)
        x = index % columns * cell + (cell - picture.width) // 2
        y = index // columns * cell + (cell - picture.height) // 2
        atlas.alpha_composite(picture, (x, y))
    indices = {glyph: index for index, glyph in enumerate(glyphs)}
    data = {'source': 'Noto Color Emoji', 'font_sha256': hashlib.sha256(args.font.read_bytes()).hexdigest(),
            'cell': cell, 'columns': columns, 'glyphs': len(glyphs),
            'aliases': {sequence: indices[glyph] for sequence, glyph in sorted(sequences.items())}}
    target = ROOT / 'resources/gamer-card'
    atlas.save(target / 'emoji.png', optimize=True)
    (target / 'emoji.json').write_text(json.dumps(data, ensure_ascii=False, separators=(',', ':')) + '\n', encoding='utf-8')
    for required in ('🃏', '❤️'.replace('\ufe0f', ''), '👨\u200d👩\u200d👧\u200d👦', '👍🏽', '🇹🇷'):
        if required not in data['aliases']:
            raise SystemExit(f'Missing required emoji sequence: {required!r}')
    print(f'{len(sequences)} sequences, {len(glyphs)} sprites, {atlas.width}x{atlas.height}')

if __name__ == '__main__':
    main()
