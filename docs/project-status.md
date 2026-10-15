# Project status — 15 October 2026

This is the current status entry point. Older milestone documents describe
historical builds; a stable SDK version or old ZIP mentioned there does not
replace the current release pin. User observations are distinguished from
automated checks and from remaining native Windows verification.

## Platform and delivery rules

Windows 10/11 x64 is the product target. ReXGlue is pinned to
`0.10.0.24-dev.gbd833a2` in `sdk-version.json`; the matching custom runtime pair
is pinned in `third_party/rexglue-runtime/manifest.json`. Preserve the game,
configuration and saves. English filenames; English default with Turkish UI.
Show each completed stage before starting the next. Ask about unclear UX
requirements; while awaiting answers continue only independent authorized work.
Don't publish original game data, personal logs or preview screenshots.

## Current evidence

| Area | Implemented / checked | Native Windows evidence / limit |
|---|---|---|
| Game startup and races | Original XEX plus TU 0.0.1.1; startup/race callback repairs | User reports working gameplay |
| Qatar collision data | Serialized coordinate/endian repair; 192 regressions | User drove 4–5 Time Trial laps without falling through the track |
| PS3 / Xbox / keyboard prompts | Original supplied PS3 artwork, correct texture pitch and replay row; new dark/orange keyboard font | User accepted PS3/replay fixes; latest keyboard visual check pending |
| Career liveries | Scoped synchronous GPU readback, preserved flags and ABI | First career-race visual result remains unverified in available evidence |
| LAN/Radmin | Automatic discovery and IP fallback; session/socket integration | User verified two-PC racing; not proof of every disconnect/rejoin scenario |
| Revised lobby admission | Retained selected discovery result, valid null leave slots; 67 kernel, 20 generated-wrapper checks and 3 two-process modes | New fix's native Radmin replay pending |
| Live settings | F4 MotoGP UI, 40 live controls, safe launcher persistence; 336 Windows/Wine checks and actual launcher child-exit reload | Native GPU/live gameplay result pending |
| Quit menu | Uppercase native QUIT GAME entry; Press Start hook moved from boot movies to actual frontend update; 75 Windows/Wine checks, including real Backspace mapping and callback registration | Repaired native Press Start path and final menu appearance await user verification. See quit-menu.md. |
| Achievements | Launcher page + F7 list; 50 original icons, 1000 G, live read-only progress, search/filters and shared pop-up/sound preferences; 286 runtime checks and 58,070 actual launcher UI checks | EN/TR launcher previews inspected; native game GPU appearance and listening check pending |
| FSR 2/3 | Experimental AMD providers; neutral depth/motion and per-frame history reset | Proper temporal reconstruction and scene render-cost reduction absent; Windows GPU dispatch unverified |
| Ultrawide | Experimental horizontal-plus projection and centered HUD | Full Windows camera/cutscene/culling coverage unverified |
| Performance | Reduced avoidable queries/logging/locking; synthetic path checks | No measured native game FPS improvement |
| Runtime safety / crash reports | TLS, thread IDs, owned paths, dates; external native exception monitor | Dumps help diagnose complete exits; lobby returns are a different failure category |

## Work sequence

1. Keyboard prompt redesign delivered separately; visual review pending.
2. Achievement UI, notification and quiet sound completed; Windows ZIP ready.
3. Quit menus follow-up implemented in the launcher achievement update: uppercase
   `QUIT GAME`, corrected live frontend hook for Press Start Back/B; [75 targeted checks](quit-menu.md).
4. In-game live settings completed: only immediate controls appear; no restart-only section. [Scope and 336 targeted checks](in-game-settings.md).
5. Launcher achievement page completed, retaining the existing MotoGP design.
   [Read-only save paths, original catalog and shared notification options](achievements.md).
6. Launcher Controls simplification: ask the user for details before designing.
7. Final launcher redesign: ask the user for details before designing.

The FPS/physics investigation was explicitly cancelled and reverted. Don't
resume speculative simulation or handling changes. Frame generation is excluded
by the user's instruction. Keep experimental FSR choices available with their
limitations; don't present compatibility dispatch as full temporal support.

## Source map

- User observations and current order: ongoing conversation.
- [Roadmap](roadmap.md), [launcher requests](launcher-next-steps.md).
- [LAN findings](lan-lobby-stability.md), [crash reporting](crash-reports.md).
- [Runtime safety / validation limits](stability-quality-control.md).
- [Experimental rendering](experimental-rendering.md).
- [Career readback](career-textures.md), [collision repair](collision-data.md).
- [Keyboard prompts](keyboard-prompts.md), [achievement UI](achievements.md), [live settings](in-game-settings.md).
- Build provenance: SDK pin, runtime manifest, Git commit, per-ZIP SHA-256.
