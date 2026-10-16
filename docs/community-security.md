# Community service security review — 16 October 2026

Scope: the Python control API, authenticated UDP relay, SQLite identity/stats,
Windows control/UDP client, protocol transitions, packaging and the supplied
Linux/Windows deployment instructions. This was a source review plus bounded
adversarial testing against owned in-memory and loopback services. No public
VDS, provider firewall, OS image, administrator account or remote target was
available or tested. This is not a claim that all future vulnerabilities have
been eliminated.

## Confirmed pre-fix findings and fixes

| Finding | Pre-fix evidence / impact | Change |
|---|---|---|
| High: authenticated UDP source spoofing and reflection | A validly MACed packet immediately replaced the client's endpoint without proving reception at that address. Registration is public, so key possession alone was insufficient; a forged source could receive discovery replies or redirected room traffic. | MGO2 requires an unpredictable, authenticated 16-byte challenge to be echoed from the exact candidate endpoint. No directory/gameplay forwarding before proof. One bounded deferred packet, five-second challenge lifetime, rate-limited retries; an unverified reply is smaller than the triggering packet. A failed rebind leaves the previous endpoint intact. |
| High: stale join acknowledgement crosses room boundaries | A player could have pending requests at two hosts. After joining one, a delayed `JOINED` from the other overwrote membership; closing the obsolete room could clear the new room state. Reproduced with three clients. | Recheck destination membership, ticket generation, pending age, request/session/key and capacity at admission. Remove competing pending joins, and purge pending state on disconnect. Closing a room only clears clients still in that room. |
| High: persistent storage exhaustion | 1,100 submissions with unchanged scores produced 1,100 replay-history rows for one profile. Repeating writes/registrations could grow the database indefinitely. | 256 most-recent operation IDs per profile, bounded registration rate, account ceiling, SQLite page quota, bounded batch/column counts and overload responses. Existing identities remain valid when registration capacity is reached. |
| Medium: arbitrary stats columns reached persistent/native results | An unrecognised column ID 65535 and invalid type/value could be stored and returned. Large rows could also exceed the Windows response cap. | Accept only the nine columns/types initialised by native routine `822FEB80`: `1,2,4,5,6,7,13,0x8011` as canonical signed int32 and `3` as int64. Reject duplicate views/columns and out-of-range ratings atomically. Maximum 100 rows × nine columns stays below 64 KiB. Historic unsupported columns are filtered on read. |

## Additional hardening from source review

- **HTTP parsing and response handling:** exact allowlisted methods/paths,
  required Host, canonical decimal Content-Length, token-valid header names,
  duplicate-header/control-character rejection and no Transfer-Encoding/Expect
  processing. Endpoint body caps are 16–1,750 bytes; headers are capped at 8 KiB
  and 64 fields. Header/body/write/close have deadlines. Generic storage errors
  do not expose SQL or paths. Ambiguous framing was hardened; an end-to-end
  proxy request-smuggling exploit was **not** established in this review.
- **Connection exhaustion:** reserve global/per-address slots before TLS starts,
  rather than counting only connections that completed a handshake. Defaults:
  128 sockets globally and 16 per direct/client address; TLS handshake five
  seconds, header/body five seconds each, drain/shutdown three seconds each.
  Proxy sockets still share the global pre-header cap. The Caddy template also
  bounds headers/body/timeouts. All sockets/tasks are released on shutdown.
- **Proxy trust:** the old backend applied one rate quota to Caddy's loopback
  address, unintentionally sharing it across all players. The template now
  overwrites the client-IP header and authenticates the local proxy with a
  private random key. The backend requires both loopback and that key before
  trusting the address. Direct TLS ignores forwarded-IP claims. A proxy-key
  header sent to an unconfigured backend fails closed. Caddy's unauthenticated
  local admin endpoint and expanded-config autosave are disabled.
- **Bounded work:** capped rate tables do not evict active quotas on address
  churn. Profile/API, IP registration, global registration and UDP budgets are
  separate. UDP ingress defaults to 20,000 packets/s and 16 MB/s globally;
  authenticated peers get 1,000 packets/s and 128 KB/s. Egress caps are 32 MB/s
  globally and 256 KB/s per peer, with bounded bursts. Discovery fanout has a
  separate 64/s global limit, and host replies are deduplicated per search.
- **Database responsiveness:** parameterised SQL, native schema allowlist,
  50 ms statement/operation deadline, short lock timeout and a shared 0.25 CPU
  second/s storage-work budget. Rank pages use indexed ordering instead of
  materialising every row/payload into a windowed rank query. Default account
  capacity is 10,000; SQLite database pages are capped at 256 MiB. WAL, backups,
  temporary files and logs still need host-level disk limits/headroom.
- **Room isolation:** full roster/header/member/virtual-endpoint validation,
  admitted-room-only traffic and original data framing validation. No arbitrary
  destination IP routing, filesystem endpoints, uploads, command execution,
  executable deserialisation, remote extension loading or dynamic plugins exist
  in the public API. No server RCE or SQL injection was found in the reviewed
  paths; this is a bounded finding, not a proof of absence.
- **Secrets and client:** credentials remain random 256-bit values protected by
  Windows DPAPI, stored server-side only as hashes. TLS certificate verification
  stays on; cross-origin redirects are disabled and checked. Responses have
  size and elapsed-time bounds. The profile read is bounded to 4,097 bytes, including concurrent file changes.
  Proxy keys are created exclusively with mode 0600 on Linux, never printed,
  included in packages, placed on process arguments or hardcoded in source.
  Windows administrators must restrict the key/state directory ACL themselves.
- **Deployment:** dedicated non-admin service user; Linux template adds private
  state, restrictive umask, memory/process/file-descriptor limits, capability
  removal and kernel/device isolation. Caddy/OS/Python are installed and updated
  by the VDS administrator; dependencies are not bundled or silently installed.

## Compatibility and upgrading

Update **both** `MotoGP0910-Windows-x64-community-online.zip` and
`MotoGP0910-Community-Server.zip`. The Online HTTP namespace is `/v2` and UDP
magic is `MGO2`; old clients/servers cannot silently skip address verification.
The original inner **LAN02** compatibility value and Offline/LAN behavior stay
unchanged. SDK/runtime versions and simulation timing are unchanged.

Back up the existing SQLite database before upgrading and keep Windows `.profile`
files. The database schema remains compatible; do not delete the database to
upgrade. Stop the old service before replacing public sources, deploy the new
Caddy template and create the private proxy key once. Use the same key for
`--proxy-secret-file` and `proxy_config.py run`. Keep TCP 30820 private; expose
only HTTPS and the chosen UDP relay port. Players receive the HTTPS address,
**never** the proxy key, TLS private key or server administrator credentials.
See `community-online.md` / packaged `README.md` for exact platform commands.

`--max-players` and `--max-db-mib` are explicit administrative capacity settings.
Do not raise limits without reserving host memory/disk and reviewing traffic.
An existing database larger than the configured page quota fails startup with
an explicit request to back up and raise the limit; data is never truncated.
A full account quota intentionally rejects new registration rather than
exhausting storage; existing profiles can still reconnect. Public registration
remains open and is not an invitation/account ownership check.

## Validation

- **46 server tests** on Linux Python and official Windows Python 3.12.8 /
  Proactor under Wine: admission and stale-room attacks, forged endpoints,
  nonce expiry/replay/MAC direction, poisoned rosters, malformed packet fuzz,
  quota exhaustion, strict HTTP variants, connection cleanup, pending TLS,
  real HTTP, signed native ratings, atomic rollback and persistence.
- **12 real HTTPS/deployment checks per platform**, using Caddy 2.10.2 and a
  temporary explicitly trusted localhost certificate. Covers proxy/header
  overwrite, missing/wrong proxy key, certificate rejection, v1/path allowlist,
  body size, direct Python TLS and no secret autosave. Caddy's QA binary matched
  the official release SHA-512 manifest; it is not shipped with the server.
- Real Windows game-side clients: **31 checks**, restart of the same database,
  **31 repeated checks**, plus **20 original generated-title/kernel statistics
  checks**, against each Linux and Windows backend. Address proof, maximum-size
  opaque guest datagrams, join/leave/rejoin and leaderboard persistence passed.
- Existing LAN: **96 service, 67 kernel and 20 generated-wrapper checks**, plus
  independent-process automatic/direct/delayed connection scenarios.
- Affected Windows targets rebuilt without new compiler warnings. Packaged DLLs
  must match `third_party/rexglue-runtime/manifest.json`; tests/profiles/private
  state/original game files are not runtime deliverables.

Reproduce with `python -m unittest discover -s server -p 'test_*.py' -v`,
`scripts/verify-online.py`, and
`scripts/verify-online-security.py --caddy /path/to/caddy`.
The HTTPS harness uses local OpenSSL for disposable certificates and never
turns off client certificate verification. Both harnesses terminate their owned
services and remove temporary state. Tests do not scan other machines.

## Limits that still matter

This service is not an authoritative race simulator or anti-cheat. A modified
client can invent its **own** leaderboard results inside the allowed numeric
schema; preventing that needs trusted result validation or authoritative
simulation, not a different password/hash. It cannot write another player's
record without that player's credential. Race hosts necessarily control their
own room and opaque original-game traffic; this audit does not certify every
original guest packet parser against hostile race participants.

Application quotas reduce resource abuse but cannot guarantee availability
against distributed identities or a flood saturating the provider's network.
They may also reject legitimate bursts; monitor 429/503 rates before raising
limits. No IP bans, secret default accounts or public administrative API were
introduced. Provider DDoS controls, patched OS/Python/Caddy, firewall/SSH/service
account settings and actual disk capacity must be checked on the real VDS by its
administrator. No such machine was accessed in this task. A real Internet race
and native Windows GPU/UI validation remain pending.
