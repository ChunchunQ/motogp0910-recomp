# Development roadmap

The [current evidence matrix](project-status.md) is the status authority.
Historical milestone documents retain their original test scope and do not
supersede the current SDK pin or [installation instructions](game-layout.md).

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
8. Required three-package DLC import and portable runtime mounting, direct ISO/TU
   setup, Steam shortcuts/artwork and shared JSON/save-folder migration.
9. Shared NOBK account sign-in, token refresh and PC linking; Remember me,
   country display and website account/recovery actions. The baseline account
   system was inherited from ChunchunQ, not introduced by Rider License.
10. Native timed-board projection, local splitscreen, non-career rider selection,
    PC menu cleanup and native quit confirmation.
11. Rider License Overview/Records with Rider Rank, Tracks Held, Records Held,
    Boards Entered, selected-board map/lap/date and bounded bio/general emoji.
12. Launcher update-service client/helper with local rollback; version 1.1.11.
    The manifest endpoint remains empty pending operator configuration.

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
- Validate the configured NOBK production integration end to end: account
  sign-in/linking, Internet racing and native leaderboard UI. The production
  origin is configured; this does not establish deployment health or race
  acceptance. Independent server operators must verify TLS/firewalls/routing;
  hosting remains separate from the player client.
- Finish website DLC catalogue/outline publication and establish native Moto2
  statistics mapping and independent season storage/query behavior. Preserve
  the temporary Rider License 2009 scope until that work is agreed.
- Verify Rider License native GPU sharpness, controller input, first-load failure
  recovery and lobby circuit context; the latter is not yet integrated.
- Configure a MotoGP-specific launcher manifest and exercise a real update,
  cancellation, failed restart and rollback before enabling public updates.
- Verify adaptive resistance on real DualSense/Edge hardware. The user's
  8BitDo does not provide that hardware feature.
- Keep FSR 2/3 and ultrawide limitations explicit; temporal reconstruction needs
  valid engine inputs and real GPU evidence before broader claims.

## Deferred or excluded

NOBK website accounts and PC linking are implemented; a separate custom
account/password/recovery-code service was never adopted. The obsolete standalone
account proposal and completed launcher/profile checklists have been consolidated
into the current feature documents. Code signing remains deferred. Do not write
the final README until the user declares the entire project complete; the root
placeholder remains unchanged.
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
