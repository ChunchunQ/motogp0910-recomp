# Keyboard prompt refresh

Selected style: **dark keycaps with orange MotoGP accents**. This implementation
originated in the 14 October 2026 UI milestone. Current delivered features and
remaining acceptance work are listed in [project status](project-status.md).

The original thin rectangular outline and ASCII stroke font are replaced by
rounded dark keycaps, a subtle rim/shadow, a compact orange top accent and
Open Sans Semibold lettering. Font coverage is rasterized into native solid
UI geometry. Adjacent identical coverage spans are merged vertically to avoid
one draw per pixel. No new texture loader, external overlay or platform font
installation is required. The generated font is embedded in the executable.

The original game's clipping, draw order, position, render scale and opacity
remain in use. Labels still read the user's saved key bindings, including
modifier combinations and punctuation. Proportional font advances determine
prompt widths. Supported keyboard replacements include the existing inline
glyphs, prompt bars, tutorial prompts and both replay/Second Chance controller
rows. Xbox and original PS3 artwork paths are unchanged. The lower replay
media controls keep the original pictures.

## Validation

The Windows game and optional probes compiled against the pinned SDK. **237
actual native UI checks passed under Wine**, including original PS3 texture
rows, replay-controller/media separation, source-vertex restoration, native
calling conventions, custom modifier/punctuation bindings and transparent,
partially faded and opaque keyboard caps. **92 binding and real SDL virtual
controller checks passed**, covering keyboard/Sony/Xbox transitions.

A before/after software preview was generated directly from the production
geometry exporter, `tools/keyboard_prompt_preview.cpp`. It is a design preview,
not a captured Windows game frame; no screenshots are committed. Native
Windows gameplay visuals and physical controller behavior remain user checks.

## Reproducibility

The Apache-2.0 source font, notice and license are in `third_party/open-sans/`.
The Windows launcher embeds its license/notice; no separate license folder is
packaged. The renderer itself needs no extra font files. To regenerate the compiled coverage data:

```sh
python3 scripts/generate-keyboard-font.py
```

This requires Pillow only for regeneration, not for building or running the
game. The generated header records the source font SHA-256. For a software
geometry preview, compile/run `tools/keyboard_prompt_preview.cpp` with a
C++20 compiler; its output contains the actual keycap polygons and colors.
