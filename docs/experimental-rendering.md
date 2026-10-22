# Experimental FSR and horizontal-plus ultrawide

Current sources use the exact nightly pin in `sdk-version.json` and matching
custom runtime manifest. The older stable v0.10.0 milestone is preserved as a
separate historical rollback package; see [migration](sdk-nightly-migration.md). Neither the SDK nor the
original game assets are silently updated by selecting these settings.

## FSR 2 and FSR 3

Graphics offers Bilinear, CAS, FSR 1, FSR 2 and FSR 3 with six FSR quality
modes. FSR 2/3 remain experimental even though the current selector captions
omit the Experimental suffix; their help retains the limitations. The Windows D3D12
presenter uses the AMD FidelityFX API and the
providers in third_party/fidelityfx. Provider enumeration against the actual
shipped DLL identifies FSR 2.3.3 and FSR 3.1.4 as distinct implementations.

The [Downpour SDK source](https://github.com/LittleBitUA/rexglue-sdk-dpour/tree/5497095131e8c6174ab698c5ea1ee4a60c0e395f)
uses a compatibility path with synthesized inputs and frame resets. MotoGP's
adaptation replaces color-as-depth/motion with typed R32_FLOAT far-depth and
R16G16_FLOAT zero-motion textures, explicitly selects the provider, restores
D3D12 resource states and descriptor heaps, and uses one shared RCAS pass.
Context destruction waits for outstanding GPU work. Failure falls back to
spatial upscaling and is logged with FSR_EXPERIMENTAL.

This is **not full temporal reconstruction**: the game does not export correct
motion vectors, depth or jitter for this path. History resets each frame.
Quality modes resample the already rendered image; they do not reduce the
native scene's GPU workload. Auto at 1:1 can use CAS without dispatching an
upscaler. Use Native AA to request the provider at 1:1. Frame generation is
not included. Linux remains on the spatial compatibility path. FSR 1 is the
recommended fallback if the experimental path looks worse or fails.

## Ultrawide

Graphics offers Auto, 16:9, 21:9 and 32:9. Auto retains the native camera for
narrow windows and selects the actual aspect for wider windows up to 4:1.
21:9 and 32:9 remain marked Experimental pending broad gameplay testing.
The game keeps its native 1280x720 video mode and existing internal scale.
The presenter displays it at the selected aspect. The native perspective
setter at 0x821E2108, called by 0x821CCF50, adjusts only the shared projection's
horizontal axis by (16/9)/aspect. The vertical axis and depth mapping stay
unchanged, so wider modes reveal more world rather than cropping or stretching
16:9 geometry. State-restoration calls are excluded to prevent double scaling.

UI sprites/glyphs passing through the native quad batch keep their aspect in
the central 16:9 area. Original vertices are restored after submission,
including on exceptions. Pre-rendered movies and UI paths outside that batch
may need further title-specific adjustments. This does not claim every camera,
cutscene or culling edge case has been visually verified.

## Validation

- Actual Windows/Wine provider enumeration: separate FSR 2/3 provider IDs.
- 51,681 production launcher layout/settings checks: English/Turkish, five
  pages, 75/100/125/150% scale, FSR quality/aspect persistence and child args.
- 637 native engine/projection/HUD checks on Linux and Windows/Wine: existing
  effect flags, native matrix copying, horizontal coverage, vertical/depth
  preservation, original input preservation, state-restoration exclusion, and actual native HUD batching with aliased input vertices.
- 222 native controller artwork/draw checks on Linux.
- 91 launcher configuration and child-process checks on Windows/Wine.

CPU/provider tests do not establish successful Windows GPU shader execution or
race stability. Linux graphical startup results are recorded separately in
launcher-validation.md. No new rendering QA captures are transferred; historical preview screenshots
remain unchanged.

The actual Windows game GPU smoke test stopped at adapter initialization: the
cloud Wine environment exposes no Direct3D 12 adapter with feature level 11_0.
It therefore cannot verify FidelityFX context creation or GPU dispatch here.
Linux X11 startup uses an independently rebuilt, matching runtime.

183 actual runtime paint-planning/input/audio/video-mode checks passed on
Windows/Wine, including all six FSR quality modes and wide presentation ratios.
Linux full-game startup reached Main Menu at 1680x720 / 21:9 and remained
alive for over four minutes. The previously recorded lavapipe white menu-ribbon
artifact remains; this startup does not verify races or all wide cameras.
