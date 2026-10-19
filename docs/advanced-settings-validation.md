# Advanced graphics settings validation

The update adds six native controls, preserves the selected display mode when
choosing 1.5x, and provides concise English/Turkish help for all ten Advanced
settings. All new controls default to `game`. Original assets, tweak files,
saves and user configurations are excluded from the development ZIP.

## Executed checks

| Probe | Result | Scope |
| --- | --- | --- |
| `engine_advanced_probe` | 240 passed | Production wrappers and actual recompiled title consumers described below |
| `engine_settings_probe` | 661 passed | Native camera parameters, scope restoration, declared property bounds, defaults and PPC ABI |
| `launcher_layout_probe` | 78,241 passed | Real Win32 UI, both languages, four scales, 58 dropdowns, five Advanced rows, all choice values, help, persistence and forwarding |
| `in_game_settings_probe` | 392 passed | Live CVar updates, supported choices, presenter state, JSON persistence, input and F1 drawing |
| `launcher_probe` | 112 passed | Saved configuration, typed SDK flags, arguments and real child/public-entry launch |
| `launcher_layout_probe --graphics-only` | 93 passed | FSR versions, real JSON round trips, 1.5x preservation of default Borderless, labels and defaults |

These are Windows PE tests executed under Wine with the pinned custom runtime,
the supplied original patched game in tool mode and an isolated Xvfb display.
No runtime ABI or provider upgrade is included. Each test writes into its own
fixture directory, preserving the user's files.

## Native consumer evidence

`engine_advanced_probe` links the production `engine_settings.cpp` and the real
generated title functions. The settings consumers themselves are not replaced.
Test-only observers bypass or stop named unrelated graphics, lighting or
allocation boundaries so the CPU fixtures can execute without a D3D12 adapter.

| Setting | Observed behavior | Boundary / limitation |
| --- | --- | --- |
| Vignette | `0x8237FA00` publishes the selected alpha into native FSFX output; repeated frames restore the camera value | Its leading graphics matrix/setup call is bypassed |
| Motion blur + vignette | All vignette strengths retain their selected alpha with blur default or Off; blur Off publishes zero blur alpha and velocity; an inactive camera stage stays inactive | Parameter publication, not shader pixels |
| Bloom | `0x82385428` publishes the scaled interpolated intensity; its source value restores exactly on return/unwind | Stop before unrelated per-material lighting updates |
| Particles | `0x82274570` computes and stores the expected count from a known template at each selected density | Stop before timing/allocation; no particles are rendered |
| Draw distance | `0x823908C0` passes the selected, bounded far-clip scale to frustum construction | Observe the argument before unrelated camera matrix construction |
| Soft shadows | `0x8238CFD8` selects soft or hard rendering according to the actual native distance comparison | GPU setup, draw and teardown are observation boundaries |
| Shadow filtering | The real pipeline enters GPU setup with the selected native filter byte; original neighboring bytes and flag restore | GPU setup is stopped; filtered shadow pixels are unverified |

The blur/vignette interaction was corrected during this validation: clearing
the shared speed-FX enable byte also disabled vignette. Blur Off now scopes only
the blur alpha and velocity while preserving the original camera enable state.

The ten settings and their labels fit five aligned rows at 75–150% scales.
In that validated build, Debug/crash diagnostics followed them vertically and
required scrolling. A subsequent layout change places both checkboxes side by
side below the last row; that layout change has not been runtime-tested.

The final ZIP's public root launcher also passed a separate smoke check: real
mouse/keyboard selections for all six new settings were saved to root
`config.json`, an unrelated JSON entry was preserved, and changing tabs and
selecting 1.5x retained both Borderless and the Advanced values. The public
process exited cleanly, with no second config or empty logs directory.

## Reproduction

Source `scripts/cloud-env.sh` in the prepared cloud workspace. Build the
`engine_advanced_probe`, `engine_settings_probe`, `launcher_layout_probe`,
`launcher_probe` and `in_game_settings_probe` targets in
`out/build/windows-nightly-release`, then use `scripts/verify-windows.py` for the
full suite or run the named probes in isolated executable folders. The native
consumer probe takes the extracted patched game root as its only argument.
The UI probe takes the game root and a screenshot directory; `--graphics-only`
selects the short graphics regression group. Probes remain excluded from release
builds and packages.

## Remaining limits

This environment has no usable native Windows D3D12 adapter. These checks do not
establish correct shader pixels, image quality, GPU performance, rain/race
stability or physical controller behavior. Draw distance remains Experimental.
FSR 2/3 still uses the documented neutral motion/depth inputs and history resets.
