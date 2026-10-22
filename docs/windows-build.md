# Building on Windows 10/11 x64

Use current sources from the [repository](https://github.com/inflamesgo/motogp0910-recomp),
not an old source ZIP or preview executable. The SDK/toolchain pin lives in
`sdk-version.json`; matching custom runtime DLLs are checked through
`third_party/rexglue-runtime/manifest.json`. Current sources include the startup
and race callbacks described in [analysis.md](analysis.md).

## Prerequisites

1. Visual Studio 2022 or newer, including Community 2026, with **Desktop
   development with C++**, MSVC x64/x86 tools and Windows 10/11 SDK. An existing
   newer VS does not require installing VS 2022 separately. Alternatively use
   [VS Build Tools](https://aka.ms/vs/17/release/vs_BuildTools.exe).
2. **C++ Clang Compiler for Windows**, Clang **20+**. VS 2026's Clang 22 meets
   this requirement. An external [LLVM 20.1.8 Windows x64 installer](https://github.com/llvm/llvm-project/releases/download/llvmorg-20.1.8/LLVM-20.1.8-win64.exe)
   is another option. Use MSVC-targeting LLVM, not MinGW GCC/Clang.
3. **C++ CMake tools for Windows**, providing CMake **3.25+** and Ninja. The
   helper discovers VS tools and passes Ninja's path explicitly. Standalone
   tools can be installed with:

```powershell
winget install --exact --id Kitware.CMake
winget install --exact --id Ninja-build.Ninja
```

Without winget, use the [CMake installer](https://cmake.org/download/) and
[Ninja Windows ZIP](https://github.com/ninja-build/ninja/releases), then put
`cmake.exe` and `ninja.exe` on PATH. Open a new PowerShell after installation.
The build helper prepares the VS x64 environment automatically and prints its
selected tool paths. A full VS 2026 build was not run on this Linux host;
VS discovery has targeted coverage. Missing MSVCP140/VCRUNTIME140 DLLs require
the [Microsoft x64 Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe).

## Supported input hashes

`scripts/build-windows.ps1` verifies these original inputs before building:

| Input | SHA-256 |
| --- | --- |
| Stock `default.xex` | `54245417e996ed0aa561a5a803fd810e87684bd0b43082f316db711b3ee786ae` |
| Original TU container | `441df6cc2e20ac8aeeed475138c1714d477f965423d8c5c9b0202e3a18d8141f` |
| Extracted `default.xexp` | `0419205ba10dc65eaa7b0bb62abc64cada46119ece5ed749e4ba20c06a6bc079` |

The TU patches stock 0.0.0.1 to supported 0.0.1.1 in memory. The original-input
parameter set accepts the original TU container; `-PreparedInputs` accepts a
verified XEX/patch directory. Do not publish these files or their private URLs.

## Obtain sources

```powershell
git clone https://github.com/inflamesgo/motogp0910-recomp.git
cd motogp0910-recomp
```

For an existing checkout, preserve local changes and use `git pull --ff-only`.
GitHub **Code → Download ZIP** is also supported, but includes historical
archives. When updating a source ZIP, preserve `out/`, `assets/` and generated
code caches; back up custom source edits first.

The helper downloads and verifies the pinned official SDK and generates code
locally from your XEX/TU. The standard PowerShell build does not require Linux,
xwin or building SDK sources. CMake stages the verified custom runtime pair
automatically. [Runtime reproduction](../third_party/rexglue-runtime/readme.md)
is separate from the normal game build.

## Build and run

`GameRoot` must contain the stock `default.xex`; `TitleUpdatePath` is the
original TU file. Replace these examples with your actual paths:

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
$GameRoot = 'D:\Games\MotoGP0910\game'
$TitleUpdate = Join-Path $GameRoot 'TU_11K61VG_0000004000000.0000000000081'
.\scripts\build-windows.ps1 `
  -GameRoot $GameRoot `
  -TitleUpdatePath $TitleUpdate `
  -Jobs 4 *>&1 | Tee-Object -FilePath '.\build-windows.log'
.\scripts\run-windows.ps1 -GameRoot $GameRoot
```

The execution-policy change applies only to the current shell. XEX/TU alone
are sufficient to compile; running requires the complete disc data and all
three supported DLC packages in the portable installation. See [DLC setup](dlc.md). Suggested
starting parallelism: Jobs 2 for 8 GB RAM, 4 for 16 GB, 8 for 32 GB+, subject
to CPU resources. Reduce Jobs after memory exhaustion. Subsequent builds reuse
unchanged objects.

The helper verifies original hashes, extracts the TU and stages XEX/patch copies
under `assets/`. It does not alter the original stock XEX or copy the entire
game directory. It generates code, builds the application/headless probe and
stops on command failures. The headless probe verifies the patched title,
registered callbacks, all 7,144 CRT initializer targets and targeted runtime
behavior; it explicitly does not claim gameplay.

Output is **`out\build\win-amd64-release\motogp0910.exe`** with matching DLLs
and the Xenos plugin. Running an old executable does not test a new build.
The run helper supplies the full game root and only adds a verified patch
when absent; it refuses to replace a different existing patch. Saves go to
`out\userdata`, the game log to **`out\motogp-windows.log`**. Retain caches
between builds; deleting them forces regeneration/recompilation.

For diagnosis, provide `build-windows.log` after a build failure or the game
log after a runtime failure. Full Linux menu startup and native Windows user
observations are documented separately from CPU probes.

The optional headless probe's `--launch-cpu` mode executes the real guest entry
briefly without a GPU, using SDK audio/input. Its observation timeout exits 4;
that is not a gameplay-success result and the normal build does not use it.
