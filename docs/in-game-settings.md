# In-game live settings

F1 opens the MotoGP menu while the game is running. It uses the same Open Sans,
dark cards and orange accents as F7 achievements, with English/Turkish labels
chosen from the existing launcher language preference. The old SDK F4 binding
(including an explicitly saved F4) migrates to F1; another custom shortcut is
preserved. Alt+F4 retains its normal window-close behavior. A remapped `bind_settings`
is respected. Return, Esc or the overlay key closes it; controller navigation
uses the D-pad/left stick and A/Cross, with B/Circle to return.

Only settings with a working live consumer are offered. There is no restart-only
section and no changes to physics, session membership or game files.

| Page | Live controls | Application path |
|---|---|---|
| General | Automatic / Xbox / PS3 / keyboard prompts | Existing per-frame prompt selection |
| Graphics | FPS limit, VSync, keep aspect ratio | Existing frame scheduler / GPU timing / presenter reads |
| Graphics | Bilinear, CAS, FSR 1, experimental FSR 2/3; quality, sharpness and color dithering | Actual `Presenter::SetGuestOutputPaintConfigFromUIThread`, not just startup CVars |
| Advanced | Speed blur, depth of field, dynamic shadows, shadow distance | Existing scoped native engine hooks |
| Audio | Master volume, mute | Real output-gain callback and output mute |
| Controls | All 25 mappings, key capture / alternatives / unbind / per-action default | SDK MnK reads current mappings; prompt labels use the same values |

GPU selection, internal rendering scale, host display mode/size/Hz, shader
storage, native MSAA, FXAA, anisotropic filtering, game language, game folder,
keyboard-enable toggle, input backend and LAN setup are deliberately absent. Their current startup or
cache paths do not provide a verified safe live change through this menu.
The pinned MnK driver ignores key releases while disabled; toggling it off/on
with a held key can latch that key. Its live keyboard-enable toggle is therefore
omitted rather than exposing that known edge case. Rebinding itself is live.
The keep-aspect checkbox changes presentation letterboxing; it does not change
the camera's startup ultrawide policy.

FSR 2/3 remains experimental with the same existing limitations: neutral
motion/depth, per-frame history reset, no full temporal reconstruction, no scene
render-cost reduction and no frame generation. This feature does not change
that scope. FSR sharpness maps to the launcher's 0–100% control (reduction stops
`2 * (1 - percent / 100)`); CAS sharpness maps to gain `percent / 100`.

## Persistence and input

Opening reads the actual active values, including launcher CLI overrides.
Changes take effect immediately; **Save settings** persists them for future
launcher use. Return does not implicitly save. **Revert unsaved** restores the
last saved live values, including the real presenter configuration.

The app retains the SDK-resolved config path. Saving merges only changed managed
keys into the latest TOML, preserves unrelated tables, paths, LAN settings and
other values, and refuses conflicting external edits to the same key. An
exclusive temporary file, flushed atomic replacement and first-file backup
protect the existing config. A failed save leaves live values available and
reports the error. The launcher reloads the file when its child game exits,
because its controls are disabled during the session; the next PLAY cannot
silently overwrite the newly saved values with an old snapshot.

The overlay does not pause the simulation or network: pause offline gameplay
before adjusting settings; LAN participants keep running. Opening takes a
reference-counted SDK input blocker immediately. Input polling, keystroke queue
draining and release masking stay on the existing guest frame hook. Native input polling is serialized with mapping edits to avoid racing the SDK's direct string readers. A native
keystroke import gate also covers the SDK path that bypasses its state blocker.
Closing waits for held buttons to be released before returning control.
Keyboard/mouse DOWN events reach ImGui before being intercepted ahead of MnK;
UP events reach MnK to release keys held before opening. Mouse-look capture is
temporarily disabled while editing and restored after release, without saving
that temporary setting. Focus changes rearm controller input from neutral.
Title-screen exit confirmation cannot open behind the settings menu.

The menu remains app-owned. It never uses the SDK dialog's self-deleting Close.
UI styles, fonts and navigation flags are scoped/restored for the other overlays.

## Verification

`in_game_settings_probe` exercises the shipped Windows runtime, presenter paint
planning, real input system and import wrapper, configuration round trips into
the actual launcher parser, external conflicts and temporary-file collisions,
and actual ImGui pages in both languages at multiple viewport sizes. It does
not execute Windows GPU shaders or replace native gameplay/visual verification.
The existing achievement and exit-menu probes cover shared-style and title
regressions. All fixtures and previews belong under ignored `out/`.

The current Windows/Wine run passed **337 targeted checks**, plus **237 native
button replacement**, **280 achievement UI/storage/sound**, **72 native quit menu**
and **54,325 actual launcher UI checks** (including a real child-process exit and
reload/save of in-game values). Five input-precedence scenarios also passed.
Changed application/probe sources compiled without warnings. Native Windows
GPU rendering, listening and gameplay interaction remain to be verified locally;
CPU planning and ImGui draw/layout checks do not establish shader correctness.
