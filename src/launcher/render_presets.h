#pragma once
#include <array>
#include <utility>

namespace launcher {
struct RenderPreset { int width, height, scale; };
inline constexpr std::array<RenderPreset,3> kRenderPresets{{
    {1280,720,1}, {2560,1440,2}, {3840,2160,3}}};
// MotoGP's source image is 1280x720. ReXGlue's internal scales are integers,
// so 1440p and 4K are exact 2x/3x presets; a fake fractional 1080p is omitted.
template<class Settings>
int MatchingRenderPreset(const Settings& s) {
  if((s.guest_width && s.guest_width!=1280) || (s.guest_height && s.guest_height!=720))return -1;
  for(unsigned i=0;i<kRenderPresets.size();++i)
    if(s.scale_x==kRenderPresets[i].scale && s.scale_y==kRenderPresets[i].scale)return i;
  return -1;
}
template<class Settings>
void ApplyRenderPreset(Settings& s,int index) {
  const auto& preset=kRenderPresets.at(index);
  s.scale_x=s.scale_y=preset.scale;
  s.guest_width=1280;s.guest_height=720;
}
template<class Settings>
std::pair<int,int> RenderDimensions(const Settings& s) {
  return {(s.guest_width?s.guest_width:1280)*s.scale_x,
          (s.guest_height?s.guest_height:720)*s.scale_y};
}
}
