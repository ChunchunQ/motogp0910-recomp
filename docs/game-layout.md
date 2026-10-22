# Game data layout

The public Windows entry point is `MotoGP09-10Launcher.exe`. The launcher version
is **1.1.11**; the supported original title update is **0.0.1.1**. Build pins live
in `sdk-version.json` and `third_party/rexglue-runtime/manifest.json`.
See [Windows builds](windows-build.md), [launcher setup](launcher.md) and
[DLC installation](dlc.md). Historical archives are not a current-release index.

```text
MotoGP0910/
├── MotoGP09-10Launcher.exe
├── config.json                  # created/migrated locally; not shipped
├── bin/
│   ├── motogp0910.exe
│   ├── motogp0910-ui.dll
│   ├── motogp0910-crash-monitor.exe
│   ├── motogp0910-update-helper.exe
│   ├── rexruntime.dll
│   ├── rexgpu-xenos.dll
│   ├── other packaged DLLs, map and runtime manifest
│   └── gamecontrollerdb.txt
├── resources/
├── dlc/                         # user-installed packages; not shipped
└── game/                        # user-supplied original disc files
    ├── default.xex
    ├── default.xexp
    ├── TU_11K61VG_0000004000000.0000000000081
    ├── xenondata/
    ├── nxeart/
    └── remaining original disc files/directories
```

## Updating an installation

1. Close the game and launcher. Extract the complete matching application
   package into the program directory, retaining your existing data.
2. Preserve `config.json`, `game/`, `dlc/`, custom paths and user saves. Keep
   `bin/` and `resources/` together with the public launcher; do not mix runtime
   DLLs from different builds.
3. Open the public launcher and use its readiness checks. The original disc,
   supported TU/patch and all three DLC packages are required in every mode.
   Direct `bin/motogp0910.exe` starts enforce the DLC prerequisite as well.

Development packages omit original game/DLC data, personal settings, empty logs,
README files and separate license folders. Required notices are embedded in the
launcher. GitHub Actions wraps the prepared application ZIP in an artifact ZIP;
extract both layers to reach the application folder.

## Discovery and saves

- An explicit `--game_data_root` takes precedence over saved/default paths.
  `run-windows.ps1 -GameRoot` points to the directory with the original XEX
  and complete disc data.
- Otherwise the game uses the saved root from `config.json`, then looks for
  `game/default.xex` under the installation root and finally legacy flat data.
  A game executable inside `bin/` resolves the installation root to its parent,
  even when started from a different working directory.
- Launcher and game share root `config.json`. Legacy TOMLs migrate only when
  JSON is absent and remain as exact `.legacy.bak` files. Custom save/cache roots
  and unrelated supported settings are preserved.
- Default Windows saves, achievements and cache are under
  `Documents/MotoGP 09-10`, honoring redirected/OneDrive Documents. The verified
  legacy migration is described in [portable settings](portable-launcher.md).
  Never move saves into `game/` or confuse them with portable `dlc/`.
- Runtime logs are created under the installation's `logs/` directory. The SDL
  mapping database defaults to `bin/gamecontrollerdb.txt` in this layout.

An absent patch can be extracted from the supported original TU. Existing patch
files and the stock XEX are not overwritten by game startup; patching occurs in
memory and unsupported title versions are rejected.
