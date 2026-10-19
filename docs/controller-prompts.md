# In-game controller prompts

Registered prompts switch between Xbox, PlayStation and saved keyboard
bindings. PlayStation prompts now use the original PS3 controller textures
supplied by the user, replacing the previous drawn symbols.

Keyboard prompts now use dark rounded keycaps, orange MotoGP accents and
Open Sans Semibold lettering. [Design, coverage and validation](keyboard-prompts.md).

## Installation and use

Extract the complete Windows ZIP. Replace the EXEs and both runtime DLLs,
and keep `resources/controller/ps3-prompts.bin` in its supplied subdirectory.
Also keep `resources/controller/ps3-controls.bin` for the Controls screen.
Keep your existing `game/`, settings and saves. The launcher and Qatar
collision repair are retained. The new package also includes the scoped career texture readback repair;
see [career texture notes](career-textures.md).

Automatic switching follows meaningful input, not idle connection or minor
stick drift. Use SDL for native Sony detection. XInput wrappers present an
Xbox controller; test PS artwork with an Xbox pad using:

```powershell
.\motogp0910.exe --input_backend=xinput --button_prompt_style=playstation
```

`--button_prompt_style=xbox`, `keyboard` and `auto` select the other modes.
These switches change pictures, not controller mappings.

## Native Control Settings screen

The full controller diagram now follows the existing automatic/manual prompt
style: Xbox retains the game's own Xbox 360 image; PlayStation uses the original
DualShock 3 diagram from entry **251**, hash **14D50C26**, in the user's supplied
PS3 `sharedfrontend.mgb`. The new `ps3-controls.bin` contains its unchanged
1024x512 BC3 blocks. The adjacent JSON records source-bank, payload and pack
SHA-256 hashes. It is the PS3 game's own diagram, including its leader lines.
No replacement controller illustration was downloaded or generated.

The DS3's left stick and D-pad occupy different positions. The temporary native
left-label rows are reordered **0,1,2,5,6,7,8,3,4**, matching the PS3 lines. The
profile's mappings are never rewritten. Default / Alternative A / Alternative B
continue to supply the actual native action names dynamically; those names
retain the selected game language. The image swaps in the original native UI
batch with restored UVs and preserves the existing camera/projection/fades.

For Keyboard style, the diagram becomes two columns of dark keycaps with orange
MotoGP accents. The original localized action text stays in place. Keycaps show
current keybind alternatives, unbound controls and both directions of each
stick axis; changing an F1/launcher binding updates the list. Changing the native
control preset changes the corresponding action labels immediately. Opaque
backgrounds are kept away from those labels because the native renderer submits
text before the diagram. Stable keycap geometry is cached; live changes rebuild
it. No new keyboard gameplay mapping or controller behavior is introduced.

Auto follows meaningful input as before; use SDL for native Sony identification.
A Sony controller hidden behind an XInput virtual Xbox device still needs manual
PlayStation prompt selection. Manual Xbox / PlayStation / Keyboard selections
also apply to this diagram.

`extract-ps3-prompts.py PS3_SHARED_FRONTEND_MGB` now reproducibly extracts both
small button artwork and the full diagram. Keep original banks outside Git.
The supplied `ps3-ui-assets-v2.zip` already contained the needed bank; no new
PS3 dump or extraction was required from the user.

Windows/Wine validation: **259 native button/diagram checks**, including original
GPU-visible pixels/pitch/UVs, ABI and vertex restoration, all three preset label
sets, live keybind alternatives/unbound state, native keyboard batching and
protection of localized label regions. **171 production window/input checks**
exercise real SDL virtual controllers and input switching. These are CPU/UI
checks; the final 3D Controls screen still needs a native Windows visual check.

## Artwork coverage

All 42 controller texture entries identified in the supplied PS3
`sharedfrontend.mgb` are included in the small resource pack. Their BC3 blocks
are unchanged; the JSON catalog records names, dimensions and hashes.
Xbox prompts retain the Xbox game's own artwork.

PlayStation replacements cover the 21 registered glyphs: cross, circle,
square, triangle, L1/R1, L2/R2, SELECT, left/right sticks, left-stick directions,
and D-pad with all four directions. Duplicate source entries are retained.
The LT/RT/LB/RB/B row in Second Chance and replay also changes style. Its atlas
UVs are reset to the full replacement image and restored after drawing.
The v3 build corrects the swapped rows reported in v2: the upper controller
row is replaced, while the lower rewind/fast-forward/camera/stop row is
retained. All eight full-replay button sprites (LB/LT/RB/RT/A/B/X/Y) are
supported as well. Their identities were checked against the decoded
`ingame.mgb` artwork, not inferred from sprite indices.

No separate START, PS/Guide or text-labelled L3/R3 bitmap was found in this
identified set. Stick prompts use the original stick pictures. Literal PRESS
START text stays unchanged.

Loading-tip icons use a separate immediate quad path (`0x82388DA8`, called
from `0x822BAD6C`) that bypasses the ordinary prompt hook. This path now uses
the same registered texture-to-glyph mapping and selected prompt style. The
game still resolves each tip token through its active control preset; keyboard
labels then read the saved binding for that resolved button. Each keyboard
icon's width is adjusted before the next text segment is positioned, and the
original width property is restored when the tip finishes rendering. Loading
also refreshes automatic input style without waiting for a gameplay frame.
This follow-up change is included in the compiled UI/log update. No runtime
tests were run for that build, at the user's request.

## Rendering

The pack is uploaded to guest physical memory as linear 64×64 BC3 textures.
Source rows contain 16 BC3 blocks (256 bytes); the v0.10.0 GPU loader aligns
rows to 32 blocks (512 bytes), including linear base textures. The upload
now pads each row to 512 bytes and advertises a 128-texel pitch. This repairs
the corrupted symbols reported with the first PS3 artwork build. The native
probe compares all 16 GPU-visible rows for each of the 21 glyphs to the source
pack and checks zero padding, rather than testing descriptors alone.
PlayStation pictures use the native textured quad path, retaining position,
color, alpha and layering. Keyboard caps use saved bindings and native solid
quads. Guest vertices and PPC state are restored after drawing. Original game
archives are not modified.

| Hook | Purpose |
| --- | --- |
| `0x82388C38` | Registered glyph and selected replay quad replacement |
| `0x822BA980`, `0x822BA6C0`, `0x82388DA8` | Loading-tip input style, keyboard spacing and immediate icon replacement |
| `0x8227E440` | Identify verified replay controller sprite calls |
| `0x82358448` | Keyboard cap spacing in prompt bars |
| `0x82284E98`, `0x82284E88`, `0x82284DB0` | Inline glyph recognition and sizing |
| `0x823D1008` | Per-frame style and Windows XInput observation |

The supplied ReXGlue v0.10.0 runtime pair exposes the existing shared SDL
function table, keeping input observation in the same SDL instance as the
backend. The SDK/code generator version is unchanged.

## Build checks

The current Windows executables passed 222 native draw/pixel-row/texture descriptor,
replay crop and PPC ABI checks, plus 92 binding and real SDL virtual-controller
checks under Wine. The native draw probe also passed on Linux. It reads the
actual patched XEX instructions to check all four replay drawing loops:
5-button and 8-button upper rows, plus their 5-icon and 8-icon lower rows.
This catches the swapped-row defect that the earlier synthetic fixture missed. These checks do not validate gameplay visuals on a Windows GPU or a physical Sony pad.
The current cloud window probe could not open its X display; its earlier
171-check result is not counted as a new-build verification. Additional game
testing was stopped at the user's request; the user will test locally.

Reproduce the pack from the supplied bank:

```sh
python3 scripts/extract-ps3-prompts.py path/to/sharedfrontend.mgb
```

Optional targets: `button_prompt_probe`, `button_prompt_input_probe`,
`button_prompt_window_probe`, `button_prompt_gpu_probe`. The GPU diagnostic
is excluded from the Windows download. No screenshots are included in this
change.
