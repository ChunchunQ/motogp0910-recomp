# Turkish fonts and Motegi rain log QA — 17 October 2026

The supplied three-lap wet Twin Ring Motegi session log contains 78,630 lines:
703 errors, 77,844 warnings and 75 informational entries. It ends with normal
window shutdown and title termination, with no recorded fatal exception or
GPU device loss. This is an audit of that supplied log, not a replay of the race.
The original log and personal paths are excluded from the repository/package.

| Recorded category | Count | Change / disposition |
| --- | ---: | --- |
| XCTD compression query reported as unimplemented | 702 errors | Extracted host-directory files have no Xbox filesystem compression metadata. Preserve the negative `INVALID_PARAMETER` capability result, initialize output, retain position and zero returned bytes. Unsupported non-host backends retain an error. |
| Missing `\\Device` parent while opening image volume | 1 error, 2 warnings | Register `\\Device\\Image` to the actual active game tree and resolve exact registered volume roots before parent traversal. No fabricated directory or file. |
| Type-zero texture fetch descriptor | 77,836 warnings | All 38 unique captured descriptors are unbound, some with stale payload words. Keep the existing null texture key; static shader usage includes conditionally unused samplers. Remove the incorrect per-draw warning. Other malformed types retain warnings; no invalid-fetch bypass flag is enabled. |
| Stub filesystem device information | 2 warnings | Return real host-backed and virtual raw-cache disk / mounted / read-only characteristics; unsupported backends retain a diagnostic. |
| Stub volume dismount | 2 warnings | Validate the file handle; host-directory and SDK null-cache mounts have no guest filesystem driver/cache to dismount. Keep live host mounts and handles intact; reject invalid handles and unsupported backends. |
| Optional `ShaderDumpxe:\\CompareBackEnds` developer path absent | 1 warning | Keep the absent result, consistent with the SDK's existing optional-path policy; ordinary missing device paths still report errors. |
| Experimental FSR 2/3 | 1 warning | **Retained.** Synthesized depth/motion and potential spatial fallback remain actual limitations. This build does not make temporal FSR production-ready. |

These corrections address every error category recorded in the supplied log.
They do not certify that a new Windows race has zero errors. The texture change
preserves previous rendering behavior; it is not evidence that a rain-specific
visual defect was reproduced or fixed.

## Font correction

All four native font banks regenerate the twelve Turkish letter cells
ÇçĞğİıÖöŞşÜü from matching original ASCII bodies, preserving native advances
and baselines. The smaller stock Ç/ç cells are replaced. İ uses a compact native
dot, ı retains its full stem, and all diacritics/cedillas are checked for spacing
and clipping. Unrelated font blocks and physical metrics are preserved.
See [font details and native atlas proofs](turkish-localization.md#native-fonts).

## Reproducible checks

- `build-turkish-localization.py --check`: 2,102 message IDs and ten overlay files.
- `check-turkish-fonts.py --decoded-fonts OWNER_DECODED_FONTS`: 1,768 checks of
  native geometry, mark placement, unchanged data and actual BC3 pixel bodies.
- `localization_probe COMPLETE_GAME_ROOT resources/localization/tr/overlay`:
  74,885 checks of native text/UTF-8/font parsing, real VFS loading, fallbacks,
  achievements and Options navigation. Windows PE/Wine and Linux passed.
- `file_compat_probe COMPLETE_GAME_ROOT`: **2,146 checks**, including 702 repeated real kernel compression
  queries plus real volume-root opens, host and all three null-cache volume metadata, rejected buffers/handles,
  preserved position/data and valid/invalid dismount checks. Final Windows
  probe also checks the absent optional shader directory without diagnostics.
- `texture_binding_probe`: 80 checks using the pinned SDK's exact CPU parser
  for all 38 captured descriptor patterns, valid bind, unbind and rebind. No
  fake GPU backend and no inferred rendering result.
- `scripts/verify-windows.py --build-dir out/build/windows-nightly-release
  --game-root OWNER_COMPLETE_GAME_ROOT`: all 25 Windows PE/Wine checks passed,
  including localization, file compatibility, stability, input, presentation,
  launcher layout, achievements, quit navigation, live settings and triggers.

The optional texture parser target requires
`-DMOTOGP_SDK_VALIDATION_SOURCE=/path/to/pinned/patched/sdk`; see the
[runtime build notes](../third_party/rexglue-runtime/readme.md).
Original game data is separately owned and is not distributed.

Wine's host service setup messages are harness limitations, distinct from
in-game log entries. CPU/UI probes do not exercise Windows D3D12 race rendering,
physical controller transport or all career states. Final acceptance requires
extracting the entire new package and repeating the same three-lap rainy
Motegi run on native Windows, then inspecting the newly generated log.

An additional real-title Linux software-Vulkan check displayed the updated
Turkish Press Start text and first-run menu. This run also showed placeholder
presentation/artifact limitations in the software renderer; it is not visual
sign-off for native Windows D3D12. It began before the last null-cache metadata
adjustment; final host/null-cache behavior was subsequently checked through
both Linux and Windows probes against the rebuilt runtime.
