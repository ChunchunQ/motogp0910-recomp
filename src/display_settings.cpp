#include "display_settings.h"
#include <rex/cvar.h>
#include <rex/logging.h>
#include <stdexcept>
#if REX_PLATFORM_WIN32
#include <Windows.h>
#endif
REXCVAR_DEFINE_STRING(display_mode,"auto","Display","Startup mode: auto, fullscreen, borderless, windowed")
    .allowed({"auto","fullscreen","borderless","windowed"}).lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
REXCVAR_DEFINE_INT32(host_refresh_rate,0,"Display","Actual fullscreen monitor refresh rate in Hz; 0 automatically selects a supported rate")
    .range(0,1000).lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
namespace motogp {
namespace {
std::string startup_mode;
std::string EffectiveMode() {
  auto mode=rex::cvar::Query<std::string>("display_mode");
  if(mode=="auto" || rex::cvar::GetFlagSource("fullscreen")>rex::cvar::GetFlagSource("display_mode"))
    return rex::cvar::Query<bool>("fullscreen")?"borderless":"windowed";
  return mode;
}
}
void ApplyDisplayStartupFlags() {
  const auto mode=EffectiveMode();startup_mode=mode;
  if(rex::cvar::Query<bool>("fullscreen")!=(mode!="windowed"))
    rex::cvar::SetFlagByName("fullscreen",mode=="windowed"?"false":"true");
}
void ApplyHostDisplay(rex::ui::Window* window) {
#if REX_PLATFORM_WIN32
  const auto mode=startup_mode.empty()?EffectiveMode():startup_mode;
  // Legacy direct launch needs no additional runtime export.
  if(rex::cvar::Query<std::string>("display_mode")=="auto" &&
     rex::cvar::GetFlagSource("host_refresh_rate")==rex::cvar::Source::kDefault)return;
  using Configure=int32_t(*)(uint32_t,void*,int32_t,int32_t,int32_t,int32_t);
  const auto module=GetModuleHandleW(L"rexruntime.dll");
  const auto configure=module?reinterpret_cast<Configure>(GetProcAddress(module,"Motogp_ConfigureHostDisplay")):nullptr;
  if(!configure)throw std::runtime_error("Replace both runtime DLLs with the matching display-settings build.");
  const auto refresh=mode=="fullscreen"?rex::cvar::Query<int32_t>("host_refresh_rate"):0;
  const auto result=configure(1,window->GetNativeWindowHandle(),mode=="fullscreen"?2:mode=="borderless"?1:0,
      rex::cvar::Query<int32_t>("window_width"),rex::cvar::Query<int32_t>("window_height"),refresh);
  if(result<0)throw std::runtime_error("The selected fullscreen resolution/refresh rate is unavailable. Choose Automatic / desktop or a supported display mode.");
  REXLOG_INFO("HOST_DISPLAY: mode={} requested_refresh={} Hz; guest video timing unchanged",mode,refresh);
#else
  (void)window;
#endif
}
}
