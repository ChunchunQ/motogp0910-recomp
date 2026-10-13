# In-game controller prompts

The game now switches its registered controller glyphs between Xbox,
PlayStation and the actual configured keyboard bindings. No replacement
textures, external icon packs or changes to original game files are needed.

## Usage

Replace the game EXE and **both** runtime DLLs with the matching package.
The default `button_prompt_style=auto` follows meaningful input. A connected
pad or minor stick drift does not steal keyboard prompts. Sony devices are
recognized through SDL gamepad type or vendor 0x054c; Xbox/other mapped pads
use Xbox artwork. Keyboard switching respects `mnk_mode` and window focus.

To test PlayStation graphics with an XInput controller in PowerShell:

```powershell
.\motogp0910.exe --input_backend=xinput --button_prompt_style=playstation
```

A → cross, B → circle, X → square, Y → triangle. Shoulder and trigger
prompts become L1/R1 and L2/R2; Back/View becomes the Create/Share symbol.
Generic stick and directional artwork is retained for PlayStation.

Force `--button_prompt_style=keyboard` or `xbox` for comparison;
`--button_prompt_style=auto` restores automatic switching. These switches
change prompt visuals, not controller button mappings. Use SDL for native
Sony recognition: an XInput wrapper presents an Xbox controller and cannot
identify the physical Sony hardware. Testing forced PS graphics with an
Xbox pad does not validate physical DualSense detection.

Keyboard caps use saved keybind settings, including modifiers, alternatives,
unbound actions and compact stick/directional groups. Supported registered
glyphs are replaced in prompt bars and inline font icons. Literal game text
such as PRESS START is not a registered glyph and is retained.

## Native rendering

The 21 registered glyphs were identified by drawing their IDs in the same
native frame as the artwork, using texture IDs at `0x8292CC60` and texture
handles through `0x8292CCB8`. Case distinguishes shoulder and trigger glyphs.
D-pad IDs 1/3/4/2 mean Up/Down/Left/Right; S means Back, not Start.

Strong hooks preserve the original generated functions:

| Address | Role |
| --- | --- |
| `0x82388C38` | Replace recognized native UI quads; forward other art |
| `0x82358448` | Reserve keyboard cap width in prompt bars |
| `0x82284E98`, `0x82284E88` | Recognize and measure inline glyphs |
| `0x82284DB0` | Expand inline glyphs around their native center |
| `0x823D1008` | Latch style per frame and observe Windows XInput |

Vectors and ASCII strokes go through the game's original solid-quad path,
preserving native positions, projection, opacity, draw order and upscaling.
Original vertices and PPC call state are restored after replacement.
This is not a screen-space legend or a post-presentation overlay.

On Windows, SDL is statically linked inside the runtime. The supplied
v0.10.0 runtime exports `Motogp_GetSharedSDL`, a versioned function table
that lets the observer use the **same** SDL instance as the game's input
backend. An old runtime DLL cannot supply this table. The SDK version,
code generator and existing C++ plugin ABI remain unchanged.

## Validation

- Linux: 44 real-XEX native draw/vertex/ABI checks and 43 binding/SDL virtual
  controller checks passed.
- Windows PE under Wine: 44 native draw and 45 input/binding checks passed.
  The input probe also verifies that SDL function
  pointers belong to rexruntime.dll. Virtual Sony/Xbox devices exercise
  real event watches, drift rejection, keyboard priority, focus and removal.
- Existing launcher/runtime/game CPU probes passed: 58 / 140 / 50 checks.

These checks are not a Windows GPU gameplay test. Physical DualSense
recognition and menu/race prompt appearance are to be tested locally.

Build the optional probes with CMake targets `button_prompt_probe` and
`button_prompt_input_probe`; pass the prepared 0.0.1.1 XEX directory to the
former. The latter creates virtual controllers and needs no physical pad.
