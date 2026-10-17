# Development roadmap

The [current evidence matrix](project-status.md) is the status authority.
Historical milestone documents retain their original test scope and do not
supersede the current SDK pin or package linked from the [README](../README.md).

## Delivered work

1. Original game layout under `game/`, direct EXE startup, retained explicit
   keyboard/controller preferences and packaged SDL mapping database.
2. Missing callback/initializer/function boundaries, collision serialization,
   scoped career readback, runtime stability and native crash diagnostics.
3. Xbox/PS3/keyboard prompts, dynamic native control diagrams, F1 live settings,
   F7/launcher achievements and the user-accepted Press Start exit/white accent.
4. Turkish written localization, native font extension and Marketplace removal.
   Current font QA normalizes all twelve Turkish letters across four banks.
5. LAN discovery/session/socket integration and separate community Online
   service for Linux/Windows, relay and native leaderboard persistence.
6. User-selected Racing Lines launcher with artwork, file verification, native
   hover help and conditional multiplayer settings.
7. Supplied wet Motegi log audit and compatibility fixes; see
   [recorded findings and retest boundary](motegi-rain-qa.md).

## Acceptance work still needed

- Repeat the three-lap wet Motegi run on native Windows with the complete new
  executable/DLL/font package and inspect the newly generated log.
- Visually check Turkish glyphs, long strings, career/tutorial screens and HUD
  across the actual Windows renderer. Atlas/CPU checks are supporting evidence.
- Exercise every mode/track/class, restarts, save reloads and long sessions.
- Check career-generated liveries, sound balance/synchronization and hotplug,
  focus changes and physical controller variants.
- Validate LAN retry/disconnect/rejoin and larger grids on separate machines.
  Previous successful two-PC racing does not cover every network condition.
- Deploy the community service to a real Linux or Windows VDS; verify TLS,
  firewall/routing, Internet racing and native leaderboard UI. Hosting runs
  separately from the game client and does not require every player to have
  administrator access to the VDS.
- Verify adaptive resistance on real DualSense/Edge hardware. The user's
  8BitDo does not provide that hardware feature.
- Keep FSR 2/3 and ultrawide limitations explicit; temporal reconstruction needs
  valid engine inputs and real GPU evidence before broader claims.

## Deferred or excluded

Portable username/password accounts remain the
[documented proposal](community-accounts-plan.md), not implemented behavior.
Code signing and the final packaged README are deferred until release.
The Claude migration was cancelled; its obsolete handoff was removed.
Frame generation is excluded. The speculative FPS/physics investigation was
cancelled and reverted; do not resume it without new authorized evidence.

## Quality priorities

Preserve saves/configuration and existing offline behavior. Fix reproducible
crashes, hangs and data corruption first, then functional races, graphics,
audio, input and measured performance. Keep Windows connection quality and
clear UX central; preserve LAN when improving Online. Record the actual
reproduction, owning code, regression, delivered build and unverified boundary
for every fix. Do not claim a headless or Wine pass establishes full gameplay.

The supplied TU patches stock 0.0.0.1 to **0.0.1.1** in memory. The runtime
checks this supported version and keeps original files unchanged. Applying
this TU does not establish that it was the last official TU ever published.
