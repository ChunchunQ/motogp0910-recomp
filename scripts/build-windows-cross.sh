#!/usr/bin/env bash
set -euo pipefail
project=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$project"
source scripts/cloud-env.sh
python3 scripts/check-game-inputs.py
cmake -S . -B out/build/windows-cross-release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$project/cmake/windows-cross-toolchain.cmake" \
  -DCMAKE_PROJECT_INCLUDE="$project/cmake/windows-host-tools.cmake" \
  -DCMAKE_PREFIX_PATH=/workspace/shared/rexglue-win-sdk/win-amd64
cmake --build out/build/windows-cross-release \
  --target motogp0910 motogp_headless_probe --parallel 4
python3 - <<'PY'
from pathlib import Path
import shutil
source = Path('/workspace/shared/msvc-redist/Contents/VC/Redist/MSVC/14.44.35112/onecore/x64/Microsoft.VC143.CRT')
destination = Path('out/build/windows-cross-release')
for file in source.glob('*.dll'):
    shutil.copy2(file, destination / file.name)
PY
