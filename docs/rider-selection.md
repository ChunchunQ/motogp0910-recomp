# Rider selection availability

The native rider-selection menu now treats its rider-unlock query as available
in Quick Race, Arcade, Vanilla Championship, Time Trial and Local Split Screen.
Non-career multiplayer setup selects the Arcade frontend mode and shares this
selection path. Career and Split Screen Career keep their original behavior.
Unknown, tutorial, minigame, replay and startup modes are unchanged.

This changes a read-only query result, not the profile's unlock records. Native
progression, unlock descriptions, class restrictions, rewards and achievements
are not completed or written by this override. Riders still have to exist in
the currently installed base/DLC content and pass the game's existing roster
filter. Missing DLC is not supplied or entitled by this change.

## Native evidence and scope

- The frontend name table at `0x828814D0` identifies `FEMODE_CAREER` as 3,
  `FEMODE_VANILLACHAMP` as 4, `FEMODE_TIMETRIALS` as 5, and
  `FEMODE_SS_CAREER` as 10. These are separate from leaderboard race-mode IDs.
- `sub_821E4EB0` reads the frontend mode at `0x828DA060`.
- `sub_82316958` selects frontend mode 2 for non-career session setup and 3
  for its career branch.
- The rider-unlock lookup `sub_822FC460` searches type-1 entries by rider ID
  and season, returning a record whose byte at offset 32 reports availability.
- `sub_822586D8` calls that lookup with return address `0x82258A40`. At this
  site, a null record already means available. The hook returns null only at
  this caller in the whitelisted frontend modes, preserving the original
  record and all other lookup results.

This revision has been reviewed against the generated native code and included
in a local Windows build without tests. It has not been checked in native
Windows gameplay. In particular, career
progression, rider selection in each mode and mixed-DLC multiplayer require
native verification before claiming end-to-end coverage.
