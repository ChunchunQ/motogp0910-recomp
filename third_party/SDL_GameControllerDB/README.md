# SDL GameControllerDB

Upstream: https://github.com/mdqinc/SDL_GameControllerDB

`gamecontrollerdb.txt` is the exact, unmodified file supplied by the user.
SHA-256: `e4e5cb4b96e36b439531c5fdfd648b56e69d579844cb970e680a40d9b314b786`.
It contains 2241 platform-specific entries, including 860 Windows entries.
The upstream zlib license is included in `LICENSE.txt`.

Windows packaging copies this database beside `motogp0910.exe` and includes
the license. ReXGlue v0.10.0's SDL backend uses `hid_mappings_file`, whose
default is `gamecontrollerdb.txt`. Explicit configuration overrides take
precedence. The XInput backend does not use this database.
