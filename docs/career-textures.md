# Career rider and bike textures

The user reported noisy/striped textures on the custom rider and bike in the
first career race, while ordinary riders in other modes rendered correctly.

The game's livery builder at `0x82370A60` renders a customized texture per mip,
resolves it (`0x82372B08`), waits for its GPU fence (`0x823CDE00`), then locks
and copies the result on the CPU (`0x823C0B80`, `0x824A0F08`). With the SDK's
default GPU-only resolves, the CPU copy can read stale guest RAM.

The game hook acquires a nested scope for this builder. A small C ABI in the
custom GPU plugin overrides `GetReadbackResolveMode` to `kFull` only
while that scope is active. The existing backend synchronizes its resolve
readback before copying it into guest RAM, including its resolution-scale
downsampling path. No game shader or original game data is replaced. The
user's readback flags are preserved; scope release also runs on exceptions.

## Installation

Extract the complete matching Windows package, keeping the public launcher,
`bin/` and `resources/` together. Replace the game and matched runtime DLLs as
one set; preserve `game/`, `dlc/`, root `config.json` and saves. The controller
mapping database is under `bin/`. See [current layout](game-layout.md). This package
retains the validated PS3 prompt/replay and Qatar collision fixes. The launcher
also removes the lower-left SDK/platform label and its background box so the
sidebar stripes remain visible.

## Validation and remaining check

The production scope hook and packaged GPU API passed 13 checks / 16 guest
calls on both Linux and Windows/Wine, covering nested scope release, exceptions,
null-builder calls, calling convention and all four user readback settings.
The Windows runtime/launcher checks passed 140 / 72. With the new runtime pair,
222 native prompt, 92 SDL input, 192 collision and 50 startup/guest checks passed.
The launcher was opened under Wine and the uncovered stripes were inspected.

The Linux Vulkan plugin and Windows D3D12 pair were built. A real Linux game
startup used the new plugin, but career gameplay visual verification was stopped
at the user's request. The user will check the first Qatar career race locally
on Windows, especially the custom rider's suit and bike paint. Successful CPU
and ABI checks alone do not establish that the visible texture defect is fixed.
A `LIVERY_READBACK` debug-level log message confirms entry into the custom texture builder.

This report originally covered stable v0.10.0 at
`f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`. The readback repair is retained in the
current pinned nightly runtime; `sdk-version.json` and the runtime manifest
are authoritative. See [migration](sdk-nightly-migration.md) and
[the runtime reproduction instructions](../third_party/rexglue-runtime/readme.md).
