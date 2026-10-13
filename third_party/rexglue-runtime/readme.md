# ReXGlue v0.10.0: spatial FSR runtime

This is a matched `rexruntime.dll` / `rexgpu-xenos.dll` pair compiled from
ReXGlue v0.10.0, commit `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`.
The official v0.10.0 code generator, headers and import library remain in use.
`manifest.json` pins the archive and each DLL independently.

The official Windows runtime was built without FidelityFX support. This pair
turns on the **spatial FSR 1 EASU/RCAS and CAS shader bytecode already shipped
in v0.10.0**, using `cmake/sdk-spatial-fsr.cmake`. It does not fetch or enable
the separate temporal FidelityFX SDK: FSR 2/3, frame generation and vendor
quality presets are not exposed by the MotoGP launcher.

The existing patch, `cmake/sdk-explicit-video-mode.patch`, makes the kernel respect
an explicitly selected guest video mode even if it equals the compiled default.
Without it, a 1280×720 selection could silently inherit a 1920×1080 host window
size, defeating the distinction between native rendering and spatial upscaling.
This is a modified v0.10.0 runtime, not a newer SDK version.

`src/shared_sdl_bridge.cpp` also adds `Motogp_GetSharedSDL`, a versioned C
function table for the runtime’s own SDL instance. This is necessary because
Windows SDK consumers otherwise link an independent static SDL instance,
whose event watches cannot observe the game’s controller input. The bridge
is added by the same CMake include, without changing the existing C++ ABI.
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

The script verifies the exact source commit, initializes submodules, applies both
checked patches idempotently and builds only `rexruntime` and `rexgpu-xenos`.
Output is under `out/rexglue-spatial-source/out/win-amd64`. Locally rebuilt
DLLs can have different hashes with a different compiler; the checked-in
archive is not silently replaced.

For the prepared cloud toolchain:

```bash
source scripts/cloud-env.sh
python3 scripts/build-spatial-sdk.py \
  --sdk-source /workspace/shared/rexglue-sdk \
  --build-dir /workspace/shared/rexglue-fsr-build \
  --toolchain cmake/windows-cross-toolchain.cmake
```

`stage-spatial-runtime.py` and `.ps1` verify checksums before copying the pair.
CMake invokes staging after copying the official SDK runtime, including IDE
builds. No original game data is contained in this archive.
