# Collision record endian repair

Reported symptom: the rider and camera fall below the Qatar track in Time
Trial, usually within the first ten corners, at both 720p and 1440p and at
60 FPS or higher. Enabling gpu_allow_invalid_fetch_constants removed the
texture-fetch warnings in the supplied log but did not change the fall.
That GPU option has not been made a default by this change.

A separate, reproducible data-conversion defect was found in the real game
functions sub_823559C8 (76-byte triangle records) and sub_82355AA8 (40-byte
shape records), called by the track loader sub_82355B40.

Collision records contain serialized little-endian floats. Before their
bytes are reversed, their bit patterns can represent signaling NaNs.
The generated lfs-to-double/stfs conversion quiets these NaNs, changing
0x00400000 in the temporary value, or 0x00004000 in the decoded coordinate.
For example, an intended finite float with bits 0x43AB807F is read as
0x7F80AB43, quieted and ultimately stored as 0x43ABC07F. The resulting
coordinate is 0.5 units larger. This is data corruption during decoding,
not an intentional change to physics or a render-resolution setting.

The hooks call the original routines and preserve their return values,
integer/index fields and guest call state. They repair only floating-field
stores using byte reversal of the original serialized integer bits.
No compressed game assets or original XEX/TU files are changed. ReXGlue
remains pinned to v0.10.0 and the matched runtime DLL pair is unchanged.

The optional physics_asset_probe loads the actual patched 0.0.1.1 XEX and
calls the real registered conversion functions. Eight finite float patterns
exercise both formats, including cases whose serialized bytes look like
NaNs. It checks all decoded float fields, complete record bytes, integer
fields, return values, stack, LR and nonvolatile registers.

- Patched: 192/192 checks passed on Linux and Windows PE under Wine.
- Original Windows routines, invoked with --original: 54 float-field and
  6 complete-record mismatches reproduced (expected exit code 1).
- Local Windows confirmation: the user reported 4–5 Qatar Time Trial laps
  without falling below the track with the collision-data-fix build. The
  supplied log identifies a GTX 1650 SUPER, patch 0.0.1.1 and repair activity
  during actual asset loading. The cumulative counters reached at least
  1024 triangle records and 64 shape records. These are repaired conversion
  counts across loads, not necessarily distinct records; rate-limited logs
  do not give the final exact total. The run ended through normal window
  closure. This validates the reported route in that test; other tracks and
  modes have not been verified by this report.

Build optional target physics_asset_probe, then run it with the prepared
XEX directory. Adding --original deliberately bypasses the hooks and is
expected to fail for the problematic serialized values.

For local Qatar testing, replace the EXE with the supplied Windows package,
keep the current game folder/configuration and repeat the original route.
Repair activity appears in the log as PHYSICS_ASSET_FIX. A clean diagnostic
run can use the separately tested graphics option to avoid texture warnings:

```powershell
.\motogp0910.exe --gpu_allow_invalid_fetch_constants=true --log_file=logs\qatar-collision-test.log
```

If the fall persists, this log will help establish whether repaired records
were encountered during the actual track load; it does not yet log vehicle
positions or contact-query results.
