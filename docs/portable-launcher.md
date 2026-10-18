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
trackbar. Native selection popups use 150 ms linear opening and closing fades,
a small separation below the field, rounded panels matching the closed field
background, a subtle inset highlight and orange selection text.
The uploaded reference launcher's bundled WPF resources were inspected to verify
its Fade behavior, bottom placement and popup spacing; reference art/binaries
are not distributed. The implementation preserves native combo input.
Native close immediately ends input capture; a passive, nonactivating snapshot
finishes the closing fade and frees its timer/bitmap. Reopening dismisses the
previous snapshot. Windows reduced-motion settings disable both transitions.

Window size combines common aspect ratios, monitor-reported modes and native
dimensions, excluding sizes larger than the physical display. Internal rendering
adds 4x (5120×2880). The pinned renderer requires integer scale factors, so a
fractional 1.5x option is not exposed. FSR labels omit the Experimental suffix;
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

## Validation

The Windows PE/Wine suite passed 29/29 entries, including 70,109 production
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
