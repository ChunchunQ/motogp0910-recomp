# Portable launcher and shared settings

The public entry point is `MotoGP09-10Launcher.exe`. Keep `bin/`, `resources/`,
`game/` and `logs/` beside it. The game, UI library, crash monitor, runtime
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
a small separation below the field, rounded charcoal panels and orange
selection text.
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

## Validation

The Windows PE/Wine suite passed 29/29 entries, including 69,426 production
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
