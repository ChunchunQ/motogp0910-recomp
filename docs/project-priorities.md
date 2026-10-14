# Model work priorities — 14 October 2026

Scope inspected: this conversation, current checkout, Git history, roadmap,
launcher requests, stability/career/collision/network/rendering/crash documents,
source layout, Windows validation/packaging scripts and published ZIP inventory.
No other personal projects or external account systems were inspected.

The main objective is a reliable, usable Windows recompilation. The highest
return comes from consolidating existing evidence and making builds/tests
repeatable. It does not come from another SDK/launcher reference search.

The following numbers rank downstream benefit, not implementation permission
or complexity. The agreed visible-feature sequence stays achievements → urgent
exit menus → in-game live settings → Controls UX → final launcher UX.

| Rank | Group | Finished artifact to pursue | Existing material to reuse |
|---|---|---|---|
| 1 | QUICK WINS | Full feature/evidence matrix: implemented, automated, user-verified, pending, rejected; each tied to build/hash | Current project-status.md, roadmap, docs, conversation, commits, probe results |
| 2 | QUICK WINS | One current Windows release record plus rollback index; consistent readme links and hashes | downloads, package-windows.py, SDK pin, runtime manifest, .map files |
| 3 | QUICK WINS | Close the active UI delivery, then real Quit game/Press Start back confirmation with save/LAN-safe exit | New achievements implementation, original generated menu/state paths, shutdown and crash-monitor code |
| 4 | DEEP WORK | Risk-based native Windows validation plan and machine-readable evidence; each failure becomes a targeted regression | Existing 27 tool sources, verify-windows.py, career/LAN/rendering evidence limits |
| 5 | DEEP WORK | LAN failure contract, real session/QoS option semantics and reconnect/retry regressions | Node/guest/socket bridge, expired-results and null-slot leave cases, XNetGetOpt option 6 warning |
| 6 | DEEP WORK | Save/config upgrade/recovery contract with isolated migration/rollback tests | SDK save paths, launcher backups/conflicts, game/ layout, achievement store and new UI preferences |
| 7 | DEEP WORK | In-game settings implementation with a setting-by-setting live/restart capability matrix | Launcher core, SDK cvar lifecycle, audio/display/engine/prompt hooks, F4/F7 patterns |
| 8 | DEEP WORK | Controls interaction specification and implementation based on game actions and existing bindings | 25 binding specs, capture/parser code, original prompts and previous controller tests; ask user for intended simplification |
| 9 | SYSTEMS | One Windows release pipeline: build → targeted regression → matching DLL/map/resource checks → package → report | Existing Windows/LAN/crash runners, packaging, SDK hashes and font/icon assets |
| 10 | SYSTEMS | Anonymized failure-case library with hypothesis/observation/test/accepted-or-rejected verdicts | Collision vs fetch warning, LAN drop vs crash, empty CLI args, PS3 texture pitch/replay row, cancelled physics investigation |
| 11 | SYSTEMS | SDK patch ownership and migration ledger with reproducible patch replay and rollback | cmake patches, build-spatial-sdk.py, pinned source, runtime manifest, sdk-nightly-migration.md |
| 12 | EXPERIMENTS | FSR decision gate based on actual Windows GPU captures and correct depth/motion/jitter/history feasibility | Existing AMD provider integration and experimental-rendering.md limitations |
| 13 | EXPERIMENTS | Ultrawide camera/HUD/culling evidence matrix, with fixes only for demonstrated failures | Shared projection and quad hooks, 637 existing engine/projection checks, native viewport captures |
| 14 | EXPERIMENTS | Native frame-time baseline and measured optimizations without speculative handling/simulation changes | frame_pacing, livery readback scope, runtime hot paths, shader cache and matched builds |

## Highest-return starting sequence

1. Complete and maintain the evidence matrix; project-status.md is the current
   canonical status and corrects stale LAN/native-validation claims.
2. Make one current-release record and rollback index. Never delete older
   packages blindly or rewrite history; archive and label their scope.
3. Finish active user-visible work: achievements delivered, exit menus next.

Underrated: save/config preservation across upgrades and rollback. A rendering
fix can be retried; lost career progress damages the project more seriously.

Poor uses of model capacity now: more Linux renderer tuning for a Windows-only
priority, repeated Downpour/FIFA/GOW UI browsing after the relevant patterns are
already known, new labels around the existing FSR compatibility path without
real inputs, frame generation that the user excluded, or reopening the cancelled
FPS/physics theory without new evidence. Keep documented experimental options;
this is a prioritization decision, not removal authorization.

One substantial handoff: the automated Windows build/regression/package pipeline
and evidence reconciliation can largely be implemented by an agent from existing
scripts. The user only supplies/authorizes a native Windows GPU runner if needed
and checks the small set of actual gameplay/visual/audio acceptance scenarios.
The current cloud environment cannot substitute for that GPU evidence.

Future task contract: one tracked record per request containing intent, accepted
scope, proof/reproduction, acceptance checks, exact delivery build, status, and
an explicit list of deferred/rejected actions. Update it when the user changes
priority, rather than accumulating another conversational plan or ZIP-only note.
