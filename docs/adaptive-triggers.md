# DualSense adaptive triggers — 15 October 2026

Experimental native DualSense / DualSense Edge resistance profiles are available
in **Launcher → Controls → DualSense adaptive triggers…** and **F1 → Controls**.
The master switch defaults to **off**. The controller backend must be **SDL**.
This does not require, or change, the selected button artwork.

## What Downpour actually does

The public Downpour SDK implements this in its SDL input driver, not a guest
Xbox trigger API. It sends a 47-byte SDL PS5 effect payload at controller
connection. Right and left 11-byte slots begin at offsets 10 and 21. SDL handles
USB report IDs, Bluetooth framing and CRC. The title README calls this
“Level-1” support. Its nine launcher parameters control ambient/static effects;
there is no connection to game weapons, speed, RPM or gear changes in that code.

Sources inspected before implementation:

- [Downpour SDK SDL implementation, pinned revision](https://github.com/LittleBitUA/rexglue-sdk-dpour/blob/5497095131e8c6174ab698c5ea1ee4a60c0e395f/src/input/sdl/sdl_input_driver.cpp#L28)
  (options and effect encoder; connection application starts at line 576).
- [Downpour controller support description](https://github.com/LittleBitUA/DownpourRecomp/blob/a183ff0b21f5e504526102754a04f2a9534a97b7/README.md).
- [Our pinned SDL PS5 driver](https://github.com/libsdl-org/SDL/blob/147a8ee32dbf9ac02f3794964490687b6bbda1bc/src/joystick/hidapi/SDL_hidapi_ps5.c):
  `DS5EffectsState_t`, `HIDAPI_DriverPS5_InternalSendJoystickEffect`.
- [DualSense Windows legacy effect layout](https://github.com/Ohjurot/DualSense-Windows/blob/main/VS19_Solution/DualSenseWindows/include/DualSenseWindows/DS5State.h)
  and [independent resistance percentage API](https://github.com/BadMagic100/DualSenseAPI/blob/a121e53ed806ec8ecacd92741ff048c3ce7e585b/DualSenseAPI/AdaptiveTrigger.cs).

MotoGP uses an independent small implementation of this protocol approach;
Downpour's SDK is not substituted for our pinned ReXGlue version. Legacy effect
modes use full-byte position/force values. Our 0–9 position and 0–8 strength
controls are scaled to those byte ranges, rather than sending small UI steps
unchanged. No additional HID library, service or controller middleware is added.

## Options and behavior

| Option | Values |
|---|---|
| Master | Off / On |
| Right and left effect | Off; Feedback (constant resistance); Weapon (resistance section) |
| Start position | 0–9: released to fully pressed |
| End position | 0–9; Weapon requires end greater than start; unused in Feedback |
| Strength | 0–8; zero releases that side |

Initial profiles match Downpour's screenshot: R2 Weapon 3 / 6 / 5 and L2 Feedback
2 / 0 / 4. They have no physical effect until enabled. “Weapon” is the effect
name, not a claim of MotoGP game-state integration. Vibration/calibration modes
are not exposed.

The launcher dialog edits pending values; **Apply** accepts them, **Cancel**
preserves the previous values. **Save settings / PLAY** persists them. F1 updates
profiles live and can save the same keys back to `motogp0910.toml` without
restarting. On an XInput backend these live controls are hidden; choose SDL in
the launcher before starting, since changing backend is not a live operation. Full-profile revert avoids invalid intermediate start/end ranges.

The service uses the runtime's SDL instance through an additive, versioned C
export. The shared input V1 table and SDK pin stay unchanged. It only targets
SDL devices identified as PS5, including Edge; Xbox, 8BitDo and DualShock 3/4
never receive these reports. Forcing PlayStation artwork does not enable them.
There is no analog input remapping or physics modification.

A 100 ms worker observes settings and hotplug changes. It sends only changed
reports or a new connection, not every game frame. Turning a side/master off,
losing window focus, changing away from SDL or normal shutdown explicitly sends
an off effect. Both trigger enable bits remain set on reset: omitting them would
leave the controller's previous resistance active. Focus return/reconnection
reapplies the current profile. Rejected reports show a status message and are
not retried/logged continuously until settings, focus or connection changes.
Rumble, light, speaker and microphone flags are left unset.

## Hardware verification boundary

The user owns an **8BitDo Ultimate**, which cannot reproduce DualSense trigger
resistance. USB and Bluetooth framing are supplied by SDL's existing driver;
neither transport's physical resistance has been tested here. Steam Input,
DS4Windows or device hiding can expose an Xbox device instead of the native pad.
Use native SDL access, initially over USB, when a DualSense tester is available.
The F1 status reports SDL acceptance/failure, not proof of physical feel.
Abrupt process termination cannot run the normal release path.

## Automated evidence

The actual Windows EXE and custom runtime were built and exercised under Wine.
`adaptive_triggers_probe` uses the production service and real runtime SDL
virtual DualSense, Edge, Xbox, DS4 and 8BitDo devices, with effect callbacks. It
checks reference packet bytes, full-range scaling, zero/invalid/off behavior,
independent sides, live changes, focus, disable, shutdown, reconnection, rejected
writes and no reports to unsupported devices. This verifies software/transport,
not physical motors or native Windows GPU rendering.

Run the targeted Windows probes with `scripts/verify-windows.py`; build
`adaptive_triggers_probe`, `in_game_settings_probe`, `quit_menu_probe`,
`launcher_layout_probe`, `button_prompt_window_probe` and
`runtime_stability_probe` first. Probe logs and UI previews stay under ignored
`out/adaptive-triggers/`; no user screenshots or raw game banks are committed.

Current targeted results: **200 trigger software/transport checks**, **345 live
settings checks**, **109 quit checks**, **171 real window/input transitions**,
**168 runtime safety checks** (including 8,000 concurrent thread IDs), and
**60,132 launcher UI checks**, plus all five input-default source scenarios.
Launcher figures include existing caption hit-test/layout coverage; they are
not a physical controller benchmark. English/Turkish modal previews were
inspected locally and are not included in Git or the delivery archive.
