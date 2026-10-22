# Turkish written localization — 16 October 2026

Choose **General → Game language → Türkçe** in the launcher, save, then launch.
Launcher language is independent. Extract the complete ZIP, including
`resources/localization/tr/overlay`. The game directory and saves stay in place.
Direct launch supports `--game_text_language=turkish`.

## Scope

- All 2,102 native message IDs reviewed: 1,697 translated; the other 405 are
  names, teams, motorcycle models, brands, classes, units, digits, or words
  already written identically in Turkish (for example Normal/Pastel).
- Native menus, HUD/race messages, career tutorials, sponsors/staff UI, riding
  advice, network/session errors, motorcycle setup and upgrade descriptions.
- Five localized Lua data files: display fields in bikes, tracks, upgrades,
  unlockables, and credits. Staff/song/company names remain names; production
  roles, music-credit prose and notices are translated. Data/physics fields,
  identifiers, paths, numeric settings and credits layout values stay intact.
- All 50 achievement names and descriptions, both in-game and in the launcher.
  Original IDs, score, icons and unlock records are retained.
- Native added main-menu tile reads **OYUNDAN ÇIK** with its accepted white
  layered accent. F1 settings, F7 achievements and exit confirmation follow
  Turkish game text. Their existing launcher-language behavior is retained for
  original game languages. Adaptive-trigger UI is otherwise unchanged.
- Existing voices/music/videos remain in use. This does not dub audio or
  repaint words embedded inside a video/logo/photograph. No separate subtitle
  files were present in the supplied dump; localized written tutorial/riding
  text comes from the native message inventory.

## Context review — 20 October 2026

The latest review corrects 97 message translations and five achievements.
Race-weekend wording, online racing terminology, slipstream/catch-up descriptions,
career staff/sponsor instructions, tyre/clutch explanations and riding advice now
use their racing or menu context. In particular, the two identical English
`Select an Empty Slot.` prompts have distinct staff/sponsor translations by ID.
Official circuit names such as TT Circuit Assen and Misano World Circuit are
preserved. Rider, team, motorcycle and company names remain names; historical
track names are not replaced with modern sponsored names.

The catalogs, generated message bank, affected display scripts, achievement
table and manifest are synchronized. The text generator retains verified font
manifest entries, so a prose-only revision does not require regenerating fonts.
Catalog/token/hash checks passed for all 2,102 messages and ten overlay files.
The independent Lua parser comparison passed 6,140 non-display checks, with
609 translated display fields. These are data checks; this revision was not
built or exercised in the running game. Native text wrapping remains to be
reviewed visually.

### Launcher language review

The launcher has a separate source-level Turkish review covering all seven
settings pages, help text, achievement filters/status messages, the DualSense
editor, ISO/title-update import, Steam integration and update notifications.
Literal translations such as “doğal PlayStation”, “oyunun şiddeti” and
“stok default.xex” are replaced with descriptions of the actual behavior.
The sharpening control also covers CAS, so its Turkish label is now
“Görüntü keskinliği”. The Online tab retains its English name in Turkish, as
requested; instructions referring to that tab use “Online” too.

Turkish UI retains the product term “Launcher”, including its language label,
version footer, tooltips and update messages. Other prose uses “günlük” and
“oyun güncellemesi”. Footer action buttons preserve the English horizontal
padding and spacing, growing with translated text using the rendered button
font. Layout is refreshed after language and DPI changes without shrinking text.
Actual filenames/folders (`default.xex`, `default.xexp`, `game`, `logs`), setting
keys, key-binding tokens and product names such as NOBK Online, Steam, DualSense,
PlayStation and AMD FSR stay unchanged. The update-file picker remains unrestricted
and localizes only its All files caption. The existing ISO probe expectations
match the revised Turkish labels. Source checks preserve the 25 binding keys,
defaults and English controller labels. No new application build, UI run or push
was performed; native text fitting and OS-owned dialog buttons remain separate
from this prose review.

## Native fonts

All four native faces now normalize **ÇçĞğİıÖöŞşÜü**: 48 glyph cells.
The original font's Ç/ç cells had different bodies and bearings from C/c;
retaining them made Turkish letters visibly smaller. Every Turkish body now
comes from its matching original ASCII letter, including its baseline and
advance. Native cedilla tails and umlaut dots are isolated from visible ink;
İ uses a compact native dot and ı retains the original i stem. Only the missing
breve is drawn, at the face's own scale.

The atlas is extended vertically. Original upper-half BC3 blocks remain
byte-exact, and unrelated glyph metrics preserve pixel geometry. New cells
reuse native body blocks at matching compression-block alignment wherever
possible, avoiding a second lossy encoding of the letter body. Top marks are
separated from the body and checked for clipping. Only Turkish selects these
fonts; original disk files are unchanged.

`check-turkish-fonts.py` passed **1,768** native-bank comparisons across all
four faces: body size, baseline, advance, line height, mark placement, dot
removal, original atlas blocks and unrelated glyph metrics. It also renders
Impact and small Arial word sheets directly from shipped BC3 atlases and native
metrics. These locally inspected font proofs are not gameplay screenshots and
are intentionally not tracked on main. Existing preview proofs are preserved. Regenerate them with `--preview-dir` below.

## Resource loading

The SDK has no Turkish Xbox language enum. Launcher value 1000 is UI-only;
the saved/native `user_language` remains 1, while `game_text_language` selects
Turkish text. The native language-to-filename routine therefore still selects
English voice paths and supported resources.

A read-only merged VFS device replaces one message bank, five script files and
four font banks, while all other entries use original game data. A single
physical-root alias selects that tree. Open directory handles retain the merged
entries, so absolute, case-insensitive and directory-relative native opens all
see the same translated file and its actual size. Original host entries are
fully discovered first, including beneath directory symlinks, so nontranslated
scripts, voices and music remain accessible. Original disk files are never
rewritten, and choosing another game language restores ordinary loading on the
next launch. There is no render-loop search or per-frame translation work.

### Fix for English menus with only OYUNDAN ÇIK translated

The previous file-level symbolic links passed a direct `ResolvePath`/`Entry::Open`
probe, but the game's `NtCreateFile` uses VFS `OpenFile`: it resolves the **parent**
and calls `GetChild`, bypassing those leaf links. A regression reproduced this
before the fix: direct reads returned Turkish while the actual native open path
returned the original English bank. Native OpenFile and directory-handle paths
are now tested explicitly, rather than treating a direct resource read as proof
that the title loaded it.

The nonfunctional **Xbox LIVE Marketplace** row is removed in **every language**:
the validated Options ID list terminates after Credits before native row
initialization. Its saved cursor clamps to Credits. The native list renderer
and navigation encounter five valid rows and the normal terminator.

## Rebuilding / validation

Catalogs under `resources/localization/tr/` are editable sources. The committed
overlay lets ordinary Windows builds run without extraction tools. To regenerate
it from the owner's separately decoded files:

```sh
python scripts/build-turkish-localization.py --decoded LOCAL_DECODED_DIRECTORY
python scripts/build-turkish-fonts.py --decoded-fonts LOCAL_DECODED_FONTS
python scripts/check-turkish-fonts.py --decoded-fonts LOCAL_DECODED_FONTS --preview-dir out/font-proof
python scripts/generate-turkish-achievements.py
python scripts/build-turkish-localization.py --check
texlua tools/localization_data_check.lua LOCAL_DECODED_SCRIPTS resources/localization/tr/overlay/scripts
```

The font generator needs Pillow >= 11.2 with DDS/BC3 encoding. The local research
TD/LZX extractor is not bundled or linked into the game. Original decoded banks,
source dumps, fonts and logs remain outside Git; selected derived font proofs are documented.

Windows PE checks run under Wine with the pinned actual SDK and patched title:

- The native message relocation and ID lookup read every translated message;
  the native UTF-8 decoder is checked against every written codepoint, including
  inline button glyphs. Native font parsing/lookup checks all four faces.
- Actual read-only VFS routing, filename case handling, incomplete-overlay
  rejection, independent voice enum, achievement metadata preservation, and
  native five-row Options wraparound are checked in `localization_probe`.
- The revised probe checks absolute/physical paths, three directory-handle
  scopes, every replacement through actual VFS `OpenFile`, and byte-exact
  fallback for previously unopened rider-script/music files. The complete
  symlinked game fixture passed **74,885** checks on Windows PE/Wine and Linux;
  most checks are per-codepoint/font-metric comparisons, not playthroughs.
- A separate Lua interpreter loaded the original/translated data in empty
  environments: **6,140** non-display values/keys matched; **610** display fields
  changed. All five files passed a real Lua parser.
- Launcher uses the sixth language choice and round-trips it through saved
  settings/CLI without changing launcher language or passing 1000 to the SDK.
- Existing quit, achievements, live settings, input and adaptive-trigger checks
  run against the matching Windows runtime.

`verify-windows.py` now includes `localization_probe`; pass a **complete** game
root containing XEX/XEXP and xenondata, rather than an XEX-only build fixture.

These checks are CPU/UI validation, not a native Windows GPU playthrough.
The corrected loader was also exercised in the real running title with the
Linux Vulkan software renderer: Press Start and the main menu showed Turkish,
and all **2,102 live relocated message strings** matched the Turkish catalog.
This additionally verifies the engine's actual loading/decoding, rather than
only the probe's manual relocation fixture. The small-font pixel defect found
in that visual check was corrected in the generator as described above.
Final glyph appearance, long-text wrapping, every career screen and captions
inside video playback still need native visual review. The supplied dump had
no separate subtitles to retime. Burned-in video/art text is not translated.

See the [Motegi rain log audit](motegi-rain-qa.md) for the current runtime checks and native Windows retest limits.
