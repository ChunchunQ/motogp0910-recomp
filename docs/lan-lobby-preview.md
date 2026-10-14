# LAN lobby foundation — 14 October 2026

Historical foundation milestone: for the later guest socket/XNet integration and
current user-test build, see [Windows LAN multiplayer](lan-multiplayer-test.md).
The tests below apply to the foundation revision, not the untested integration.

This first implementation milestone adds a native UDP discovery/session
service and a bridge for MotoGP's original XAM session calls. **It is not yet
playable multiplayer.** The regular Windows game and launcher keep offline
behavior by default. The game EXE uses the supplied MotoGP 09/10 cover; the
launcher EXE uses the separately supplied MotoGP logo.

## Implemented and checked

- Automatic interface selection prefers an active **Radmin VPN** adapter.
  An explicit local IPv4 can override it. Discovery sends directed/global
  broadcasts and searches the local adapter's ports **30810–30817/UDP**.
  A host owns its service; no central directory or dedicated server is used.
- Direct host IPv4 discovery uses the same validated advertisements, actual
  endpoints, session IDs, keys and join handshake. It does not fabricate a room.
- Session keys are random. Full/closed rooms are excluded; wrong keys and
  over-capacity joins fail. Heartbeats, expiry, explicit leave and host-close
  notification remove obsolete memberships. Caches and packet queues are bounded.
- An opaque host/client datagram channel retains game bytes and guest port
  numbers. **It is not yet connected to MotoGP's WSA receive/send imports.**
- The guest bridge creates actual kernel `Session` handles/native objects,
  fills `XSESSION_INFO`, returns 92-byte search records with slot counts,
  host-name/context metadata, and performs actual client-to-host joins.
  It handles local/remote membership, details, delete and lobby/start/end state.
- Production imports intercept the implemented session messages only when
  explicitly enabled. Offline calls forward to the exact packaged runtime's
  original exports. The stored offline profile XUID/save ownership is unchanged.
- No guest buffer pointer is retained in room advertisements or the UDP worker.
  Guest memory ranges/protections and output sizes are checked. Real kernel
  overlapped completions/event signalling are used; searches currently spend a
  bounded 300 ms in the requesting call before completion.

The mode is developer-only at this milestone. There is no user-facing LAN
launcher page yet. `lan_enabled=false` is the default. Internal settings are
`lan_bind_ip` (empty: select Radmin/LAN), `lan_host_ip` (empty: automatic), and
`lan_room_name`. The normal game should remain in offline mode for playing.

## Validation

Windows x64 binaries ran under Wine, using the packaged runtime and the
original generated TU `0.0.1.1` title code:

- `lan_service_probe`: 96 protocol/discovery/session checks, including malformed
  packets/IPs, two actual hosts, deduplication, bad key, full room, heartbeat,
  real opaque packets both ways, close and stale expiry. Direct mode disables
  the reachable automatic test target first to exercise the IP fallback.
  A separate socket test uses only a subnet-broadcast destination (no local-IP
  unicast fanout); sockets receive on all local addresses so they can accept
  broadcasts. Room replies deduplicate by session/host identity across interfaces
  and prefer the selected subnet. The Wine IP Helper API reports no physical
  adapters here, so this broadcast test uses `127.255.255.255` on loopback;
  real Radmin interface enumeration/routing remains unverified.
- `verify-lan-service.py`: distinct Windows host/client **processes**, automatic
  and direct-IP modes, real joins and byte-for-byte bidirectional opaque traffic.
- `lan_guest_probe`: 49 actual kernel/guest session checks; Turkish host names
  are encoded as guest big-endian UTF-16. No fake sentinel
  handles or membership, no change to the offline profile.
- `lan_import_probe`: calls the **original generated MotoGP wrappers** at
  `0x825D9668` (create) and `0x825D9DA8` (search), using production import hooks.
  Separate host/client Windows processes discover and join in both modes.
  The disabled/stopped mode restores the original SDK import behavior.
- `windows_icon_probe`: 110 Windows checks. Both EXEs embed the exact ICO frame
  bytes for their respective artwork and load all seven sizes (16–256 px).
- The existing Windows regression suite passed all 19 CPU/input/UI scenarios,
  including the real launcher controls (51,681 checks), SDL display/frame pacing
  (33), and runtime integration (183). The first run of three window tests had
  no live X display; those three passed after restoring Xvfb, without code fixes.

These network tests use loopback to isolate process/API behavior. They do not
verify broadcast delivery through a real Radmin adapter, Windows firewall,
the rendered Player Match UI, or synchronized racing on two Windows PCs.

Reproduce after building the optional targets:

```powershell
python scripts/verify-lan-service.py out/build/windows-nightly-release/lan_service_probe.exe
python scripts/verify-lan-service.py out/build/windows-nightly-release/lan_import_probe.exe --game-root "D:/your-game/game"
out/build/windows-nightly-release/lan_guest_probe.exe "D:/your-game/game"
```

For Wine, add `--wine /path/to/wine64` to the Python commands and supply the
native Windows VC++ CRT as in the existing Windows validation workflow.

## Required before playable LAN

1. Online admission and network player identity, retaining offline save ownership.
2. XNet address/key/connection/QoS calls and correct WSA send/overlapped receive
   ABI, pending/completed/cancelled socket operations, and guest-port routing.
3. Guest property serialization/filtering and any additional messages exercised
   by the actual Player Match path. This preview rejects unsupported private
   slots/property requests instead of pretending to support them. It does not
   yet implement host migration, arbitration, ranked services or leaderboards.
4. A simple launcher LAN control with automatic discovery as the primary flow
   and explicit host IP as fallback. A rendered lobby/race test must then verify
   the actual menu path and peer lifecycle.
5. Two physical Windows machines on Radmin: host/join, start/finish, disconnect,
   repeat joins and long races. More than a host/client pair needs further work;
   the present opaque channel is not a client-to-client mesh.

The SDK stays pinned to `0.10.0.24-dev.gbd833a2`; no third-party fork replaces
the runtime. Existing collision, PS3/replay, livery, graphics/audio/input and
launcher settings remain in the build.
