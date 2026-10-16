# Turkish written localization — 16 October 2026

Choose **General → Game language → Türkçe** in the launcher, save, then launch.
Launcher language is independent. Extract the complete ZIP, including
`resources/localization/tr/overlay`. The game directory and saves stay in place.
Direct launch supports `--game_text_language=turkish`.

## Scope

- All 2,102 native message IDs reviewed: 1,700 translated; the other 402 are
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

## Native fonts

The original fonts cover codepoints 32–399 but have empty placeholders for
Ğ/ğ, İ/ı and Ş/ş. All four native faces now contain these six letters. Original
G/g/I/i/S/s letterforms and the existing dot/cedilla are reused; the missing
breve is composed at matching scale. The atlas is extended vertically. Existing
BC3 blocks are preserved byte for byte, normalized vertical metrics rescale,
and physical advances/bearings are retained. Only Turkish selects these fonts.

## Resource loading

The SDK has no Turkish Xbox language enum. Launcher value 1000 is UI-only;
the saved/native `user_language` remains 1, while `game_text_language` selects
Turkish text. The native language-to-filename routine therefore still selects
English voice paths and supported resources.

A read-only VFS device redirects ten resolved physical filenames to the overlay:
one message bank, five script files and four font banks. Physical file aliases
avoid the SDK's first-prefix symbolic-link ordering. Original disk files are
never rewritten, and choosing another game language restores ordinary loading
on the next launch. No render-loop string search or per-frame translation runs.

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
python scripts/generate-turkish-achievements.py
python scripts/build-turkish-localization.py --check
texlua tools/localization_data_check.lua LOCAL_DECODED_SCRIPTS resources/localization/tr/overlay/scripts
```

The font generator needs Pillow >= 11.2 with DDS/BC3 encoding. The local research
TD/LZX extractor is not bundled or linked into the game. Original decoded banks,
source dumps, fonts, logs and preview images remain outside Git.

Windows PE checks run under Wine with the pinned actual SDK and patched title:

- The native message relocation and ID lookup read every translated message;
  the native UTF-8 decoder is checked against every written codepoint, including
  inline button glyphs. Native font parsing/lookup checks all four faces.
- Actual read-only VFS routing, filename case handling, incomplete-overlay
  rejection, independent voice enum, achievement metadata preservation, and
  native five-row Options wraparound are checked in `localization_probe`.
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
Final glyph appearance, long-text wrapping, every career screen and captions
inside video playback still need native visual review. The supplied dump had
no separate subtitles to retime. Burned-in video/art text is not translated.
