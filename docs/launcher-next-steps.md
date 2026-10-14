# Launcher requests — 14 October 2026

The user asked to work from simple to complex, show each finished stage before
moving on, and ask before implementing any unclear requirement.

Completed:
- Internal resolution labels: 1x - 1280 x 720 (native), 2x - 2560 x 1440,
  3x - 3840 x 2160.
- General page: game folder visible only here; Launcher language moved from
  the top-right corner; Game language below it; Button prompts below that.
- Prompt choice: Automatic / PlayStation 3 / Xbox 360 / Keyboard.
- Game language: English / French / German / Spanish / Italian, independently
  of English/Turkish launcher language.
- Output-size list capped to native monitor dimensions, independently of the
  internal rendering resolution.

- Fullscreen / borderless / windowed, real host fullscreen Hz and an independent
  optional FPS cap. Windows/Wine window and configuration tests passed.

- Advanced page with actual title hooks: speed blur, depth of field, dynamic
  shadows (game default/off), and scoped shadow distance (default/45/90/180).

Remaining, in increasing complexity:
2. Aspect ratio: auto / 4:3 / 16:9 / 21:9 / 32:9. Clarify camera/FOV versus
   presentation-only behavior before implementing if uncertain.
   Confirmed: preserve Auto / 16:9 now; test real ultrawide at the end of the queue.
3. Further Advanced controls beyond the four completed fields. The supplied Downpour
   example includes motion blur, bloom, ambient occlusion, light shafts,
   distortion, lens flares, fog, radial blur, shadow quality/resolution,
   anisotropy, detail/LOD, decals and dynamic lights. MotoGP is not UE3;
   only verified equivalent controls should be offered.
4. FSR 2/3 upscaling suspended pending the user’s preference. v0.10.0 binds
   the color resource as both depth and motion inputs, uses zero jitter, and
   resets history every frame. This is not a genuine temporal integration.
   The user was asked whether to research proper temporal inputs or provide an
   explicitly experimental build. FSR 1/CAS remains the current working path.
5. Frame generation dropped: v0.10.0 explicitly disables its FidelityFX
   frame-generation provider and the presenter has no frame-interpolation path.
   The user asked to abandon this if no working integration is available.
   The current release implements spatial FSR 1 and CAS; it does not claim FSR 2/3.

While the user is away, suspend ambiguous work, continue independent tasks,
and perform a consolidated verification at the end.

Keep ReXGlue v0.10.0 pinned and preserve the career, collision and PS3/replay
repairs. Generated filenames remain English. Do not commit preview screenshots.
