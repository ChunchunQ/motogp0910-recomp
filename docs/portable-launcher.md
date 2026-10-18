# Portable launcher and shared settings

The public entry point is `MotoGP09-10Launcher.exe`. Keep `bin/`, `resources/`,
`game/` and `logs/` beside it. The game, private launcher, crash monitor, runtime
DLLs, controller database, address map and runtime manifest live in `bin/`.
The public entry point imports only Windows system DLLs; it can start the
private launcher with the bundled VC runtime confined to `bin/`.

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
other game settings. Checkboxes use rounded red boxes with white ticks and
handle rapid repeated clicks. Focus no longer draws dotted outlines.

Sliders jump to the clicked position and continue dragging through the native
trackbar. Native selection popups fade in over 160 ms without blocking input;
Windows reduced-motion settings disable the fade. Rapid closing stops the timer
and removes the temporary popup transparency style.

Window size combines common aspect ratios, monitor-reported modes and native
dimensions, excluding sizes larger than the physical display. Internal rendering
adds 4x (5120×2880). The pinned renderer requires integer scale factors, so a
fractional 1.5x option is not exposed. FSR labels omit the Experimental suffix;
this is a label change, and temporal reconstruction limitations remain described
in the relevant help text.

## Validation

The Windows PE/Wine suite passed 29/29 entries, including 60,512 production
launcher UI assertions, 346 live-settings checks and 22 JSON/migration/native
SDK-priority checks. A targeted run passed 207 slider, focus, rapid-checkbox and
popup-animation checks. The public entry point and bin child launch preserve
Unicode arguments, working directory and exit status without root VC DLLs.
Nine native crash-report scenarios passed with reports/JSON in the root and the
runtime manifest in bin. A real Linux software-Vulkan startup also read root
JSON dimensions, loaded root localization and bin controller mappings, and wrote
root logs. These checks do not replace native Windows GPU or physical controller
validation.
