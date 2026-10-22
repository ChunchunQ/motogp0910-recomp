# Launcher updates

The Windows updater adapts the public FIFA09 launcher's NOBK backend protocol
and numeric version comparison to this C++/Win32 launcher. The source reference
is `ChunchunQ/fifa09-recomp`, revision
`711da3941d81879db06ea4f2c36c3a519ac826d9`, `launcher/Updater.cs`.
It includes a notification
panel, background download, an independent installation helper, rollback and a
restart acknowledgement. The current source is included in the main Windows
Release build. Wine preview checks are recorded below; a real update transaction
and native Windows installation acceptance remain unverified.

The manifest endpoint is deliberately empty in
`src/launcher/updates.h::kConfiguration`. An empty endpoint prevents all update
network requests and leaves the notification button hidden. The launcher
version constant is **1.1.11**, approved for the Rider License release,
and is separate from the game's title update version `0.0.1.1`. Historical
commits were grouped as 1.1.0–1.1.9, with the updater as milestone 1.1.10.
The Rider License release advances that milestone to 1.1.11. These groups do
not create historical Git tags or rewrite commits. Related fixes stay with
their feature group; CI and documentation alone do not increment the product
version. Configure the real delivery endpoint before activating the manifest.

## Approved launcher behavior

- Perform one update check per launcher opening. There are no periodic retries
  and clicking the notification button does not trigger another network check.
- Show the orange download icon to the left of Minimize only for a strictly newer version.
  Clicking it opens an animated panel downward over the right-side artwork.
  The panel contains the version, localized changelog and **Update now**.
  Closing the panel keeps the download icon available for that session.
- Disable **Update now** while this installation's game is running. The client
  checks both its game handle and native process enumeration. Installation also
  checks independently; it never closes the game.
- After **Update now**, show download percentage when the response provides a
  length, and allow cancellation during download. Metadata and download failures
  do not alter installed program files. Startup check failures are quiet.
- After a verified download, save the current launcher settings and prepare the
  installation without a second confirmation. Disable editing during preparation.
  Cancellation and launcher closing are unavailable during this final handoff.
- Restart the launcher after installation. Delete the previous-version backup
  after the new launcher has created/painted its real window, acknowledged the
  helper, and remained alive for a short confirmation interval. A small native
  progress window covers installation and rollback while the launcher is closed.

## Service connection and manifest

The NOBK operator must supply a documented **MotoGP-specific HTTPS manifest
endpoint**. Set it in `kConfiguration`. Following FIFA's `CheckAsync`,
`VersionRequestUrl` appends `?v=<major.minor.build>` to the endpoint (or `&v=`
when the operator's endpoint already has a query). A URL template is not needed.
Keep the version accurate
in every release. Leaving either the endpoint empty or version at the unset
placeholder `0.0.0` disables checks.

The request is GET with the same `v` query and
`User-Agent: NOBK-Launcher/<major.minor.build>` as FIFA, without
an account token or request body. A successful endpoint returns HTTP 200 and
these lowercase JSON fields:

```json
{
  "version": "1.1.11",
  "url": "https://nobkonline.com/downloads/motogp/MotoGP0910-Windows-x64-online.zip",
  "sha256": "<replace with the actual ZIP SHA-256: 64 hexadecimal characters>",
  "notes": {
    "en": "Short release notes.",
    "tr": "Kısa sürüm notları."
  }
}
```

This is a schema example, not a ready-to-install manifest. The operator must
supply the actual release version and digest. The digest covers the ZIP bytes,
not an extracted executable or an outer GitHub Actions artifact wrapper. Upload
the package before publishing matching metadata. Prefer versioned download URLs
so a package does not change between a check and its download.

Comparison accepts two to four numeric components, uses major/minor/build,
substitutes zero for an absent build, and ignores revision. Only strictly newer
versions qualify. Prefixes such as `v1.0.1` and prerelease suffixes are unsupported.
Localized notes fall back to English, then a generic no-notes message in the UI.
The reference's `CanInstall` rule requires HTTPS, a URL ending in `.zip`, and a
64-character checksum. Actual downloads also validate the digest as hexadecimal.
A query after `.zip` does not pass the reference's suffix rule.

### Observed reference service

Static REA metadata/CIL inspection identified the FIFA09 request as
`https://nobkonline.com/api/launcher/version?v=1.2.1`. A direct GET on 2026-10-19
returned version `1.2.3` and a **FIFA09** package. That endpoint must not be wired
unchanged into MotoGP. No MotoGP selector or manifest route was established.

The supplied MotoGP ZIP URL separately returned HTTP 206 and a ZIP signature for
a four-byte range request. This verifies the route at that time, not its full
contents, authoritative release version or SHA-256. The endpoint remains blank
until the service operator provides the MotoGP contract.

### Handoff and fallback mechanism

Set the MotoGP endpoint in `updates.h::kConfiguration`; requests and JSON match
FIFA's `Updater.cs`. The UI is unchanged. MotoGP additionally retains a local
**fallback mechanism** in `update_helper.cpp`: failed installation/startup
restores the previous program files. Backups are removed only after healthy
startup; game files, DLC, saves and settings are excluded. No extra server fields
are needed. The endpoint stays blank until configured by the operator.

## Installation transaction

`motogp0910-update-helper.exe` is built with the static Microsoft C++ runtime and
packaged under `bin`. Before handoff, the current helper is copied to a unique
Windows TEMP staging directory so it can replace installed binaries without
loading their DLLs. It uses the built-in Windows PowerShell 5/.NET ZIP reader
only for extraction; backup, file replacement, rollback and restart use native
C++. PowerShell is invoked by its absolute System32 path with no profile or
interactive prompt. No external archiver or .NET runtime is downloaded.

1. Verify the downloaded archive hash again in the client and helper. Extract
   into TEMP with bounds on entry count and expanded size. Reject traversing,
   duplicate, rooted, device/ADS, symlink and reparse paths.
2. Accept the standard package root or one enclosing directory. Require the
   public MotoGP launcher, game, UI, helper and runtime DLLs. Only an explicit
   list of application-owned program/resource paths can be installed; unrelated
   archive entries are ignored.
3. Validate destination paths, deny links/junctions, and reject overlaps with
   configured game/user/cache/update/metadata roots. Root configuration,
   account data, saves, logs, original game files and unrelated local files
   are excluded by the application path list.
4. Back up every changed existing file under `.motogp-update-<token>` beside the
   installed launcher and record its relative path and prior existence. All
   backups are taken and verified before any application file replacement.
5. Signal readiness to the old launcher. Wait for its explicit commit and full
   process exit, then recheck that the game is closed. A per-installation lock
   blocks unrelated launcher starts during replacement; only the helper's
   acknowledged restart is admitted.
6. Copy each new file to a scratch file on the destination volume, flush it,
   and atomically rename it over its destination, with bounded retries. Verify
   the installed file's hash. Files absent from the new package are not deleted.
7. Start the public launcher with a private session acknowledgement token. A
   launcher initialization failure, early exit or missing acknowledgement
   triggers rollback. The helper may stop only the unsuccessful launcher it
   started; it never stops the game. PLAY stays disabled during confirmation.
8. On a handled installation failure, restore changed files in reverse order
   and remove files introduced by this transaction. Restart the previous
   launcher after a completed rollback. If restoration cannot finish, preserve
   the backup and show an error instead of deleting the recovery files.

No elevation is requested. Installation requires a writable portable application
folder and sufficient space for the payload, previous files and scratch copy.
A restricted folder or unavailable PowerShell fails preflight while the current
launcher is still open. Following success, the helper attempts to remove the
backup and large staging files. Its own running executable can remain in TEMP.
An OS interruption or forced helper termination is outside the automatic
handled-error rollback path; an unfinished backup index remains available for
manual recovery. The startup acknowledgement verifies launcher initialization,
not a game playthrough or GPU compatibility.

## Source layout and validation

- `updates.h` / `updates.cpp`: configuration, manifest parsing, comparison,
  WinHTTP download, limits, cancellation and SHA-256 validation.
- `update_panel.inc`: launcher state, download icon/panel, animation and worker polling.
- `update_install.cpp`: preflight process, explicit commit and startup handshake.
- `update_helper.cpp`: extraction, package validation, backup/replacement,
  rollback and restart.
- `update_protocol.h`: shared names, argument quoting, process checks and hashes.
- CMake and `scripts/package-windows.py`: build and ship the helper. The public
  launcher target depends on it, including explicit launcher-only builds.

Network limits are 128 KiB per manifest and 512 MiB per ZIP. Extraction allows
at most 4,096 entries, 1 GiB per file and 2 GiB total. HTTPS certificate validation
remains enabled; HTTPS-to-HTTP redirects are denied. Metadata uses an eight-second
deadline and downloads ten minutes, checked between blocking WinHTTP calls.
Cancellation is cooperative and does not instantly interrupt an in-flight call.

The reference launcher and installer were not executed. The Windows release
cross-build and **13 targeted Windows/Wine update-preview UI checks** passed
on 20 October 2026. End-to-end download, installation and rollback have not
been executed. Before enabling a live endpoint, validate a
successful update plus cancellation, malformed/wrong-product packages, locked
files, failed restart and rollback in a disposable installation. Verify the
helper's static-runtime imports and actual UI layout on native Windows.

## Visual preview

The panel is 390 × 360 logical pixels, widened to the left over the cover art.
The changelog has 240 pixels of height, and Update now sits at the lower edge.
Its scrollbar uses the launcher's existing slim rail geometry, colors and drag
handling. The native EDIT scrollbar is omitted; wheel, keyboard and rail input
scroll the wrapped text. During download, Cancel shares the bottom action row.
This layout revision was initially built without UI tests. The later main
launcher viewport audit includes the current source; successful live-service
updates and native visual review remain separate from that coverage.

The close control now uses a 32 × 32 logical-pixel button with a larger,
antialiased vector X, a hand cursor, a translucent hover/pressed background and
keyboard focus. This polish is included in the later main Windows Release build and launcher
viewport audit. Those checks do not validate end-to-end update installation or
native Windows transition smoothness; see [local validation](https://github.com/inflamesgo/motogp0910-recomp/blob/5e91cf5/docs/rider-license-validation.md).

Run `MotoGP09-10Launcher.exe --preview-update` to show a synthetic v1.1.11
notification with English/Turkish sample release notes. Click the download icon
to expand the normal update panel. Update now shows an informational preview
message; this mode does not check an update endpoint, download a package or
start installation. Normal launches retain the configured update behavior and
do not show the synthetic notification. The sample is not a published release.
