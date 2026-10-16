# Compact Racing Lines launcher

The user selected concept B, with a further requirement: minimize and close must
sit directly over the background artwork, without a persistent title-bar tile.
The production launcher now uses that layout, with a **940 × 578 logical-pixel**
window at 100% DPI. Per-monitor DPI and small-desktop fitting remain supported.

- Six horizontal tabs replace the sidebar: General, Graphics, Advanced, Audio,
  Controls and Achievements. General remains the startup page.
- The supplied cover inspired a separate Racing Lines background embedded in the
  launcher, rather than an external image that can disappear from the package.
  The left surface stays opaque for legibility; artwork continues behind both
  title buttons. Hover adds a translucent tint; the idle buttons have no
  backing tile. Maximize remains absent.
- Scrollable child content preserves every existing setting, all 25 keyboard
  bindings, connection modes and the independent achievement scrollbar.
  Keyboard focus reveals off-screen fields. Action buttons and status remain
  pinned below the content. Settings and existing saves retain their formats.
- Closed dropdowns and client-drawn scrollbar rails use the charcoal palette. There is no native nonclient scrollbar underneath the rails. Mouse dragging, keyboard navigation and page scrolling are retained. A wheel over a closed selector scrolls its surrounding page without changing the selection; explicit open-list selection remains supported.
- The background is cached by window size, so hovering title buttons does not
  repeatedly resample the original artwork.

## Verify game files

General → **Verify game files / Dosyaları doğrula** starts a cancellable background
worker. The UI stays usable; game-folder edits and PLAY are temporarily disabled.
It checks the original executable/title update using the existing trusted hashes,
then all **4,386 `xenondata` files** against the embedded supported-disk SHA-256
reference. Same-size corruption is detected. Missing, changed and unreadable
files are reported. Additional files are permitted; the reference is never
learned from the user's possibly corrupt installation.

The verifier does not repair, download, replace or delete game files. It does not
write to saves, achievement progress or Online credentials. A completed check
writes `logs/verification-<UTC timestamp>-<pid>.txt`; **Logs** opens that folder.
Cancellation leaves files untouched and restores PLAY/folder controls. Closing
the launcher requests cancellation and joins the worker before releasing state.
The supported disk/TU combination is the same one already required for launch;
this is not a universal validator for other game editions or modified dumps.

The asset reference is `resources/verification/game-files.sha256`, embedded as
RCDATA alongside `resources/launcher/background.png`. The parser rejects absolute
or parent-traversal paths. No original game data is added to the Windows package.

## Relevant source

- `src/launcher/compact_layout.inc`: layout, child viewport, focus scrolling,
  background cache, native control painting and asynchronous verifier UI.
- `src/launcher/file_verification.h`: embedded resources, reference parsing and
  read-only SHA-256 verification.
- `src/launcher/main.cpp`: control creation, events, persistence and app lifecycle.
- `src/launcher/caption.inc`: transparent-background minimize/close painting.
- `src/launcher/achievements_page.inc`: compact achievement list/summary.
- `tools/launcher_layout_probe.cpp`: production UI, real original data and mouse tests.
- `tools/launcher_ui_probe.cpp`: separate-process production launcher interaction.

## Verification scope

Windows PE launcher/probe targets are built using the pinned patched nightly
runtime. Wine/Xvfb tests exercise real Win32 controls: English/Turkish, six pages,
75/100/125/150% scaling, scroll positions, actual native achievement-thumb dragging,
minimize/close, caption background pixels, persistence and existing render/network
settings. The file-verification tests use the real full original data tree, a
same-size altered copy, missing files, cancellation, completion and result logs.
The initial redesign run passed **65,756 actual launcher UI checks**, plus ten actions against the standalone production EXE (open/language changes, page selection, actual nested checkbox clicks, save and close). Unknown TOML keys were preserved and no game save was created. Follow-up changes and results are recorded below. Native Windows desktop/GPU user review remains separate from these checks.

[General page screenshot](screenshots/launcher-racing-lines-general.png) and
[Turkish achievements screenshot](screenshots/launcher-racing-lines-achievements.png)
show the actual compiled launcher under Wine, not the earlier generated concept.

Future development packages omit README/feature instructions, SHA256SUMS and the external licenses directory. Required third-party notices are embedded in resource 104 and can be read through the launcher system menu (Alt+Space → Third-party notices). The packager checks that the launcher contains the complete notice bundle before omitting sidecar licenses. Runtime pins and the separate archive hash remain; required resources and debug map are preserved. Existing published ZIPs are not rewritten by this policy change.

Original redesign package: `downloads/MotoGP0910-Windows-x64-launcher-racing-lines.zip`.

SHA-256: `07ae02b2cd5439a8bdf401fb82c4facbb0ebd46251b5f442e294e42f03f80ccc`.

## Quality hotfix after Windows user feedback

The earlier layout checks did not capture the reported interaction stutter and
intermittent native scrollbar paint. Page switches previously rebuilt every
selector via Localize (including synchronous display-mode enumeration), and
root repaints could paint over child controls. Native scrollbar tracking could
also repaint white after the dark overpaint. These paths were removed:

- Tabs change only outgoing/incoming page visibility, with redraw held during
  the batch and focus moved out of hidden controls. They never rebuild lists or
  re-enumerate monitor modes. Root painting clips child windows.
- Page/achievement rails are painted inside their client windows and handle
  their own capture/drag input; no WS_VSCROLL/nonclient recoloring remains.
- Closed combo fields paint once, rather than repainting a native white face
  and then covering it. Wheel input does not change closed selectors, including
  the DualSense dialog. Open lists remain intentionally selectable.
- Idle achievement polling does not invalidate unchanged progress every tick.
- Body fonts, label/field gaps and row spacing are normalized; Graphics shows
  five complete pairs of label/selector rows in its initial viewport.

The final Windows PE/Wine test run passed 72,730 checks. New regressions cover
120 rendered rapid tab switches without localization/list resets, unchanged
pending settings, closed-selector wheel behavior, actual drag input and scroll
rail pixels after parent/nonclient repaint. In this cloud Wine fixture the 120
painted switches took 3,048 ms total (slowest 60 ms). These are fixture timings,
not measurements on the user's native Windows hardware.

Previous hotfix: `downloads/MotoGP0910-Windows-x64-launcher-quality-fix.zip`.
SHA-256: `459cefc704156a7609ccf335f8f557c3d26db6e648d80da1a021e0b3e0125048`.

## Caption, typography and Windows video follow-up

The user's 48.8-second Windows recording still showed white strips and partial
control paints while scrolling Graphics, plus jumping pages / missed first
clicks. Static final-layout assertions had not exposed that behavior.

- Scrolling now moves one clipped content window, leaving every combo, slider
  and label at a stable position within that surface. The viewport uses
  WS_EX_COMPOSITED and sibling clipping to present complete child paints.
  It no longer moves and resizes each native field on every wheel tick.
- A mouse-down focus change no longer invokes automatic Reveal on its target.
  Previously a partly visible selector could move before its native hit test
  finished, making the page jump and losing the intended click. Keyboard focus
  still reveals the complete field.
- Launcher language changes are queued until the native language popup closes.
  Rebuilding its lists during a selection callback could disturb input and
  scroll position. The update batches painting and preserves pending settings.
- Caption symbols are equally sized vectors, independent of text-font glyphs.
  Minimize has a translucent neutral hover, close a translucent red hover, with
  distinct pressed feedback. Leaving restores the exact artwork. Immediate
  caption updates are buffered before presentation.
- Open Sans 3.003 Regular, SemiBold and Bold are embedded and registered only
  in the launcher process. Turkish glyph coverage, actual selected face and
  layout metrics are checked; users need no font installation. The original
  in-game prompt font remains unchanged. Font provenance and hashes are in
  `third_party/launcher-fonts/NOTICE.txt`; required notices stay inside resource 104.
- Navigation measures each translated label and centers the group with equal
  visible text gaps, including Controls / Achievements. A broader eased spatial
  fade blends the opaque form surface into the artwork without a repeating
  animation or extra background resampling on hover.

The final Windows PE/Wine run passed **74,028 UI checks**, including actual OS
mouse input opening the bottom selector on its first click without scrolling,
keyboard focus reveal, deferred language selection, stable native child
coordinates across 48 scroll changes, dark field pixels, caption hover/pressed/
leave pixels and icon bounds at 75/100/125/150% scale in both languages. Existing
file-verification, achievement, setting preservation and interaction checks
also passed. The 120 rendered tab switches took 1,934 ms total, slowest 37 ms,
in this fixture; this is not a native Windows performance measurement.

The separate production EXE passed ten open/page/checkbox/language/save/close
actions; an unknown config key remained intact and no game save was created.
Native Windows reproduction of the user's transient paint issue still needs
their follow-up: Wine passing does not prove identical Windows compositor behavior.

[Current compiled launcher](screenshots/launcher-racing-lines-general.png) and
[close-button hover](screenshots/launcher-racing-lines-caption-hover.png).

Current package: `downloads/MotoGP0910-Windows-x64-launcher-polish.zip`.
SHA-256: `7fc35e1480d7a59096b0d442dc7f41b1ccb222a837761863333801cb732a65da`.
Archive CRC and contents were checked; compared with the previous hotfix, only
`motogp0910-launcher.exe` changed. The game executable, pinned SDK runtime DLLs,
game resources, controller mappings and crash-report map are byte-identical.
No README, SHA256SUMS, sidecar licenses, personal configuration or game files
are included.
