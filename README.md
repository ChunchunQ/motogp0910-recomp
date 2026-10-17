# MotoGP 09/10 — ReXGlue Nightly 0.10.0.24-dev.gbd833a2

[Racing Lines launcher quality fix](docs/launcher-redesign.md): removed repopulating all menus during tab switches; moved scrollbars to a single render path. Mouse wheel no longer changes values in closed selection boxes.

[New Racing Lines launcher](docs/launcher-redesign.md): concept B is implemented; 940 × 578 window, horizontal tabs, minimize/close icons over the background, and cancellable game file validation inside General.

Future development packages do not include README, feature guidelines, SHA256SUMS, or a separate `licenses` folder. Required third-party notices are embedded in the launcher; read via Alt+Space → Third-party notices. Required source files are preserved.

Native PC recompilation project of the Xbox 360 version. The primary target is game version **0.0.1.1** with the provided stock XEX + title update. Windows x64 executable is built; **reaching the main menu on Linux with full game data is verified**. [Latest Windows EXE + DLL package is ready](https://github.com/inflamesgo/motogp0910-recomp/blob/main/downloads/MotoGP0910-Windows-x64-launcher-tooltip-fix.zip). Target user system is Windows 10/11; this cloud machine is Linux.

[Turkish in-game texts](docs/turkish-localization.md) were added: **General → Game language → Türkçe**. Menus, career, tutorials, race messages, technical descriptions, and 50 achievements are translated; voices are preserved. Original fonts now include Ğ/ğ, İ/ı, and Ş/ş. Non-functional **Xbox LIVE Marketplace** option was removed from Options. Visual check of the new translation on Windows GPU is pending.

[Controller prompt screen](docs/controller-prompts.md) now shows original PS3 DualShock 3 visuals or a dark/orange high-contrast keyboard list. Default / Alternative A / B descriptions update by selected layout. Achievement scrollbar is verified with real mouse dragging; launcher top bar has only minimize/close. QUIT GAME white highlight and Press Start exit flow are user-confirmed.

[F1 in-game settings menu](docs/in-game-settings.md) added: same dark/orange design as achievements, only instant-apply audio/graphics/effects/icon/keybind options. **Save** also updates launcher settings; options requiring restart are not shown here. **337-target Windows/Wine checks passed**; native Windows visual/in-game validation is pending.

[Main menu QUIT GAME and Press Start exit confirmation](docs/quit-menu.md): QUIT GAME highlight is white; Press Start exit logic fixed to read real startup state (`828DA060`). **98-target Windows/Wine checks passed**. Native Windows appearance/exit test is pending.

[Launcher achievements page, F7 menu, and soft sound](docs/achievements.md): **50 original icons / 1000 G**, earned/locked lists, search, progress bar, and shared notification/sound preferences. Existing saves are read-only and update while the game is running. **286 runtime and 59,199 launcher UI checks passed**. [Current project status and next tasks](docs/project-status.md). [Highest-impact tasks from current work](docs/project-priorities.md).

[Keyboard prompts refreshed](docs/keyboard-prompts.md): dark keys, orange MotoGP accents, and Open Sans font; real key mappings are preserved. Current interface requests are handled in order, as separate deliveries.

[Development notes and roadmap](docs/roadmap.md): [game/ layout completed](docs/game-layout.md); [refreshed English/Turkish launcher; 720p/1440p/4K, FSR 1.0, audio, and simplified key mapping](docs/launcher.md). [Stability fixes and Windows regression results](docs/stability-quality-control.md) are available; [Radmin VPN/LAN source research and route](docs/lan-research.md) is ready. [Initial LAN discovery/guest session infrastructure](docs/lan-lobby-preview.md) was verified with two separate Windows processes. [New experimental LAN integration and two-PC usage steps](docs/lan-multiplayer-test.md) is ready; **user reported smooth two-player Radmin racing**. Two first-login lobby bugs were fixed in [lobby stability patch](docs/lan-lobby-stability.md); Radmin test is pending for the new EXE. [Launcher startup crash with empty IP fixed](docs/lan-startup-fix.md); 17-target Windows/Wine startup checks passed. [Automatic crash reports](docs/crash-reports.md) and optional launcher tracing were added. User reported the game works fine after the latest race-start fix; full mode/track quality control is still incomplete.

[Experimental LAN Windows build](https://github.com/inflamesgo/motogp0910-recomp/blob/main/downloads/MotoGP0910-Windows-x64-launcher-tooltip-fix.zip): game EXE uses MotoGP 09/10 cover from user; launcher uses separate MotoGP logo. Offline gameplay stays default; Launcher always opens on General page. Clean package includes no game data, saves, personal settings, or logs. Optional [single-PC connection test](docs/lan-one-pc-test.md) script is in source repo. LAN option in Multiplayer opens automatic room discovery with optional IP fallback. [Scope, limits, and usage](docs/lan-multiplayer-test.md).

**Current usage: [latest source repository](https://github.com/inflamesgo/motogp0910-recomp/tree/main) and [native Windows build guide](docs/windows-build.md).** Current package includes prior startup fixes and `0x82271650` callback registration for race transition. Manifest now includes missing CRT callback/initializer, resource comparer, and video/UI functions with real command bounds; game routines are not replaced with stubs. [Real Linux startup screenshots](docs/screenshots/) and [steps to rerun Linux graphics attempt](docs/linux-startup.md) are in the repo.


## Verified

- ReXGlue **Nightly 0.10.0.24-dev.gbd833a2**, commit `bd833a2ab69d305f99ddf57418fc38ffec8af93d`, is in use. Version and official package hashes are pinned in `sdk-version.json`. 0.9.0 setup and outputs were removed.
- Provided XEX and LIVE/STFS title update: Title ID `434307F0`, Media ID `321F1634`.
- Title update opened with SDK STFS reader; `default.xexp` extracted. Patch applied with SDK hash/key checks preserved: `0.0.0.1` → `0.0.1.1`.
- One missing function boundary in each stock/TU analysis was determined from real disassembly. TU manifest generated code with zero unresolved calls; `--force` was not used.
- For TU version, 113 C++ translation units and 30,626 function mapping entries were generated; manifest has 63 verified function boundaries. Current Windows Release executable was built.
- Linux integration probe for the prior menu fix passed **47/47 behavior checks**: 10 asin/acos, 5 CRT callbacks, 24 registry initializers, 6 resource comparer cases, and 2 interface getter cases. All 7144 CRT initializer targets in real XEX are registered. This probe alone is not menu/race validation.
- SDK PowerPC + date tests passed **43/43 cases, 170 assertions**. Legacy chrono 1601-epoch overflow fixed by `sdk-stability.patch`; additional **19 checks under UBSan** passed for 1601–9999 range and 100 ns precision. Windows presentation path and extra fixes ship with checksum-pinned custom runtime pair.
- Current stability sweep: TLS bound/overflow protection, thread-ID race condition, startup path string lifetime, and CPU cache fixes applied. Under Windows/Wine, **168 real runtime checks**, 8,000 concurrent IDs, and launcher/login/physics/FSR scheduler tests passed. [Scope and rerun details](docs/stability-quality-control.md).

- For Linux GUI, Xvfb + Mesa lavapipe Vulkan installed. With full disk data, startup video, title screen, initial profile/difficulty setup passed and **real main menu was reached**. [Screenshot](docs/screenshots/linux-main-menu.png) was captured directly from Xvfb. ALSA null endpoint enables audio backend; audible quality was not tested.
- Windows x64 PE executable was cross-built with Clang 20/MSVC ABI and official Windows SDK. Current Windows PE regression under Wine passed **50/50 checks** (previous 47 cases + 3 callback cases for race start); real XEX 0.0.1.1 and 7144 initializer targets were verified. Windows STFS extractor output also matched expected patch SHA-256. GUI attempt in this machine stopped at Direct3D 12 initialization due to no screen/GPU access. User validated Windows main menu, rider/track selection, and Time Trial prep on GTX 1650 SUPER. Reported unregistered callback `0x82271650` after Start is fixed in current package; user reported game works properly after latest fix.
- Extended Windows launcher and matching FSR 1/CAS runtime pair were built. **58 launcher + 140 runtime + 50 game-code checks passed**; real English/Turkish GUI tabs, save/reopen, and key capture were tested under Wine/Xvfb. FSR tests validate real SDK pass scheduling; they are not race tests with new options on Windows GPU. [Settings, tests, and screenshots](docs/launcher.md).


- Xbox/PlayStation/keyboard selection for in-game saved key icons is implemented. Current Windows PE checks under Wine passed **337/337** (44 icon renders, 45 input/mapping, 58 launcher, 140 runtime, 50 game-code). Keyboard icons show real saved mappings. [Scope, auto-detection, and XInput/PS testing](docs/controller-prompts.md).

- Launcher UI refreshed with dark cards and orange/red accents matching MotoGP cover palette. Single internal resolution choice and 25 direct key fields are available. Layout across three pages in two languages at 75%/100%/125%/150% scale was validated; **72 launcher + 140 runtime checks passed**. TOML conversion that caused numeric startup settings to be passed as booleans was also fixed. [Appearance and validation](docs/launcher.md).

- Bit loss in collision-record endian conversion that altered finite coordinates was fixed. With real XEX routines, **192/192 data and ABI checks passed** on Linux and Windows/Wine; legacy Windows routine reproduces the same bug in test. User validated on Windows current build that in Qatar Time Trial they no longer fell through track for 4–5 laps; provided log shows repair activated on real collision records. [Fix and test scope](docs/collision-data.md).

- Original button overlays from user PS3 dump were added: face buttons, L1/R1, L2/R2, SELECT, analog, and D-pad directions. Converted: 21 saved icons, 5 keys above Second Chance panel, and 8 keys in full replay panel. Bottom rewind/camera row accidentally changed in v2 is preserved in v3. Under Windows/Wine, **222 render/pixel-transfer/replay-panel checks passed**; user confirmed PS icons in menu are fixed in v2; v3 replay panel in-game Windows test is left to user. 256/512-byte row-stride mismatch causing fragmented icons in first PS3 build was fixed. Extract `resources/controller/` folder from ZIP too. Separate START, PS/Guide, and text L3/R3 overlays were not found. Career overlay readback fix added to new package; Windows race visuals will be user-tested. [Scope](docs/controller-prompts.md).

- Broad readback path that writes GPU result back to CPU memory while building custom rider/bike textures in Career mode is enabled. **13 scope/ABI (16 guest calls), 72 launcher, and 140 runtime checks passed**; with current pair, PS3 icon/input/collision/startup checks also passed. In-game Career validation is left to user local Windows device by request. SDK/platform text and ribbons in lower-left launcher that obscured elements were removed. [Fix scope](docs/career-textures.md).

- DualSense / Edge adaptive triggers added as **experimental**: Launcher → Controls and F1 → Controls now have separate right/left mode, start, end, and force. Default is off; local SDL required. Downpour SDL approach was reviewed from sources. No effects sent to 8BitDo/DS3/DS4; physical DualSense resistance test not yet done. [Sources and scope](docs/adaptive-triggers.md).
- User confirmed exit confirmation now works when pressing back from Press Start. The blue leftover part above QUIT GAME was also converted to white/gray while preserving layer glow and alpha; upper-layer appearance was user-confirmed too.

## Community server: Offline / LAN / Online

**Server security update:** Online protocol is now **MGO2 / v2**; update game and server packages together. Added UDP address proof, membership checks against stale room responses, HTTP/TLS connection limits, validated proxy headers, and database quotas. [Detailed security review and remaining limits](docs/community-security.md). Existing database/profiles are preserved; LAN02 and offline gameplay are unchanged.

Second review fixed fake/replay UDP packets consuming normal player quota and invalid identities filling quota table. Limits were fixed for 20 concurrent players behind same modem and repeated reconnects; Windows Python/Wine tests passed. [Current profile ID and recovery limits](docs/community-online.md#how-player-identity-works) are documented. Portable username/password account system is **not added yet**; [design note](docs/community-accounts-plan.md) is kept for future implementation.

In the window opened by General → Multiplayer Settings button, **Connection mode** can select Offline, LAN, or experimental Online. [Linux and Windows VDS server package](https://github.com/inflamesgo/motogp0910-recomp/blob/main/downloads/MotoGP0910-Community-Server.zip) provides room discovery, validated UDP relay, and persistent results in local 378 leaderboard view. [Setup and scope](docs/community-online.md). Online needs your own HTTPS server address; no shared public server exists yet. Race simulation runs on one of the players; use **Player Match**. Official Xbox Live ranked/friends/voice services are not restored. Existing LAN option remains. Local Windows/Wine checks passed; race and live leaderboard tests over real VDS are pending.

**Turkish loading fix:** translation files being skipped in the game's real file-open path is fixed. Main menu/Press Start opens in Turkish; all 2,102 texts in running game now match catalog. Rendering of İ/ı in small font is also fixed. Extract full ZIP and choose **General → Game language → Türkçe**. [Details](docs/turkish-localization.md).

## Ready Windows package

[Current Windows x64 EXE + DLL ZIP](https://github.com/inflamesgo/motogp0910-recomp/blob/main/downloads/MotoGP0910-Windows-x64-launcher-tooltip-fix.zip). After logging into GitHub, download with **Download raw file** button on the file page. Package uses **ReXGlue Nightly 0.10.0.24-dev.gbd833a2**; Visual Studio/build tools are not required.

1. Extract ZIP into program folder; keep new EXE/DLLs at top level.
2. Put all original disc files, `default.xex`, `default.xexp`, TU package, `xenondata/`, and `nxeart/` together under **`game/`** subfolder. [Move guide](docs/game-layout.md) shows sample layout.
3. Open `motogp0910-launcher.exe`, choose settings, and press **PLAY**. **General → Launcher language** switches launcher UI to Turkish; language choice is saved. **Game language → Türkçe** switches in-game texts to Turkish; the two language selections are independent. You can also run game directly with `motogp0910.exe`. Keyboard defaults are enabled: Enter=Start, Space=A, Backspace=B; W/S/A/D=left stick, Shift+arrows=D-pad.

[Direct EXE launch and startup preference notes](docs/direct-launch.md). Current development package does not include `.cmd`, README, or feature guidelines; required game resources and licenses are preserved. User-provided `gamecontrollerdb.txt` is packaged beside EXE; used for SDL gamepad mappings. [Controller mapping notes](docs/controller-mappings.md).

When no explicit game path is given, new EXE first looks for `game/default.xex`; if not found, it uses legacy flat layout. If both locations contain XEX, `game/` is prioritized. On first run, app extracts `default.xexp` from title update in game root; it does not alter original XEX or existing patch. Game version should be **0.0.1.1**. Game binaries, patch, and disc data are not included in ZIP. Save/cache default is SDK user folder; default logs are under `logs/` next to EXE. Windows 10/11 x64 and suitable Direct3D 12 GPU are required.

On Linux, full game main menu and at least two minutes crash-free observation are verified. Current Windows PE probe passed 50 checks. User reported previous race fix and `game/` layout worked without issues. Startup preference checks in this delivery passed 5/5 under Wine in Windows PE probe; new EXE was not played on Windows GPU in this cloud environment. Full mode/track quality control and audible audio quality are still unverified. Old `MotoGP0910-Windows-x64-preview.zip` does not include these fixes.

Package SHA-256: `6a3d6e38a6273d6b887f4bb1d65b14e88bca35a104df1d9944f8c239bcde8326`.

## Build from source on Windows

No need to upload 3 GB game data to cloud. You can open this project package on your own machine and use the fully extracted game folder there. Game data is not included in this project package.

Required tools: Visual Studio **2022 or newer (including 2026)** with Desktop development with C++ components, Windows SDK, **Clang 20+**, CMake 3.25+, and Ninja. Clang 22 and C++ CMake tools in VS 2026 satisfy these requirements.

Current script automatically prepares x64 environment of installed VS version from normal PowerShell; finds Clang, CMake, and Ninja inside VS. It can also use suitable external LLVM/PATH tools. Setup links, RAM-based Jobs suggestions, and log capture command are in the [detailed guide](docs/windows-build.md).

Run this command in project folder with your file paths:

```powershell
.\scripts\build-windows.ps1 `
  -GameRoot 'D:\Games\MotoGP0910' `
  -TitleUpdatePath 'D:\Updates\TU_11K61VG_0000004000000.0000000000081' `
  -Jobs 2
```

The script downloads the official Windows AMD64 SDK package pinned in `sdk-version.json` and verifies it with SHA-256 from GitHub release. It checks provided XEX/TU hashes; builds STFS extraction tool, extracts patch, and prepares only XEX/patch under local `assets/game-tu`. It does not copy full game folder. It runs codegen, Release build, and headless integration probe. Native command failures are not hidden.

Output of successful local build: `out/build/win-amd64-release/motogp0910.exe`. SDK runtime DLLs and Xenos GPU plugin are prepared beside it. Windows cross-build is also done in cloud for same target; local script is for rebuilding on your own Windows machine.

Run attempt with full game folder:

```powershell
.\scripts\run-windows.ps1 -GameRoot 'D:\Games\MotoGP0910'
```

Run script checks original XEX hash. It adds extracted/verified `default.xexp` into full game folder only if missing; it does not overwrite an existing different patch. Real Windows test on GTX 1650 SUPER confirmed menu and Time Trial prep screen. Missing callback after Start was added in current sources; race validation should be repeated with the new package.

## Continue in cloud environment

Task already runs on an isolated machine; use the current checkout and do not create additional Git worktrees.

```bash
cd /workspace/motogp0910-recomp
bash scripts/install-cloud.sh
source scripts/cloud-env.sh
python3 scripts/check-game-inputs.py
rexglue codegen --ignore-stamp motogp0910_manifest.toml
cmake --preset linux-amd64-release -DCMAKE_CXX_COMPILER="$CXX"
cmake --build --preset linux-amd64-release --parallel 4
cmake --build --preset linux-amd64-release --target motogp_headless_probe --parallel 4
out/build/linux-amd64-release/motogp_headless_probe assets/game-tu
```

Setup on Debian 13 is under `/workspace/shared` without requiring root. Debian package signatures and SDK checksums are verified. Cloud Clang 19 satisfies SDK Clang 18+ requirement; validated with real game and SDK tests. In each new shell, `cloud-env.sh` must be sourced again. Publishing does not preserve live processes.

SDK tests:

```bash
bash scripts/smoke-sdk.sh
# After SDK source fixes are applied, 43 cases + 19 range checks pass.
# Run PowerPC subset separately:
out/smoke/build/ppc_smoke '[ppc]'
```

Do not assume SDK codegen fingerprint tracks patch file automatically. When XEX/patch changes, rerun hash checks and codegen with `--ignore-stamp`; re-analyze address boundaries for other game versions. Stock-version boundaries are not used in TU manifest.

## Input hashes

| File | SHA-256 |
|---|---|
| Stock `default.xex` | `54245417e996ed0aa561a5a803fd810e87684bd0b43082f316db711b3ee786ae` |
| Provided TU package | `441df6cc2e20ac8aeeed475138c1714d477f965423d8c5c9b0202e3a18d8141f` |
| Extracted `default.xexp` | `0419205ba10dc65eaa7b0bb62abc64cada46119ece5ed749e4ba20c06a6bc079` |
| Linux AMD64 SDK ZIP | `0a56565221a4e75f91fc71f787d9299a0d0298127b970c8a88a292f8d3a3befe` |
| Windows AMD64 SDK ZIP | `1e19d7ec9be0d9d7e1a0088a34a33b068d4dea520181e81318528ef3b37627e7` |

## File layout

`assets/game/default.xex` is the stock copy. `assets/game-tu/` contains original XEX + sibling `default.xexp` for analysis/build. `assets/updates/` keeps original title update and extracted patch. `generated/default/` is game code. These files and `out/` are not committed to Git. `generated/rexglue.cmake` is managed by SDK.

`tools/motogp_inspect` provides XEX section dump and STFS extraction with SDK loaders; it refuses to overwrite existing output directories/files. Analysis notes are in `docs/analysis.md`; raw reports for this environment are under `out/analysis/`.

Official SDK: https://github.com/rexglue/rexglue-sdk/tree/bd833a2ab69d305f99ddf57418fc38ffec8af93d

## Windows build from Linux cloud

```bash
cd /workspace/motogp0910-recomp
bash scripts/install-windows-cross.sh
bash scripts/build-windows-cross.sh
python3 scripts/package-windows.py
```

Microsoft SDK/CRT is prepared with xwin 0.10.0 from pinned VS17 manifest. Windows resources are compiled with Clang 20 to MSVC ABI; codegen runs on the same Nightly Linux host tools pinned by sdk-version.json. Compiler resource include path must come before Microsoft include paths; otherwise MMX and constexpr offsetof errors occur. SDK/library validations and static assertions are preserved. SDK packages pass checksum verification; LLVM packages pass APT signature checks. Cache and tools are under `/workspace/shared`.

Output is `out/build/windows-nightly-release/motogp0910.exe`; package is `out/dist/MotoGP0910-Windows-x64-stability-update.zip`. Wine is only for additional loading/CPU probes; it does not replace real Windows graphics validation.

Experimental FSR 2/3 and horizontal-plus 21:9/32:9 camera options: [sources, implementation and limitations](docs/experimental-rendering.md). FSR quality modes do not lower scene rendering cost; correct temporal motion/depth integration remains unfinished.

Nightly migration and validation: [migration notes](docs/sdk-nightly-migration.md). [Stable rollback package](https://github.com/inflamesgo/motogp0910-recomp/blob/main/downloads/MotoGP0910-Windows-x64-fsr-ultrawide-stable.zip) remains available.
