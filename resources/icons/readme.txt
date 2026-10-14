Windows executable icons

Artwork supplied by the user in icons.zip:
  sources/game.jpg     -> motogp0910.exe (MotoGP 09/10 cover)
  sources/launcher.png -> motogp0910-launcher.exe (MotoGP logo)

The ICOs preserve the original artwork, layout and launcher transparency.
Both contain 16, 24, 32, 48, 64, 128 and 256 pixel versions. The resources are
embedded in the EXEs; users do not need separate ICO files beside the game.
The launcher and game also set their large/small native window icons.

To regenerate after changing the source artwork:
  python scripts/build-windows-icons.py
Pillow is required for conversion only, not for normal CMake builds.

Verify the Windows binaries with:
  windows_icon_probe.exe motogp0910.exe resources/icons/game.ico motogp0910-launcher.exe resources/icons/launcher.ico

This artwork is not a launcher screenshot. Screenshots and preview composites
are not included in the repository or release package.
