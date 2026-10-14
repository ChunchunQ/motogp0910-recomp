#pragma once
#include <rex/cvar.h>

inline void ApplyMotogpVideoSettings() {
  const auto effect=rex::cvar::Query<std::string>("present_effect");
  const bool wide=rex::cvar::GetFlagInfo("aspect_ratio") && rex::cvar::Query<std::string>("aspect_ratio")!="16:9";
  if(effect!="fsr" && effect!="fsr2" && effect!="fsr3" && !wide)return;
  // ReXGlue otherwise derives the guest video mode from a custom host window
  // size. Keep a native 720p source for spatial upscaling on direct EXE launch,
  // too, while preserving explicitly configured guest modes.
  for(auto [name,value]:{std::pair{"video_mode_width","1280"},
                        std::pair{"video_mode_height","720"}}) {
    if(rex::cvar::GetFlagSource(name)==rex::cvar::Source::kDefault)
      rex::cvar::SetFlagByName(name,value);
  }
}
