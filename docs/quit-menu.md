# Native PC quit confirmation

The seventh main-menu entry, **QUIT GAME**, and Back on the initial Press Start
screen now use the title's own Yes/No dialog. The previous ImGui confirmation is
removed. No announcer voice is added: the user rejected the tutorial-word sample,
and a suitable standalone menu announcement has not been found.

The dialog opens with **Yes** selected, as requested by the user. It uses the game's existing localized
quit question, explicit Yes/No labels, renderer, fades and mapped confirm/back controls.
It does not introduce mouse buttons or host-only Enter/Escape shortcuts; the
configured virtual gamepad mappings continue to work. The caption follows game
text language, including **OYUNDAN ÇIK** for Turkish.

Release the opening button before the dialog opens. Focus changes disarm input
and reset the choice to No. Cancellation consumes held dismissal buttons before
returning control to the underlying menu. Other native dialogs and their closing
animations are allowed to finish before opening this one.

Yes defers the existing close handler to the host UI thread, saves SDK achievement
state, stops the LAN backend and requests the normal window-close path. It does
not invent a career-save operation or rewrite the user's profile.

## Native integration

All addresses refer to the supported TU 0.0.1.1 executable. Original game files,
message banks, generated source and runtime binaries remain unchanged.

- Main menu vtable: `8200CF7C`. Hook `821FC650` appends row 6 at `menu+3C`,
  stride 40, preserving the original six descriptors, IDs and empty terminator.
  The extra row fits before the native transition fields at `244`.
- Event handler `821FCCA8` consumes only that row's native accept events
  (`400` / `2000`), subject to the existing profile/loading transition guards.
- Resolve message 1291, handle `8288DB18`, through the original localized lookup
  `826EDB38`. Call `82356E40` on modal layer 4 with flags `0D` for Yes/No,
  Back cancellation. Supply messages 97/98 explicitly for Yes/No; the native
  fallback labels are Accept/Cancel. After setup, set the copied selection and
  animated position to index 0 (Yes). Keep description/options storage in guest
  memory and use an owned dispatcher thunk rather than another game callback.
- The real screen manager passes the copied payload in r3 and the result in r4:
  zero confirms; No and negative Back results cancel. Validate an ownership
  cookie and schedule close at most once.
- Initial boot title is application phase `828DA060 == 11`, identified by draw
  caller `8229CA24`, flag 0. Its update is `8229C2C0`. The secondary frontend
  path uses property `8292710C` with value 15, draw caller `822AE94C`, flag 1
  and full fade; its update is `822AE9F8`. These are distinct native states.
- Keep the frontend hooks pending while the screen manager owns the modal.
  Input polling remains on the guest thread, sharing `InputStateMutex` with
  F1 binding edits and other application-owned input consumers.

## White main-menu accent

The QUIT GAME row retains the accepted white cap and stripe. Draw overrides are
restricted to main-menu row 6 and the exact original strip/cap call sites.
Private descriptor/material views sample the original blue brightness channel
for RGB while retaining alpha, geometry, tiling and fades. Gray background passes
and all other rows retain their original materials. Guest allocations live with
the runtime so queued batches cannot retain freed resources.

## Validation status

The older ImGui confirmation and Press Start guard had previous Windows/Wine
checks and user feedback. Those results do not validate this replacement.
`tools/quit_menu_probe.cpp` now observes native service arguments, ownership,
focus/release safety and Yes/No/Back callbacks, retaining boot/frontend regression
cases. Its fixture does not establish the native renderer's appearance.

For this change the user requested a local Windows build without tests. No probes
or gameplay tests are run. Native Windows visual review, physical-controller
navigation and a complete save/exit playthrough remain unverified.
