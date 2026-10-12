#!/usr/bin/env bash
set -euo pipefail
project=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
base=/workspace/shared
cd "$project"
bash scripts/install-cloud.sh

# LLVM packages remain authenticated by APT; do not disable signature checks.
curl -fsSL -o "$base/apt/llvm.asc" https://apt.llvm.org/llvm-snapshot.gpg.key
python3 - <<'PY'
from pathlib import Path
p = Path('/workspace/shared/apt/sources.list')
line = 'deb [signed-by=/workspace/shared/apt/llvm.asc] https://apt.llvm.org/trixie/ llvm-toolchain-trixie-20 main\n'
s = p.read_text()
if line not in s:
    p.write_text(s + line)
PY
export APT_CONFIG="$base/apt/apt.conf"
/usr/bin/apt-get update
/usr/bin/apt-get --download-only -y --no-install-recommends install clang-20 lld-20 llvm-20
for package in "$base/apt/cache/archives/"*20*.deb; do
  dpkg-deb -x "$package" "$base/sysroot"
done

download_checked() {
  local filename=$1 url=$2 digest=$3
  if [[ ! -f "$filename" ]]; then
    curl -fL --retry 2 -o "$filename.part" "$url"
    mv "$filename.part" "$filename"
  fi
  printf '%s  %s\n' "$digest" "$filename" | sha256sum -c -
}
download_checked "$base/downloads/xwin-0.10.0-x86_64-unknown-linux-musl.tar.gz" \
  https://github.com/Jake-Shadle/xwin/releases/download/0.10.0/xwin-0.10.0-x86_64-unknown-linux-musl.tar.gz \
  d870eb4b2f390878af6da1ccd3cf321d22fcb72720984853b4be732ae597fc88
mkdir -p "$base/xwin-tool"
tar -xzf "$base/downloads/xwin-0.10.0-x86_64-unknown-linux-musl.tar.gz" -C "$base/xwin-tool"
if [[ ! -f "$base/windows-sysroot/.motogp-verified" ]]; then
  "$base/xwin-tool/xwin-0.10.0-x86_64-unknown-linux-musl/xwin" \
    --accept-license --cache-dir "$base/xwin-cache" \
    --manifest cmake/windows-vs17-channel.json --arch x86_64 --http-retry 2 \
    splat --output "$base/windows-sysroot"
  touch "$base/windows-sysroot/.motogp-verified"
fi

download_checked "$base/downloads/rexglue-sdk-0.10.0-win-amd64.zip" \
  https://github.com/rexglue/rexglue-sdk/releases/download/v0.10.0/rexglue-sdk-0.10.0-win-amd64.zip \
  ad4485da853fbe932e9f5dbb643f9b5fe85dc73775ebbad4df6879fe6df0c11b
download_checked "$base/downloads/msvc-14.44-redist-x64.vsix" \
  https://download.visualstudio.microsoft.com/download/pr/45d3b8dd-bced-4b37-9974-142f748d710c/2076c0dc32095338841011431766d532479b50e1c37f17638325c792f37f9fdb/Microsoft.VC.14.44.17.14.CRT.Redist.x64.OneCore.Desktop.base.vsix \
  2076c0dc32095338841011431766d532479b50e1c37f17638325c792f37f9fdb
python3 - <<'PY'
from pathlib import Path
import zipfile
base = Path('/workspace/shared')
for archive, destination in [
    ('rexglue-sdk-0.10.0-win-amd64.zip', 'rexglue-win-sdk'),
    ('msvc-14.44-redist-x64.vsix', 'msvc-redist'),
]:
    root = base / destination
    marker = root / '.motogp-verified'
    if marker.is_file():
        continue
    with zipfile.ZipFile(base / 'downloads' / archive) as package:
        for item in package.infolist():
            if not (root / item.filename).resolve().is_relative_to(root.resolve()):
                raise ValueError('Archive path escapes destination')
        package.extractall(root)
    marker.write_text('Archive verified before extraction\n')
PY
