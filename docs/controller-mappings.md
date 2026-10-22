# Gamepad mapping database

The pinned ReXGlue SDL backend reads `hid_mappings_file`, whose default is
`gamecontrollerdb.txt`. The package places this file in `bin/` beside the game EXE, outside
`game/`. SDL loads it through `SDL_AddGamepadMappingsFromFile`; success appears
as `SDL GameControllerDB: loaded ... mappings.`

The database maps buttons/axes for controller GUIDs. SDL may already recognize
a device without this file. Successful database loading does not verify every
physical controller. The XInput backend does not use this SDL database.

Relative mapping paths are resolved from the working directory. The public
launcher sets it to the installation root; direct game starts from `bin/` also
normalize it to that root. For the ordinary packaged default, the application
resolves `gamecontrollerdb.txt` to `bin/gamecontrollerdb.txt`. Custom mapping
paths remain supported; use an absolute path when it should be independent of
the installation directory. Explicit user settings are retained.

The supplied file is preserved unchanged under `third_party/SDL_GameControllerDB/`:
2,241 entries, including 860 Windows entries, SHA-256
`e4e5cb4b96e36b439531c5fdfd648b56e69d579844cb970e680a40d9b314b786`.
Upstream attribution and the zlib license remain in source; distribution notices
are embedded in the launcher, rather than a separate licenses folder.

The real Linux SDK SDL API imported 472 new mappings. A Windows PE linked to
the SDK's SDL library loaded the packaged file under Wine: Windows platform,
569 new mappings, exit 0. Default filename and a sample GUID/button mapping
were checked. Import counts depend on platform filtering and existing entries;
these checks do not replace physical controller testing. Packaged bytes match
the supplied database.
