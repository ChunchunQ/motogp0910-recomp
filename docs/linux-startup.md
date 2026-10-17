# Reproducing real game startup in the Linux cloud

Windows 10/11 is the product target. This Linux check uses complete disc data
to inspect the running game's menu; a passing headless probe or absence of
fatal messages alone does not establish that a menu rendered.

Install rootless dependencies with `scripts/install-cloud.sh`. The current
SDK is pinned in `sdk-version.json`. Xvfb provides a virtual display and Mesa
lavapipe provides CPU Vulkan rendering. The ALSA null sink opens a real SDL
audio backend but does not validate audible sound quality.

Build Linux as described in the [README](../README.md). Code generation uses
`assets/game-tu/default.xex` and `default.xexp`; runtime additionally needs the
complete original `xenondata/`, `nxeart/` and remaining disc layout.

```bash
bash scripts/run-linux-cloud.sh '/absolute/path/to/MotoGP 09-10'
```

The helper reports its display and a unique log directory. For `Display: :0`,
use another shell:

```bash
source scripts/cloud-env.sh
import -display :0 -window root out/linux-startup/frame.png
DISPLAY=:0 xdotool search --name motogp0910 windowfocus keydown Return
# Hold input across several frames, then release it:
DISPLAY=:0 xdotool keyup Return
```

`--mnk_mode=true` enables the normal SDK keyboard emulation: Enter = Start,
Space = A, Backspace = B, W/S/A/D = left stick, Shift + arrows = D-pad. Short
key presses may be missed by slow software-rendered frames; hold then release
keys and inspect captures. A black frame or CAPCOM logo is not a menu check.

The helper uses isolated `out/linux-game-userdata/` and unique
`out/linux-startup/run-*` logs. It stops only its own Xvfb process on exit.
Original game files are preserved; the SDK applies the sibling title patch
in memory. An absent patch can use the application's existing TU extraction.

In GDB, use `handle SIGSEGV nostop noprint pass`: guest memory/MMIO relies on
an SDK SIGSEGV handler. A debugger's first stop on that signal alone is not
proof of a crash. Actual aborts and unregistered calls require investigation.
