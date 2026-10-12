#!/usr/bin/env python3
"""Validate the exact XEX/update pair that the generated function map targets."""
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EXPECTED = {
    "assets/game-tu/default.xex": "54245417e996ed0aa561a5a803fd810e87684bd0b43082f316db711b3ee786ae",
    "assets/game-tu/default.xexp": "0419205ba10dc65eaa7b0bb62abc64cada46119ece5ed749e4ba20c06a6bc079",
}

def main():
    for relative, expected in EXPECTED.items():
        path = ROOT / relative
        if not path.is_file():
            raise SystemExit(f"Missing required game input: {relative}")
        with path.open("rb") as source:
            actual = hashlib.file_digest(source, "sha256").hexdigest()
        if actual != expected:
            raise SystemExit(f"Wrong game revision: {relative}: {actual}")
        print(f"OK {relative}: {actual}")

if __name__ == "__main__":
    main()
