# MotoGP 09/10: production changelog

This delivery updates the game and launcher on ChunchunQ's `nobk-online` branch.
It includes every updated end-user feature while excluding local test tools,
workspace/CI instructions, README/screenshot changes and historical build files.
The [English handoff](chunchunq-handoff.md) explains remaining service work;
[the DLC catalogue](online-dlc-catalog.json) records verified content metadata.

## Updated features

### Launcher game installation

- Add an integrated Xbox 360 ISO importer. Extract directly into the launcher's
  `game` directory, without a destination prompt or extra enclosing game folder.
- Adapt the single existing action through **Select ISO → Select title update →
  Install DLC → Verify game files**, according to the installation state.
- Show a prominent cyan missing-game/missing-update/invalid-update notice beside
  the action, prioritizing missing base files. Keep the footer dedicated to
  readiness; show progress on the action and full errors/results in dialogs.
- Accept title-update inputs through **All files** and validate file contents
  against the supported hashes. Immediately extract and verify `default.xexp`
  from a supported TU container; also accept the verified standalone patch.
- Stage installation, support cancellation during extraction, preserve existing
  game files, reject conflicting replacements and retain replaced TU files as
  backups. Do not download or bundle original game data.

Sources: `src/launcher/iso_import.h`, `iso_import_ui.inc`, readiness/verification
wiring in `main.cpp` and `compact_layout.inc`.

### Required DLC installation and portable loading

- Add **Install DLC** for the three supported MotoGP packages: Free GP Class
  Season 2010 Update, Twin Track Pack, and Free 2010 Moto2 and 125 Update.
- Accept ZIP, RAR and 7z archives or extracted folders. Both pickers support
  multiple selections with Ctrl/Shift or mouse selection; recursively find
  supported packages across the complete selection.
- Validate package identities, title/content type, container hashes and payload
  structure. Prepare the complete selection before publication, preserve existing
  content and report incompatible/conflicting inputs.
- Use a pinned portable 7-Zip reader, with integrity checks, corresponding source
  and license notices. No separately installed archiver is required.
- Install under **`<launcher>/dlc/434307F0`**, with package/header children and no
  zero-XUID parent. Include an empty `dlc/` directory in future release archives.
  Do not place active DLC in Documents or merge it into the
  base game or TU patch.
- Add runtime marketplace-content enumeration and mounting for that portable
  store, preserving SDK package-license handling and save-data operations.
- Match the launcher's dark menu styling. Restore hover installation help and
  dismiss/suspend it during source selection. Use one action for Select ISO →
  Select title update → Install DLC → Verify game files. List missing packages
  inline; no additional missing-content dialog or adjacent DLC button.
- Require all three packages locally before launch in Offline, LAN and Online,
  including direct EXE starts. Keep partial installation available for finishing
  setup. No server-side package checks or network protocol changes are added.
- Remove the successful-import information dialog. On completion, refresh the
  inline missing-package list, action and PLAY readiness immediately. Failed
  imports retain their existing error reporting.

Sources: `src/launcher/dlc_packages.h`, `dlc_archives.h`, `dlc_import.h`,
`dlc_ui.inc`, `src/dlc_paths.h`, `src/dlc_installation.h`,
`src/dlc_content.cpp/.h`, CMake/package wiring
and `third_party/7zip`.

### Steam library integration and artwork

- Add **General → Add to Steam**, adapting FIFA 09's shortcut behavior to Win32.
  Steam opens the normal MotoGP launcher with its saved settings.
- Preserve unrelated shortcut entries and supported custom fields; back up an
  existing shortcuts file before replacing it. Ask before closing/reopening a
  running Steam client.
- Load all five editable artwork files from **`resources/steam`**:
  `icon.png`, `logo.png`, `banner.png`, `cover.png`, `wide.png`.
  These Steam images are loose resources, not embedded launcher artwork.
- Use the game executable's icon as PNG. Supply the approved transparent logo,
  portrait cover with its orange top extended to a 2:3 canvas, enhanced banner
  at the existing dimensions and the approved wide composition with its title.
- Fit the General page into its existing viewport, including the Steam action
  and result. Keep spacing below game setup without adding a General scrollbar.
- Replace the hint with the full green **Added to Steam. Launch MotoGP 09/10
  from your library.** result after success, retaining it for the launcher
  session. Do not display both hint and success message simultaneously.

Sources: `src/launcher/steam_shortcut.h`, `steam_vdf.h`, `steam_page.inc`,
`compact_layout.inc`, loose resources, package/CMake wiring and FIFA attribution.

### Launcher updates

- Add one background update check per launch, numeric version comparison,
  localized release notes, verified download, progress/cancellation and a
  separate installation helper with restart acknowledgement.
- **Use FIFA 09's existing NOBK update-service protocol**: version GET query,
  `NOBK-Launcher/<version>` user agent, `version`/`url`/`sha256`/localized `notes`.
  MotoGP service/download links remain to be configured by the operator.
- Retain the native installation/startup rollback as a client fallback, with
  short explanatory comments. Preserve game content, DLC, settings and saves.
- The source constant still reads `1.1.10`, the earlier updater milestone;
  newer feature groups have not been assigned final release versions. Do not
  treat this as a completed version calculation for the entire delivery.
  Normal checks are inactive while the MotoGP manifest endpoint is empty.
  The visual preview's `1.1.11` is synthetic, not an approved next release.
- Keep the existing MotoGP update UI: widen its panel, use the slim launcher
  scroll rail, enlarge the close target with hand cursor and translucent hover,
  and place the main action lower to leave more room for notes.
- Disable installation while the game is running; serialize conflicting import,
  Steam and update operations. Provide a visual-only `--preview-update` mode.

Sources: `src/launcher/updates.h/.cpp`, `update_panel.inc`, `update_protocol.h`,
`update_install.cpp`, `update_helper.cpp` and build/package integration.

### Online records and account recovery

- Fix native **Lap Times**, **All Time Ghost** and **Monthly Ghost** entries
  showing no time or an incorrect Arcade label
  when the website already displays a valid result.
- Project uploaded lap ticks, full mode bits, track and season into the native
  read columns expected by groups 0, 2 and 5 (`1..63`, `127..189`,
  `316..378`). Preserve the int64 rating and internal/raw reads; leave the other
  189 views unchanged. Ghost replay transfer remains deferred.
- Preserve native Championship/Career/Arcade/Time Trial label handling. Provide
  explicit labels for other known modes and a neutral unknown marker, without
  guessing that every lap came from Time Trial.
- Add **Forgot password?** to the signed-out Online page, using FIFA 09's shared
  NOBK recovery page in the default browser, without hover help. Add My account
  to the same shared account page as FIFA. No new password-reset API is added.
- Use the approved unframed Online design, orange account name and optional
  API-supplied country code. Keep username/password side by side at their
  existing size. Show hosting room name after sign-in; preserve LAN placement.
- Add Remember me, enabled by default, following FIFA's token lifetime behavior
  while preserving Windows DPAPI protection. Unchecked sessions and refreshes
  stay in memory; Online PLAY still requires sign-in.
- Show launcher version in the footer instead of title-update version and fit
  all nine FPS presets into the selector without scrolling.

Sources: `src/online/stats.cpp`, `leaderboard_display.cpp/.h`,
`src/launcher/network_page.inc`, `nobk_account.h`, `ui_ids.h` and `main.cpp`.

### Match setup and rider selection

- Remove **Private Slots** from native match settings. Keep its stored value at
  zero and expose new PC-hosted room capacity as public, including capacity
  changes. Preserve the wire field and joining compatibility with other hosts.
- Make installed riders available in Quick Race, Arcade, Championship, Time
  Trial, local split screen and non-career multiplayer selection. Preserve Career
  and Split Screen Career progression and saved unlock records.
- Do not bypass content availability, provide absent DLC or grant achievements.

Sources: `src/options_menu.cpp/.h`, `src/lan/guest_bridge.cpp`,
`src/rider_selection.cpp` and application/CMake wiring.

### Input stability, mappings and small UI fixes

- Add local splitscreen entry routing for keyboard plus one controller or two
  controllers, with the controller always player 1 in the keyboard/controller
  combination and an ephemeral second local user. Retain native eligibility
  warnings and restore ordinary routing on return to the parent menus.
- Replace the host Quit confirmation with the title's native Yes/No dialog,
  defaulting to Yes, with explicit native Yes/No labels, for both Quit Game
  and Back on initial Press Start. Focus changes still reset the choice to No.
  Preserve focus/held-input safeguards and the existing close/save handler.
  No standalone Quit announcer voice is included.
- Serialize application-owned guest input imports and direct F1/quit polling
  through a shared lock. Guard focus transitions and reject stale input samples
  spanning Alt+Tab; require a fresh neutral sample before releasing UI blocking.
- Correct the controller-action labels to the native default actions, including
  tuck-in, gear-up, look-back and gestures, consistently in launcher/live settings.
- Preserve Space/Backspace confirm/back defaults; clear obsolete defaults for
  actions labelled Unassigned while retaining different custom bindings and
  explicit runtime overrides. Keep physical-controller operation independent.
- Antialias rounded slider thumbs in the launcher and styled dialogs.
- Fix launcher page-scrollbar hit testing so painted rails accept dragging.
- Add standard Ctrl+A/C/X/V/Z editing shortcuts to achievement search.

Sources: `src/input_synchronization.h`, `in_game_settings.cpp/.h`,
`in_game_settings_input.cpp`, `quit_menu.cpp`, `input_defaults.h`,
`keyboard_binding_specs.h`, and launcher paint/layout/search code.
See also `src/local_splitscreen.cpp/.h`, `splitscreen_assignment.h`,
`docs/local-splitscreen.md` and `docs/quit-menu.md`. The local splitscreen
and native confirmation changes require native gameplay review; earlier input
results do not establish coverage of these paths.

### Display selection and layout

- Add a persisted monitor selector, with the primary screen first and monitor
  numbering aligned to the Windows runtime. Rebuild available output sizes and
  refresh rates for the selected display.
- Add an explicit experimental **4:3** camera/presentation option, fitting the
  original HUD width and centering vertically on narrow output.
- Reorganize Graphics controls, move shader cache beside letterbox and refine
  displayed graphics/help labels. The Advanced effects and 1080p FSR preset
  already existed in the reference branch; they are not newly added here.

Sources: `src/launcher/hardware.h`, `core.h`, `main.cpp`, `compact_layout.inc`,
`launcher_help.inc` and `src/ultrawide.cpp`.

### Turkish language review

- Improve existing game, launcher, tooltip, error-report and achievement wording
  where literal translations obscure the action or context.
- Preserve proper circuit names and improve upgrade descriptions; regenerate
  the matching message overlay and manifest metadata.
- Keep the launcher tab named **Online**. This does not require replacing every
  explanatory occurrence of the Turkish term for an online connection.
- Retain **Launcher** in the Turkish language label, version footer, Steam help,
  account errors and update messages. Do not translate the product term.
- Size the four footer action buttons from the rendered labels, preserving the
  English padding and gaps in Turkish. Recalculate after language/DPI changes;
  do not shrink fonts or truncate the Turkish labels to fit fixed widths.
- The installed DLC scripts remain unmodified; this review does not claim a new
  translation of all DLC text.

Sources: `resources/localization/tr`, `scripts/build-turkish-localization.py`,
`src/turkish_achievements.h` and localized launcher/live-settings text.

### Default save-data directory

- Use **`Documents/MotoGP 09-10`** for default Windows user data and default
  cache, honoring redirected Documents folders and explicit custom locations.
- On first migration, copy the old default folder to staging, verify it and
  publish the complete new directory. Delete the old folder only after every
  old file has a verified matching copy in the destination. Preserve originals
  and report migration failures rather than starting with empty saves.
- An existing new directory wins. A one-time cleanup also handles folders copied
  by earlier releases; differing or unverified old data is preserved. A cleanup
  marker prevents repeated scans on subsequent launches.
- Share the path helper between game startup and launcher achievements. Keep
  portable DLC independent of the save-data path; Linux defaults are unchanged.

Sources: `src/user_data_paths.cpp/.h`, `motogp0910_app.h`,
`src/launcher/achievements.cpp` and `motogp_config` build wiring.

## Remaining integration and verification

- Moto2 statistics normalization and independently stored 2009/2010 records
  still need native evidence and coordinated website/backend design.
- The operator must configure the MotoGP update endpoint and agree on the
  actual release version. The existing `1.1.10` constant is the earlier updater
  milestone; the preview `1.1.11` is synthetic. No new release version is set.
- Ghost replay transfer and Gamer Card remain deferred; timed-board display
  fixes do not implement replay upload/download or an in-game player card.
- A previous local Windows build predates the final successful-DLC-dialog
  removal and Turkish footer polish. This extracted production branch has not
  been built or tested. Native gameplay/controller review and real-service
  update/restart/rollback acceptance remain pending.
- Preserve the upstream NOBK account linking, 60 Hz website time correction and
  QoS diagnostics already present in the target branch; those are not newly
  implemented by this delivery.
