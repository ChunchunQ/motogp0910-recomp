# Stability and quality control — 14 October 2026

Windows 10/11 remains the target. This update keeps the exact SDK pin
`0.10.0.24-dev.gbd833a2` and the existing collision, PS3/replay, career readback,
launcher and experimental rendering features.

## Corrections

- **Dynamic TLS:** the old runtime accepted a one-past-end read, counted static
  TLS bytes as dynamic slots, and allowed `slot * 4` to wrap. Reads/writes now
  validate the dynamic region using division before computing the address.
  Rejected operations preserve the output parameter and surrounding memory.
  The new Windows probe failed against the previous DLL with
  `Out-of-range TLS read accepted`, then passed against the fixed DLL.
- **Thread IDs:** concurrent constructors now obtain unique IDs atomically;
  the host processor count cache also uses synchronized local-static
  initialization and treats an unknown count as one usable processor.
- **Title launch paths:** XAM now owns the path string instead of retaining a
  view into a temporary joined string. Actual exported kernel calls validate
  empty, relative, long relative and absolute launch paths and retained flags.
- **Calendar conversion:** FILETIME keeps its 100 ns representation instead of
  converting through POSIX nanoseconds. Both calendar directions retain the
  1601 epoch and far-future dates. The previous conversion's signed overflow
  was reproduced with UBSan; the fixed range/precision test passed.
- **Warnings:** disabled host affinity no longer queries topology or warns on
  every CPU update. When requested affinity cannot be pinned, the warning is
  emitted once. Guest CPU metadata is still written before the early return.
- **Defensive cleanup:** D3D12 texture footprints/sizes have explicit initial
  values. The old loops populate the values before use in valid flows; this
  warning alone was **not** a confirmed career texture defect. Shared-SDL
  window enumeration now has explicit braces, removing its indentation warning.

## Small optimizations

FPS pacing reads its typed cvar directly rather than looking it up by name each
frame. Ultrawide HUD diagnostics use a relaxed read after the first message,
avoiding a locked exchange on every quad. Disabled affinity avoids its repeated
host query and low-core-count log flood. No guest simulation rate, physics,
render quality or save format was changed.

The Windows runtime fixture completed 100,000 unpinned CPU updates in roughly
0.5 ms under Wine. That is a synthetic path measurement, **not** a measured game
FPS increase. Frame-time/GPU comparisons still require a native Windows GPU.

## Validation

The final release DLL hashes were recorded with the test results and matched
both the runtime manifest and packaged ZIP. Fifteen Windows probe executables
completed nineteen runs under Wine, including the five input-precedence cases.

| Check | Result |
|---|---|
| Actual runtime TLS, calendar kernel calls, retained launch paths | 168 checks passed |
| Concurrent thread construction | 8,000 distinct IDs, no duplicates |
| Launcher UI, EN/TR, five pages, 75–150% scales | 51,681 checks passed |
| Native engine effects, projection/HUD and ABI | 637 checks passed |
| Collision conversion and calling convention | 192 checks, zero mismatches |
| Original PS3 artwork and replay replacement | 222 checks passed |
| Virtual Sony/Xbox input switching | 92 checks passed |
| Runtime paint planning, input/audio and native video mode | 183 checks passed |
| Actual host window modes and frame schedule | 33 checks passed |
| Launcher save/backup/conflicting edits and real child process | 91 checks passed |
| Career readback scope and restoration | 13 checks, 16 guest calls |
| Nightly deadzone and input blockers | 54 checks passed |
| Input default/config/environment/CLI precedence | All five scenarios passed |
| Native game language | Six checks passed |
| Generated startup/CRT/resource/race callbacks | Headless integration passed |
| AMD providers | Distinct FSR 2.3.3 and FSR 3.1.4 IDs verified |
| SDK PowerPC and calendar suite | 43 cases, 170 assertions passed |
| FILETIME range and 100 ns precision | 19 checks passed; separate GCC UBSan run clean |

Raw logs, fixture configs and screenshots stay under ignored `out/stability/`.
The initial GUI runs failed to create a window because the old Xvfb display was
no longer running. The final checks used a live isolated display and all passed;
those initial failures are not reported as application defects.

The application/probe build completed without compiler warnings. SDK/vendor
unused-parameter, initializer-order and MSVC deleting-destructor export warnings
remain; they have not been hidden with a blanket warning suppression.

## Repeat the Windows suite

After the normal Windows build and game-input preparation:

```powershell
cmake --build --preset win-amd64-release --target windows_validation_probes --parallel 4
python scripts/verify-windows.py --build-dir out/build/win-amd64-release --game-root 'D:\x\motogp0910\game'
```

Use the actual full extracted game folder. Each probe runs in a new isolated
folder; the suite does not overwrite the user's launcher preferences or saves.
Outputs include per-probe logs, exit codes and the exact DLL hashes.

For the cloud Windows cross build:

```bash
source out/wine-launcher-preview-env.sh
# Start an isolated Xvfb display and set DISPLAY before running GUI probes.
python3 scripts/verify-windows.py \
  --build-dir out/build/windows-nightly-release \
  --game-root 'assets/disc/extracted/MotoGP 09-10' \
  --wine /workspace/shared/sysroot/usr/lib/wine/wine64
```

The SDK source corrections are reproducible through
`cmake/sdk-stability.patch` and `scripts/build-spatial-sdk.py`. The smoke script
uses those patched source headers/tests, not the unmodified package headers.
Optional `REX_SMOKE_SANITIZERS=ON` needs the compiler's UBSan runtime installed;
the independent sanitized range run used GCC 14 because the cloud Clang runtime
was unavailable.

## Remaining checks

Cloud Wine has no suitable D3D12 adapter. A full Windows career/track/AI/restart/
long-session/save-reload sweep, visible career textures, audible sound quality,
GPU FSR dispatch and ultrawide gameplay remain unverified in this update.
FSR 2/3 retains its documented neutral motion/depth and per-frame-history-reset
limitations. Frame generation is excluded. No new Linux renderer work was done.

The complete Windows ZIP is `MotoGP0910-Windows-x64-stability-update.zip`.
Replace the EXEs, all supplied DLLs and resources together; preserve `game/`,
existing configuration and saves. Older stable/nightly ZIPs remain separate
rollback packages. Original game files, XEX, title update, logs and screenshots
are excluded from the release.
