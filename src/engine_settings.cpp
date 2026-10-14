#include "engine_settings.h"
#include "engine_settings_options.h"
#include <rex/hook.h>
#include <mutex>

REX_EXTERN(__imp__sub_82386158);
REX_EXTERN(__imp__sub_822ADAC0);
REX_EXTERN(__imp__sub_8238CFD8);

// The camera/settings copier runs immediately before the actual FSFX renderer.
// Override only the two enable bytes, leaving camera focus and blur parameters
// untouched. Default mode is a complete pass-through.
REX_HOOK_RAW(sub_82386158){
  __imp__sub_82386158(ctx,base);
  if(rex::cvar::Query<std::string>("engine_depth_of_field")=="off")base[motogp::engine::kDofSettings]=0;
  if(rex::cvar::Query<std::string>("engine_speed_blur")=="off")base[motogp::engine::kSpeedBlurSettings]=0;
}
namespace {std::recursive_mutex shadow_settings_mutex;}
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
  __imp__sub_8238CFD8(ctx,base);
}
