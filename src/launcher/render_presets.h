#pragma once
#include <array>
#include <utility>

namespace launcher {
struct RenderPreset { int width, height, scale; bool upscale_1080=false; };
inline constexpr int kUpscaled1080Preset=1;
inline constexpr std::array<RenderPreset,5> kRenderPresets{{
    {1280,720,1}, {1280,720,1,true}, {2560,1440,2}, {3840,2160,3}, {5120,2880,4}}};
// MotoGP's source image is 1280x720. ReXGlue's internal scales are integers,
// so the explicitly upscaled 1080p preset retains a 720p render target.
template<class Settings>
int MatchingRenderPreset(const Settings& s) {
  if((s.guest_width && s.guest_width!=1280) || (s.guest_height && s.guest_height!=720))return -1;
  if(s.scale_x==1 && s.scale_y==1 && s.width==1920 && s.height==1080 &&
      (s.effect=="fsr" || s.effect=="fsr2" || s.effect=="fsr3") && s.aspect=="16:9")return kUpscaled1080Preset;
  for(unsigned i=0;i<kRenderPresets.size();++i)
    if(!kRenderPresets[i].upscale_1080 && s.scale_x==kRenderPresets[i].scale && s.scale_y==kRenderPresets[i].scale)return i;
  return -1;
}
template<class Settings>
void ApplyRenderPreset(Settings& s,int index) {
  const auto& preset=kRenderPresets.at(index);
  s.scale_x=s.scale_y=preset.scale;
  s.guest_width=1280;s.guest_height=720;
  if(preset.upscale_1080){
    s.width=1920;s.height=1080;s.aspect="16:9";
    if(s.effect!="fsr2" && s.effect!="fsr3")s.effect="fsr";
  }
}
template<class Settings>
std::pair<int,int> RenderDimensions(const Settings& s) {
  return {(s.guest_width?s.guest_width:1280)*s.scale_x,
          (s.guest_height?s.guest_height:720)*s.scale_y};
}
}
