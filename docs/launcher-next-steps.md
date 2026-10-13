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

Remaining, in increasing complexity:
1. Fullscreen / borderless / windowed, real monitor Hz and a separate FPS cap.
   The user explicitly wants Hz and FPS as separate settings.
2. Aspect ratio: auto / 4:3 / 16:9 / 21:9 / 32:9. Clarify camera/FOV versus
   presentation-only behavior before implementing if uncertain.
3. An Advanced menu with actual MotoGP engine settings. The supplied Downpour
   example includes motion blur, bloom, ambient occlusion, light shafts,
   distortion, lens flares, fog, radial blur, shadow quality/resolution,
   anisotropy, detail/LOD, decals and dynamic lights. MotoGP is not UE3;
   only verified equivalent controls should be offered.
4. FSR 2/3 upscaling: verify the temporal backend and required title inputs.
5. Frame generation, if feasible, as an independent checkbox, default off.
   Upscaling must work with frame generation unchecked. The current release
   implements spatial FSR 1 and CAS; it does not claim FSR 2/3 or frame generation.

Keep ReXGlue v0.10.0 pinned and preserve the career, collision and PS3/replay
repairs. Generated filenames remain English. Do not commit preview screenshots.
