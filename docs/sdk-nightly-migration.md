# Exact SDK nightly migration

The exact nightly pin below remains current. Validation counts and selector
labels describe the migration milestone. FSR 2/3 remains experimental even
though current captions omit that suffix; [rendering limits](experimental-rendering.md)
and [project status](project-status.md) describe the current presentation.

The project now pins **0.10.0.24-dev.gbd833a2**, release tag
`nightly-20261002-bd833a2a`, source
`bd833a2ab69d305f99ddf57418fc38ffec8af93d`. Official Linux/Windows SDK package
SHA-256 hashes are in sdk-version.json. CMake checks the numeric package version
and full dev version string; runtime staging checks its SDK version, source
commit, archive and every DLL. The generator deliberately stamps its API floor
0.10.0 in the game manifest; the exact nightly is enforced separately. Rebuilt EXEs and custom DLLs are shipped together.

Stable milestone: commit `faa34fb4d908bbc847ae6eadc8df011bcf6828c5`,
`MotoGP0910-Windows-x64-fsr-ultrawide-stable.zip`.
Its SHA-256 is `3dfd68e88e30a583ad3a3f4ef4184dff8645ae889e4db5a1140c253d92f6f8e6`.
The stable package remains a complete rollback option. Do not mix its DLLs with
nightly EXEs or copy only an individual runtime DLL.

## Migration changes

- Regenerated guest code with the exact nightly host tool. Native sync/lwsync/
  eieio instructions now emit memory fences; updated atomics use the nightly API.
- Adapted Window::Create consumers to the new two-argument API.
- Ported explicit native-video-mode priority to the refactored ConfiguredVideoMode
  path. Explicit 1280x720 continues to be respected with a larger host window.
- Carried shared SDL, actual monitor Hz, scoped career readback, experimental
  FSR providers and presentation-aspect bridges into both rebuilt runtimes.
- Fixed the nightly radial-deadzone cutoff sign: negative stick directions now
  suppress drift symmetrically. Default deadzone remains zero.
- Updated Windows/cloud setup, codegen selection, build and runtime verification
  scripts. New FSR quality/aspect choices have English/Turkish translations.

The nightly provides file-I/O lifetime/overwrite corrections, input focus/hotplug/
keystroke improvements and atomic/fence fixes. It does **not** supply title motion
vectors or solve the temporal FSR limitations; the experimental labels remain.

## Validation

All 13 Windows/Wine probe programs passed after configuring the correct probe
arguments and device assignment. Checks include:

| Area | Result |
| --- | --- |
| Production launcher, EN/TR, five pages, four scales | 51,681 checks passed |
| Engine flags, native projection and central HUD batching | 637 checks passed on Linux and Windows/Wine |
| Collision records, fields, calling convention | 192 checks, zero mismatches on both platforms |
| Original PS3 artwork and replay sprite paths | 222 checks passed |
| SDL virtual Sony/Xbox classification and switching | 92 checks passed |
| Runtime paint planning, input, volume and source/output modes | 183 checks passed |
| Actual host window modes and FPS schedule | 33 checks passed |
| Launcher persistence and real child process | 91 checks passed |
| Career readback scope | 13 checks, 16 guest calls passed |
| Nightly deadzone and dialog input blockers | 54 checks passed on both platforms |
| Input default/config/environment/CLI precedence | All five scenarios passed |
| Native game language and override ABI | Six checks passed on both platforms |
| Startup, CRT registration, resource comparators, race callbacks | Headless probes passed on both platforms |
| Real AMD provider selection | Distinct FSR 2.3.3 and FSR 3.1.4 IDs verified |

Linux full-game startup reached **Main Menu** using the rebuilt nightly Vulkan
runtime and remained alive through intro and menu. The known lavapipe white
menu-ribbon artifact is still present, as it was on the stable baseline.
This is not a full race/career/ultrawide gameplay sweep. Windows GPU/FSR dispatch
could not be exercised because cloud Wine has no suitable D3D12 adapter.
No migration regression was found in these checks, so the nightly pin is retained.
The previously recorded chrono 1601-epoch overflow has subsequently been fixed
in the [stability update](stability-quality-control.md).
Screenshots and runtime logs stay under ignored out/; none are committed.
