#!/usr/bin/env python3
"""Convert the supplied artwork to Windows ICO resources without redrawing it.

Requires Pillow only when changing the artwork; ordinary builds use committed ICOs.
"""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent / "resources/icons"
SIZES = [(n, n) for n in (16, 24, 32, 48, 64, 128, 256)]
for name, extension in (("game", "jpg"), ("launcher", "png")):
    with Image.open(ROOT / "sources" / f"{name}.{extension}") as source:
        if source.width != source.height:
            raise ValueError("Icon artwork must be square; do not stretch the logo")
        source.convert("RGBA").save(ROOT / f"{name}.ico", sizes=SIZES)
    with Image.open(ROOT / f"{name}.ico") as icon:
        if icon.ico.sizes() != set(SIZES):
            raise ValueError("Missing Windows icon size")
    print(f"{name}.ico: seven sizes, 16-256 px")
