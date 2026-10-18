# MotoGP 09/10 — ReXGlue Nightly 0.10.0.24-dev.gbd833a2

A native PC recompilation of the Xbox 360 title, targeting **Windows 10/11 x64**
and the supplied stock XEX plus **0.0.1.1** title update. You must provide your
own complete game files. No original XEX, title update, patch or disc data is
included in the downloads.

[Current Windows EXE + DLL package](https://github.com/inflamesgo/motogp0910-recomp/blob/main/downloads/MotoGP0910-Windows-x64-single-launcher.zip)

Use **Download raw file** on the GitHub file page. Extract the entire package;
keep the matching executable, runtime DLLs and resources together. Preserve
existing saves, configuration and game data. Older ZIPs remain available as
historical milestones and do not contain every current fix.

## Play

1. Place all original disc files under `game/`, retaining `default.xex`,
   `default.xexp` or the original TU, `xenondata/`, `nxeart/` and remaining files.
2. Open `MotoGP09-10Launcher.exe`, choose settings and press **PLAY**. Direct
   `bin/motogp0910.exe` startup is also supported.
3. Launcher language and game text language are independent. Select **General
   → Game language → Turkish** for translated game text; original voices remain.

Keyboard defaults: Enter = Start, Space = A, Backspace = B, W/S/A/D = left
stick, Shift + arrows = D-pad. Explicit input settings are preserved.
**F1** opens live settings and **F7** opens achievements.

The title first checks `game/default.xex` in the installation root, then the
legacy flat layout. An explicit `--game_data_root` takes priority. Missing
patch data can be extracted from the supplied original TU without rewriting
the stock XEX or an existing patch. Saves/cache retain the SDK user-data
location; default game logs are in `logs/` in the installation root. The game and
launcher settings share `config.json` in that root, alongside `resources/` and
`game/`. The game/crash monitor, UI library, DLLs, controller database, map and runtime
manifest live in `bin/`. Keep the complete directory structure when extracting.
Existing game/launcher TOMLs migrate automatically; their original bytes remain
in `.legacy.bak` files. F1 saves to the same JSON file. Saves, unlock data and
cache retain their existing user-data location.

Development ZIPs omit README files, feature instructions, SHA256SUMS and separate
license folders. Required third-party notices are embedded in the launcher and
available through **Alt+Space → Third-party notices**. Source licenses remain.
Development binaries are unsigned. Repository Markdown
documentation is maintained in English; translated UI strings remain localized.

Package SHA-256: `0e81d994fb15040bde7b16b34f160ee9bf8d385a4b3a7c6dd208aeacb24301af`.

## Current features and evidence

- **Portable launcher:** the root entry point is `MotoGP09-10Launcher.exe`;
  the UI runs in that process with its library and runtime files in `bin/`,
  without a second launcher EXE. Checkboxes match the orange accent; the
  achievement list has no tooltip covering its cards. Keyboard bindings use rounded key
  buttons: click to capture, Esc to cancel, right-click to unbind. Sliders jump
  directly to the clicked point and retain dragging. Focus outlines and adapter
  VRAM suffixes are removed. Internal rendering now includes 4x (5120×2880).
  The pinned renderer supports integer scales; fractional 1.5x is not exposed.
  Native dropdowns use rounded charcoal panels with orange selection text and
  two-way fades, respecting Windows motion preferences. Rounded orange
  checkboxes accept rapid clicks, and Window size includes common and reported
  monitor modes that fit the physical display.
- **Quality audit fixes:** FSR 2/3 sharpness survives switching to CAS and back;
  achievement search handles accented Latin/Turkish case variants; normal window
  close releases adaptive trigger effects before the SDK exits. Invalid trigger
  errors use racing terminology. Automated CPU/UI and protocol checks are
  distinguished from native Windows GPU and physical hardware verification.
- **Optional debug overlay and startup skipping:** Advanced → Enable debug
  overlay permits F3 to show guest presentation timing, graphics/runtime counters,
  display settings, memory and audio/cache diagnostics. The overlay defaults off
  and does not capture gameplay input. Cross/A or Start, Space or Enter can
  request the native intro fade while preserving the final Press Start screen.
  Intro skipping needs no checkbox; Extended crash diagnostics is in Advanced.
  [Scope and checks](docs/debug-overlay-startup.md).
- **Racing Lines launcher:** dark/orange MotoGP artwork, horizontal tabs,
  native hover help, file verification, larger achievement list and an
  Online tab between Controls and Achievements. Fixed tab redraw/scroll jumps, closed combo-box
  wheel changes and native Windows tooltip registration. Slider/button painting
  now suppresses intermediate erase frames while scrolling and redraws the full
  slider fill after channel clicks. The user confirmed
  the latest tooltip fix works. [Design and validation](docs/launcher-redesign.md).
- **Turkish written localization:** 2,102 native message IDs reviewed, five data
  scripts and 50 achievements localized; voices preserved. Four native font
  banks normalize 48 Turkish glyph cells. The nonfunctional Marketplace row
  is removed in all languages. [Localization](docs/turkish-localization.md).
- **Wet Motegi log audit:** image-volume opens, compression capability queries,
  disk metadata and virtual dismount compatibility corrected. Unbound sampler
  descriptors retain null behavior without repeated false warnings. The real
  experimental FSR warning remains. [Audit and retest limits](docs/motegi-rain-qa.md).
- **Input and UI:** Xbox, supplied original PS3 artwork and live keyboard prompts;
  dynamic control layouts; white QUIT GAME accent; working Press Start Back
  confirmation accepted by the user. [Prompts](docs/controller-prompts.md),
  [keyboard](docs/keyboard-prompts.md), [quit menus](docs/quit-menu.md).
- **Achievements and live settings:** 50 original icons / 1000 G, read-only save
  progress, search/filter and shared pop-up/sound settings; F1 exposes immediate
  options and safely persists supported choices. [Achievements](docs/achievements.md),
  [live settings](docs/in-game-settings.md).
- **DualSense triggers:** experimental, off by default, native DualSense/Edge
  through SDL; no adaptive effects on 8BitDo/DS3/DS4. Physical resistance remains
  untested. [Scope](docs/adaptive-triggers.md).
- **Runtime/game repairs:** missing native callbacks and initializer boundaries,
  serialized collision-coordinate correction, scoped career livery readback,
  TLS/thread/date/launch-path fixes and native crash reporting. The user
  verified races and 4–5 Qatar Time Trial laps without falling through the road.
  [Analysis](docs/analysis.md), [collision](docs/collision-data.md),
  [career](docs/career-textures.md), [stability](docs/stability-quality-control.md),
  [crash reports](docs/crash-reports.md).

Automated Windows PE checks run under Wine; native Windows D3D12 gameplay is
separate evidence. Real Linux software-Vulkan startup reached the main menu.
All modes/tracks, long career sessions, audible sound quality and physical
controller combinations are not comprehensively verified. See the
[current evidence matrix](docs/project-status.md) and [roadmap](docs/roadmap.md).

## Offline, LAN and community Online

**Online → Connection mode** selects Offline, LAN or experimental Online.
The page stays inside the launcher and uses its normal Save / PLAY buttons.
Offline remains the default; unrelated fields are hidden for the selected mode.
LAN supports automatic discovery and optional direct host IPv4 across a local
or compatible virtual LAN. The user reported successful two-PC racing;
reconnects, larger grids and every network environment still need testing.
[LAN usage](docs/lan-multiplayer-test.md), [optional one-PC test](docs/lan-one-pc-test.md).

The separate [Linux/Windows community server package](https://github.com/inflamesgo/motogp0910-recomp/blob/main/downloads/MotoGP0910-Community-Server.zip)
provides HTTPS room discovery, authenticated UDP relay and persistent results
for 378 native leaderboard views. Configure your own HTTPS endpoint; no shared
public service is deployed. A player hosts the race simulation. Use **Player
Match**; official Xbox Live ranked/friends/invites/voice are not restored.
Protocol **MGO2/v2** requires matching game/server versions; LAN02 is retained.

[Deployment and identity](docs/community-online.md),
[security review and remaining limits](docs/community-security.md),
[future account proposal](docs/community-accounts-plan.md).
The portable username/password design remains a proposal. Local Linux/Windows
Python backend and client checks passed; a public VDS Internet race and live
leaderboard UI still require deployment testing. Connection quality and clear
Windows UX remain priorities; no security review guarantees an issue-free host.

## Build on Windows

Use Visual Studio 2022 or newer (including 2026), Desktop development with C++,
MSVC x64 tools, Windows SDK, **Clang 20+**, **CMake 3.25+** and Ninja. The helper
finds VS tools and prepares the x64 environment from ordinary PowerShell.
See the [complete Windows build guide](docs/windows-build.md).

```powershell
git clone https://github.com/inflamesgo/motogp0910-recomp.git
cd motogp0910-recomp
.\scripts\build-windows.ps1 `
  -GameRoot 'D:\Games\MotoGP0910' `
  -TitleUpdatePath 'D:\Updates\TU_11K61VG_0000004000000.0000000000081' `
  -Jobs 2
.\scripts\run-windows.ps1 -GameRoot 'D:\Games\MotoGP0910'
```

The script verifies SDK and game input hashes, extracts the TU, stages only
XEX/patch build inputs, generates code, builds Release and runs a headless
integration probe. It does not copy the entire disc or modify original files.
Output: `out/build/win-amd64-release/`. Script-run saves: `out/userdata`;
script-run log: `out/motogp-windows.log`.

## Cloud development

Use the existing isolated checkout. Rootless setup lives under `/workspace/shared`.
Source the environment helper in every new shell. The actual SDK pin and package
checksums are in `sdk-version.json`; custom runtime checksums are in
`third_party/rexglue-runtime/manifest.json`.

```bash
bash scripts/install-cloud.sh
source scripts/cloud-env.sh
python3 scripts/check-game-inputs.py
rexglue codegen --ignore-stamp motogp0910_manifest.toml
cmake --preset linux-amd64-release -DCMAKE_CXX_COMPILER="$CXX"
cmake --build --preset linux-amd64-release --parallel 4
cmake --build --preset linux-amd64-release --target motogp_headless_probe --parallel 4
out/build/linux-amd64-release/motogp_headless_probe assets/game-tu
bash scripts/smoke-sdk.sh
```

Use the checkout's configured nightly preset/build directory when already
prepared. Code generation must be refreshed when XEX/patch changes; do not
assume its fingerprint tracks sibling patch bytes. Function boundaries are
specific to the analyzed game version. [Real graphical startup](docs/linux-startup.md)
and [runtime reproduction](third_party/rexglue-runtime/readme.md) document the
remaining cloud details. Shipping original game data is excluded.

## Input identity and source layout

Title ID: `434307F0`; Media ID: `321F1634`.

| Input | SHA-256 |
| --- | --- |
| Stock `default.xex` | `54245417e996ed0aa561a5a803fd810e87684bd0b43082f316db711b3ee786ae` |
| Original TU | `441df6cc2e20ac8aeeed475138c1714d477f965423d8c5c9b0202e3a18d8141f` |
| Extracted `default.xexp` | `0419205ba10dc65eaa7b0bb62abc64cada46119ece5ed749e4ba20c06a6bc079` |

`src/` contains title hooks, launcher, UI and transport; `tools/` contains native
probes/extraction helpers; `scripts/` handles setup, builds, checks and packages;
`cmake/` owns pinned SDK patches; `resources/` contains shipped UI/localization
assets; `server/` contains the community service; `docs/` holds English guides
and evidence; `downloads/` preserves published milestone archives. Generated
code and private game inputs are local build artifacts. Required third-party
source notices are retained. Documentation policy is recorded in [AGENTS.md](AGENTS.md).
