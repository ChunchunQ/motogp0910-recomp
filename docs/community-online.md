# Community Online / LAN / Offline

This adds a **community service**, with public participation and no invitation or
Xbox account requirement. Players use the same compatible Windows build and
provide their own game data. The same server program runs on **Linux or Windows
with Python 3.11 or newer**, using only Python's standard library.

## What the VDS runs

- A directory for native Player Match rooms.
- An authenticated UDP relay carrying the original guest UDP/VDP traffic.
  Clients initiate outbound connections, so they do not need Radmin or router
  port forwarding. The VDS needs an inbound UDP port.
- SQLite player identities and native game leaderboard records. Results survive
  server restarts. Discovery rooms and active connections expire after a restart.

One player's game still runs the race simulation, as in the original peer-hosted
multiplayer. The VDS is **not an unattended dedicated race simulation**. Leaving
as the race host ends that session. This service does not restore Microsoft's
Xbox Live accounts, friends, invitations, voice, Marketplace or official ranked
arbitration. Use **Player Match** for community racing. It has no authoritative
anti-cheat: a modified client can submit fabricated records for its own identity.

## Player setup

In **General → Connection mode** choose:

| Mode | Behavior |
|---|---|
| Offline / Çevrimdışı | Original local game and saves; no community or LAN connection |
| LAN / Radmin | Existing broadcast room discovery and optional host IPv4 fallback |
| Online / Çevrimiçi | Community HTTPS handshake, server room discovery, UDP relay and persistent native leaderboards |

For Online enter the administrator's **HTTPS origin**, for example
`https://play.example.com`, in **Online server**. Set a player name and optionally
a host room name, then launch and use the original Player Match host/search UI.
Everyone must enter the same server address. There is deliberately no invented
public default server: the user has not purchased/deployed the VDS yet.

LAN IPv4 fields are for LAN. They do not select an Internet interface or route
Online traffic. Online selects the route to the VDS automatically.

Existing `lan_enabled = true` configurations load as LAN if `network_mode` was
never set. Explicit `network_mode = 'offline'` wins over a stale legacy LAN flag.
These mode choices apply at game launch; they are not added to the F1 live menu.

## Linux server

**Security update:** Online now uses **MGO2 / `/v2`**. Upgrade the game and
server together; old Online binaries are deliberately rejected. LAN keeps LAN02.
Keep the existing database and player profiles. See [security review](community-security.md)
(`SECURITY.md` in the server ZIP) for the fixes and exact limits.

1. Extract `MotoGP0910-Community-Server.zip` to a dedicated folder. Install
   Python 3.11+ and Caddy using the operating system's normal package mechanism.
2. Start the service from that folder:

   ```sh
   python3 proxy_config.py init
   ./start-server.sh --proxy-secret-file data/proxy.key
   ```

   Or `python3 community_server.py --proxy-secret-file data/proxy.key`.
   Run `init` once; it creates a private key without printing it and refuses to
   overwrite an existing key. Keep `data/proxy.key` on the VDS, never in clients.
   The control API listens on
   `127.0.0.1:30820`; the UDP relay listens on `0.0.0.0:30821`.
3. Point a DNS A record such as `play.example.com` to the VDS IPv4. Use a direct
   DNS record: a web-only CDN proxy does not carry the game's UDP relay traffic.
4. Copy `Caddyfile.example` to `Caddyfile`, replace `play.example.com`, then run
   `python3 proxy_config.py run`. The helper reads the same private key into
   Caddy's environment, without a shell or secret command-line argument. Caddy
   obtains a publicly trusted TLS certificate. Both processes must use the same
   key file. The template overwrites spoofable client-IP headers; the backend
   accepts them only from loopback with that key.
   On Linux, permission to bind ports 80/443 should come from the distribution's
   Caddy service and its narrow `CAP_NET_BIND_SERVICE`, rather than running the
   game backend as root. A manual Caddy process needs the equivalent permission;
   the Python backend only needs its unprivileged 30820/30821 ports.
5. Allow inbound **TCP 80/443** for HTTPS/certificate issuance and **UDP 30821**
   for the relay, in both the provider firewall and the operating system firewall.
   Keep TCP 30820 private to the machine.
6. `https://play.example.com/health` should report `service: motogp0910`.
   Enter `https://play.example.com` in both launchers.

`motogp-online.service.example` is an optional systemd template. Adjust its paths
and create the `motogp` service user before installing it. It keeps writable
service state in `/var/lib/motogp-online`; the example's Caddy process is managed
separately. Create the key as the dedicated user at
`/var/lib/motogp-online/proxy.key` before starting the example service:
`python3 proxy_config.py init --secret-file /var/lib/motogp-online/proxy.key`.
For Caddy, use `proxy_config.py run --secret-file /var/lib/motogp-online/proxy.key`
under an account allowed to read it. If using separate Caddy/service users,
provision the same key privately for both or supply `MOTOGP_PROXY_SECRET` to
Caddy through the administrator's secret manager; do not make the service's
private state directory world-readable. The template also applies a dedicated
user, restrictive file permissions, memory/process/file-descriptor limits and
Linux service isolation. The scripts do not install services or change firewalls.

## Windows server

Install Python 3.11+ (the standard python.org Windows distribution) and Caddy.
Extract the same server ZIP, open PowerShell in the extracted folder, and run:

```powershell
python .\proxy_config.py init
powershell -NoProfile -ExecutionPolicy Bypass -File .\start-server.ps1 --proxy-secret-file data/proxy.key
```

In another console, edit the same `Caddyfile` example and run:

```powershell
python .\proxy_config.py run
```

The DNS, HTTPS and inbound port requirements are identical to Linux. On Windows
Server allow TCP 80/443 for Caddy and UDP 30821 for Python in Windows Defender
Firewall. The provider's firewall must also permit these ports. For unattended
startup, install the commands with the server administrator's preferred service
manager. Run Python/Caddy as a dedicated non-administrator account, and restrict
the `data` directory and proxy key's Windows ACL to service/backup administrators.
Linux mode bits do not enforce Windows ACLs. No elevated installation is performed
by the launcher. Do not run these game services as Administrator or LocalSystem.

An administrator who already has a trusted certificate may omit Caddy and run
Python directly with `--http-bind 0.0.0.0 --http-port 8443 --cert fullchain.pem
--key privkey.pem`. Omit `--proxy-secret-file` for this direct TLS mode.
For that example open TCP 8443 and use `https://play.example.com:8443` in clients.
The private key stays on the VDS. Public cleartext HTTP is
rejected. Client certificate verification is never disabled. Loopback HTTP is
allowed solely for local development.

## State, identity and operating limits

Default service options:

```text
--database data/community.sqlite3
--http-bind 127.0.0.1 --http-port 30820
--udp-bind 0.0.0.0 --udp-port 30821
--max-clients 256 --max-rooms 128
--max-players 10000 --max-db-mib 256
# Behind the supplied Caddy template, also pass:
--proxy-secret-file data/proxy.key
```

Back up SQLite with its online backup mechanism (or stop the server before
copying the database and its WAL files). Do not delete the database during an
upgrade: that loses identity associations and leaderboard history.

The client saves a separate DPAPI-protected community credential per server in
`%LOCALAPPDATA%\MotoGP0910\Online`. It is not stored in TOML, logs, crash summaries
or build archives. This credential identifies a player without touching the
offline profile/save XUID. A second simultaneous process using the same online
profile is rejected; two different Windows users/PCs have independent profiles.
Regular shutdown disconnects the ticket; after an unexpected process death the
server permits reconnect after its stale-connection interval. Do not distribute
personal profile files. Changing the server origin or Windows user creates a
separate identity; this initial version has no account recovery/transfer UI.

UDP packets carry a session identifier and HMAC-SHA256 with a secret negotiated
over HTTPS. The secret never appears in UDP. Client/server directions use
separate MAC domains; 128-packet windows allow reordering while rejecting
replays. Before binding an initial or changed UDP source address, the relay
requires an authenticated, unpredictable return-routability challenge/response.
An unverified address receives at most an 84-byte challenge in response to a
packet of at least 116 bytes; discovery and gameplay are withheld until proof.
The relay routes only between authenticated players admitted to the
same room, not to arbitrary IP addresses. UDP payloads are authenticated, not
encrypted. Connection slots are reserved before TLS negotiation. Bounded HTTP/UDP budgets,
strict binary schemas, 10,000 registered profiles, 256 replay IDs per profile and
a default 256 MiB SQLite page quota limit application resource use. HTTP storage
work has a 50 ms query deadline and a shared CPU-time budget so expensive ranking
queries cannot repeatedly block the relay. Quotas may reject requests with
429/503; they do not make legitimate and malicious public registrations
cryptographically distinguishable. SQLite WAL/logs/backups and the operating
system need their own disk headroom. These controls do not stop an attack that
saturates the VDS provider's network before packets reach this process.

## Native statistics integration

The game uses 378 native view IDs, built from 21 track slots, three classes and
six groups. The original routines at `822FEB80`, `82300088` and `822FF518` set
property `0x20000003` as the signed 64-bit rating. The game compares ratings with
**greater-is-better**, and writes timed results as negative values; some score
modes write positive ratings. Consequently all 378 views keep the greatest
rating for each player, preserving the associated columns as one result. A
slower lap cannot replace a faster lap. Duplicate operation IDs are idempotent within the most recent 256 writes for
that player. The current native schemas all use `best`, so replaying an older
result outside that bounded window still cannot worsen the stored best rating.
No `sum`/`last` schemas are enabled by the shipped configuration.
Tied ratings have a stable update-time/player tie break.

Original `XSessionWriteStats` wrapper `825DA018` uses a **24-byte** message with
XUID at `+8`, view count at `+16`, view pointer at `+20`; the SDK stub's unaligned
20-byte layout is not used. Property records are 24 bytes, view records 12.
By-rank/by-XUID wrappers `8239C808`/`8239C868` use 136-byte specifications.
Native outputs are read-results 8 bytes, views 16, rows 48, columns 24, with
big-endian values and guest-relative pointers. Column `0xFFFA` is the rank.

The game also creates a **0x707 statistics-only session** in solo play. It stays
independent of the peer race node, does not appear in the room directory and
can coexist with an active lobby. Closing it does not cancel race receives.

Online statistics writes copy native stack-backed input before returning
`IO_PENDING` and perform HTTPS on the kernel completion worker. Reads through
real kernel enumerators also use the SDK completion mechanism. Offline and LAN
keep their previous stats behavior. The custom server accepts writes only for
the authenticated client's own identity.

## Validation and remaining deployment work

Current targeted results (16 October 2026):

| Check | Result |
|---|---|
| Server behavior and adversarial regressions | 46 tests passed on Linux Python and Windows Python 3.12.8 / Proactor under Wine; includes spoofed endpoints, delayed admission, malformed HTTP, quotas, pending TLS and persistence |
| HTTPS and shipped Caddy configuration | 12 checks against each server platform; trusted/untrusted TLS, forged headers, allowlist, oversized requests and direct Python TLS |
| Windows client against each server platform | 31 protocol/identity/relay/statistics checks, repeated after restarting the same database |
| Original generated title and Windows kernel | 20 checks, including statistics-only sessions, copied stack inputs, asynchronous event completion, flush ordering and bounded native outputs |
| Existing LAN path | 96 service, 67 kernel and 20 generated-wrapper checks; automatic/direct/delayed separate-process scenarios passed |
| Actual launcher settings and EN/TR UI | 97 integration checks and 61,071 layout/input/catalog checks passed; General-page preview inspected |

No new compiler warnings were emitted by the affected Windows targets. The SDK
and verified runtime DLL hashes remain unchanged. The Windows server checks use
the official Windows Python runtime under Wine, rather than a native Windows
Server installation.

Run server checks with `python -m unittest discover -s server -p 'test_*.py' -v`
from the repository. `scripts/verify-online.py BUILD --game-root GAME` exercises
actual Windows client PEs, optionally under Wine with `--wine PATH`, against a
short-lived local service. It checks directory/relay admission, original opaque
payloads, reconnect, persistent identity, signed rating order, idempotency and
service restart. `online_guest_probe` additionally calls original generated
XDK wrappers and the pinned Windows kernel, including solo statistics sessions,
real event completion and output capacity rejection.
`scripts/verify-online-security.py --caddy /path/to/caddy` separately checks real
loopback HTTPS with a dedicated test CA/certificate and the shipped proxy
configuration, including rejection of untrusted TLS. Add `--server-python PATH`
and `--wine PATH` for the Windows backend from Linux. Both programs clean up
their temporary owned services. These tests do not certify public DNS, public
certificate issuance or the actual VDS firewall.

The VDS has not been provided and no public service has been deployed. A real
Internet race, live leaderboard UI after a real lap, and native Windows GPU
behavior still require the deployed endpoint and two players. Previous user
confirmation of LAN/Radmin racing remains valid evidence for the original LAN
path, not for the new Internet relay.
