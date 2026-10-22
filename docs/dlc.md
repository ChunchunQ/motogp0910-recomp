# Xbox 360 DLC import

The launcher supports the three supplied MotoGP 09/10 marketplace packages:

| Content | Native marker | Payload |
| --- | --- | --- |
| Free GP Class Season 2010 Update | `DLCPack01.meta` | 187 files |
| Twin Track Pack | `DLCPack02.meta` | 20 files, including Istanbul and Shanghai track data |
| Free 2010 Moto2 and 125 Update | `DLCPack03.meta` | 341 files |

All three inspected containers have Xbox title ID `434307F0`, marketplace content
type `00000002`, and a `LIVE` STFS header. They are **additional content**, not
title-update patches: they are not renamed to `default.xexp`, merged into
`xenondata`, or used to replace `default.xex`. The supported base game and title
update remain prerequisites. Original game/DLC files are not distributed with
the launcher.

## User flow

The single General game-files action follows **Select ISO → Select title update
→ Install DLC → Verify game files**. Once the base game and title update are
present, **Install DLC** replaces verification until all three packages are
installed. There is no separate adjacent DLC button. The source menu uses the
launcher's dark dropdown colors and highlighted rows, with hover help suppressed while
choosing a source.
The button shows installation help on hover; clicking it dismisses that tooltip.
Choose **Select archives…** for ZIP, RAR or 7z files, or **Select folders…** for
folders obtained by unpacking the archive. Both pickers support Ctrl/Shift and
mouse range selection, including selecting the three sibling package folders.
The importer searches nested folders and installs all supported packages found
in the complete selection as one staged transaction; there is no need to select
the three files separately. Duplicate package IDs across selected sources are
rejected before any installed content is published.
The current archive flow accepts the supplied package IDs and directory layout;
encrypted archives are rejected. A separately installed archiver is unnecessary.
When all three packages have valid content headers, recognition markers and the
expected file counts/sizes, the action becomes **Verify game files** and PLAY
can become available. A partial installation keeps **Install DLC**, lists the
missing packages inline and leaves PLAY disabled. No missing-DLC dialog is
opened. This structural check is cached between launcher actions and repeated
immediately before launch; it does not replace the importer's SHA-256 checks.
Direct game-executable starts enforce the same prerequisite once during startup,
log missing content and stop without an additional dialog. A successful import
also opens no information dialog; completion updates the inline status directly.
Import errors retain their existing error dialog.

The operation runs on a worker. Cancellation is available while reading and
extracting; the short publication phase disables the cancel button. The launcher
waits for cleanup if closed. Game launch, verification/ISO import, Steam shortcut
changes and launcher installation cannot start through the launcher during an
import. An external running copy of the game also blocks publication.

Content becomes visible on the next game start. The native game enumerates
marketplace packages and checks their `DLCPack0N.meta` files. Offline, LAN and
Online use the same local content store. Multiplayer participants still need
compatible installed content; the original game's compatibility checks remain.
All three supported packages are mandatory in **Offline, LAN and Online** for
this client release. This is a local installation prerequisite. The community
server does not distribute missing DLC or check/negotiate an installed-package
mask; no network fields or requests were added. Older clients remain outside
this local enforcement and are not rejected by a new server-side package check.
Native multiplayer behavior with these packages still requires gameplay review.

## Storage and preservation

The destination is the public launcher's own `dlc` folder. It is independent of
`user_data_root`, and no XUID folder is inserted between `dlc` and the title ID.
Release archives include an empty `dlc/` directory beside `game/`; no installed
DLC content is packaged. Preserve the empty directories when distributing the
application ZIP. Marketplace content is shared across local game profiles in
this installation:

```text
LAUNCHER_FOLDER/
  dlc/
    434307F0/
      00000002/PACKAGE_ID/...extracted content...
      Headers/00000002/PACKAGE_ID.header
```

The `.header` uses the pinned SDK's `XCONTENT_AGGREGATE_DATA` ABI and preserves
the package's own display name, file name and license bits. The GP update has
mask zero; the other two supplied packages have mask one. No global license
setting is forced or entitlement fabricated.

The game routes its four imported content operations (enumeration, CreateEx,
close and delete) through a portable marketplace-content adapter. Enumeration
uses normal SDK enumerator handles and opening mounts the package through the
SDK's `ContentPackage`, preserving the native VFS paths and package license.
Only marketplace content uses the portable path; saved-game content still
passes through the original SDK imports and retains its configured location.
The adapter opens installed DLC rather than creating, truncating or deleting
marketplace assets. The SDK binaries, ABI and version pins are unchanged.

Older DLC under `Documents/motogp0910/0000000000000000/434307F0` is not deleted
or installed into the portable DLC folder automatically. The default user-data
migration may also preserve a copy under `Documents/MotoGP 09-10`; that copy is
not used for marketplace content. With the game closed, move the old `434307F0` directory
into the launcher's `dlc` directory, retaining its `00000002` and `Headers`
children, or import the original packages again. The game enumerates only the
new portable DLC path. The launcher updater protects the complete `dlc` tree,
and local DLC is excluded from Git and development packages.

The importer verifies each package's size and SHA-256 before handing it to the
existing SDK STFS reader, checks its title/content type, and verifies payload
counts, total sizes and recognition markers. Paths are checked before writing.
Extraction is staged under a unique directory on the destination filesystem.
The complete selection is prepared before any package is published. Inputs are
opened for reading and are never modified. Existing game data, patches, settings,
save files and DLC packages are not overwritten.

If an installed package is byte-identical, it is retained. If its files or
metadata differ, installation stops with an explanation. Newly published paths
are journaled and removed on a cooperative cancellation or publication failure.
A process kill/power loss can leave staging or a partially published selection;
there is no claim of crash-atomic installation across multiple directories.
Only newly created paths participate in rollback.
Imports sharing the same DLC root are serialized with a named mutex so
two local launcher installations cannot publish or roll back each other's work.

## Archive dependency and validation limits

The portable reader is the pinned, unmodified official **7-Zip 26.04 x64** console
program and DLL. The launcher verifies both executable hashes before starting
the reader, lists the archive before extraction, accepts only the expected DLC
files, and extracts those files flat into staging. It never runs an executable
from the selected archive. Formats, permissions and password errors are reported
without changing installed content. Original RAR/7z package samples were not
provided; those paths use the same reader and selection rules.

Packaging includes the reader, its complete corresponding source and embedded
license notices. The launcher updater allows only those specific reader/source
resource paths. SDK and game-code pins are unchanged.

The supplied ZIP was downloaded and its three packages were extracted with the
existing inspection tool. Their metadata, payload inventories and native marker
names were checked against the recompiled content-discovery routine and pinned
SDK source. Reader hashes and x64 PE headers were verified against the official
distribution. These are artifact/source inspections. A local Windows release build was
produced on 21 October without tests. The
subsequent success-dialog removal and Turkish footer polish are also included in
the 22 October main Release build. Its later launcher checks exercise readiness
with isolated inventory fixtures; they do not perform a real DLC import or race.
See [the validation report](https://github.com/inflamesgo/motogp0910-recomp/blob/5e91cf5/docs/rider-license-validation.md). No native Windows installation/gameplay result is claimed for the
complete final source state. Imported DLC assets, including their language scripts, remain
unmodified. Native visual/gameplay review and RAR/7z sample imports remain pending.
