MotoGP 09/10 original PS3 controller artwork

ps3-prompts.bin contains the 42 standalone controller texture entries
(102 through 143) extracted from the user's supplied PS3 sharedfrontend.mgb.
Only those controller textures are included; no complete game bank or fonts.
The 64x64 BC3 compressed pixel blocks are unchanged.
ps3-prompts.json records source/pack SHA-256 hashes and all texture entries.

Keep this directory at resources/controller beside motogp0910.exe.
Reproduce the pack with scripts/extract-ps3-prompts.py and your source bank.

Full Control Settings diagram: ps3-controls.bin is entry 251 (14D50C26) from
that same supplied PS3 sharedfrontend.mgb: original 1024x512 BC3 data, unchanged.
ps3-controls.json records source/payload/package hashes. Keep it with this build.
Default/Alternative presets retain native action labels; PS3 label positions
follow its own leader lines. Keyboard mode displays current bindings instead.
