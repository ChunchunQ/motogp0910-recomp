# PC quit menu — 15 October 2026

The original main menu now has a seventh **QUIT GAME** entry. The original six
choices and their ID mapping remain unchanged. The new row uses the title's
existing menu renderer and navigation; it isn't a floating host button.

## Current repair — boot Press Start path, 15 October 2026

The user correctly reported that Back/B still failed after the preceding build.
The previous probe covered the secondary frontend title, not the real boot
Press Start path. Native boot draw **8229C848** calls **8229C178** at **8229CA20**
with **r4=0** (return address **8229CA24**), in mode **11**. The old guard wrongly
classified flag 0 as in-game and rejected it. The other native call site is the
secondary frontend title at **822AE948** (return **822AE94C**), flag 1 / mode 15.

Match these exact callers and modes. The boot path has no alpha>=1 requirement;
its flag-0 branch does not consume that fade argument. Poll both original boot
update **8229C2C0** and secondary frontend update **822AE9F8**. A fresh Back edge
after an observed eligible draw requests the existing confirmation and blocks
the original title update until dismissal. Unknown callers don't arm exit.
Initial intro screens without an observed Press Start draw remain unaffected.

The 78-check probe now includes the real boot call-site instructions, flag-0 /
mode-11 observation, actual Backspace-to-B mapping, boot hook, held-input and
cancel behavior. Native Windows retesting remains necessary; do not equate the
probe with a completed native playthrough. Bounded PC_QUIT_TITLE_DRAW / BACK log
entries record the actual call site, mode, focus and input arming for diagnosis.
Confirmation language is loaded before constructing the dialog.

The blue QUIT GAME accent color remains a **separate pending preference**.
The user has been asked for a color name/HEX value; do not choose it for them.
Uppercase QUIT GAME is retained.

## Previous attempted repair — 15 October 2026

The user reported that Backspace/controller B did not open the initial Press
Start confirmation. The earlier 72-check probe called the boot update directly,
so it did not establish which callback actually ran on that screen. That attempt identified **822AE9F8**, installed by the final boot transition,
as the live frontend update and moved the hook there, assuming **8229C2C0**
only covered boot movies. That assumption was incomplete: the latter also
updates the real boot Press Start screen. The current repair above hooks both
callbacks and checks their actual draw call sites. The new label is **QUIT GAME**.
The subsequent user report exposed the additional boot-path guard error fixed above.

## Implemented behavior and intended controls

On the initial Press Start screen, press the mapped B/back action once to ask
whether to quit. This supports Xbox B, PlayStation Circle and the user's current
keyboard B binding (Backspace by default). It does not add this behavior to the
in-race Press Start overlay.

Both paths open a dark/orange confirmation, defaulting to Cancel. Use D-pad or
left stick to change the choice, A/Cross to select, B/Circle to cancel. Keyboard
Left/Right/Tab changes selection; Enter selects; Esc/Backspace cancels. Mouse
buttons also work. Existing keyboard mappings continue to supply virtual A/B.
English/Turkish confirmation labels follow the launcher language at startup;
the added native main-menu caption is "QUIT GAME".

Opening presses must be released before controller confirmation is armed.
SDK input blocking prevents the game receiving modal actions and masks held
buttons until release after cancellation. Device polling stays on the guest
frontend thread; the ImGui thread reads atomic snapshots. Focus changes reset
the confirmation choice/arming, preventing held input on focus regain from
requesting or confirming exit. Shift+Arrow can reach both keyboard and virtual
D-pad input; those channels aren't applied twice to the choice.

Confirmed exit is deferred to the host UI thread, saves the existing SDK
achievement state, stops the LAN backend, and calls Window::RequestClose.
That is the SDK's normal window-close path, including its existing title
termination / exit-0 behavior. This does not invent a new career-save operation
or rewrite the user's profile. Native profile/loading transition flags block
the new menu action while those main-menu transitions are in progress.

## Native integration

All addresses are for the enforced TU 0.0.1.1 executable.

- MainMenu RTTI resolves its vtable at `8200CF7C`. The original constructor
  `821FC4F8` initializes six IDs plus a sentinel at object offsets `20..38`.
- `821FC650` builds the descriptors at `this+3C`, stride 40 bytes. After this
  original setup, preserve its empty terminator in the next available slot,
  copy the Options entry's presentation fields, and supply our stable guest
  UTF-8 label. This fits before the object's transition fields at `244`.
  The original IDs and six descriptors are preserved; text pointer publication
  follows descriptor/terminator initialization.
- `821FCCA8` is the native main-menu event handler. Its selected index comes
  from screen state `+18`; accept bits are `400` / `2000`. Consume only the
  seventh entry, keeping the original handler for other choices and Back.
- The secondary title renderer `822AE390` checks the property pointer at
  `8292710C` for mode 15 before calling `8229C178` with flag 1. Require full fade
  for this exact caller. Flag 0 alone does not identify an in-game overlay;
  the real boot title also uses flag 0, with a different caller and mode.
- Startup initialization `8229CA58` installs boot update `8229C2C0` and draw
  `8229C848`. Observe its actual flag-0 call in mode 11 and poll the boot update.
  The final boot transition installs secondary update `822AE9F8` at
  `8229C630` / `8229C640`. Both updates share the same fresh-edge/modal gate;
  skip the original active update while confirmation owns input.
- When the F1 input gate is active, do not poll MnK bindings from the quit hook;
  F1 may be editing those strings on the UI thread. Require neutral input again
  after the other overlay releases ownership.

No original disk files, message banks or generated source files are modified.
The label's guest allocation remains alive with the runtime so rendered batches
cannot retain a freed text pointer. Runtime DLL hashes and LAN protocol remain
unchanged.

## Validation

**78 targeted Windows/Wine checks passed**, using the real patched title and
runtime. They cover descriptor/ID preservation and idempotence; original native
navigation reaching the seventh row and wrapping in both directions; ABI
preservation; every label character and its terminator through the original
UTF-8 decoder 826EC558; native accept masks and profile transition guards; title-mode,
fade and in-game exclusions; actual main/title input hooks, actual Backspace-to-B mapping and native frontend callback-registration instructions; real SDK input
blockers and dismissal masking; held input/focus safety; real ImGui confirmation
and cancellation; and one exit request per explicit confirmation.

Two timing regressions first failed against the earlier candidate: UI-first
cancellation reopening a held-B prompt, and virtual D-pad input arriving one
frame after its physical keyboard arrow. Both pass after the release gate and
cross-frame direction ownership fixes.

The achievement probe's **286 checks also passed** against the same runtime.
The new probe is included in windows_validation_probes and verify-windows.py.
Game saves, user preferences and original assets aren't test fixtures; checks
run in isolated output folders. Raw diagnostics remain under ignored out/.

```powershell
cmake --build --preset win-amd64-release --target quit_menu_probe --parallel 2
.\out\build\win-amd64-release\quit_menu_probe.exe `
  'D:\Games\MotoGP0910\game' .\third_party\open-sans\OpenSans-Semibold.ttf
```

These checks don't establish the final appearance of the seven-row game menu
on a native Windows GPU or a complete user-profile save/exit playthrough.
Native Windows visual/exit validation remains a local test. Extract the complete
Windows ZIP; preserve the existing game directory, saves and configuration.
