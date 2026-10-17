# Captured UI and native font proofs

The Linux startup PNGs are direct Xvfb captures of the real title with complete
disc data and TU 0.0.1.1, originally captured with ReXGlue v0.10.0:

- [Intro video frame](linux-intro.png)
- [Title screen](linux-title.png)
- [Main menu after first-run setup](linux-main-menu.png)

These demonstrate the recorded Linux startup, not current Windows GPU races.
See the [project README](../../README.md) for current SDK and evidence limits.

## Windows launcher captures

- [English](launcher-english.png)
- [Turkish](launcher-turkish.png)

These are real Windows PE launcher interfaces captured under Wine/Xvfb, not
Windows game rendering tests. See [launcher validation](../launcher.md).

## Turkish font proofs

- [Impact native atlas words](turkish-impact-native-proof.png)
- [Small Arial native atlas words](turkish-arial-small-native-proof.png)

These word sheets render the shipped BC3 glyph pixels with their actual native
metrics. They are generated font proofs, not gameplay captures or substitute
screenshots. See [font QA](../turkish-localization.md#native-fonts).
