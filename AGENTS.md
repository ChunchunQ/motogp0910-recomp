# Repository working instructions

- Write all new and updated Markdown documentation in English. Preserve exact
  localized UI labels, glyph samples, identifiers and source quotations where
  necessary to explain behavior; explanatory prose must be English.
- Development Windows packages omit README files, feature readmes, SHA256SUMS
  and separate license folders. Preserve source licenses and embedded launcher
  notices. The user will add the final release README later.
- Keep original game data, user configuration, saves and personal logs out of
  commits/packages. Preserve historical release ZIPs; use unique archive names.
- Windows 10/11 x64 is the product target. Distinguish Wine/CPU/UI checks from
  native Windows GPU playthroughs and physical controller testing.
- Preserve Offline, LAN and community Online modes. Do not make speculative
  physics/handling changes or add frame generation; both are outside scope.
- Use the SDK/toolchain pins in sdk-version.json and the custom runtime manifest.
