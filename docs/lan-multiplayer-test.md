# Windows LAN multiplayer — experimental test build

14 October 2026. SDK: **0.10.0.24-dev.gbd833a2**. Game: TU **0.0.1.1**.
Protocol: **LAN02**; both PCs must use this exact package. The older LAN
foundation package is incompatible.

This build connects the original title's session and UDP/VDP socket imports to
our native LAN transport. The user has now reported a successful two-player
Radmin race; early lobby retries remain a known issue addressed in the
[lobby stability fix](lan-lobby-stability.md). The initial LAN integration
delivery skipped new runtime/gameplay tests at the user's request. The subsequent [launcher startup fix](lan-startup-fix.md) passed
17 targeted real-SDK Windows/Wine checks. [Crash diagnostics](crash-reports.md)
passed 9 native process/fault scenarios. Windows EXEs compiled successfully;
packaging verifies matching runtime DLL hashes and ZIP integrity. This does not
replace native two-PC gameplay checks for the revised executable.

## Two-player Windows observations — 14 October 2026

The user found and joined a room using two local instances, then connected two
PCs through Radmin and reported stable racing. Logs show real bidirectional
guest packet exchange, two-member sessions and successful session starts.
Early attempts include a discovery-cache timeout and invalid leave parameters;
the latest patch fixes these and adds targeted Windows regression coverage.
Native Radmin testing of the revised patch is pending. Larger grids, poor
network conditions, reconnects and extended gameplay are not yet validated.

## Native Windows host observation — 14 October 2026

The user reached the rendered Player Match lobby without crashing. Their log
confirms automatic selection of an active Famatech Radmin VPN adapter, native
UDP bind on the discovery range, successful XGI create (`B0010`) and local join
(`B0012`), each with result `0`, and one member. This validates this user's local
host startup and lobby path. No second peer or game packet exchange appears in
the supplied log. Remote discovery, client join and synchronized racing still
need validation. Personal IPs, room IDs, original log and screenshots are not
committed here.

For one Windows PC, use the [two-instance test instructions](lan-one-pc-test.md).

## Usage: two Windows PCs

1. Extract the complete matching ZIP on both PCs. Preserve game data, settings
   and saves; do not mix old EXEs with new runtime DLLs.
2. Join the same physical or compatible virtual LAN. No specific VPN brand is
   required. Both peers must be reachable on that network.
3. Select **General > Multiplayer Settings > Connection mode > LAN** on both
   launchers. Use different names; player names allow at most 15 UTF-8 bytes,
   room names 31. The default Player name gets a per-process suffix.
4. Initially leave Local IPv4 and Host IPv4 empty. If automatic adapter selection
   chooses the wrong network, set each PC's Local IPv4 to its own LAN address.
   These settings apply at the next game launch.
5. Allow `motogp0910.exe` through Windows Firewall for the selected network.
   Discovery/transport uses **UDP 30810–30817**; ordinary same-LAN use does not
   require Internet router port forwarding.
6. Use the original **Online/Xbox Live > Player Match** flow. One PC creates a
   room; the other searches and joins the discovered result.
7. If discovery fails, close the joining game and set Host IPv4 in Multiplayer
   Settings to the host's reachable address on that LAN. Restart and search
   again. The host can leave Host IPv4 empty.
8. Begin with two players. Check joining, race start, remote movement, race end
   and rejoining after departure. Select Offline to disable LAN.

No official Xbox Live login, account/password or dedicated server is needed.
Ranked session creation is explicitly rejected. Official friends/invites,
ranked arbitration, voice and host migration are outside the LAN bridge.
Community Online leaderboards are a separate mode; see
[community deployment](community-online.md).

## Diagnosing a failure

Collect the latest game log from both PCs and identify the host, failure stage
(menu/search/join/start/race/rejoin) and whether discovery or explicit Host IPv4
was used. `LAN_LOBBY` records adapter/endpoint selection, `LAN_SESSION` records
session messages and `LAN_PACKET` identifies initial game traffic. A recorded
send alone does not prove remote reception or synchronized racing. Logs may
contain local/virtual LAN addresses; review them before sharing.

## Implemented in this delivery

- Native subnet/global discovery, direct-IP fallback, actual session IDs/keys,
  heartbeat admission/leave/expiry and a bounded host-distributed peer roster.
- Scoped network identity and online admission for local player 0. The stored
  offline profile XUID is unchanged; save/profile calls keep their original
  identity. Network callers get an independent per-process identity.
- App-local XNet address/connection/QoS calls and virtual UDP/VDP guest sockets.
  Guest ports stay separate from the host transport ports. Original datagram
  bytes are copied through the transport; peer-to-peer forwarding uses the
  host's roster. This is not a public internet or authenticated service.
- Scatter/gather WSA send/receive ABI, pending overlapped receives, real kernel
  event completion, result/error reporting, cancellation and socket cleanup.
  Pending receives retain copied buffer descriptors, not temporary stack
  `bytes/flags` output pointers. The receiver stops before guest memory teardown.
- Bounded copied contexts and scalar/string/binary property serialization,
  filtering and correct search row slot/property/context pointers. Public and
  private lobby capacities are represented. QoS metadata comes from the host;
  Internet bandwidth and latency are not measured.
- Multiplayer dialog: Offline / LAN / Online selection, player/room names, automatic local adapter selection
  with explicit IPv4 override, optional host IPv4 fallback. EN/TR localization.
- Separate game cover and launcher logo icons, existing graphics/controller,
  collision, career livery readback and stability changes remain in the package.

## Remaining verification and limits

The user has verified local discovery/join and reported successful two-player
Radmin racing with the preceding executable. Lobby retry regressions are now
tested under Wine; native testing of the updated fix is pending. Wine in this
cloud reports no physical IP Helper adapters, so its network checks use
loopback and cannot substitute for Radmin routing/gameplay checks.

Initial use is two players. Multi-client mesh and private slots have code
paths but are not validated in this delivery. Nonzero WSA completion routines,
TCP-based multiplayer and blocking scalar receive are not implemented by this
bridge; the inspected title VDP path uses events, overlapped receives and
nonblocking sockets. No leaderboards, ranked arbitration, voice verification
or host migration is claimed. Disabling LAN forwards original imports to the
pinned runtime. Existing offline fixes were validated in earlier builds; the
startup fix passed 17 targeted checks, but no broad new gameplay/regression run was performed.
