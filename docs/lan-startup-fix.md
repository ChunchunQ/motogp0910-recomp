# LAN launcher startup fix — 14 October 2026

The reported game log ended immediately after the title update was applied;
the launcher reported exit code **3221226505 (0xC0000409)**. That status alone
does not identify a particular stack-buffer error: an unhandled C++ exception
can also terminate the application through this fast-fail path.

The actual packaged SDK command-line parser reproduced the argument corruption:

```text
--lan_bind_ip= --lan_enabled=true
    lan_bind_ip becomes "--lan_enabled=true"
--lan_host_ip= --lan_player_name=inflames
    lan_host_ip becomes "--lan_player_name=inflames"
```

CLI11 consumes the following option as the value of the empty string option.
The saved TOML can still enable LAN, so the adapter constructor subsequently
rejects that malformed IP. `OnPostSetup` previously allowed that exception to
escape, before the `LAN_LOBBY` ready message was written.

The launcher now omits empty string command-line options. It already saves
all managed settings to `motogp0910.toml` **before** creating the game process;
empty Auto IP fields and intentionally unbound keys load from that file. Nonempty
values and other command-line overrides retain their priority. No SDK/DLL change
is required. LAN startup publishes its bridge only after both the node and socket
bridge have been constructed. If initialization fails, the app logs
`LAN_STARTUP_FAILED`, displays the actual error, cleans up and explicitly informs
the user that play continues in **offline mode**; it does not claim LAN is active.

## Targeted validation

Windows x64 `lan_import_probe.exe` ran with the unchanged packaged runtime under
Wine. `startup-auto` constructs arguments with the **production launcher helper**,
parses them with the **actual SDK parser**, loads the saved TOML, sets up the real
kernel, loads the supplied TU XEX, and starts the production LAN bridge.

**17 checks passed**, covering enabled LAN, both Auto IPs, the reported player and
room names, intentional unbinding, real XEX loading, malformed-IP rejection with
original offline imports restored, and actual generated session create/delete.
Three successive automatic startup/shutdown cycles also completed. The earlier
10 baseline generated-wrapper checks passed before the fix during investigation.

```powershell
lan_import_probe.exe "D:\your-game\game" startup-auto
```

These are API/startup checks using the cloud's loopback fallback. They do not
verify Windows GPU presentation, native physical/Radmin adapter enumeration,
rendered Player Match menus or two-PC racing. The user still supplies that
verification. The new ZIP contains both updated EXEs and the matching runtime;
keep existing game data, settings and saves. Neither a user log nor screenshot is
committed to the repository.
