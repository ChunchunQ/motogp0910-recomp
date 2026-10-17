# Project status — 18 October 2026


**Launcher Racing Lines:** User-selected concept B is implemented as a compact 940 × 578 launcher with horizontal tabs, scrollable settings and title buttons drawn over the artwork. General adds read-only, cancellable SHA-256 game-file verification. See [scope and validation](launcher-redesign.md).

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

The latest [launcher package](launcher-redesign.md) passed
all 27 Windows PE/Wine suite entries with its matching custom Release DLLs.
Advanced gates F3 behind an off-by-default checkbox; manual intro skipping uses
the native transition with Cross/A or Start, Space or Enter. Final checks include
76 debug/timing/layout checks, 12 owning-module GPU API checks, 127 native
quit/startup/input checks and 87,871 launcher UI assertions. All eight sliders
also pass 58 targeted real channel-click repaint checks. Extended crash
diagnostics is in Advanced; intro skipping no longer needs a checkbox. Native Windows
D3D12 overlay/gameplay and physical controller validation remain separate.

| Area | Implemented / checked | Native Windows evidence / limit |
|---|---|---|
| Game startup and races | Original XEX plus TU 0.0.1.1; startup/race callback repairs | User reports working gameplay |
| Qatar collision data | Serialized coordinate/endian repair; 192 regressions | User drove 4–5 Time Trial laps without falling through the track |
| PS3 / Xbox / keyboard prompts | Original supplied PS3 artwork, correct texture pitch and replay row; new dark/orange keyboard font | User accepted PS3/replay fixes; latest keyboard visual check pending |
| Career liveries | Scoped synchronous GPU readback, preserved flags and ABI | First career-race visual result remains unverified in available evidence |
| LAN/Radmin | Automatic discovery and IP fallback; session/socket integration | User verified two-PC racing; not proof of every disconnect/rejoin scenario |
| Community Online / VDS | Separate Offline/LAN/Online launcher modes; Linux/Windows Python service, HTTPS identities, authenticated UDP relay and native 378-view persistent leaderboards | Local Windows PEs exercised with Linux and Windows Python backends under Wine; public VDS/TLS/firewall and Internet race pending. Use Player Match; no official Xbox Live ranked/friends/voice. See community-online.md. |
| Community service security | MGO2 UDP endpoint proof, strict room transitions, pre-TLS admission, authenticated Caddy proxy, bounded SQLite/HTTP/UDP work; 46 server tests and 12 real HTTPS checks per Linux/Windows backend | Windows Python/clients exercised under Wine; deployed VDS OS/firewall/provider DDoS controls and Internet race remain unverified. See community-security.md. |
| Revised lobby admission | Retained selected discovery result, valid null leave slots; 67 kernel, 20 generated-wrapper checks and 3 two-process modes | New fix's native Radmin replay pending |
| Live settings | F1 MotoGP UI, 49 live controls, safe launcher persistence; 345 Windows/Wine checks and actual launcher child-exit reload | Native GPU/live gameplay result pending |
| Turkish text / native Options | 2,102 message IDs, five translated scripts, four extended native fonts, 50 achievements; normalized 48 Turkish glyphs across four banks; read-only VFS overlay; Marketplace row removed | Automated native CPU/UI checks passed; Windows GPU text/layout review pending. See turkish-localization.md. |
| Quit menu | Uppercase entry; white stripe and upper cap with brightness/alpha retained; separate boot/frontend guards; 109 checks | User confirmed Press Start Back now opens exit confirmation. User also accepted the white upper cap. See quit-menu.md. |
| Achievements | Launcher page + F7 list; 50 original icons, 1000 G, live read-only progress, search/filters and shared pop-up/sound preferences; 286 runtime checks and 60,132 actual launcher UI checks | EN/TR previews inspected. Native scrollbar now passes actual mouse-drag input; title bar has only minimize/close. Native game GPU/listening check pending |
| DualSense adaptive triggers | Downpour SDL approach researched first; nine shared launcher/F1 parameters, native PS5/Edge targeting, explicit reset on disable/focus/shutdown; 200 software/transport checks | Experimental and default off. User owns 8BitDo; physical DualSense USB/Bluetooth resistance remains untested. See adaptive-triggers.md. |
| FSR 2/3 | Experimental AMD providers; neutral depth/motion and per-frame history reset | Proper temporal reconstruction and scene render-cost reduction absent; Windows GPU dispatch unverified |
| Ultrawide | Experimental horizontal-plus projection and centered HUD | Full Windows camera/cutscene/culling coverage unverified |
| Performance | Reduced avoidable queries/logging/locking; synthetic path checks | No measured native game FPS improvement |
| Runtime safety / crash reports | TLS, thread IDs, owned paths, dates; external native exception monitor | Dumps help diagnose complete exits; lobby returns are a different failure category |

## Current QA and documentation update — 17 October 2026

The [wet Motegi log audit](motegi-rain-qa.md) addresses all recorded error
categories and the repeated null-descriptor diagnostic. Final checks passed:
1,768 font geometry/block comparisons, 74,885 native localization checks,
2,146 real kernel/VFS checks and 80 exact SDK texture-parser checks; all 25
Windows PE/Wine suite entries passed. A new native Windows rainy race remains
required. Experimental FSR 2/3 warnings are retained.

Repository Markdown prose is now English. Exact translated UI labels/glyph
samples remain localized where useful. The obsolete cancelled Claude handoff
was removed; source licenses, required assets and historical ZIPs are retained.
See [repository instructions](../AGENTS.md). The current package is linked
from the [README](../README.md).

## Historical controls / UI update — 15 October 2026

- Native Control Settings follows Xbox / original PS3 DualShock 3 / Keyboard.
  The supplied PS3 bank already contained the diagram; no new upload needed.
- User chose a dark/orange keyboard **key list**, not a full keyboard image.
  Real keybind alternatives and native action names follow live bindings and
  Default / Alternative A / B. No gameplay mappings are rewritten.
- Settings shortcut is **F1**; old F4 migrates while a different custom bind
  is retained. Alt+F4 still closes the window normally.
- Previous scrollbar issue now has a real OS mouse-drag regression; the launcher
  caption visibly contains only working minimize/close buttons.
- Press Start now reads boot application phase **828DA060** separately from the
  frontend property. The user confirmed the corrected Press Start Back/B exit.
  That input path is preserved in this update.
- User chose **white** for the QUIT GAME accent; the seventh tile uses an isolated
  white/gray texture view preserving native alpha, geometry and upper-cap
  brightness layers. User accepted the final white cap.

Screenshots and raw game banks stay out of Git. Read-only achievement progress,
LAN/Radmin, original button artwork and the cancelled physics investigation's
revert are preserved.

## Work sequence

1. Keyboard prompt redesign delivered separately; visual review pending.
2. Achievement UI, notification and quiet sound completed; Windows ZIP ready.
3. Quit menus follow-up implemented in the launcher achievement update: uppercase
   `QUIT GAME`, corrected live frontend hook for Press Start Back/B; [98 targeted checks](quit-menu.md).
4. In-game live settings completed: only immediate controls appear; no restart-only section. [Scope and 345 targeted checks](in-game-settings.md).
5. Launcher achievement page completed, retaining the existing MotoGP design.
   [Read-only save paths, original catalog and shared notification options](achievements.md).
6. Native controls diagram and F1 / previous UI follow-up completed; native visual review pending.
7. DualSense adaptive triggers and remaining white cap follow-up implemented;
   [source research, settings and hardware verification boundary](adaptive-triggers.md).
8. Turkish written localization and Marketplace removal completed; native visual review pending. [Scope and validation](turkish-localization.md).
9. Community Online service and native leaderboards implemented at the user's new priority. Public VDS deployment and Internet playtest require the future server endpoint. LAN remains available.
10. Launcher redesign and Controls interaction refinements delivered; current design and acceptance notes are in launcher-redesign.md.

The FPS/physics investigation was explicitly cancelled and reverted. Don't
resume speculative simulation or handling changes. Frame generation is excluded
by the user's instruction. Keep experimental FSR choices available with their
limitations; don't present compatibility dispatch as full temporal support.

## Source map

- User observations and current order: ongoing conversation.
- [Roadmap](roadmap.md), [launcher requests](launcher-next-steps.md).
- [LAN findings](lan-lobby-stability.md), [crash reporting](crash-reports.md).
- [Community server, modes, deployment and original statistics ABI](community-online.md).
- [Community service security review, fixed vulnerabilities and limits](community-security.md).
- [Runtime safety / validation limits](stability-quality-control.md).
- [Experimental rendering](experimental-rendering.md).
- [Career readback](career-textures.md), [collision repair](collision-data.md).
- [Keyboard prompts](keyboard-prompts.md), [achievement UI](achievements.md), [live settings](in-game-settings.md).
- Build provenance: SDK pin, runtime manifest, Git commit, per-ZIP SHA-256.
