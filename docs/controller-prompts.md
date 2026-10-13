# In-game controller prompts

Registered prompts switch between Xbox, PlayStation and saved keyboard
bindings. PlayStation prompts now use the original PS3 controller textures
supplied by the user, replacing the previous drawn symbols.

## Installation and use

Extract the complete Windows ZIP. Replace the EXEs and both runtime DLLs,
and keep `resources/controller/ps3-prompts.bin` in its supplied subdirectory.
Keep your existing `game/`, settings and saves. The launcher and Qatar
collision repair are retained. The career rider/bike texture investigation
is paused and is **not** included in this build.

Automatic switching follows meaningful input, not idle connection or minor
stick drift. Use SDL for native Sony detection. XInput wrappers present an
Xbox controller; test PS artwork with an Xbox pad using:

```powershell
.\motogp0910.exe --input_backend=xinput --button_prompt_style=playstation
```

`--button_prompt_style=xbox`, `keyboard` and `auto` select the other modes.
These switches change pictures, not controller mappings.

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
Timeline and camera/media icons are left intact.

No separate START, PS/Guide or text-labelled L3/R3 bitmap was found in this
identified set. Stick prompts use the original stick pictures. Literal PRESS
START text stays unchanged. Additional full-replay controls outside the five
verified sprite IDs remain unconverted pending identification.

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
| `0x8227E440` | Identify verified replay controller sprite calls |
| `0x82358448` | Keyboard cap spacing in prompt bars |
| `0x82284E98`, `0x82284E88`, `0x82284DB0` | Inline glyph recognition and sizing |
| `0x823D1008` | Per-frame style and Windows XInput observation |

The supplied ReXGlue v0.10.0 runtime pair exposes the existing shared SDL
function table, keeping input observation in the same SDL instance as the
backend. The SDK/code generator version is unchanged.

## Build checks

The current Windows executables passed 143 native draw/pixel-row/texture descriptor,
replay crop and PPC ABI checks, plus 92 binding and real SDL virtual-controller
checks under Wine. The native draw probe also passed on Linux. These checks
do not validate gameplay visuals on a Windows GPU or a physical Sony pad.
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
