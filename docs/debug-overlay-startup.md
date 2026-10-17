# Diagnostic overlay and startup input

The Windows development package adds an optional diagnostic overlay and manual
startup skipping. These changes preserve the existing launcher paint fixes,
localization resources and game/runtime compatibility fixes.

## Launcher options

Advanced contains **Enable debug overlay** (default off) and **Extended crash
diagnostics**. Debug permission is stored in the game TOML; crash diagnostics
retain the existing launcher preference and behavior. They apply on the next
game launch. Launcher language and game text language remain independent.

Manual intro skipping is enabled without a launcher checkbox. Saving or
launching from the launcher also replaces a stale disabled intro preference
from the previous development build. The internal runtime switch remains
available for diagnostic checks.

F3 opens the project overlay only when permission is enabled. The unconditional
SDK debug binding is replaced before gameplay, preserving its configured key.
The owned callback is unregistered during shutdown. The overlay is read only
and does not capture mouse, keyboard or controller input.

The three diagnostic panels provide guest presentation interval/FPS and its
history, UI rate, draw calls, vertices, command stalls, dispatch/interrupt
counters, threads, APC/lock counters, process resident memory, display dimensions,
render scaling, backend, VSync, MSAA, upscaling, FSR quality, aspect, input,
configured network mode, audio and texture/pipeline cache counters. These are
not GPU utilization, GPU temperature, actual GPU execution duration, Internet
latency or VRAM measurements.

Guest timing uses the existing pacing/frame-boundary hook. GPU snapshots are
read through a versioned C export owned by the GPU module; runtime totals and
gauges use live runtime counters. Missing GPU bridge/instrumentation is shown
as unavailable rather than manufactured zero readings. Counter collection can
still be absent on an uninstrumented path; a zero value is not proof of coverage.
The matching custom runtime includes the GPU bridge and lightweight counter
instrumentation in Release builds. Tracy profiling remains disabled in Release.
The runtime archive and DLL hashes are recorded in the existing manifest.

## Manual startup skipping

Cross/A or Start on a controller, or fresh Space/Enter on the keyboard, requests
the existing native startup fade/cleanup path. Default input mappings are not
rewritten. The request requires a focused, initialized, non-final startup entry;
minimum native startup frames, count/index bounds and the final Press Start
entry are checked. Holding an accepting button cannot request another entry,
and the SDK's release mask prevents it activating the next menu.

The native startup updater is `8229C2C0`. It reads the request byte at
`82942CB2`, current/count words at `82942CC0`/`82942CC4`, and frame guard at
`82942CBC`. The owning hook preserves the request through the fade and restores
it after the native index advances; it does not jump indices, delete videos,
skip race cutscenes or bypass save/profile initialization.

## Validation

The final Windows PE/Wine run passed all 27 suite entries using the same DLL
hashes as the development ZIP: 76 debug option/timing/F3/ImGui checks, 12 GPU
diagnostic ABI checks, 127 quit/startup/input checks, 99 launcher configuration
checks and 87,871 actual launcher UI checks. The ZIP was checked for matching
runtime hashes and absence of original game data, personal config/logs, readmes,
checksum files and a separate license folder.

The Windows verification script includes the timing/F3/ImGui probe and the GPU
ABI probe alongside the native startup/input and launcher regressions. The GPU
probe checks both counter ownership and the actual packaged Release DLL export,
including its version, instrumentation availability and output bounds.

An earlier real running Linux software-Vulkan check accepted Space and advanced
startup entry 0 to 1; F3 drew the detailed overlay. That check exposed missing
GPU counters and a renderer label, which motivated the owning-module bridge,
separate timing hook and actual backend identification in this package.

Wine CPU/UI checks and Linux software-Vulkan observations do not replace native
Windows D3D12 gameplay, final visual inspection or physical controller testing.
Keep saves, original game data and historical archives intact.

## Distributed files

| File | Purpose | Effect of removing it |
| --- | --- | --- |
| `motogp0910.map` | Resolves native addresses in crash reports | Gameplay can run; diagnosis loses readable address mapping |
| `rexruntime.dll` | Runtime, kernel, input, audio and common systems | Game cannot run |
| `rexgpu-xenos.dll` | Xenos graphics implementation | Game cannot render/run normally |
| `msvcp140.dll` | Microsoft C++ standard library | May prevent startup unless a compatible runtime is installed |
| `msvcp140_atomic_wait.dll` | Microsoft atomic wait support | May prevent startup; keep the matching shipped file |
| `vcruntime140.dll`, `vcruntime140_1.dll` | Microsoft compiler/exception runtime | May prevent startup; keep the matching shipped files |
| `runtime-manifest.json` | Identifies the exact custom runtime and hashes for packaging/diagnostics | Not the game engine; removing it loses runtime provenance in reports |

The optional diagnostic files are retained during QA. Required third-party
notices remain embedded in the launcher; development archives omit readmes,
SHA256SUMS and separate license folders.
