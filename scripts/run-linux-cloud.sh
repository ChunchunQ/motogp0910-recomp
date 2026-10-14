#!/usr/bin/env bash
set -euo pipefail

# Reproduce the graphical startup check in this rootless cloud environment.
# This opens the real game; screenshots must be inspected to validate a menu.
project_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
if [[ $# -lt 1 ]]; then
  echo 'Usage: bash scripts/run-linux-cloud.sh FULL_GAME_ROOT [extra application arguments]' >&2
  exit 2
fi
game_root=$(realpath "$1")
shift
[[ -f "$game_root/default.xex" && -d "$game_root/xenondata" ]] || {
  echo 'Game root must contain default.xex and the complete xenondata directory.' >&2
  exit 2
}
source "$project_dir/scripts/cloud-env.sh"
binary="${MOTOGP_CLOUD_BINARY:-$project_dir/out/build/linux-amd64-release/motogp0910}"
[[ -x "$binary" ]] || { echo 'Build the Linux Release target first.' >&2; exit 2; }
for tool in Xvfb xdotool; do
  command -v "$tool" >/dev/null || { echo "Missing $tool; run scripts/install-cloud.sh." >&2; exit 2; }
done
mkdir -p "$project_dir/out/linux-startup"
run_dir=$(mktemp -d "$project_dir/out/linux-startup/run-XXXXXX")
mkdir -p "$run_dir/runtime" "$run_dir/cache" "$project_dir/out/linux-game-userdata"
chmod 700 "$run_dir/runtime"
export XDG_RUNTIME_DIR="$run_dir/runtime" XDG_CACHE_HOME="$run_dir/cache"
export VK_DRIVER_FILES="$REXGLUE_SYSROOT/usr/share/vulkan/icd.d/lvp_icd.json"
[[ -f "$VK_DRIVER_FILES" ]] || { echo 'Mesa lavapipe ICD is missing.' >&2; exit 2; }
export SDL_VIDEODRIVER=x11 SDL_AUDIODRIVER=alsa
export ALSA_CONFIG_PATH="$run_dir/asound-null.conf"
cat >"$ALSA_CONFIG_PATH" <<'EOF'
pcm.!default { type null }
ctl.!default { type hw card 0 }
EOF
# Allocate a free X display rather than changing an existing desktop/server.
screen="${MOTOGP_CLOUD_SCREEN:-1280x720}"
[[ "$screen" =~ ^[0-9]+x[0-9]+$ ]] || { echo "Invalid MOTOGP_CLOUD_SCREEN" >&2; exit 2; }
Xvfb -displayfd 3 -screen 0 "${screen}x24" -nolisten tcp -ac \
  3>"$run_dir/display" >"$run_dir/xvfb.log" 2>&1 &
xvfb_pid=$!
trap 'kill "$xvfb_pid" 2>/dev/null || true; wait "$xvfb_pid" 2>/dev/null || true' EXIT
for ((i=0; i<50; ++i)); do
  [[ -s "$run_dir/display" ]] && break
  kill -0 "$xvfb_pid" || { cat "$run_dir/xvfb.log" >&2; exit 1; }
  sleep 0.1
done
[[ -s "$run_dir/display" ]] || { echo 'Xvfb did not become ready.' >&2; exit 1; }
export DISPLAY=":$(cat "$run_dir/display")"
printf 'Display: %s\nLogs: %s\n' "$DISPLAY" "$run_dir"
export LD_LIBRARY_PATH="$(dirname "$binary"):$LD_LIBRARY_PATH"
"$binary" --game_data_root="$game_root" \
  --user_data_root="$project_dir/out/linux-game-userdata" \
  --log_file="$run_dir/game.log" --mnk_mode=true "$@"
