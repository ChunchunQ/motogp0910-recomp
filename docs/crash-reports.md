# Windows crash reports

Crash capture is included with the LAN launcher startup fix. The original
empty-IP launcher/SDK argument bug is documented in [lan-startup-fix.md](lan-startup-fix.md).

## Usage

Extract the complete ZIP and keep `bin/motogp0910-crash-monitor.exe` with the
other game/runtime binaries. Preserve existing game data, settings and saves.
Enable **Advanced > Extended crash diagnostics** (default on), then start with
PLAY in `MotoGP09-10Launcher.exe`. See [installation layout](game-layout.md).

After a crash, use **Logs**. A `crash-...` directory contains `report.txt`,
`crash.dmp` when dump writing succeeds, the latest game log and current
settings. **Report** includes the latest crash directory as `native-crash/`.
Review diagnostic contents before sharing the ZIP.

Disabling the monitor removes external debugger-based capture. Direct EXE
startup retains best-effort in-process handlers, with narrower coverage for
forced exits. Monitoring can add overhead. Closing the launcher does not
terminate the independently running game/monitor.

Normal output is `PROGRAM_FOLDER/logs/crash-.../`; fallback is
`%LOCALAPPDATA%/MotoGP0910/crashes/`, then the Windows temporary directory.
Names contain UTC time, process ID and thread ID.

## Captured data and coverage

The text report records exception code/address/parameters, native thread
registers, module paths/base addresses, a native stack with module offsets,
SDK version and the image SHA-256. Where readable MSVC x64 standard-exception
RTTI is available, the exception type and message are included. A matching
`motogp0910.map` is shipped for resolving native game function addresses;
source line names are not guaranteed without appropriate local PDB symbols.

The external monitor launches the original game under the Windows debug API.
It lets the application's exception handlers receive first-chance exceptions;
only the initial debugger breakpoint is consumed. On an **unhandled second-chance
exception**, the game is stopped long enough to write the report and a standard
minidump, then normal fatal termination continues. Thus fast-fail
`0xC0000409` can be captured even when in-process exception filters are bypassed.
A nonzero exit without an exception produces an exit-status report; it does
not invent an exception address or a diagnosis. Normal zero exits and handled
exceptions produce no crash report.

Direct game EXE startup installs a native unhandled-exception filter and a C++
terminate handler before runtime/presentation setup. They offer best-effort
reports for ordinary native/C++ failures. Forced termination, early loader
failure, stack/heap corruption or an unwritable disk can bypass or defeat
in-process reporting. A failure that the GPU/game handles and exits normally
may appear only in the game log. Memory dumps include selected memory fragments,
configuration and local paths; this is diagnostic data, not a guarantee that
every crash can be completely diagnosed from one report.

## Validation

The production reporter/monitor were exercised in isolated Windows x64 fixture
processes under Wine. **9 scenarios passed.** Actual fatal instructions/exceptions
were raised, and
successful dump files were checked for the **MDMP header, valid stream ranges
and the actual native exception stream**, not merely filename existence.

Scenarios cover normal exit, an internally handled exception, access violation,
fast-fail with parameter `7`, unhandled C++, abnormal exit without an exception,
and direct EXE access violation/C++. The real launcher helper also starts a
monitored delayed child and exits, verifying that the child survives its parent.
Reports retain the faulting module, stack/register data, C++ message when
available, log and settings. Tests use small fixtures, never the user's game.

```powershell
python scripts/verify-crash-reports.py --build-dir out/build/win-amd64-release
```

Build optional `crash_report_probe` first. For cloud Wine validation, add
`--wine /path/to/wine64`. Native Windows GPU/Radmin racing remains a user test;
these diagnostic checks do not validate multiplayer synchronization.
