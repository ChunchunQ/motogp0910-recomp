# Portable launcher and shared settings

The public entry point is `MotoGP09-10Launcher.exe`. Keep `bin/`, `resources/`
and `game/` beside it. Runtime logging creates `logs/` in this root when needed;
development packages omit that empty directory. The game, UI library, crash monitor, runtime
DLLs, controller database, address map and runtime manifest live in `bin/`.
The public entry point imports only Windows system DLLs; it can start the
UI in the same process with the bundled VC runtime confined to `bin/`.
There is only one launcher executable; `bin/motogp0910-ui.dll` contains its UI
implementation. The public EXE owns the embedded artwork, fonts and notices.

The launcher and game share `config.json` in the installation root. Game flags
retain their names and native JSON string, boolean and number types. Launcher
preferences live under `launcher`, for example:

```json
{
  "window_width": 1920,
  "window_height": 1080,
  "audio_master_volume": 80,
  "launcher": {
    "language": "en",
    "crash_monitor": true
  }
}
```

Existing game/launcher TOMLs migrate automatically when JSON is absent. Their
exact original bytes are retained as `.legacy.bak` files. Existing JSON takes
precedence; malformed settings are preserved and reported rather than replaced.
Unknown supported fields and nested tables survive saves. Live F1 changes merge
with unrelated edits and reject conflicting changes. The two writers share an
exclusive temporary-file path. The pinned SDK receives a private, temporary
format adapter, removed after startup; all user-facing edits target JSON.

Default logs and configuration stay in the installation root, including direct
`bin/motogp0910.exe` starts. Saves, achievement unlock data and cache retain their
existing user-data location. Explicit command-line/environment overrides retain
their SDK priority. Turkish paths remain supported.

Keyboard bindings are rounded buttons. Left-click captures a key or chord,
Esc cancels, and right-click clears the binding. Keyboard-only reset preserves
other game settings. Checkboxes use rounded orange boxes with dark ticks and
handle rapid repeated clicks. Focus no longer draws dotted outlines.

Sliders jump to the clicked position and continue dragging through the native
trackbar. Value-label text changes suppress the native STATIC control's
synchronous solid-brush repaint, then draw the artwork and new text together
in a buffer. Audio and sharpness percentages retain the artwork behind them
while changing; hidden labels keep their visibility state.
Native selection popups use 150 ms linear opening and closing fades,
a small separation below the field, rounded panels matching the closed field
background, a subtle inset highlight and orange selection text.
The uploaded reference launcher's bundled WPF resources were inspected to verify
its Fade behavior, bottom placement and popup spacing; reference art/binaries
are not distributed. The implementation preserves native combo input.
Native close immediately ends input capture. One nonactivating visual surface
handles opening, closing and rapid direction changes; it is destroyed after
fade-out. Reopening during fade-out reuses that window and its visible opacity.
Windows reduced-motion settings disable both transitions.

Window size combines common aspect ratios, monitor-reported modes and native
dimensions, excluding sizes larger than the physical display. Internal rendering
adds 4x (5120×2880). The pinned renderer requires integer scale factors, so a
fractional 1.5x internal option is not exposed. This update adds
an explicit **1.5x - 1080p (Experimental)** FSR output preset: 720p internal
rendering, the selected FSR 1 / 2 / 3 mode and a 1080p window-size setting.
Bilinear/CAS selections default to FSR 1 when this preset is chosen. FSR 2/3
retains its existing temporal-input limitations. The preset preserves the
selected mode, including default Borderless. Borderless uses desktop resolution;
windowed/fullscreen targets the 1080p setting. This behavior passed the targeted
real-control graphics checks. The original preset passed targeted launcher
and CPU runtime checks under Wine; native Windows GPU upscaling remains
unverified. FSR labels omit the Experimental suffix;
this is a label change, and temporal reconstruction limitations remain described
in the relevant help text.

The Online tab sits between Controls and Achievements. General language and prompt
fields form one evenly spaced column. All Offline/LAN/Online
fields are embedded in the launcher and use its normal Save / PLAY transaction;
there is no network modal or General-page multiplayer button. Only relevant
fields and labels are shown, with hidden values retained. Network controls are
disabled while a game runs. Achievement cards have no list-wide hover tooltip.

## Paint coherence update

The user's native Windows recording showed missing checkbox labels/ticks during
clicks and abrupt list opening despite the earlier Wine interaction checks.
Synchronous `WM_DRAWITEM` button paints now use the same complete offscreen
image as `WM_PAINT`; closed combos, popup rows and modal controls also buffer
their full paint. The opening clock starts at the first timer frame so a blocked
native show transaction cannot consume the fade before it is displayed.
A delayed-start regression, synchronous paint-path comparisons and exact
closed/open background-pixel checks are included. Game-language popups initially
show all six languages. Online mode labels omit Experimental in both languages;
the underlying service scope is unchanged. The revised native Windows
composition and fade cadence still need comparison on the user's machine.

Long dropdowns retain native selection and keyboard behavior with a bounded
viewport and a visible in-client scrollbar. Dragging or clicking the rail scrolls
without committing an option; wheel input also reaches the remaining rows.
Synchronous row buffers exclude the rail, preserving its thumb during native
hover and selection repainting as well as full list paints.
All 21 main-window selectors and both adaptive-trigger selectors were audited in
English and Turkish, including real mouse selection of each list's final row.
The game-language list fits all six entries initially. Combo class detection is
case-insensitive during painting, scaling and keyboard-focus reveal.

All 25 keyboard actions display controller equivalents in compact parentheses,
including `(A/Cross)`, `(RT/R2)`, `(LS/L3)` and `(Back/Select)`. Existing binding
IDs and default keys are unchanged. The labels fit both languages at 75%, 100%,
125% and 150% UI scale. Live settings use matching English/Turkish descriptions.
An incomplete game directory reports `Found N missing files` using only the
missing-file count; core, changed and unreadable errors remain in the full report.
The report location and no-files-changed notice are preserved.

## Closing, controls and accent update

The reference resources select WPF `PopupAnimation.Fade`, transparent popup
rendering, bottom placement, a four-pixel gap and a bounded scroll viewport.
The corresponding .NET 8 popup implementation animates one visual root for
150 ms and delays destruction until closing finishes. The C++ implementation
uses the same lifetime model with a dedicated layered visual surface. Native
ComboLBox retains keyboard/mouse selection and accessibility behind it. Native
owner drawing is printed into the visual surface; hover/scroll changes request
coalesced redraws. The native input window never becomes a second visible copy.
Interrupted transitions start from the current visible opacity and reuse the
same visual HWND. The original fade revision was compiled without runtime/UI
tests; the current targeted graphics checks do not validate transition smoothness.
Native Windows visual validation remains pending.

The adaptive-trigger checkbox exclusively uses owner drawing for both native
focus notifications and ordinary paints, preventing a second themed tick or
focus rectangle. Its selectors use the same 32-pixel rows as the main launcher.
Hover help is suspended while a dropdown is open so tooltips cannot cover its
options; it resumes after closing the list.
Active buttons, checkboxes, selectors and sliders use a hand cursor; disabled
controls and text fields retain their native cursors. Pages share General's
upper padding, preserving the existing Achievements layout.

An open popup receives pointer hit testing and explicitly forwards mouse input
to its native list, including events delivered to the field by native capture.
Scrollbar drags remain with that list even outside the popup bounds. Wheel
input over any scrollable popup moves its viewport without changing the saved
selection; it accumulates partial deltas, follows the Windows scroll-line
preference and clamps at either end. Window size also supports wheel input over
its open field. Popup input cannot fall through to the launcher's page rail.
Closing visuals become click-through immediately; closed selectors retain
their existing page-scrolling behavior. This input update was build-only.

All launcher accents share `#ff6a13`, including PLAY, the logo suffix, selected
tabs, popup text, sliders, checkboxes and adaptive-trigger controls. Pressed and
disabled states derive darker colors from the same orange palette. The upscaler
list orders CAS before FSR 1; displayed order does not change saved mode IDs or
their separate sharpness values.

The 25 keyboard rows name actual Default game actions with compact controller
equivalents. A/Confirm defaults to Space and B/Back defaults to Backspace. Exact
old shipped pairs (`Semicolon,Space` and `Quote,Backspace`) upgrade when loaded
from configuration. Other custom combinations, unbound choices and explicit
SDK command-line/environment/runtime overrides are preserved. Reset uses the
new single-key defaults. See [native action mapping](pc-menu-cleanup.md).

## Validation

The Advanced settings update passed 78,241 real launcher UI checks, 240 native
consumer checks for the six new settings and independent blur/vignette control,
661 native engine/scope checks, 392 live-settings checks, 112 launcher integration
checks and 93 targeted graphics checks under Wine. The UI includes both
languages, four scales, 58 dropdowns, all Advanced choice values, label/field
tooltips, save/restore and single forwarding of launch arguments. Native tests
execute the production wrappers and actual recompiled consumers with explicit
CPU observers around unrelated GPU, lighting and allocation work. They do not
execute shaders or validate native Windows race visuals. The shadow filter is
observed at the native GPU-entry boundary. See [the exact test scope](advanced-settings-validation.md).

The 1080p update passed 93 targeted real-control Windows/Wine assertions in
English and Turkish, covering FSR 1/2/3 preservation, real JSON save/reload,
exact source/output launch arguments, switching away from the preset, its
Experimental label, Unlimited, default texture filtering and checkbox hover
help. The shipped runtime also passed 183 CPU integration assertions, including
its actual 720p-to-1080p FSR 1 pass planner and quality-mode handling. These are
not GPU shader execution or gameplay checks; the full historical suite below
was not rerun for this update.

The previously published build (`69fb8ed`) passed the Windows PE/Wine suite's
31/31 entries, including 70,809 production
launcher UI assertions, 346 live-settings checks and 22 JSON/migration/native
SDK-priority checks. The full UI run covers all eight sliders, focus, rapid
checkbox input, popup opening/closing, pointer selection and the inline network
page across both languages and four UI scales. The public entry point preserves
Unicode arguments, root working directory and
UI return status while loading its UI in process without root VC DLLs.
Nine native crash-report scenarios passed with reports/JSON in the root and the
runtime manifest in bin. A real Linux software-Vulkan startup also read root
JSON dimensions, loaded root localization and bin controller mappings, and wrote
root logs. These checks do not replace native Windows GPU or physical controller
validation.
