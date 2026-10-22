# Launching the game directly

Extract the complete current Windows package into the program directory, keeping
existing game data, configuration and saves. Double-click `bin/motogp0910.exe`;
no command prompt or special keyboard argument is required. Keep `bin/` and
`resources/` with the public launcher and root `config.json`. Original disc,
supported TU and all three DLC packages are required, including direct starts.
See [installation layout](game-layout.md) rather than selecting an old ZIP.
Development packages omit README files, launch scripts and separate license
folders; required third-party notices are embedded in the launcher.

Keyboard defaults: Enter = Start, Space = A, Backspace = B, W/S/A/D = left
stick, Shift + arrows = D-pad. Existing controller backends and bindings are
preserved. The optional [launcher](launcher.md) remains available.

## Preserving explicit preferences

Before input initialization, the application enables `mnk_mode` only if its
source is still the SDK default. Explicit saved, environment, CLI and runtime
values remain unchanged. In root `config.json`, set `"mnk_mode": true` or remove
an explicit false value to enable keyboard emulation. Controller users can
retain `false`. Legacy TOMLs migrate only when JSON is absent; editing a legacy
backup does not change the active configuration. The SDK still receives a
private temporary TOML adapter internally.

## Validation

The Windows `input_defaults_probe` uses the real SDK config registry and the
production input-startup helper. Five Wine scenarios passed with exit 0:

| Inputs | Result |
| --- | --- |
| No custom setting | Keyboard enabled |
| TOML false with custom backend/bindings | Keyboard disabled; mappings retained |
| Environment false over TOML true | Keyboard disabled |
| CLI false over environment/TOML true | Keyboard disabled |
| CLI true over environment/TOML false | Keyboard enabled |

Every scenario also checks that a later runtime false value is retained. These
are precedence checks, not physical controller or Windows GPU gameplay tests.
Packaging separately checks ZIP integrity, output hashes and original-data
exclusion. See [game layout](game-layout.md) for data discovery and save paths.
