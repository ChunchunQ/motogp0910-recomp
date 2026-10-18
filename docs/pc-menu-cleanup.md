# PC frontend menu cleanup

The native multiplayer mode selector displays **Online** and omits Xbox LIVE
Party. Splitscreen and Leaderboards retain their original action IDs. The pinned
SDK's Party UI returns a failure; no community party implementation replaces it.
The Game Settings storage selector is omitted because the SDK always returns
the same virtual storage device. Existing save paths and save data are preserved.

Create Match and Custom Match omit Premium Content. Their native action-order
arrays are compacted, preserving AI and Private Slots rather than hiding only
the label. The backing premium selection uses its first choice: Off for hosting
and Any for searching. Console DLC ownership is not unlocked or emulated.

All edits are guarded by the supported title's native vtables. Unknown layouts,
duplicate IDs and missing terminators are rejected before any list is changed.
Existing focus follows its original action ID when rows move; removed selections
clamp to a remaining row. Original disc files and message banks are not rewritten.

| Native menu | Vtable | Removed native action ID |
| --- | --- | --- |
| Multiplayer modes | `0x8203A0C4` | `1` (Party) |
| Game Settings | `0x820338B4` | `5` (storage selector) |
| Create Match | `0x82035884`, `0x8203C464` | `7` (Premium Content) |
| Custom Match | `0x82035E0C` | `7` (Premium Content) |

The scoped Online override targets message handle `0x8288D258` (ID 731).
Other Xbox LIVE error/help messages retain their original meaning. Offline,
LAN and community Online implementation and service requirements are unchanged.

Keyboard action descriptions use the title's actual Default layout constructor
`0x822F0AD0`, rather than Alternative A at `0x822F0B78`. A means Gear up and B
means Gear down; X is Rear brake, Y is Celebration, LT is Front brake, RT is
Accelerate, LB is Tuck in and RB is Change view. Native diagram labels distinguish
stick presses from full two-axis sticks. Camera look-around remains separate
from the rider action mapping. The game can still select its alternative layouts.

The Windows PE CPU checks load the real supported XEX, verify the guarded
vtables and Default constructor, check Online lookup, compact each native menu,
preserve focus/action IDs and exercise real native navigation across the new
terminators. These checks do not replace native Windows GPU screenshots or
an Internet race between physical PCs.
