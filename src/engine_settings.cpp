#include "engine_settings.h"
#include "engine_settings_options.h"
#include <rex/hook.h>
#include <mutex>
#include <charconv>

REX_EXTERN(__imp__sub_82386158);
REX_EXTERN(__imp__sub_822ADAC0);
REX_EXTERN(__imp__sub_8238CFD8);
REX_EXTERN(__imp__sub_8237FA00);
REX_EXTERN(__imp__sub_82385428);
REX_EXTERN(__imp__sub_8237B370);
REX_EXTERN(__imp__sub_82273780);
REX_EXTERN(__imp__sub_82274570);
REX_EXTERN(__imp__sub_82274838);
REX_EXTERN(__imp__sub_823908C0);

namespace {
std::recursive_mutex shadow_settings_mutex,particle_settings_mutex,post_settings_mutex,lod_settings_mutex;
int ChoiceValue(const char* key){
  const auto value=rex::cvar::Query<std::string>(key);
  if(value=="game")return -1;if(value=="off")return 0;if(value=="on")return 1;
  int number=-1;const auto result=std::from_chars(value.data(),value.data()+value.size(),number);
  return result.ec==std::errc{} && result.ptr==value.data()+value.size()?number:-1;
}
void RunParticles(PPCContext& ctx,uint8_t* base,void(*native)(PPCContext&,uint8_t*)){
  const std::lock_guard lock(particle_settings_mutex);
  const auto density=ChoiceValue("engine_particle_density");
  motogp::engine::PropertyOverride override(base,motogp::engine::kParticleDensity,
      static_cast<uint32_t>(density),false,density>=0);
  native(ctx,base);
}
}

// The camera/settings copier runs immediately before the actual FSFX renderer.
// Preserve native camera parameters and the shared speed-FX enable byte.
// Blur and vignette have independent parameters at their consuming boundary.
REX_HOOK_RAW(sub_82386158){
  __imp__sub_82386158(ctx,base);
  if(rex::cvar::Query<std::string>("engine_depth_of_field")=="off")base[motogp::engine::kDofSettings]=0;
}
// The outer render path uses Shadows/Enabled to decide whether to invoke the
// dynamic-shadow renderer. Keep all of its other work and native return values.
REX_HOOK_RAW(sub_822ADAC0){
  const std::lock_guard lock(shadow_settings_mutex);
  motogp::engine::PropertyOverride disabled(base,motogp::engine::kShadowEnabled,0,true,
      rex::cvar::Query<std::string>("engine_dynamic_shadows")=="off");
  __imp__sub_822ADAC0(ctx,base);
}
// This renderer compares rider/bike distances with Shadows/RenderDistance.
// Restore the original tweak on return, including exceptions and nested calls.
REX_HOOK_RAW(sub_8238CFD8){
  const std::lock_guard lock(shadow_settings_mutex);
  const auto distance=rex::cvar::Query<int32_t>("engine_shadow_distance");
  motogp::engine::PropertyOverride override(base,motogp::engine::kShadowDistance,
      std::bit_cast<uint32_t>(static_cast<float>(distance)),false,distance>0);
  const auto soft_distance=ChoiceValue("engine_soft_shadow_distance");
  motogp::engine::PropertyOverride soft_override(base,motogp::engine::kSoftShadowDistance,
      std::bit_cast<uint32_t>(static_cast<float>(soft_distance)),false,soft_distance>0);
  __imp__sub_8238CFD8(ctx,base);
}

// The FSFX consumer copies the speed vignette alpha separately from blur.
REX_HOOK_RAW(sub_8237FA00){
  const std::lock_guard lock(post_settings_mutex);
  const auto blur=rex::cvar::Query<std::string>("engine_speed_blur")=="off"?0:-1;
  motogp::engine::FloatScale blur_alpha(base,motogp::engine::kSpeedBlurSettings+4,blur);
  motogp::engine::FloatScale blur_velocity(base,motogp::engine::kSpeedBlurSettings+8,blur);
  motogp::engine::FloatScale vignette(base,motogp::engine::kVignetteAlpha,ChoiceValue("engine_vignette"));
  __imp__sub_8237FA00(ctx,base);
}
// This publisher consumes the interpolated bloom state after the camera has
// selected its preset. Scaling its current intensity also covers those presets.
REX_HOOK_RAW(sub_82385428){
  const std::lock_guard lock(post_settings_mutex);
  motogp::engine::FloatScale bloom(base,motogp::engine::kBloomCurrentIntensity,
      ctx.r3.u32==motogp::engine::kBloomObject?ChoiceValue("engine_bloom"):-1);
  __imp__sub_82385428(ctx,base);
}
REX_HOOK_RAW(sub_8237B370){
  const std::lock_guard lock(shadow_settings_mutex);
  const auto filter=ChoiceValue("engine_shadow_filter");
  motogp::engine::PropertyOverride override(base,motogp::engine::kShadowFilter,
      static_cast<uint32_t>(filter),true,filter>=0);
  __imp__sub_8237B370(ctx,base);
}
// Particle density is consumed in both spawn/update and draw preparation.
REX_HOOK_RAW(sub_82273780){RunParticles(ctx,base,__imp__sub_82273780);}
REX_HOOK_RAW(sub_82274570){RunParticles(ctx,base,__imp__sub_82274570);}
REX_HOOK_RAW(sub_82274838){RunParticles(ctx,base,__imp__sub_82274838);}
REX_HOOK_RAW(sub_823908C0){
  const std::lock_guard lock(lod_settings_mutex);
  motogp::engine::ScaledPropertyOverride override(base,motogp::engine::kFarClipScale,ChoiceValue("engine_draw_distance"));
  __imp__sub_823908C0(ctx,base);
}
