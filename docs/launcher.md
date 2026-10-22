# MotoGP 09/10 Launcher

Open `MotoGP09-10Launcher.exe`, select the game directory and press **PLAY**.
The current [Racing Lines redesign](launcher-redesign.md) uses a compact horizontal-tab layout, charcoal fields, amber/red cover-inspired art and a scrollable settings area. Minimize/close sit directly over the art. General includes cancellable, read-only game-file verification.
PLAY is anchored at the lower right; Save / Defaults / Logs / Report sit above the
status line. Context help uses standard Windows hover tooltips instead of permanent
explanation paragraphs.
**Online → Connection mode** edits network settings inside the launcher: Offline hides all parameters, LAN shows only local/virtual-network fields, and Online shows NOBK sign-in or the signed-in account and hosting room field. Switching modes or tabs preserves entered values. Save / PLAY applies them;
Online sits between Controls and Achievements.
The launcher opens on **General** every time. The default language is English; **General
→ Launcher language → Türkçe** switches the whole
interface immediately and retains the choice in the shared root `config.json`.
File names stay English. No CMD file is required.

## Importing an Xbox 360 ISO

General uses one game-file action in the existing button position:
**Select ISO → Select title update → Install DLC → Verify game files**. The base-game check
requires the supported stock executable and a nonempty `xenondata` directory;
full data hash verification remains separate. A bold cyan notice beside the
button says **Game files are missing** first, then **Title update is missing**
or **Title update is invalid**. It disappears when the base game and update are ready;
missing DLC is then listed inline.
The footer stays **Game is not ready** until normal launch validation succeeds;
the existing green ready message is preserved. Progress appears on the action
button; detailed errors and verification results remain in dialogs/reports.

Select ISO opens only the image picker. Its destination is always the launcher's
own `game` directory; there is no destination picker or additional game subfolder.
The launcher reads raw Xbox 360 XDVDFS images, including the known full-disc
partition offsets and game-partition images. It does not mount the image,
download game data, decrypt unsupported images or run a separate extractor.
The parser bounds image reads, rejects directory cycles, duplicate Windows
paths, traversal and reserved filenames, and limits directory depth/count/size.
Non-ASCII disc filenames and split/compressed image formats are unsupported.

Extraction runs in a worker with progress and cancellation. It creates a new
staging directory, checks the stock executable first, then validates extracted
data against the embedded file hashes. A verified extraction is installed
directly into `game`. If that directory exists, matching files are preserved,
conflicting files cause an error, and only absent files are moved in. The merge
journals new files/directories and removes them if interrupted cooperatively.
Existing installations and the ISO are never overwritten. Cancellation or failure
removes the staging directory;
the active game path changes only after successful extraction. A process kill
or power failure can leave a temporary staging directory for manual removal.

The disc is not assumed to contain title update 0.0.1.1. A matching update or
patch already in the previously selected folder is copied into the new folder
when absent there. Otherwise **Select title update** opens a file picker for
the supported TU container or `default.xexp`. The picker shows **All files**;
the file's contents, not its extension, must match a known hash before installation.
For a TU container, the launcher immediately extracts `default.xexp` with the
same pinned STFS reader used by the game's startup path and verifies the
extracted patch's size and SHA-256. The original TU and verified patch are
installed together; an already extracted, verified patch is also accepted.
Replaced update files are retained with a `.previous-...` suffix. If installing
the pair fails, earlier replacements are rolled back. This replaces the earlier
behavior that copied only the container and deferred extraction to game startup.
The imported folder is saved independently of
other pending settings. ISO import is unavailable while the game, verification
or launcher update is running.

The optional launcher probe contains synthetic XDVDFS parser, malformed-image,
cleanup and adaptive-button/notice checks. The Windows release cross-build and
**23 targeted Windows/Wine checks** passed for the earlier revision on
20 October 2026. The subsequent direct-folder installation, step-by-step update
selection and persistent-footer revision is built locally without running tests
at the user's request. A real supported ISO import and native Windows UI
validation remain pending.

The subsequent All files picker and immediate TU-patch extraction changes were
initially source-only. They are included in the later main Windows Release build
and isolated launcher regression. Actual supported ISO/TU imports and native
Windows visual acceptance remain separate; see [local validation](https://github.com/inflamesgo/motogp0910-recomp/blob/5e91cf5/docs/rider-license-validation.md).

## Required DLC

Xbox 360 DLC import is documented in [DLC installation](dlc.md).
The single game-files action follows **Select ISO → Select title update →
Install DLC → Verify game files**. All three supported packages are required
before PLAY in Offline, LAN or Online; missing packages are listed inline.
**Install DLC** replaces verification while installation is incomplete. It accepts a ZIP/RAR/7z archive or the main
extracted folder and installs the supported packages together into
`dlc/434307F0` beside the public launcher, without a zero-XUID parent directory.
This is independent of the title-update import and preserves the original game.

## Add to Steam

**General → Steam → Add to Steam** adds `MotoGP09-10Launcher.exe` as a non-Steam
game for the active or most recently used local Steam account. If multiple
accounts exist and none can be identified, sign in to Steam before retrying.
Steam launches the launcher; press **PLAY** there to start the game with the
normal saved settings. Moving the installation requires adding its new path.

Steam must be closed while its shortcuts file is edited. If it is running, the
button asks before closing it and restarting it afterwards. Cancellation changes
nothing. The General form uses tighter vertical spacing so Steam's button,
hint and result remain within the existing viewport without a General scrollbar.
One text area shows the hint, progress or result. After a successful addition,
the green **Added to Steam** result replaces the hint for the rest of the launcher
session, including when switching pages or languages.
The operation runs in a worker; launching the game and installing a
launcher update are blocked while it runs. No Steam password or network API is
used, and no Steam game/content files are modified.

The binary shortcuts reader preserves unrelated entries, tags, launch options,
play history and unknown supported fields. Unsupported or malformed input is
rejected. An existing `shortcuts.vdf` receives a unique `.motogp-….bak` backup;
replacement uses a flushed temporary file and Windows atomic replacement after
checking that Steam stayed closed and the original file did not change.

Steam artwork is read only from five editable loose files in `resources/steam`:
`icon.png`, `logo.png`, `banner.png`, `cover.png` and `wide.png`. The icon is the
same game icon used by `motogp0910.exe`, rather than the launcher icon. Cover
maps to Steam's portrait grid, wide to its horizontal grid, banner to the hero
background and logo to its separate foreground logo. The supplied logo has a
transparent background. The supplied portrait cover uses a 2:3 canvas, with
the orange design extended into the top margin and white space below, so Steam
can display its full width without cropping. The horizontal cover has a separate
1840×860 composition for Steam's 92:43 library tile. No Steam images are
embedded in an executable, cropped automatically or generated from the launcher
background. Replacing a file and pressing **Add to Steam** again refreshes this
shortcut's artwork. Other games' artwork is preserved. Missing optional PNGs
are reported without generating substitutes; the loose icon remains required.
Artwork failure does not undo a valid shortcut.
The interface and results are localized in English and Turkish.

The behavior reference is
[ChunchunQ's FIFA 09 launcher](https://github.com/ChunchunQ/fifa09-recomp),
`launcher/SteamShortcut.cs` at revision `711da3941d81879db06ea4f2c36c3a519ac826d9`.
Its MIT attribution is retained in
[the source license](../third_party/fifa09-launcher-LICENSE.txt) and the embedded
launcher notices. Native Windows/Steam acceptance remains pending for this
extracted integration branch.

The later main source was built and checked as described in the external
[main validation report](https://github.com/inflamesgo/motogp0910-recomp/blob/5e91cf5/docs/rider-license-validation.md). The selective integration worktree
was not separately rebuilt.

## Graphics

| Setting | Runtime behavior |
| --- | --- |
| GPU selection | Automatic or a named DXGI hardware adapter; original adapter indices are retained |
| Native 2× MSAA | Existing SDK native MSAA toggle, enabled by default |
| Window size | Automatic or common sizes limited by the monitor native dimensions; borderless uses desktop size |
| Display mode | Fullscreen with a selected monitor mode, desktop borderless, or windowed |
| Refresh rate | Actual host Hz for fullscreen; desktop Hz in borderless/windowed |
| FPS limit | Independent optional guest-present cap; it does not unlock the title cap or change the guest video-mode Hz |
| Internal resolution | Native 720p, a 1080p FSR output preset, or internal 1440p / 2160p / 2880p at exact 2× / 3× / 4× scales |
| Bilinear | Original default presentation path |
| AMD FSR 1 | Spatial EASU enlargement followed by RCAS sharpening when the rendered image is smaller than the output |
| AMD FSR 2 / 3 | Experimental D3D12 upscalers with neutral motion/depth inputs and per-frame history reset; six quality modes |
| Camera aspect | Auto, 16:9, 21:9, 32:9; horizontal-plus wide modes remain Experimental |
| AMD CAS | Contrast-adaptive sharpening; the SDK also uses CAS when FSR has no enlargement to perform |
| Sharpness | FSR RCAS reduction mapped from 0–100% strength; CAS uses its own 0–1 coefficient |
| Anisotropic filtering | Game setting, off, or 1×–16× |
| Anti-aliasing | Off, FXAA or FXAA Extreme, applied by the real SDK swap post-effect path |
| VSync / aspect ratio / dithering | Actual presentation flags |
| Shader cache | SDK persistent shader storage; changing this does not delete existing cache files |

The current package includes a matched modified **ReXGlue Nightly
0.10.0.24-dev.gbd833a2** runtime with its
bundled FSR 1 / CAS shader bytecode enabled. Keep both included runtime DLLs;
using the old official DLL makes the FSR setting unavailable. See
[the pinned runtime and reproduction instructions](../third_party/rexglue-runtime/readme.md).
FSR 2/3 labels omit Experimental; their temporal limitations remain. Frame generation remains excluded. See [sources and limitations](experimental-rendering.md).

FSR 1 is a final-image upscaler; it does not add temporal anti-aliasing or invent
extra detail. For example, 1× native rendering to a 1920×1080 window allows
720p-to-1080p upscaling. At a higher internal rendering scale, there may be
nothing left to upscale. Explicit custom guest video modes in an existing
configuration remain respected. Direct `bin/motogp0910.exe` launch also reads saved
settings and maintains the native source mode when FSR is selected.

A single internal resolution selector replaces the independent X/Y controls.
Selecting an integer preset explicitly saves a native 1280×720 source plus the
matching integer draw scale; output window/fullscreen resolution stays separate.
Existing asymmetric scales and explicit custom source modes are shown as
**Custom** with their dimensions and preserved until a standard preset is chosen.
The additional **1.5x - 1080p (Experimental)** preset keeps internal rendering at
1280×720 / 1× and selects a 16:9 camera and a 1920×1080 window size. It preserves a
selected FSR 1, 2 or 3 mode, or chooses FSR 1 when Bilinear or CAS was selected.
Changing between the three FSR modes preserves the preset and the selected
mode through saving and loading. FSR 2/3 still uses neutral motion/depth inputs
and resets history each frame; this preset does not improve that integration.
The 1.5× label describes the output-to-source size ratio. This is
spatial upscaling, not fractional internal rendering. The preset preserves the
selected display mode, including the default Borderless.
Windowed/fullscreen uses the 1080p window-size setting; Borderless uses desktop
resolution, so its actual upscale ratio depends on the desktop. The preset
requires 1920×1080 to fit the selected display. Changing output size,
aspect or effect to incompatible values returns the selector to
native 720p and preserves the user's new choices. The combination is recognized
again when loading the shared JSON configuration; no separate preset flag is
needed. The update passed targeted launcher and CPU runtime checks under Wine;
native Windows GPU upscaling remains unverified. Borderless preservation passed
the targeted real-control graphics checks in English and Turkish.

## Audio

The master slider controls real output gain, from 0% to 100%, for stereo and
surround. Mute remains available independently. The game's own audio menu
continues to provide its separate music/effects options. Launcher edits apply
on the next game launch.

## Keyboard and controllers

All 25 SDK controller actions appear as rounded key buttons in a scrollable list.
Click a button, then press a key or chord. Escape cancels capture; right-click
clears the binding. Reset keys restores only keyboard mappings, while Defaults
resets all launcher-managed settings. Existing alternative key lists remain
supported and are preserved until that action is rebound.

Actions labeled **Unassigned** start without keyboard bindings. The former
shipped L3/R3 and right-stick defaults are upgraded to empty bindings when
loaded from configuration; different custom bindings and explicit environment,
command-line or live overrides are preserved. Empty bindings are supported by
the SDK and do not disable a physical controller's sticks or buttons.

SDL supports Xbox, PlayStation and other mapped USB/Bluetooth controllers.
The included `gamecontrollerdb.txt` stays in `bin/`, outside `game/`.
XInput can be selected separately. The game now replaces registered button
glyphs for PlayStation and actual keyboard bindings; see [controller prompts](controller-prompts.md)
for automatic switching and forced-style testing.

## Existing settings and data

The launcher and game use one root `config.json`. Unknown supported keys,
nested tables and custom save/cache roots are retained. Existing game/launcher
TOMLs migrate automatically and remain as exact `.legacy.bak` backups. F1 saves
the same JSON. External edits are protected from accidental overwrite; saves
use atomic replacement. See [portable settings and validation](portable-launcher.md).

The standard game layout remains `game/default.xex`, `game/xenondata/` and the
other original disk files. The supported title update is **0.0.1.1**.
`nxeart/` may legitimately be empty. Basic file checks validate the stock XEX
and supported patch/update; they do not promise that every disk asset is valid.
**Open logs** and **Error report** remain available. Reports are created locally
and never uploaded automatically.

## Verification history

The following counts describe earlier launcher milestones, including the old
three-page UI. Current source has seven pages; the latest main build and phased
Wine validation are recorded in [the Rider License report](https://github.com/inflamesgo/motogp0910-recomp/blob/5e91cf5/docs/rider-license-validation.md).
Preserve those evidence boundaries rather than treating old totals as a new run.

- Windows game, launcher and runtime DLL pair built successfully.
- 72 launcher checks: numeric launch arguments are verified against actual SDK
  parsers, alongside advanced configuration round trips, SDK flags, preservation,
  validation, Unicode and an actual child process receiving its arguments/CWD.
- 140 runtime checks: FSR 1 EASU/RCAS pass planning and coefficients; native versus
  host video modes; real master gain and converted samples; real MnK driver
  delivering `Ctrl+K` / `Space` to guest A and `Shift+Up` to guest D-pad.
- 50 existing game-code regression checks passed with the new runtime, including
  the race-start callback previously responsible for a crash.
- Production Win32 UI tested under Wine/Xvfb across all three pages, both
  languages and 75% / 100% / 125% / 150% layout scales (24 layouts). Visible
  controls fit without overlaps and localized static text fits its height.
  Presets, all 25 binding fields, capture/cancel/reset, checkbox toggles, actual
  save/reload and preservation of custom render settings are exercised by
  `launcher_layout_probe`.
- The compiled launcher EXE was also driven externally: all pages and both
  languages, advanced settings, real Ctrl+J keyboard capture and saving were
  verified through the production message loop.
- Both Python and PowerShell staging verified against the pinned DLL hashes.

The FSR test uses the real SDK CPU pass planner and a supplied surface size; it
is **not a GPU pixel/shader execution or Windows race-playthrough test**. Native
Windows GPU rendering and listening/playing with the new settings still need
validation on the user's computer. Defaults keep bilinear presentation and
native 720p rendering; higher resolutions and upscaling are opt-in.

The launcher serializes launch arguments using typed configuration nodes
(the parser retains a toml++ representation internally while root storage is JSON). Earlier
boolean conversion of numeric nodes could turn scale/volume/sharpness values
into `true`; the new regression checks prevent that.

## Local UI inspection

`launcher_layout_probe` can save actual Win32 window captures to a local
output directory supplied on its command line. These optional captures are
useful for inspecting language and layout changes.

## Reference projects

[DownpourRecomp](https://github.com/LittleBitUA/DownpourRecomp) was inspected as
an example of useful settings; its launcher design is not copied. FXAA,
anisotropic filtering, internal scale and persistent shader storage are useful
here and have actual v0.10.0 implementations. Downpour-specific UE3 patches,
mouse-camera tuning, its fork's FSR 3 / frame limiter and parameters are not imported as
nonfunctional MotoGP controls. The later
implemented SDL resistance profiles are documented separately in
[adaptive triggers](adaptive-triggers.md).

[FIFA Street PC Port](https://github.com/samuelit0davila/fifa-street-pc-port)
was also inspected through its C# launcher sources and published screenshots.
Its GPU adapter selection is useful for multi-GPU systems and was implemented
with native DXGI enumeration, preserving the SDK's adapter indices. Its title-specific
memory/readback compatibility presets and refresh rate/FPS tuning are not
blindly applied to MotoGP. No design/assets/code from either launcher are copied.

Downpour's actual v1.1.8 `PlayDownpour.exe` was also opened under Wine and its
Graphics, Advanced, Controls and Debug pages inspected. Native 2× MSAA is the
additional supported setting retained from that inspection. Its launcher-only
music settings are unrelated to game output volume; MotoGP's slider controls
the actual game's output stage.


## General preferences update

The General page now contains the game folder, Launcher language, Game language
and Button prompts. The folder and launcher language are hidden on the other
pages. Graphics, Audio and Controls retain their own settings. Internal render
presets are labelled 1x / 2x / 3x / 4x with exact dimensions, with the separate
1080p choice explicitly labelled as FSR upscaling. Window-size choices are
limited by the target monitor's preferred physical dimensions, falling back to
its current desktop size when native-mode reporting is unavailable. Existing
oversized output values show Automatic / desktop; disk configuration changes
only when the user saves or launches. Internal render resolution is independent
of this output-size filter.

Game language choices are English, French, German, Spanish, Italian and Turkish. The
launcher forwards user_language; a scoped title hook lets an explicit selection
take precedence over the game's internal language tweak, uses the original
localization-file mapping and restores that tweak. Six native locale/ABI checks
passed under Wine, including preservation of the original direct-launch override.
Launcher English/Turkish and game language remain independent.

The production UI probe passed 44,500 layout, configuration and CLI assertions
across all four pages, English/Turkish and 75–150% UI scales. The output tests
include 1080p, 1366x768, ultrawide and 4K monitor limits. These tests do not check
a physical monitor's EDID or game text rendering on a Windows GPU. No new
screenshots are stored in the repository.


## Display modes, monitor Hz and FPS cap

The matching v0.10.0 runtime uses its own SDL instance to select a real host
fullscreen resolution/refresh mode. Only the monitor's reported Hz values for
the selected resolution appear in the launcher. Automatic selects a supported
mode near the desktop rate; unsupported explicit requests are rejected.
Borderless uses desktop resolution and Hz; windowed keeps desktop Hz.
The refresh control is disabled in these two modes, and output size is disabled
for borderless. SDL restores the display mode when fullscreen ends.

The optional FPS cap sleeps at the title's native presentation boundary
(`0x823D1008`, which invokes `VdSwap`). It leaves video_mode_refresh_rate,
physics constants and game speed settings alone. Zero adds no limit. This cap
cannot remove an existing in-game/VSync limit. Previously saved fullscreen
booleans migrate to their existing borderless/windowed behavior.

Validation: 33 pacing and real SDL/Win32 window checks passed under Wine/Xvfb,
including borderless desktop bounds, fullscreen at the desktop mode, restoration
of windowed decorations and rejection of unsupported modes. Physical 60/144 Hz
monitor switching and Windows GPU gameplay are not verified by a virtual desktop.
The four-page bilingual UI passed 46,119 layout/configuration assertions.
91 launcher integration checks and 140 runtime planner checks passed.


## Advanced: native MotoGP engine controls

Advanced is independent of Graphics. It exposes only identified MotoGP
controls, rather than Unreal Engine configuration names:

| Setting | Behavior |
| --- | --- |
| Motion blur (speed) | Game default or Off; scopes the blur alpha and velocity to zero while preserving the independent vignette |
| Depth of field | Game default or Off; affects the native camera DOF enable byte |
| Dynamic shadows | Game default or Off; scopes the title Shadows/Enabled flag while rendering |
| Shadow render distance | Game default or 45 / 90 / 180; scopes Shadows/RenderDistance during the native shadow pass |
| Vignette intensity | Game default, Off or 25 / 50 / 75 / 100%; scales speed-related edge darkening independently of blur |
| Bloom intensity | Game default, Off or 25 / 50 / 75 / 100%; scales the interpolated bloom intensity |
| Shadow filtering | Game default, Off or On; scopes the native shadow filter flag |
| Soft shadow distance | Game default or 45 / 90 / 180; controls the soft-shadow threshold within the overall shadow range |
| Particle density | Game default, Off or 25 / 50 / 75 / 100%; overrides the native particle density percentage |
| Draw distance (Experimental) | Game default or 50 / 75 / 100 / 125 / 150%; scales the instance far-clip property within its native bounds |

The six additional settings passed native CPU consumer and production UI
checks; their GPU rendering has not been visually checked in a race. All ten
settings have concise English/Turkish hover explanations on
both the label and field, and occupy five aligned rows of two columns. Debug
and crash diagnostics sit side by side below the last row, within the page
viewport. The F1 menu exposes the same choices
and persists them through the shared JSON configuration. Every new setting
defaults to `game`; existing configurations retain their original behavior.

The vignette override scales only the alpha float at `0x8337BE64` during the
FSFX consumer `0x8237FA00`, restoring its exact original bits on return. Bloom
scales the current camera intensity at `0x82892554` only while `0x82385428`
publishes the interpolated state; it leaves preset selection and transitions
intact. Percentages are relative to the game's current strength and cannot
force an effect whose native strength is zero. Shadow filtering scopes the
initialized `0x83487A8C` byte around `0x8237B370`. Soft-shadow distance scopes
`0x83487CC4` in the existing `0x8238CFD8` wrapper. Particle density scopes the
initialized integer at `0x829177BC` during native spawn/update/preparation
consumers `0x82273780`, `0x82274570` and `0x82274838`. Instance visibility scopes
the float property at `0x834F6CA8` during `0x823908C0` and clamps scaled values
to the native declared range. Scopes restore originals on return, nesting and
exceptions. Original game data and tweak files remain unchanged.

The DOF override is applied after `0x82386158` copies camera settings. Motion
blur Off now scopes its alpha and velocity floats to zero during `0x8237FA00`,
so it does not disable the shared speed-FX stage that also owns vignette.
Original camera parameters are restored after consumption, and no disabled
camera effect is forced on. Shadow enable/distance overrides
wrap `0x822ADAC0` / `0x8238CFD8`; initialized registry pointers are checked and
the original values are restored on return, nested calls and exceptions.
The original default.tweaks and game data files are not edited.

The original four settings keep their existing choices. The blur/vignette
interaction above was corrected during native consumer validation. The new
shadow-filter On choice uses the identified native Boolean flag. The other new
controls adjust existing effect values; they do not create missing parameters.
AO, lens flares, decals and shadow texture resolution remain outside the menu
pending separate native rendering analysis.

Before this update, the five-page English/Turkish production
UI passed 50,408 layout/configuration
assertions at 75–150% scales. The native engine probe passed 277 checks on Linux
and Windows/Wine for actual camera flags, preservation of parameters and PPC
ABI, scope nesting/exception restoration and the pinned shadow-distance consumer.
These CPU checks do not prove every GPU rendering effect visually correct.

FSR 2/3 experimental providers and horizontal-plus ultrawide are now available.
See [the implementation, sources and practical limitations](experimental-rendering.md).
Frame generation remains excluded.

## Online account presentation and persistence

The signed-in Online page is unframed: an orange account name, optional country
code, **My account** and **Sign out**. The account button follows FIFA 09's
`MyAccount_Click` and opens `https://nobkonline.com/hesabim/` in the browser.
Country comes only from the account API's optional `country` field on login or
token refresh; absent or invalid codes are hidden. Existing saved sign-ins without
a country continue to load, and a successful refresh can fill it in.

Username and password retain the original side-by-side 281-by-30 logical-pixel
fields. The sign-in form adds **Remember me (sign in automatically next time)**,
enabled by default, plus the account explanation and an accurate Online launch
requirement. Online still refuses PLAY without a signed-in account; it does not
fall back to Offline. Offline and LAN retain their existing account-free behavior.

Remember me follows FIFA's lifetime semantics. Checked saves a token protected
with the existing Windows DPAPI storage; unchecked keeps it only in launcher
memory, including token refresh and account linking before PLAY. Passwords are
never persisted. The preference is retained for subsequent sign-ins. Signing out
clears the cached token and returns to the form. Forgot password has no hover
help. Room name appears below the signed-in account actions in Online; LAN keeps
its original room-name layout, and tab/mode changes preserve field values.

The footer displays the configured launcher version instead of the title-update
version. The FPS selector has room for all nine presets, including 240 FPS.
These edits are included in main `8a33ea9` and selective preview `ab7d3f7`.
The main Windows Release build and isolated launcher checks are recorded in
[local validation](https://github.com/inflamesgo/motogp0910-recomp/blob/5e91cf5/docs/rider-license-validation.md); a full native Windows gameplay
or live account-service acceptance run is not implied.
