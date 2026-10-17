# Gamepad mapping database

The pinned ReXGlue SDL backend reads `hid_mappings_file`, whose default is
`gamecontrollerdb.txt`. The package places this file beside the EXE, outside
`game/`. SDL loads it through `SDL_AddGamepadMappingsFromFile`; success appears
as `SDL GameControllerDB: loaded ... mappings.`

The database maps buttons/axes for controller GUIDs. SDL may already recognize
a device without this file. Successful database loading does not verify every
physical controller. The XInput backend does not use this SDL database.

Relative mapping paths are resolved from the working directory. The launcher
sets that directory to the program folder. For shortcuts, set **Start in** to
the EXE directory, or supply an absolute `hid_mappings_file` path. Explicit
user settings are retained.

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
