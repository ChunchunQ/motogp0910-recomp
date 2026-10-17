# Racing Lines launcher

The user selected concept B, with a further requirement: minimize and close must
sit directly over the background artwork, without a persistent title-bar tile.
The original compact layout used a **940 × 578 logical-pixel** window. The
current expanded layout is **1040 × 732 logical pixels** at 100% DPI. Per-monitor DPI and small-desktop fitting remain supported.

- General, Graphics, Advanced, Audio, Controls and Achievements are regular pages.
  General includes a Multiplayer Settings button opening a separate owned
  dialog. General remains the startup page.
- The supplied cover inspired a separate Racing Lines background embedded in the
  launcher, rather than an external image that can disappear from the package.
  The left edge stays opaque for legibility and blends into artwork inside
  the content column; artwork continues behind both title buttons. Hover adds a translucent tint; the idle buttons have no
  backing tile. Maximize remains absent.
- Scrollable child content preserves every existing setting, all 25 keyboard
  bindings, connection modes and the independent achievement scrollbar.
  Keyboard focus reveals off-screen fields. Action buttons and status remain
  pinned below the content. Settings and existing saves retain their formats.
- Closed dropdowns and client-drawn scrollbar rails use the charcoal palette. There is no native nonclient scrollbar underneath the rails. Mouse dragging, keyboard navigation and page scrolling are retained. A wheel over a closed selector scrolls its surrounding page without changing the selection; explicit open-list selection remains supported.
- The background is cached by window size, so hovering title buttons does not
  repeatedly resample the original artwork.

## Verify game files

General → **Verify game files** starts a cancellable background
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

Previous package: `downloads/MotoGP0910-Windows-x64-launcher-polish.zip`.
SHA-256: `7fc35e1480d7a59096b0d442dc7f41b1ccb222a837761863333801cb732a65da`.
Archive CRC and contents were checked; compared with the previous hotfix, only
`motogp0910-launcher.exe` changed. The game executable, pinned SDK runtime DLLs,
game resources, controller mappings and crash-report map are byte-identical.
No README, SHA256SUMS, sidecar licenses, personal configuration or game files
are included.


## Expanded layout and multiplayer editor

The earlier achievement viewport was only 152 pixels high with cards at least
98 pixels tall. It could show roughly one full achievement. The launcher now
uses a **1040 × 732** window and a **610 × 480** settings viewport. Achievement
cards use a 76-pixel minimum with measured wrapping for longer descriptions,
40-pixel icons and a 330-pixel list. Four normal cards fit completely at once.
The description uses Open Sans 12; titles stay semibold 14. Longer text still
expands the row instead of being cut off. The original filters, real unlock
save, sounds, popup preferences and independent scroll rail are retained.

Seven measured navigation labels fit in the wider settings column, with visual
creation order matching keyboard Tab order. The background preserves its source
aspect ratio and crops from the left; its soft fade follows the content edge.
Small desktops still fit the whole launcher automatically.

**Multiplayer** opens an owned **640 × 548** client window. The mode, player/room
names, automatic LAN local/host IP fields and community HTTPS URL move out of
General. Apply validates the same supported inputs and changes only pending
network settings. Cancel changes nothing. Save / PLAY stores the existing keys
and forwards the existing game arguments. Unrelated pending graphics/input
settings stay intact. The modal is disabled while a game is running, since
these options apply on the next launch. LAN / Radmin discovery and IP fallback
stay available alongside Offline and Online; no transport or server logic changes.

Graphics puts Upscaler sharpness immediately below Upscaling / sharpening and
FSR quality, above VSync, aspect ratio and other display toggles. It is visible
in the initial viewport, and explains how to enable it when Bilinear is selected.

The adaptive trigger audit confirms 0x02 is section resistance. Launcher and F1
now use Constant resistance / Resistance zone terminology. Mode-specific unused
fields are disabled, while config identifiers and physical effect bytes retain
compatibility. The explicit fixed-resistance explanation avoids implying a
speed/RPM-driven racing effect. See [trigger scope](adaptive-triggers.md).

Compiled Windows UI screenshots: [achievements](screenshots/launcher-racing-lines-achievements.png),
[graphics](screenshots/launcher-racing-lines-graphics.png),
[multiplayer](screenshots/launcher-racing-lines-multiplayer.png) and
[trigger editor](screenshots/launcher-racing-lines-triggers.png).

The final Windows PE / Wine run passed **90,877 launcher UI checks** across
English/Turkish and 75/100/125/150% scaling. New regressions cover four-card
list capacity, seven-tab spacing and keyboard order, the real Multiplayer tab
opening/closing its owned modal, all three modes, retained settings, validation
failures, disabled unrelated fields, Cancel, serialization and exact outbound
mode arguments. Mode-specific trigger fields and themed Windows checkbox
custom-draw routing are covered too. Prior rapid scrolling, first mouse click,
closed-selector wheel, language, achievement and file verification checks pass.
The 120 rendered tab switches took 2,045 ms total (slowest 42 ms) in the fixture.

The actual runtime's **200 adaptive trigger software/transport checks** and
**345 live setting, input, persistence and UI checks** also passed. Packet
encoding and physical parameters are unchanged. Physical DualSense feel and
native Windows desktop/compositor behavior remain outside this cloud test.

Previous package: `downloads/MotoGP0910-Windows-x64-launcher-expanded.zip`.
SHA-256: `dbf44a117a39f827f72fa34d1f6f985ee1d4bb03ed3857ff4c5cac6534881f59`.
The package omits README, SHA256SUMS and sidecar licenses. Archive checks confirm
that only the launcher, game EXE and matching crash map differ from the previous
package. Pinned runtime DLLs and all external game/UI resources remain identical.


## General Multiplayer Settings button and artwork composition

The Multiplayer navigation tab was removed at the user's request. **General →
Multiplayer Settings** now opens the same owned editor. The button appears only
on General, next to Button prompts, and stays disabled while the game runs.
The old General network explanation was removed; connection-mode context and
mode-specific LAN / Online instructions live in the owned window.

The reference sketch indicated a direction, not an exact boundary. The picture
now fits the full window width without zooming to fill the taller window. A
soft lower fade removes its unused bottom edge, while an eased horizontal blend
starts inside the content column. The text fields and achievement cards keep
solid readable faces. Backgrounds around them, including labels, navigation,
checkboxes, sliders, viewport and scroll gutters, share a cached full-window
bitmap. This prevents an opaque child panel from cutting off the artwork or
producing mismatched rectangular patches. Scrolled child source rectangles are
clamped to the cached image, and WM_PRINTCLIENT uses the same composition for
themed parent-background requests. The size cache and clipped single-surface
scrolling remain in use.

Validation: the rebuilt Windows launcher passed 87,293 real UI assertions under
Wine/Xvfb, including matching child/WM_PRINTCLIENT background pixels across
DPI scales and both languages, actual General-button modal opening, six-tab
keyboard order, scrolling, selector input and preserved multiplayer validation.
120 painted page switches took 2,499 ms total (48 ms slowest). The separate
production EXE passed open, tab/control actions, language changes, save and close;
an unknown config key was preserved and no game save was created.

Previous package: `downloads/MotoGP0910-Windows-x64-launcher-composition.zip`.
SHA-256: `42c50b5f494a1b92499908918af0deed54e50388518d03e3065c538426b5860b`.
ZIP CRC and membership checks passed. Against the previous expanded package,
only `motogp0910-launcher.exe` changed; the game EXE/map, pinned DLLs and external
resources are byte-identical. No README, SHA256SUMS or license sidecar is shipped.
Required notices remain embedded in launcher resource 104. Screenshots above
were refreshed from this compiled build. Native Windows user validation remains
pending.


## Footer, contextual help and mode-specific multiplayer fields

The outer window and artwork composition remain unchanged. PLAY is now a
**220 × 64** primary action at the lower right, with a larger 20-pixel font.
Save / Defaults / Logs / Report occupy the former status row; status and the
indicator dot are underneath those actions. Both rows stay outside scrolling.
General's folder controls move down by eight logical pixels; Verify game files
uses a balanced 184 × 32 button, with its explanation moved to hover help.
The achievement Search label has 54 pixels of width, centered vertically beside
a separately spaced edit field. A real font-extent check now covers this label,
including Turkish and all existing scaling cases.

The Audio paragraph is removed. Routine General, Graphics, Advanced and Controls
instructions, plus the normal achievement footer note, no longer consume page
space. Required help is attached to relevant controls using a shared native
`tooltips_class32` helper (`src/launcher/tooltips.h`), with localized main-window
text in `src/launcher/launcher_help.inc`. Tooltips use the Windows theme, appear
after 550 ms, wrap to a DPI-scaled width, and dismiss on page/scroll changes.
Text is owned for the lifetime of each registered tool and updated in place
on language changes. Relevant labels also carry help so disabled selectors can
explain themselves. Experimental FSR limitations, key capture, read-only file
verification and the F1/F7 shortcuts remain discoverable on hover. Actual save
errors and achievement retry warnings stay visible. DualSense retains its short
hardware requirement; detailed resistance guidance moves to tooltips without
changing the controller protocol or profile format.

The multiplayer editor displays only relevant fields and resizes to remove
unused rows: **640 × 158** client pixels for Offline, **640 × 316** for LAN or
Online at 100%. Player / room fields appear in both connected modes. LAN shows
only optional local / host IPv4 inputs; Online shows only the HTTPS server.
All fields and their values remain alive while hidden, and keyboard navigation
skips hidden disabled controls. Apply / Cancel, connection validation and settings
serialization retain their existing behavior. A mode change completes its
repaint before returning. Labels say **LAN**, with application-neutral local /
virtual network help; no specific virtual LAN product is named in the UI.

Current screenshots: [General](screenshots/launcher-racing-lines-general.png),
[achievements](screenshots/launcher-racing-lines-achievements.png),
[Audio](screenshots/launcher-racing-lines-audio.png),
[native hover help](screenshots/launcher-racing-lines-tooltip.png),
[Offline](screenshots/launcher-racing-lines-multiplayer-offline.png),
[LAN](screenshots/launcher-racing-lines-multiplayer.png) and
[Online](screenshots/launcher-racing-lines-multiplayer-online.png).


Validation: the final rebuilt Windows PE passed **86,327** actual UI assertions
under Wine/Xvfb. These include native mouse-hover appearance, correct tooltip
ownership, English/Turkish tooltip text, in-place localization, experimental
FSR help, larger right-aligned PLAY, status/action order, complete Search text,
all six pages and 75/100/125/150% scaling. The existing real data verifier,
achievement saves and original icons, partial-selector mouse regression,
scrolling and setting round trips remain covered. Multiplayer tests check
visibility, compact mode heights, visible-control bounds, mode-specific
validation and serialization, Apply / Cancel and the real General button.
120 painted tab changes took 2,632 ms total (72 ms slowest) without rebuilding
selectors or changing pending settings. Current screenshots were refreshed from
this exact probe; native popup capture includes the separate tooltip window.

Previous package: `downloads/MotoGP0910-Windows-x64-launcher-refined.zip`.
SHA-256: `92e21e3463b1e0734cd75df18024268dcd8978c3a140561d504ac149fb5b526e`.
ZIP integrity and membership checks passed. Only the launcher EXE differs from
the previous composition package: game EXE/map, runtime DLLs and external
resources are byte-identical. The archive contains no README, SHA256SUMS,
license sidecars, original game data, saves, logs or personal configuration.
Required notices remain embedded. Native Windows user review remains pending.

The exact packaged production EXE also passed the separate open, nested control
actions, English/Turkish switching, save and close checks. An unrelated TOML key
was retained and no game save was created. Both test fixtures were byte-compared
against their final build outputs.


## Windows tooltip registration startup hotfix

The user reported `Cannot attach launcher tooltip` in the refined package on
native Windows. The tooltip helper used `sizeof(TOOLINFOW)` (72 bytes on x64),
including the v6-only `lpReserved` tail, while the executable has no v6 common-
controls activation dependency. The default Windows v5 control can reject that
registration. Wine accepted the oversized structure, so normal hover tests did
not catch the Windows compatibility boundary.

The shared helper now uses the v2 layout through `lParam` (64 bytes on x64),
then retries the v1 layout through `lpszText` (56 bytes) if registration is
rejected. Both contain every field used by our hints. Each entry remembers the
accepted size for later localized text updates; native query tests also use
compatible layouts. This changes neither activation contexts nor native control
styles. No global common-controls v6 manifest is introduced.

Hover help is optional: failed window creation or individual tool registration
cannot raise a launcher action/startup error. A rejected entry is discarded so
a future update can retry, while other controls continue to register. Diagnostic
messages go only to the debugger. Settings, saves, the layout and game/runtime
binaries are unaffected.

Regression coverage places a strict compatibility subclass in front of the real
Win32 tooltip to reject the old oversized registration under Wine. It verifies
v2 success, localized updates without duplication, v1 fallback and its update
size, total registration rejection without exceptions, successful retry after
rejection and an unavailable tooltip window. The existing real mouse-hover test
and UI suite still run normally. This is an explicit compatibility simulation;
validation on the user's native Windows machine remains separate.

Final UI result: **86,339** actual Windows PE UI checks passed under Wine/Xvfb,
including the 12 added compatibility/failure assertions. 120 painted tab changes
took 2,388 ms total (56 ms slowest).

Current package: `downloads/MotoGP0910-Windows-x64-launcher-tooltip-fix.zip`.
SHA-256: `6a3d6e38a6273d6b887f4bb1d65b14e88bca35a104df1d9944f8c239bcde8326`.
ZIP integrity and membership checks passed; only `motogp0910-launcher.exe`
differs from the previous refined package. Existing users can replace just that
EXE in the original program folder. Other files and settings need no changes.
No README, SHA256SUMS, external licenses or personal/game data are shipped.

The separate exact production EXE also passed open, nested control actions,
language switching, save and close under Wine; unrelated configuration was
preserved and no game save was created. Native Windows confirmation is pending.
