# Binary analysis and startup repair record

Addresses below apply only to the supplied binaries identified by hashes in
[README.md](../README.md). Historical investigations began with ReXGlue v0.10.0;
the current SDK pin is `sdk-version.json`. Original binary dumps are private.

| Version | XEX entry | Missed discovery target | Validated boundary |
| --- | --- | --- | --- |
| Stock 0.0.0.1 | `0x823A6688` | `0x825F15C8` | `[0x825F15C8, 0x825F15D0)` |
| TU 0.0.1.1 | `0x823A5E10` | `0x82709EE0` | `[0x82709EE0, 0x82709FD8)` |

## Initial discovery and title update

Stock analysis could not resolve the reference from `0x825F15D4`. Disassembly
identified `addi r3,r3,8` followed by a tail branch to registered `0x825EFF28`:
an eight-byte this-adjustment thunk. Its explicit boundary completed stock
code generation without unresolved calls. Stock rules were not copied blindly
to the patched image.

TU analysis encountered an unresolved branch from `0x8270A0A4` to `0x82709EE0`.
The target begins with `fabs f13,f1`, returns through `0x82709FB4` and
`0x82709FD4`, and ends before the next routine at `0x82709FD8`. Wrapper
`0x8270A0A0` selects r4=1. Ten real native checks covered asin with r4=0 and
acos with r4=1 across five inputs.

The LIVE/STFS TU contains `default.xexp`. The runtime's sibling patch loader
verified source signature, image key and patch-block hashes, then applied
0.0.0.1 → 0.0.1.1 in memory. Stock XEX bytes remain unchanged. These boundaries
expose original instructions to code generation; they do not replace routines
with stubs. `--force` was not used. Successful discovery is not proof of every
game function's correctness.

A Windows STFS helper extracted the expected patch SHA-256
`0419205ba10dc65eaa7b0bb62abc64cada46119ece5ed749e4ba20c06a6bc079`.
An isolated packaged startup from another working directory generated that
patch when absent, then stopped at unavailable D3D12 initialization on the
Linux host. That test established patch preparation, not gameplay.

## Native Windows callback failure: `0x8270DF08`

The 12 October user log showed D3D12/SDL setup on GTX 1650 SUPER and successful
TU application, then an indirect call to unregistered `0x8270DF08`.

| Address | Instruction | Word |
| --- | --- | --- |
| `8270DF08` | `cmpwi cr6,r3,0` | `2F030000` |
| `8270DF0C` | `bnelr cr6` | `4C9A0020` |
| `8270DF10` | `b 8270EA60` | `48000B50` |

The next routine starts at `8270DF14`. CRT initializer `8270EAD0` constructs
the callback through split immediates; `823B1840` calls it with `bctrl`.
The missing standard prologue and split address explain the discovery gap.
Boundary `[8270DF08,8270DF14)` emits the original three instructions: zero
calls existing TLS cleanup, nonzero returns. BO=4/BI=26 confirms `bnelr`.

The old table reproduced the missing-registration failure. The corrected
probe tested 0, 1, 7FFFFFFF, 80000000 and FFFFFFFF, preserving registers and
stack. The zero fixture temporarily sets TLS slot `82941AF0` to FFFFFFFF to
exercise disabled-TLS cleanup and restores it. Active guest heap/TLS teardown
is outside that focused test. Ten math plus five callback checks passed on
Windows PE/Wine; that did not establish complete Windows startup.

## CRT initializers, resource comparator and video interfaces

A second user log exposed `0x82826D90`. All 7,144 entries in the TU CRT table
`82874614..8287B5B4` were checked; 24 registration initializers were missing.
They load constant node/name/payload/size fields and tail-call `82384B98`.
Boundaries stop at terminal branches, excluding padding; for example
`82826E30..82826E54`. The probe runs all 24, checks five-word nodes and list
heads, then restores modified fixture memory.

Full-data Linux startup next exposed `82500660`, a prologue-free leaf comparing
packed 16-bit key maxima in two linked lists. Returns are at `825006D0` and
`825006E4`; the next pdata routine begins at `825006E8`. Six fixtures cover
empty lists, ordering, equality and multi-node cases without stubbing it.

Video startup required `825F9BE0`, a Bink unlock callback tail-calling vtable
slot +20 in six instructions; `825F9BF4` is its terminal `bctr` and
`825F9BF8` the next pdata routine. Twenty-one additional eight-byte
`addi r3,r3,offset; b registered_target` thunks were validated in related
interface tables. Their targets were already registered.

Menu preparation exposed getter `826543E8`: null output returns 37, valid
output receives the object's +28 field and returns 0. Both paths belong to
`[826543E8,82654408)`, despite the old split discovery at `826543F8`.
Thirteen additional missing leaf/wrapper entries were disassembled. Split
suffixes were reunited for `8258CA20..8258CA40` and `825FE308..825FE33C`.
Exception metadata's internal labels were not registered as ordinary calls;
`82848684` in a packed numeric table pointed into padding and was rejected as
a false positive. At that milestone the manifest held 62 validated boundaries.

## Real Linux menu observation

Xvfb 1280 × 720 and Mesa lavapipe rendered the real intro/title, initial profile
and difficulty screens, then Championship, Career, Arcade, Time Trial,
Multiplayer and Options without a modal. The menu was observed for at least
two minutes with no fatal/unregistered-call message. The approximately
nine-minute session was deliberately terminated with SIGTERM, not reported
as a successful normal game exit. [Captured frames](screenshots/README.md)
are direct captures.

The milestone CPU probe passed 47 cases: 10 math, 5 callback, 24 initializer,
6 comparator and 2 getter checks, plus all 7,144 initializer registrations.
ALSA null output opened six-channel/48 kHz SDL audio without testing audible
quality. GDB must pass the SDK's guest memory/MMIO SIGSEGV handling through;
a first debugger signal stop alone is not a crash.

## Race-start failure: `0x82271650`

The 13 October Windows log reached menu, rider/track selection and Losail
Time Trial preparation, then failed on unregistered `0x82271650` after Start.
Disassembly validated the 15-instruction wrapper `[82271650,8227168C)`.
It loads float pointers from +12 of parameter objects at `82917A44`,
`82917A68`, `82917A8C`, moves r7 to r10, selects r6=8 and tail-calls
`82270FC8` at `82271688`. `8227168C` is padding, `82271690` a separate short
wrapper and `82271698` the next pdata function. Initializer `827A2D38`
constructs its address and writes object offsets 8068, 10244 and 12420.

The manifest adds the real routine, bringing the analyzed boundary count to
63. Parameters, helper algorithm and GPU settings remain unchanged. Three
signed-direction fixtures run the real indirect callback/helper and check
clamping, r6, floats, LR/stack/nonvolatile return and preserved dormant output.
Temporary parameter pointers and memory are restored. Windows PE/Wine passed
50 cases and all initializer targets. This is not a complete race simulation;
the user subsequently reported working gameplay with the corrected package.

## File layout, direct launch and launcher milestones

The game-root change prioritizes explicit paths, then `game/default.xex`, then
legacy flat data. Wine checks used Unicode/space-containing paths and another
working directory. Correct patch location/hash, unchanged stock XEX, retained
existing patch bytes/mtime and explicit-path precedence were verified. See
[layout](game-layout.md).

Direct startup enables keyboard emulation only when `mnk_mode` remains the SDK
default; five real configuration precedence scenarios passed. See
[direct launch](direct-launch.md). The unchanged supplied controller database
loaded through actual SDK SDL on Linux and Windows PE/Wine; counts and physical
hardware limits are in [mapping notes](controller-mappings.md).

The native launcher uses toml++ to preserve unmanaged settings, retain a first
backup, detect external changes and save atomically. `CreateProcessW` receives
the selected game root/options with the executable folder as working directory;
custom save/cache paths remain intact. Early 46-case launcher checks covered
Unicode arguments/CWD and EN/TR GUI save/reload, defaults and bounded reports.
A Wine-only folder-picker problem came from missing COM/driver registration,
not original game data. An empty `nxeart` is accepted; `xenondata` must contain
data. Current launcher behavior/tests supersede those historical UI counts;
see [launcher](launcher.md) and [redesign](launcher-redesign.md).

The [Motegi rain audit](motegi-rain-qa.md) records the current kernel/VFS,
unbound sampler diagnostics and font corrections. It distinguishes original
user observations, real executed probes and remaining native Windows tests.
