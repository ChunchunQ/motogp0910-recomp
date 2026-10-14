# MotoGP achievement interface

The custom F7 overlay and lower-right unlock notification use a dark MotoGP
palette, orange highlights and packaged Open Sans. Search and All / Unlocked /
Locked filters expose the real title catalog. Progress and gamerscore come from
the existing SDK achievement manager, including the original icons. A small
trophy replaces an unavailable icon. The UI never unlocks achievements itself
or rewrites the user's achievement progress.

English/Turkish UI labels follow `General > Launcher language` at game startup.
Catalog names/descriptions remain the title's extracted metadata. Reopen the
game after changing the launcher language.

F7 toggles the list; Esc or Close returns to the game. The current remapped SDK
achievement binding also works. Close requests defer the SDK-owned unique_ptr
reset until after drawing, using a weak lifetime token. They do not invoke the
SDK base dialog's self-deleting Close method. A pending close cannot reopen a
menu already dismissed by F7. Notifications don't capture gameplay input.

## Quiet unlock sound

An original 450 ms two-note bell accompanies the first visible frame of a new
notification. The default cue volume is 25%, applied to an already soft
waveform; at 100% game master volume its measured peak is below 0.065 full scale.
The envelope has a gentle attack and zero-valued ends to avoid clicks. The cue
is generated as PCM, needs no downloaded audio assets, and runs asynchronously
through Windows WinMM. No system mixer volume is changed.

F7 > Unlock sound / volume disables or adjusts the cue. Game master volume and
mute also apply; mute/disable interrupts active playback within the worker's
20 ms polling interval, excluding driver scheduling. Sound preferences save
separately as `achievement-ui.toml` under the configured user data root. Existing
game, launcher and achievement save files are preserved. Device failure leaves
the visual notification available and produces one warning per sound worker.

Guest-thread notifications enter a bounded, synchronized queue. Repeated events
with the same achievement ID/unlock timestamp are suppressed. Opening F7 or
loading existing progress does not play a sound. Notifications wait while the
game lacks focus; individual visible cards last 4.5 seconds with short fades.

## Windows validation

The actual pinned runtime's ImGui, achievement manager, dialog registration and
keybinding dispatch were exercised under Wine: **280 checks passed**. Coverage:

- Actual unlock persistence, progress and score; no changes from viewing the UI.
- Search and filters, empty catalog, long titles, Turkish font glyphs, EN/TR
  layouts at 640x480, 720p, 1080p, 4K and an ultrawide viewport.
- Toast contents remain inside the card, and scoped styles/fonts don't leak.
- Concurrent duplicate events, bounded bursts, FIFO timing, fades, background
  focus and exactly one cue request per new visible notification.
- Preference reload/clamping, mute/master/disabled gain, waveform duration,
  peak and endpoints, Windows sound-worker teardown.
- Actual F7/Esc deferred ownership, stale callbacks and remapped SDK binds.

Build has no warnings in the changed application/probe sources. These checks
are not a native Windows GPU screenshot or a listening test on the user's
speakers. Production draw data was rendered locally with fixture achievements
for EN/TR previews; those images/logs/configs stay in ignored `out/achievements/`.
The release contains no fixture achievements or preview screenshots.

```powershell
cmake --build --preset win-amd64-release --target achievement_ui_probe --parallel 2
.\out\build\win-amd64-release\achievement_ui_probe.exe `
  .\third_party\open-sans\OpenSans-Semibold.ttf .\out\achievement-validation
```

The probe is also included in `windows_validation_probes` and
`scripts/verify-windows.py`. The full runner requires the same prerequisites
as the existing Windows suite. Extract the complete release, including
`resources/ui/OpenSans-Semibold.ttf`; its license/notice ship under `licenses/`.
