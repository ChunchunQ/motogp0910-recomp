# In-game controller prompts: investigation

Requested behavior: show PlayStation symbols for the active DualSense/controller
and the actual configured keyboard bindings after keyboard input, automatically.

This is not implemented in the current package. `gamecontrollerdb.txt` maps
physical input to the guest Xbox controller, but does not change the game's
frontend artwork. The keyboard editor changes actual game inputs, not textures.

The disk's font/frontend resources (including `xenondata/fonts/arial_bold_17pt.font`
and frontend `.mgb` files) begin with the XCTD compression magic `0x0FF512ED`.
These are not loose PNGs that can simply be replaced. The generated game code
contains the compressed-file recognition path around `0x823B381C`, and the SDK
kernel also recognizes XCTD file information. No validated mapping from those
packed resources to the Xbox prompt glyphs has been obtained yet.

Replacing only a launcher label or adding an unrelated legend would not satisfy
the requested in-game replacement. The next dedicated implementation needs a
validated resource decoder or UI draw hook, exact glyph identifiers, an active
input-device observer (not merely the first connected controller), and tests for
menu/race prompts, device switching and arbitrary keyboard bindings. Existing
Xbox prompts stay intact until that replacement can be verified.
