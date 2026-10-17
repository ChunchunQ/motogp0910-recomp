# Testing LAN with two instances on one Windows PC

Two game processes can exercise real discovery, joining and packet transport
on one PC. This does not validate routing, broadcast or firewall behavior
between separate physical or virtual LAN peers.

1. Close existing game windows and extract the complete current Windows ZIP.
   Copy `scripts/test-lan-one-pc.ps1` from source beside the EXE; the normal
   package omits this optional helper. Preserve `game/`.
2. Open PowerShell in the program directory:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\test-lan-one-pc.ps1
```

3. In the first window, use **Online > Player Match** to create a room.
4. Alt+Tab to the second window, search for a Player Match and join.
5. Check both players, select Ready, start a race, and verify the remote rider
   and movement updates. Sharing a lobby alone does not validate racing.
6. For failures, collect `logs/lan-host.log`, `logs/lan-client.log` and any
   relevant `logs/crash-.../` directory.

No virtual LAN application is needed for this loopback test. Both instances
use `127.0.0.1` and separate `lan-test/host` and `lan-test/client` user data,
cache and metadata; normal saves and launcher settings are preserved. New
profiles may require initial setup screens. Input follows the focused window.

The helper selects 960 × 540 windows, 1× internal resolution and disabled native
MSAA to reduce shared GPU/RAM load. Resource pressure from two instances must
be considered separately. Native GPU race results still need local testing.

From a source checkout, specify the actual executable folder:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test-lan-one-pc.ps1 -ProgramDirectory 'D:\your-build-folder'
```
