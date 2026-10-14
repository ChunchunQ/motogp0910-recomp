# Launcher update validation — 14 October 2026

ReXGlue remains pinned to v0.10.0 / f5337cdc947ff6d4c4196737e2c807a48f2a1fc2.
Both new EXEs were built as Windows AMD64; the archive includes the matched
manifest-verified runtime pair, original PS3 prompt pack and controller database.

| Check | Result |
| --- | --- |
| Five-page EN/TR UI, 75/100/125/150% scales, persistence/CLI | 50,408 assertions passed |
| Launcher configuration, preservation and real process launch | 91 checks passed |
| Host fullscreen/borderless/windowed and frame scheduling | 33 checks passed in Wine/Xvfb |
| Runtime presentation planner / settings | 140 checks passed |
| Native game language / PPC ABI | 6 checks passed |
| Native camera flags, parameter preservation, shadow override scope | 277 checks passed on Linux and Windows/Wine |
| Original PS3/replay artwork, descriptors and draw ABI | 222 checks passed on Linux and Windows/Wine |
| Actual shared SDL with virtual Xbox/Sony inputs and keyboard switches | 92 checks passed |
| Collision coordinates / endian conversion / ABI | 192 checks, no mismatches |
| Startup, initializers, math, resource comparator and race-start callback | 50 behavior checks; 7,144 initializer targets registered |

A new Linux/Vulkan/lavapipe run with the complete disk data reached the main
menu with a 30 FPS cap and the supported engine effects disabled. Keyboard
menu navigation was observed, and the main menu remained alive for more than
two minutes without an unregistered-call abort. The run was then stopped
intentionally with SIGTERM. The pre-existing bright/white menu ribbons on
this software Vulkan backend are also present in the older validation images;
they are not established as a new launcher regression.

This environment does not provide a physical Windows D3D12 GPU/monitor.
Physical display refresh switching, real controller hardware, complete races,
career appearance and every GPU effect still need local Windows verification.
CPU probes and reaching the menu do not establish that all gameplay is correct.

No new screenshots or raw gameplay logs are committed. Package CRC, every
member's SHA-256 and EXE-to-build equality are checked before publication.
FSR 2/3 is suspended pending clarification; frame generation is dropped.
Auto/16:9 remains the current aspect behavior. True ultrawide and further
engine controls remain pending, not advertised as completed features.

The experimental FSR/ultrawide milestone is documented in
[experimental-rendering.md](experimental-rendering.md). Linux reached Main Menu
at 21:9; Windows GPU initialization is unavailable in the cloud Wine environment.

Exact Nightly migration passed the checks in [sdk-nightly-migration.md](sdk-nightly-migration.md).
