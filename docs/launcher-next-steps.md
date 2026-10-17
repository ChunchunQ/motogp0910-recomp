# Historical launcher requests — 14 October 2026

These are milestone notes. The implemented Racing Lines launcher and current
follow-up work are documented in [launcher-redesign.md](launcher-redesign.md)
and [project-status.md](project-status.md).

The user asked to work from simple to complex, show each finished stage before
moving on, and ask before implementing any unclear requirement.

Completed:
- Internal resolution labels: 1x - 1280 x 720 (native), 2x - 2560 x 1440,
  3x - 3840 x 2160.
- General page: game folder visible only here; Launcher language moved from
  the top-right corner; Game language below it; Button prompts below that.
- Prompt choice: Automatic / PlayStation 3 / Xbox 360 / Keyboard.
- Game language: English / French / German / Spanish / Italian, independently
  of English/Turkish launcher language.
- Output-size list capped to native monitor dimensions, independently of the
  internal rendering resolution.

- Fullscreen / borderless / windowed, real host fullscreen Hz and an independent
  optional FPS cap. Windows/Wine window and configuration tests passed.

- Advanced page with actual title hooks: speed blur, depth of field, dynamic
  shadows (game default/off), and scoped shadow distance (default/45/90/180).

Current rendering work:
- Experimental FSR 2.3.3 / FSR 3.1.4 providers and six quality modes are included.
  Typed neutral depth/motion inputs and per-frame history reset are temporary;
  proper temporal reconstruction remains incomplete. FSR 1/CAS is the safe path.
- Experimental Auto / 16:9 / 21:9 / 32:9 horizontal-plus camera and central HUD
  are included; broad native Windows GPU/ultrawide gameplay testing is pending.
- Frame generation is excluded at the user's request.
- The exact SDK pin is now Nightly 0.10.0.24-dev.gbd833a2. Stable v0.10.0 remains
  available as a separate rollback package.
- Further Advanced controls require verified equivalents in MotoGP's engine;
  Downpour's UE3 controls cannot be copied directly.
See [rendering scope](experimental-rendering.md), [migration](sdk-nightly-migration.md)
and [stability work](stability-quality-control.md).

While the user is away, suspend ambiguous work, continue independent tasks,
and perform a consolidated verification at the end.

Keep the exact SDK pin in sdk-version.json and preserve the career, collision and PS3/replay
repairs. Generated filenames remain English. Do not commit preview screenshots.

## In-game access — 15 October 2026

F1 live settings is delivered in the achievement menu's MotoGP theme. Only
immediate controls appear; the user explicitly rejected a restart-only section.
Saving updates the shared launcher TOML; the launcher reloads on child exit.
See [live settings](in-game-settings.md) for the consumer matrix, safe persistence,
input ownership, SDK keyboard-toggle exclusion and native Windows verification
limits. Controls redesign and the final launcher redesign remain later stages
whose UX requirements must be clarified with the user.

DualSense adaptive triggers now have separate launcher/F1 controls; see
[the researched SDL approach and hardware limits](adaptive-triggers.md).
