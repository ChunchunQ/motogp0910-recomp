#include "button_prompts.h"
#include "button_prompt_geometry.h"
#include "button_prompt_input.h"
#include "button_prompt_mapping.h"

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/memory.h>
#include <rex/ppc/context.h>
#include <SDL3/SDL.h>
#include <cstring>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <string>
#include <vector>
#if REX_PLATFORM_WIN32
#include <Windows.h>
#include <Xinput.h>
#endif

REXCVAR_DEFINE_STRING(button_prompt_style, "auto", "Input",
    "In-game button symbols: auto, xbox, playstation, keyboard. Manual selection also supports controller wrappers.");

REX_EXTERN(__imp__sub_82388C38);
REX_EXTERN(__imp__sub_82358448);
REX_EXTERN(__imp__sub_823D1008);
REX_EXTERN(__imp__sub_82284E98);
REX_EXTERN(__imp__sub_82284E88);
REX_EXTERN(__imp__sub_82284DB0);

namespace motogp::prompts {
namespace {
std::atomic<bool> ready{false};
ActiveInput active;
std::atomic<Style> frame_style{Style::Keyboard};
std::atomic<bool> focused{false};
rex::ui::Window* input_window = nullptr;
thread_local char measured_glyph=0;
thread_local unsigned inline_depth=0;
#if REX_PLATFORM_WIN32
std::array<DWORD,4> xinput_packets{};
std::array<bool,4> xinput_seen{};
void PollXInput(bool initialize=false) {
  if(rex::cvar::Query<std::string>("input_backend")!="xinput")return;
  unsigned present=0;
  for(unsigned user=0;user<4;++user) {
    XINPUT_STATE state{};
    if(XInputGetState(user,&state)!=ERROR_SUCCESS) {xinput_seen[user]=false;continue;}
    ++present;
    const auto& pad=state.Gamepad;
    const bool meaningful=pad.wButtons || pad.bLeftTrigger>XINPUT_GAMEPAD_TRIGGER_THRESHOLD ||
        pad.bRightTrigger>XINPUT_GAMEPAD_TRIGGER_THRESHOLD ||
        std::abs(int(pad.sThumbLX))>XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ||
        std::abs(int(pad.sThumbLY))>XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ||
        std::abs(int(pad.sThumbRX))>XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ||
        std::abs(int(pad.sThumbRY))>XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
    if(initialize || (focused.load(std::memory_order_relaxed) && meaningful &&
       (!xinput_seen[user] || state.dwPacketNumber!=xinput_packets[user])))active.Xbox();
    xinput_seen[user]=true;xinput_packets[user]=state.dwPacketNumber;
  }
  if(!present && active.style()==Style::Xbox)active.Keyboard();
}
#else
void PollXInput(bool=false) {}
#endif

// Glyph IDs are 0x700 + these characters in the title-updated game. They
// are texture handles in the separate table at 0x8292CCB8, not font pixels.
std::string Label(char code,Style style) {
  auto* glyph=FindGlyph(code);
  return glyph?LabelFor(*glyph,style,[](const char* name){return rex::cvar::Query<std::string>(name);}):std::string{};
}
Style SelectedStyle() {
  const auto value=REXCVAR_GET(button_prompt_style);
  if(value=="xbox") return Style::Xbox;
  if(value=="playstation") return Style::PlayStation;
  if(value=="keyboard") return Style::Keyboard;
  return active.style();
}
bool SDLCALL Observe(void*, SDL_Event* event) {
  active.Observe(*event,focused.load(std::memory_order_relaxed));
  return true;
}
class KeyboardObserver final : public rex::ui::WindowInputListener, public rex::ui::WindowListener {
  void OnGotFocus(rex::ui::UISetupEvent&) override { focused.store(true); }
  void OnLostFocus(rex::ui::UISetupEvent&) override { focused.store(false); }
  void OnKeyDown(rex::ui::KeyEvent& event) override {
    if(!focused.load(std::memory_order_relaxed) || !rex::cvar::Query<bool>("mnk_mode")) return;
    const auto key=event.virtual_key();
    if(key!=rex::ui::VirtualKey::kShift && key!=rex::ui::VirtualKey::kControl &&
       key!=rex::ui::VirtualKey::kMenu) {
      active.Keyboard();
    }
  }
  void OnMouseMove(rex::ui::MouseEvent& event) override {
    if(focused.load(std::memory_order_relaxed) && rex::cvar::Query<bool>("mnk_mode") && rex::cvar::Query<bool>("mnk_mouse") && (event.dx()!=0 || event.dy()!=0)) {
      active.Keyboard();
    }
  }
} keyboard_observer;

// Interpolate inside the original sprite instead of assuming pixel coordinates
// or reproducing the game's projection matrices in a host overlay.
void Emit(PPCContext& ctx,uint8_t* base,const Geometry& geometry) {
  const PPCContext entry=ctx;
  const uint32_t pointers[]={ctx.r3.u32,ctx.r4.u32,ctx.r5.u32,ctx.r6.u32};
  uint8_t saved[4][24];float corners[4][3];
  for(unsigned v=0;v<4;++v) {
    std::memcpy(saved[v],base+pointers[v],24);
    for(unsigned c=0;c<3;++c)corners[v][c]=rex::memory::load_and_swap<float>(base+pointers[v]+4*c);
  }
  for(const auto& quad:geometry.quads) {
    for(unsigned v=0;v<4;++v) {
      const float u=quad.points[v].x,t=quad.points[v].y;
      for(unsigned c=0;c<3;++c) {
        const float value=corners[0][c]*(1-u)*(1-t)+corners[1][c]*u*(1-t)+corners[2][c]*u*t+corners[3][c]*(1-u)*t;
        rex::memory::store_and_swap<float>(base+pointers[v]+4*c,value);
      }
      rex::memory::store_and_swap<uint32_t>(base+pointers[v]+12,quad.color);
    }
    PPCContext draw=entry;draw.r7.u64=0;
    __imp__sub_82388C38(draw,base);
    ctx=draw;
  }
  for(unsigned v=0;v<4;++v)std::memcpy(base+pointers[v],saved[v],24);
}

}
void StartInputObserver(rex::ui::Window* window) {
  input_window=window;
  focused.store(window->HasFocus());
  active.Initialize(rex::cvar::Query<bool>("mnk_mode"));
  PollXInput(true);
  frame_style.store(SelectedStyle());
  window->AddInputListener(&keyboard_observer,1);
  window->AddListener(&keyboard_observer);
  motogp::shared_sdl::AddEventWatch(Observe,nullptr);
  ready.store(true);
}
void StopInputObserver() {
  ready.store(false);
  motogp::shared_sdl::RemoveEventWatch(Observe,nullptr);
  if(input_window) {
    input_window->RemoveInputListener(&keyboard_observer);
    input_window->RemoveListener(&keyboard_observer);
  }
  input_window=nullptr;

}
}

namespace motogp::prompts {
float MeasureButtonHalfWidth(char code,float original,Style style) {
  return style==Style::Keyboard && FindGlyph(code)?original*LabelAspect(Label(code,style)):original;
}
bool DrawReplacement(PPCContext& ctx,uint8_t* base,Style style,bool inline_glyph) {
  if(style==Style::Xbox || !ctx.r7.u32) return false;
  const auto table=rex::memory::load_and_swap<uint32_t>(base+0x8292CCB8);
  if(table) for(unsigned i=0;i<22;++i) {
    if(rex::memory::load_and_swap<uint32_t>(base+table+4*i)!=ctx.r7.u32) continue;
    const char glyph=char(rex::memory::load_and_swap<uint32_t>(base+0x8292CC60+4*i)-0x700);
    auto* definition=FindGlyph(glyph);
    if(!definition || (style==Style::PlayStation && definition->shared)) break;

    const auto color=rex::memory::load_and_swap<uint32_t>(base+ctx.r3.u32+12);
    const float x0=rex::memory::load_and_swap<float>(base+ctx.r3.u32),y0=rex::memory::load_and_swap<float>(base+ctx.r3.u32+4);
    const float x1=rex::memory::load_and_swap<float>(base+ctx.r4.u32),y1=rex::memory::load_and_swap<float>(base+ctx.r4.u32+4);
    const float x3=rex::memory::load_and_swap<float>(base+ctx.r6.u32),y3=rex::memory::load_and_swap<float>(base+ctx.r6.u32+4);
    const float width=std::hypot(x1-x0,y1-y0),height=std::hypot(x3-x0,y3-y0);
    if(!std::isfinite(width) || !std::isfinite(height) || height<.000001f || width<.000001f) break;
    const auto label=Label(glyph,style);
    const float factor=inline_glyph && style==Style::Keyboard?LabelAspect(label):1.f;
    auto geometry=MakeGeometry(glyph,style,label,width/height*factor,color);
    if(factor!=1)for(auto& q:geometry.quads)for(auto& p:q.points)p.x=.5f+(p.x-.5f)*factor;
    if(geometry.quads.empty()) break;
    Emit(ctx,base,geometry);return true;
  }
  return false;
}
}
REX_HOOK_RAW(sub_82388C38) {
  using namespace motogp::prompts;
  if(ready.load(std::memory_order_relaxed) && DrawReplacement(ctx,base,frame_style.load(std::memory_order_relaxed),inline_depth!=0)) return;
  __imp__sub_82388C38(ctx,base);
}
REX_HOOK_RAW(sub_82358448) {
  using namespace motogp::prompts;
  const auto index=ctx.r3.u32,width=ctx.r5.u32,height=ctx.r6.u32;
  __imp__sub_82358448(ctx,base);
  if(!ready.load(std::memory_order_relaxed) || index>=64) return;

  if(frame_style.load()!=Style::Keyboard) return;
  const char glyph=char(base[0x8336EB7C+index]);
  if(!FindGlyph(glyph)) return;
  const auto label=Label(glyph,Style::Keyboard);
  const float h=rex::memory::load_and_swap<float>(base+height);
  const float w=rex::memory::load_and_swap<float>(base+width);
  const float requested=h*LabelAspect(label);
  rex::memory::store_and_swap<float>(base+width,std::max(w,requested));
}
REX_HOOK_RAW(sub_82284E98) {
  const auto code=ctx.r3.u32;
  __imp__sub_82284E98(ctx,base);
  motogp::prompts::measured_glyph=ctx.r3.u8 && code>=0x700 && code<0x780?char(code-0x700):0;
}
REX_HOOK_RAW(sub_82284E88) {
  using namespace motogp::prompts;
  __imp__sub_82284E88(ctx,base);
  if(ready.load(std::memory_order_relaxed))ctx.f1.f64=MeasureButtonHalfWidth(measured_glyph,float(ctx.f1.f64),frame_style.load(std::memory_order_relaxed));
}
REX_HOOK_RAW(sub_82284DB0) {
  using namespace motogp::prompts;
  struct Scope {Scope(){++inline_depth;}~Scope(){--inline_depth;}} scope;
  __imp__sub_82284DB0(ctx,base);
}
REX_HOOK_RAW(sub_823D1008) {
  using namespace motogp::prompts;
  if(ready.load(std::memory_order_relaxed)) {
    active.SetKeyboardEnabled(rex::cvar::Query<bool>("mnk_mode"));
    PollXInput();
    frame_style.store(SelectedStyle(),std::memory_order_relaxed);
  }
  __imp__sub_823D1008(ctx,base);
}
