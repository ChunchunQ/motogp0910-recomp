#!/usr/bin/env bash
set -euo pipefail

# Rootless Debian 13 setup; package signatures and checksums remain enabled.
base=/workspace/shared
sdk_commit=f5337cdc947ff6d4c4196737e2c807a48f2a1fc2
sdk_digest=0388edb3f8fb1444f1b0ad5d440cb02d9690ac20a76b04fd6b7dcc19f59328a6
mkdir -p "$base/apt/lists/partial" "$base/apt/cache/archives/partial" "$base/apt/empty" "$base/downloads" "$base/sysroot"
cat > "$base/apt/sources.list" <<'EOF'
deb [signed-by=/usr/share/keyrings/debian-archive-keyring.pgp] https://deb.debian.org/debian trixie main
deb [signed-by=/usr/share/keyrings/debian-archive-keyring.pgp] https://security.debian.org/debian-security trixie-security main
EOF
cat > "$base/apt/apt.conf" <<'EOF'
Dir::Etc::Parts "/workspace/shared/apt/empty";
Dir::Etc::main "-";
Dir::Etc::sourcelist "/workspace/shared/apt/sources.list";
Dir::Etc::sourceparts "/workspace/shared/apt/empty";
Dir::State::lists "/workspace/shared/apt/lists";
Dir::Cache "/workspace/shared/apt/cache";
Dir::Cache::archives "/workspace/shared/apt/cache/archives";
Debug::NoLocking "true";
APT::Get::List-Cleanup "false";
EOF
export APT_CONFIG="$base/apt/apt.conf"
/usr/bin/apt-get update
/usr/bin/apt-get --download-only -y --no-install-recommends install \
  clang-19 lld-19 cmake ninja-build pkg-config libgtk-3-dev \
  libx11-xcb-dev libxss-dev libvulkan-dev
for package in "$base/apt/cache/archives/"*.deb; do
  dpkg-deb -x "$package" "$base/sysroot"
done

source_dir="$base/rexglue-sdk"
if [[ ! -d "$source_dir" ]]; then
  git clone --depth 1 --branch v0.10.0 https://github.com/rexglue/rexglue-sdk.git "$source_dir"
fi
if [[ "$(git -C "$source_dir" rev-parse HEAD)" != "$sdk_commit" ]]; then
  echo "SDK checkout must be at v0.10.0 ($sdk_commit); refusing to change an existing checkout." >&2
  exit 1
fi
git -C "$source_dir" submodule update --init --recursive --depth 1 --jobs 4

archive="$base/downloads/rexglue-sdk-0.10.0-linux-amd64.zip"
if [[ ! -f "$archive" ]]; then
  curl -fL --retry 2 -o "$archive.part" \
    https://github.com/rexglue/rexglue-sdk/releases/download/v0.10.0/rexglue-sdk-0.10.0-linux-amd64.zip
  mv "$archive.part" "$archive"
fi
printf '%s  %s\n' "$sdk_digest" "$archive" | sha256sum -c -
if [[ ! -f "$base/rexglue-install/.v0.10.0-verified" ]]; then
  python3 - "$archive" "$base/rexglue-install" <<'PY'
import os, pathlib, sys, zipfile
archive, destination = sys.argv[1:]
root = pathlib.Path(destination)
with zipfile.ZipFile(archive) as package:
    for item in package.infolist():
        target = (root / item.filename).resolve()
        if not target.is_relative_to(root.resolve()):
            raise ValueError('Archive path escapes destination')
    package.extractall(root)
    for item in package.infolist():
        mode = item.external_attr >> 16
        if mode:
            os.chmod(root / item.filename, mode)
(root / '.v0.10.0-verified').write_text('Verified official GitHub release SHA-256\n')
PY
fi
project_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
source "$project_dir/scripts/cloud-env.sh"
[[ "$(rexglue --version)" == "0.10.0" ]]
clang-19 --version
cmake --version
