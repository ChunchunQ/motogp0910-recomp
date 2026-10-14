# LAN lobby retry fixes

The user reported successful two-player racing over Radmin VPN, with no issues
during that race, but several failed lobby attempts before it started. The
uploaded logs are kept outside the repository; personal addresses, peer/session
identifiers and screenshots are not published.

## Findings

- A local client searched at `19:23:56` and attempted to join at `19:24:23`.
  Create returned `80070490` (not found), followed by failed cleanup on an
  uncreated session. The discovery cache expires after 3.5 seconds, while the
  original game's results screen retains its selected room indefinitely.
- `B0013` (leave local/remote) repeatedly returned `80070057` (invalid parameter)
  during local and Radmin attempts. Both original generated leave wrappers
  (`825D9A28` and `825D9AD0`) explicitly send a null private-slot array. The
  bridge mistakenly required the join-only array on leave too.
- Failed attempts in the longer logs proceed through end/delete and create
  another room in the same process. They establish lobby/session failures,
  rather than native process crashes at those points.
- The later Radmin log creates/joins a two-member room, starts it successfully,
  continues for roughly eight minutes, and ends with window-close/termination
  messages. Combined with the user's report, this is a successful two-player
  Radmin gameplay observation, not proof of every network scenario.
- One uploaded log is dated the previous day, contains no LAN startup markers,
  and ends with an unregistered guest-function fatal. Another stops early in
  startup without a fatal explanation. These do not establish the same cause
  as the lobby failures. A native `crash-*/report.txt` and dump are needed if
  either program actually exited unexpectedly.

## Changes

Keep at most 64 copied room candidates from the last successful guest search.
Joining a room selected on the game's search screen can use that snapshot even
after discovery expiry. XNet address, connect and QoS paths use the same
candidate list. Fresh live/current records take precedence. A subsequent
successful search replaces the snapshot. No guest pointers are retained.

The snapshot is only a join candidate: the normal keyed UDP handshake still
checks the running host, current room, capacity and admission. Closed hosts
cannot produce successful fake membership. Discovery visibility and live-peer
heartbeat timeouts remain unchanged; this avoids keeping disconnected peers
alive just to accommodate a slow selection on the search screen.

Only join validates/reads the private-slot array. Leave still validates its
count and local-index/XUID arrays, updates membership atomically, and uses the
original completion mechanism. Invalid input leaves membership unchanged.
The LAN wire protocol and matching runtime DLLs are unchanged. Both PCs should
nevertheless upgrade together to exercise the fixes on host and client.

## Validation

Windows x64 game and probes compiled against the pinned SDK. The actual kernel
probe checks discovery expiration before a delayed join, retained XNet address
resolution, wrong-key rejection, local/remote leave with null slot pointers,
membership counts, rejoin, invalid input and rejection after host closure.
The generated-wrapper probe exercises the original title's local/remote leave
functions and real overlapped completion/events. Separate Windows processes
under Wine exercise automatic discovery, direct fallback and delayed joining.
67 kernel/guest checks and 20 generated-wrapper checks passed, along with three
separate-process discovery/join scenarios. These are targeted regressions;
new native Windows/Radmin gameplay verification
of this revised executable is still pending.

## Remaining observations

`NetDll_XNetGetOpt: option 6 unimplemented` repeats during the successful race.
The pinned SDK returns an error without supplying this option's output; the
title then reads that output without checking the return. This deserves a
separate implementation based on the correct option contract. This patch does
not invent clock/statistic values or suppress that error, and the supplied
logs do not establish it as the cause of the initial lobby failures.

Other file/compression and GPU fetch warnings also appear in successful runs.
They are not sufficient evidence of the lobby's cause. Native crash reports
remain included in the package for any complete process exit.
