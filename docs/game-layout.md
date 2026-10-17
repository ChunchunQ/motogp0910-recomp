# Game data layout

Use the current package linked from the [README](../README.md).
The active SDK pin is in `sdk-version.json`; the title requires TU **0.0.1.1**.
See the separate [launcher guide](launcher.md).

```text
MotoGP0910/
├── motogp0910-launcher.exe
├── motogp0910.exe
├── motogp0910-crash-monitor.exe
├── rexruntime.dll
├── rexgpu-xenos.dll
├── other packaged runtime DLLs
├── gamecontrollerdb.txt
├── resources/
└── game/
    ├── default.xex
    ├── default.xexp
    ├── TU_11K61VG_0000004000000.0000000000081
    ├── xenondata/
    ├── nxeart/
    └── remaining original disc files/directories
```

## Updating an installation

1. Close the game and extract the complete package into the program directory.
2. Keep all original disc files together under `game/`, including XEX, patch/TU
   and the original directory structure. EXEs and runtime DLLs stay above it.
3. Open the launcher or double-click the game EXE. Keyboard emulation defaults
   to enabled; a CMD script is unnecessary.

The package does not include or automatically move original game data, XEX,
TU or patch files. Development packages omit readmes and separate license
folders; required notices remain embedded in the launcher.

## Discovery and saves

- An explicit `--game_data_root` always wins. `run-windows.ps1 -GameRoot` should
  point to the directory containing the original XEX and complete disc data.
- Otherwise, `game/default.xex` beside the EXE takes priority over a legacy
  `default.xex` beside the EXE. The executable directory is used even when
  launched from another working directory.
- Default saves/cache remain in the SDK user-data location. Do not move saves
  into `game/`; retain an explicit `--user_data_root` when updating.
- Preserve `motogp0910.toml` and launcher settings when moving program folders.

When a patch is absent, the title extracts `default.xexp` from the original TU.
It does not rewrite the stock XEX or replace an existing patch. The runtime
applies the patch in memory and rejects title versions other than 0.0.1.1.
