# Local splitscreen input

Selecting **Multiplayer → Splitscreen** evaluates playable devices once at entry.
With two physical controllers, the first two connected controller slots become
players 1 and 2. With one physical controller and enabled gamepad-style keyboard
emulation, the controller always becomes player 1 and the keyboard becomes player 2.
The SDK's idle NOP device and USB-keyboard passthrough never count as players.

Other device combinations keep the native eligibility check and warning. No
unconditional bypass of the two-player requirement is used. Ordinary input
polling and hotplug detection continue through the pinned SDK; no extra timer or
per-frame eligibility scan is introduced.

During the local session, user 1 has a temporary, distinct local identity named
**Player 2**. It is signed in locally, has no Xbox LIVE flag and gains no community
account or network privileges. The title rejects Xbox LIVE guest accounts in
this entry check, so this identity deliberately has no LIVE guest flag either.
SDK profile reads/writes continue to reject user 1, preserving the primary
profile and saves. This does not implement persistent second-player career saves.

The two selected device ordinals stay assigned for the session. Disconnecting
one does not promote the remaining controller into the other player's slot or
switch the session between keyboard/controller and two-controller modes. The
native controller-reconnection handling remains in charge. A replacement device
can occupy its vacant SDK ordinal; controller identity after reconnect follows
the SDK's enumeration behavior.

Returning to the parent Multiplayer/Main Menu or leaving the split frontend mode
restores ordinary SDK slot assignment and the prior native local-user selection.
An entry rejected by native validation also restores that state. Held buttons
are consumed across the transition to avoid activating the next screen twice.

## Integration points

- Install `Assignment` in `OnPostSetup`, before guest input polling starts.
  It delegates to `SlotAssignment` outside a split session. Session transitions
  and SDK input access share `InputStateMutex`.
- `8239B160`, return site `821E5F38`, is the native Multiplayer action-2 guard.
  Refresh devices and the native connection/sign-in cache on this action only.
  Native `82369C38` refreshes the user manager at `834F9494`; original primary,
  secondary and owner setters preserve its bookkeeping. Never hold the input
  mutex while calling native code that polls XAM input again.
- Existing XAM sign-in/name/XUID hooks delegate only user 1 during the session
  to `HandleUserQuery`. Existing Online user-0 behavior remains unchanged.
- Native screen presentation `8235ACB0` detects parent menu vtables `8203A0C4`
  and `8200CF7C`. Mode transition `821E4EE0` tracks `FEMODE_SS_LOCAL` (9) and
  `FEMODE_SS_CAREER` (10), without adding a frontend tick hook.

This is local play only. No relay, room protocol, leaderboard or server changes
are required. The user requested a local Windows build without tests: keyboard
plus controller, two physical controllers, reconnect behavior and native warning
appearance still require a Windows gameplay check.
