# Pinned custom ReXGlue runtime

The current runtime is built from exact Nightly **0.10.0.24-dev.gbd833a2**,
commit `bd833a2ab69d305f99ddf57418fc38ffec8af93d`.
`sdk-version.json` pins the official code generator/headers/import library;
`manifest.json` verifies the custom archive and every DLL independently.
The previous stable milestone is commit `faa34fb` and its separately named ZIP.
Replace the complete EXE/DLL package together; stable and nightly C++ ABIs differ.

The runtime enables bundled FSR 1 / CAS shaders and the separate pinned AMD
FSR 2.3.3 / FSR 3.1.4 providers using `cmake/sdk-temporal-fsr.cmake`.
FSR 2/3 remains Experimental: neutral depth/motion, per-frame reset, no frame
generation. See [practical limits](../../docs/experimental-rendering.md).

The nightly explicit-video-mode patch preserves native 720p source priority.
The nightly deadzone patch corrects negative-direction radial cutoffs.
The stability patch rejects dynamic TLS overruns and slot multiplication overflow,
owns title launch paths, assigns atomic thread IDs, safely initializes the host
processor cache, and preserves FILETIME precision/range. Disabled host affinity
avoids topology lookup and warning spam. Texture footprint initialization is
explicit (defensive cleanup, not a confirmed career rendering fix).
The ultrawide patch adds an atomic presentation aspect bridge; title hooks
adjust camera projection and central HUD batching.

`src/shared_sdl_bridge.cpp` also adds `Motogp_GetSharedSDL`, a versioned C
function table for the runtime’s own SDL instance. This is necessary because
Windows SDK consumers otherwise link an independent static SDL instance,
whose event watches cannot observe the game’s controller input. The bridge
is added by the same CMake include, without additional changes to the selected SDK C++ ABI.
It also exports `Motogp_ConfigureHostDisplay` v1 to select an actual SDL
fullscreen mode/Hz, desktop borderless, or windowed mode on the UI thread.
Invalid or unsupported modes are rejected before changing the window.

`cmake/sdk-livery-readback.patch` and `src/livery_readback_bridge.cpp` add
a scoped synchronous resolve readback override. The game enters this scope
only while generating custom career rider/bike liveries and copying their
resolved mipmaps on the CPU. It preserves the user's readback flag and
restores the ordinary GPU path when generation finishes. Both DLLs must
come from this manifest. See [career texture notes](../../docs/career-textures.md).

The DLLs were cross-compiled with Clang 20.1.8, the MSVC 14.44 / Windows SDK
10.0.26100 ABI, Release, D3D12 enabled, Vulkan disabled, and Tracy enabled.
Cross compilation also needs SSSE3 enabled and case aliases for Windows SDK
headers (such as `ObjBase.h`) on a case-sensitive Linux filesystem.

To reproduce from a Windows Visual Studio x64 developer shell with Git,
Python 3, CMake, Ninja and Clang 20+ available:

```powershell
python scripts/build-spatial-sdk.py --sdk-source out/rexglue-spatial-source --jobs 4
```

The script verifies the exact source commit, initializes submodules, applies the
checked project patches idempotently and builds only `rexruntime` and `rexgpu-xenos`.
Output is under `out/rexglue-spatial-source/out/win-amd64`. Locally rebuilt
DLLs can have different hashes with a different compiler; the checked-in
archive is not silently replaced.

For the prepared cloud toolchain:

```bash
source scripts/cloud-env.sh
python3 scripts/build-spatial-sdk.py \
  --sdk-source /workspace/shared/rexglue-sdk-nightly \
  --build-dir /workspace/shared/rexglue-nightly-windows-build \
  --toolchain cmake/windows-cross-toolchain.cmake
```

`stage-spatial-runtime.py` and `.ps1` verify checksums before copying the pair.
CMake invokes staging after copying the official SDK runtime, including IDE
builds. No original game data is contained in this archive.

## Motegi rain log compatibility

`cmake/sdk-motegi-runtime-compat.patch` handles exact device-root opens,
host-volume and SDK virtual raw-cache disk metadata, negative XCTD capability queries on extracted
host files, and validated host/null-cache virtual dismount handles. The title registers
`\Device\Image` against its actual active game root, including the localized
merged tree. Type-zero texture bindings keep their null resource behavior;
conditionally unused samplers no longer emit a warning on every draw. Actual
malformed descriptor types retain diagnostics. Missing optional
`ShaderDumpxe:\CompareBackEnds` developer data is quiet; missing ordinary
paths still report failures.

`file_compat_probe` exercises the packaged runtime's real kernel/VFS paths.
For the additional CPU texture-parser check, configure the game build with
`-DMOTOGP_SDK_VALIDATION_SOURCE=/path/to/pinned/patched/sdk` and build
`texture_binding_probe`. Its generated translation unit extracts the exact
SDK parser body, failing if expected source boundaries change. It tests all
38 unique unbound descriptors from the supplied race log, plus valid-bind,
unbind and rebind transitions; it does not simulate GPU rendering.
See [audit and limits](../../docs/motegi-rain-qa.md).
