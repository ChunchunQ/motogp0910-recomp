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

## Native Windows follow-up — implemented, 15 October 2026

The user reported the achievement list's vertical scrollbar did not work.
The old probe only exercised keyboard/Home/End, so it did not validate dragging.
The list now uses its own DefWindowProc-based window class, with explicit parent
scroll-message routing. The updated probe drives **real OS mouse input** to drag
the native scrollbar thumb; it passes alongside keyboard/wheel handling.

The launcher has a client-drawn title bar containing only minimize and close.
This removes Windows' disabled maximize slot while retaining window movement,
Alt+F4, system-menu access and the two requested actions. Actual caption paint,
hit testing, blocked maximize and minimize/close commands are checked. Its
32-pixel header participates in the existing DPI/work-area scaling.

## Launcher Achievements page

The launcher uses its current Racing Lines tabs, dark cards and orange accent.
**Achievements** shows the real title catalog: **50 achievements / 1000 G**, with
all 50 original icons loaded from the user's own XEX. No extracted achievement
artwork, fabricated unlocks or fixture progress ships in the release.

The page contains earned/total counts, earned/available gamerscore, percentage,
a progress bar, search, All / Unlocked / Locked filters and a scrolling list.
Mouse wheel, scrollbar, arrows, Page Up/Down and Home/End scroll the list.
Names/descriptions follow General > Game language; interface labels follow
Launcher language. General remains the default launcher page.

Catalog loading runs in a background tool-mode SDK runtime, without executing
the game or initializing a GPU/audio backend. Its user root is deliberately
empty so the SDK cannot migrate or write the player's unlocks. The launcher
reads the actual progress separately, using the current timestamped TOML format
and the legacy array format without converting it. Unknown IDs do not inflate
catalog progress. A transient missing/partially written file retains the last
good snapshot; changing profile clears the previous profile's snapshot.

Default progress path: Windows `Documents/MotoGP 09-10/achievements/434307F0.toml`.
The launcher and game share the [user-data migration](portable-launcher.md):
when the new default folder is absent, the old `Documents/motogp0910` tree is
copied and verified before publication. A byte-identical legacy folder is then
removed; conflicting or unverifiable legacy data is retained. Existing old-folder
cleanup is also guarded and attempted once, as described in the migration guide. Existing new
folders and custom roots are never merged or overwritten.
The Windows Known Folder API honors redirected/OneDrive Documents. Configured
user_data_root takes precedence over REX_USER_DATA_ROOT; relative paths resolve
against the portable installation root, matching the child game's working directory.
metadata_root / REX_METADATA_ROOT and the SDK's existing metadata discovery
also apply. The launcher never unlocks, resets or rewrites achievements.

The page remains usable while the game is running; visible progress refreshes
within about 500 ms of a successful SDK save. Pop-up in game and Unlock sound
are independent, immediately saved preferences shared with F7. The running game
refreshes them within about 500 ms. Turning off visual pop-ups does not disable
unlock tracking or force sound off. Opening either list plays no unlock sound.
Concurrent preference changes merge only changed fields, retain volume and
unknown fields, use a per-file Windows mutex and atomic file replacement, and
refuse to replace corrupt preferences. A missing game folder shows a readable
error; reopening the page retries catalog loading.

## Quiet unlock sound

An original 450 ms two-note bell accompanies the first visible frame of a new
notification. The default cue volume is 25%, applied to an already soft
waveform; at 100% game master volume its measured peak is below 0.065 full scale.
The envelope has a gentle attack and zero-valued ends to avoid clicks. The cue
is generated as PCM, needs no downloaded audio assets, and runs asynchronously
through Windows WinMM. No system mixer volume is changed.

F7 > Pop-up in game controls visual notifications; Unlock sound / volume disables or
adjusts the cue. Game master volume and
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
keybinding dispatch were exercised under Wine: **286 checks passed**. Coverage:

- Actual unlock persistence, progress and score; no changes from viewing the UI.
- Search and filters, empty catalog, long titles, Turkish font glyphs, EN/TR
  layouts at 640x480, 720p, 1080p, 4K and an ultrawide viewport.
- Toast contents remain inside the card, and scoped styles/fonts don't leak.
- Concurrent duplicate events, bounded bursts, FIFO timing, fades, background
  focus and exactly one cue request per new visible notification.
- Preference reload/clamping, mute/master/disabled gain, waveform duration,
  peak and endpoints, Windows sound-worker teardown.
- Actual F7/Esc deferred ownership, stale callbacks and remapped SDK binds.

The actual launcher window additionally passed **59,199 checks** across
all six pages, EN/TR and 75/100/125/150% scales. This includes loading all original
XEX catalog entries/icons, legacy/current save byte preservation, search/filters,
actual mouse dragging, caption controls, scrolling, transient save failures, profile
isolation, shared preference writes,
and keeping achievements usable during a running child. Existing launcher
settings persistence and child-exit reload checks remain included.

Build has no warnings in the changed application/probe sources. These checks
are not a native Windows GPU screenshot or a listening test on the user's
speakers. Production draw data was rendered locally with fixture achievements
for EN/TR previews; those images/logs/configs stay in ignored `out/achievements/` and
`out/launcher-achievements/`.
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
