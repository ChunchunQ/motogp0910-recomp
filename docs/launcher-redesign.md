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
  title buttons. Hover changes only the glyph color. Maximize remains absent.
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
The final run passed **65,756 actual launcher UI checks**, plus ten actions against the standalone production EXE (open/language changes, page selection, actual nested checkbox clicks, save and close). Unknown TOML keys were preserved and no game save was created. Native Windows desktop/GPU user review remains separate from these checks.

[General page screenshot](screenshots/launcher-racing-lines-general.png) and
[Turkish achievements screenshot](screenshots/launcher-racing-lines-achievements.png)
show the actual compiled launcher under Wine, not the earlier generated concept.

Future development packages omit README/feature instructions, SHA256SUMS and the external licenses directory. Required third-party notices are embedded in resource 104 and can be read through the launcher system menu (Alt+Space → Third-party notices). The packager checks that the launcher contains the complete notice bundle before omitting sidecar licenses. Runtime pins and the separate archive hash remain; required resources and debug map are preserved. Existing published ZIPs are not rewritten by this policy change.

Current package: `downloads/MotoGP0910-Windows-x64-launcher-racing-lines.zip`.

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

Current hotfix: `downloads/MotoGP0910-Windows-x64-launcher-quality-fix.zip`.
SHA-256: `459cefc704156a7609ccf335f8f557c3d26db6e648d80da1a021e0b3e0125048`.
