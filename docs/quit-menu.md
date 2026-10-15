# PC quit menu — 15 October 2026

The original main menu now has a seventh **QUIT GAME** entry. The original six
choices and their ID mapping remain unchanged. The new row uses the title's
existing menu renderer and navigation; it isn't a floating host button.

## User-reported follow-up — repaired, 15 October 2026

The user reported that Backspace/controller B did not open the initial Press
Start confirmation. The earlier 72-check probe called the boot update directly,
so it did not establish which callback actually ran on that screen. The live
frontend update is **822AE9F8**, installed by the final boot transition; the
previous hook **8229C2C0** only runs the boot movies. Move the hook to the live
callback, retain title-mode/fade/fresh-press gates, and add a check of the real
patched callback-registration instructions. The new label is **QUIT GAME**.
Native Windows confirmation of the repaired path remains pending.

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
- The title renderer `822AE390` checks the property pointer at `8292710C` for mode 15
  before calling `8229C178` with startup flag 1. Observe only this startup path
  at full fade. The in-game path calls the same function with flag 0.
- Startup initialization `8229CA58` installs boot update `8229C2C0` and draw
  `8229C848`. The final boot transition constructs update pointer `822AE9F8`
  at `8229C630` / `8229C640` and installs the real frontend callbacks. Hook
  **822AE9F8**; while confirmation is open, skip its original input processing.
- When the F4 input gate is active, do not poll MnK bindings from the quit hook;
  F4 may be editing those strings on the UI thread. Require neutral input again
  after the other overlay releases ownership.

No original disk files, message banks or generated source files are modified.
The label's guest allocation remains alive with the runtime so rendered batches
cannot retain a freed text pointer. Runtime DLL hashes and LAN protocol remain
unchanged.

## Validation

**75 targeted Windows/Wine checks passed**, using the real patched title and
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
