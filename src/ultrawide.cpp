#include "ultrawide.h"
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/ui/window_listener.h>
#include <atomic>
#include <array>
#include <bit>
#include <cmath>
#include <numeric>
#include <stdexcept>
#if REX_PLATFORM_WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif
REXCVAR_DEFINE_STRING(aspect_ratio,"auto","Display","Camera/presentation aspect: auto, 16:9, 21:9, 32:9")
 .allowed({"auto","16:9","21:9","32:9"}).lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
REX_EXTERN(__imp__sub_821E2108);
REX_EXTERN(__imp__sub_82388C38);
namespace motogp::wide {
namespace {
std::atomic<float> horizontal_scale{1};
rex::ui::Window* active_window=nullptr;
using SetAspect=void(*)(uint32_t,uint32_t);
SetAspect set_aspect=nullptr;
void Update(rex::ui::Window* window) {
  const auto choice=rex::cvar::Query<std::string>("aspect_ratio");
  uint32_t x=16,y=9;
  if(choice=="21:9"){x=21;y=9;}else if(choice=="32:9"){x=32;y=9;}
  else if(choice=="auto"){
    const auto w=window->GetActualPhysicalWidth(),h=window->GetActualPhysicalHeight();
    // Preserve the 16:9 camera on narrow displays, letterboxing instead of
    // clipping the central HUD. Wider windows get a horizontal-plus camera.
    if(w && h && double(w)/h>16.0/9.0+0.01 && double(w)/h<=4.0){x=w;y=h;}
  }
  const auto divisor=std::gcd(x,y);x/=divisor;y/=divisor;
  SetRatio(x,y);
  REXLOG_INFO("ULTRAWIDE: aspect={}:{} horizontal_scale={}; native 720p framebuffer retained",x,y,HorizontalScale());
}
class Listener final:public rex::ui::WindowListener {
 void OnResize(rex::ui::UISetupEvent& event)override{if(set_aspect)Update(event.target());}
} listener;
}
void SetRatio(uint32_t x,uint32_t y){
  if(!x || !y || double(x)/y<16.0/9.0-0.00001 || double(x)/y>4.0)throw std::runtime_error("Invalid camera aspect ratio");
  horizontal_scale.store((16.f/9.f)/(float(x)/y),std::memory_order_release);
  if(set_aspect)set_aspect(x,y);
}
float HorizontalScale(){return horizontal_scale.load(std::memory_order_acquire);}
void Start(rex::ui::Window* window){
#if REX_PLATFORM_WIN32
  auto module=GetModuleHandleW(L"rexruntime.dll");
  set_aspect=module?reinterpret_cast<SetAspect>(GetProcAddress(module,"Motogp_SetPresentationAspect")):nullptr;
#else
  set_aspect=reinterpret_cast<SetAspect>(dlsym(RTLD_DEFAULT,"Motogp_SetPresentationAspect"));
#endif
  if(!set_aspect){
    if(rex::cvar::Query<std::string>("aspect_ratio")!="16:9")throw std::runtime_error("Matching ultrawide runtime is missing. Extract the complete build ZIP.");
    return;
  }
  active_window=window;Update(window);window->AddListener(&listener);
}
void Stop(){if(active_window)active_window->RemoveListener(&listener);active_window=nullptr;if(set_aspect)set_aspect(0,0);horizontal_scale.store(1);set_aspect=nullptr;}
void DrawUiQuad(PPCContext& ctx,uint8_t* base){
  const auto scale=HorizontalScale();
  if(scale==1){__imp__sub_82388C38(ctx,base);return;}
  const std::array<uint32_t,4> vertices{ctx.r3.u32,ctx.r4.u32,ctx.r5.u32,ctx.r6.u32};
  static std::atomic<bool> logged{false};
  if(!logged.load(std::memory_order_relaxed) && !logged.exchange(true,std::memory_order_relaxed))REXLOG_INFO("ULTRAWIDE_UI: first native quad x={},{}",rex::memory::load_and_swap<float>(base+vertices[0]),rex::memory::load_and_swap<float>(base+vertices[1]));
  std::array<uint32_t,4> saved{};
  for(unsigned i=0;i<4;++i)saved[i]=rex::memory::load_and_swap<uint32_t>(base+vertices[i]);
  for(unsigned i=0;i<4;++i){const auto x=std::bit_cast<float>(saved[i]);
    rex::memory::store_and_swap<float>(base+vertices[i],0.5f+(x-0.5f)*scale);
  }
  struct Restore {uint8_t* base;const std::array<uint32_t,4>& vertices;const std::array<uint32_t,4>& saved;
    ~Restore(){for(unsigned i=0;i<4;++i)rex::memory::store_and_swap<uint32_t>(base+vertices[i],saved[i]);}
  } restore{base,vertices,saved};
  __imp__sub_82388C38(ctx,base);
}
}
// This native setter publishes the perspective projection to the engine's
// shared matrix used by rendering and subsequent view-projection calculations.
// Alter only the main camera call. State restoration and other matrices keep
// their already-correct contents, avoiding repeated scaling.
REX_HOOK_RAW(sub_821E2108){
  const bool camera=ctx.lr==0x821CD0A0;
  __imp__sub_821E2108(ctx,base);
  const auto scale=motogp::wide::HorizontalScale();
  if(!camera || scale==1)return;
  constexpr uint32_t matrix=0x82994870;
  const auto w=rex::memory::load_and_swap<float>(base+matrix+60);
  const auto perspective=rex::memory::load_and_swap<float>(base+matrix+44);
  if(w!=0 || perspective!=1)return;
  for(unsigned row=0;row<4;++row){auto* value=base+matrix+row*16;
    rex::memory::store_and_swap<float>(value,rex::memory::load_and_swap<float>(value)*scale);}
}
