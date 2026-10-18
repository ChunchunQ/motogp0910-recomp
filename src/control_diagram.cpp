#include "control_diagram.h"
#include "localization.h"
#include "button_prompts.h"
#include "button_prompt_mapping.h"
#include "ps3_prompt_textures.h"
#include "ultrawide.h"
#include <rex/hook.h>
#include <rex/memory.h>
#include <rex/cvar.h>
#include <cstring>
REX_EXTERN(__imp__sub_82236A50);
REX_EXTERN(__imp__sub_822011C8);
namespace motogp::controls {
namespace {
thread_local bool drawing=false;
thread_local motogp::prompts::Style style=motogp::prompts::Style::Xbox;
std::string Binding(const char* key){
  const auto raw=rex::cvar::Query<std::string>(key);std::string out;
  for(size_t at=0;at<=raw.size();){const auto end=raw.find(',',at);const auto token=raw.substr(at,end==std::string::npos?raw.size()-at:end-at);
    if(!token.empty()){if(!out.empty())out+=" / ";out+=motogp::prompts::PrimaryLabel(token);}
    if(end==std::string::npos)break;at=end+1;
  }
  return out.empty()?(motogp::localization::Turkish()?"ATANMADI":"Unbound"):out;
}
}
bool IsDrawing(){return drawing;}
std::array<std::string,18> KeyboardRows(){
  auto stick=[](const char* up,const char* left,const char* down,const char* right){return Binding(up)+" / "+Binding(left)+" / "+Binding(down)+" / "+Binding(right);};
  return {Binding("keybind_left_trigger"),Binding("keybind_left_shoulder"),Binding("keybind_back"),
    Binding("keybind_lstick_press"),stick("keybind_lstick_up","keybind_lstick_left","keybind_lstick_down","keybind_lstick_right"),
    Binding("keybind_dpad_up"),Binding("keybind_dpad_left"),Binding("keybind_dpad_right"),Binding("keybind_dpad_down"),
    Binding("keybind_right_trigger"),Binding("keybind_right_shoulder"),Binding("keybind_y"),Binding("keybind_b"),
    Binding("keybind_x"),Binding("keybind_a"),Binding("keybind_start"),
    stick("keybind_rstick_up","keybind_rstick_left","keybind_rstick_down","keybind_rstick_right"),Binding("keybind_rstick_press")};
}
motogp::prompts::Geometry KeyboardDiagram(const std::array<std::string,18>& labels,uint32_t color){
  using namespace motogp::prompts;Geometry result;result.quads.reserve(8000);const auto alpha=color&0xFF000000;
  for(size_t side=0;side<2;++side)for(size_t row=0;row<9;++row){
    const float y=(20.f+38.5f*row)/512.f,x=side?.52f:.02f;
    result.Rect(x+.008f,y+33.f/512.f,.444f,1.f/512.f,alpha|0x303540);
    const float cap_x=side?.532f:.282f,cap_y=y+3.f/512.f,cap_w=.185f,cap_h=26.f/512.f;
    auto cap=MakeKeyboardGeometry(labels[side*9+row],cap_w*2/cap_h,color);
    for(auto quad:cap.quads){for(auto& p:quad.points){p.x=cap_x+p.x*cap_w;p.y=cap_y+p.y*cap_h;}result.quads.push_back(std::move(quad));}
  }
  return result;
}
bool PrepareLabels(PPCContext& ctx,uint8_t* base,motogp::prompts::Style selected){
  if(ctx.lr!=0x82236D08||selected!=motogp::prompts::Style::PlayStation||!motogp::prompts::PS3ControlArtworkReady())return false;
  std::array<uint32_t,9> original;for(unsigned i=0;i<9;++i)original[i]=rex::memory::load_and_swap<uint32_t>(base+ctx.r1.u32+0x110+4*i);
  for(unsigned i=0;i<9;++i)rex::memory::store_and_swap<uint32_t>(base+ctx.r1.u32+0x110+4*i,original[kPS3LeftRows[i]]);
  return true;
}
bool DrawDiagram(PPCContext& ctx,uint8_t* base,motogp::prompts::Style selected){
  const auto original=rex::memory::load_and_swap<uint32_t>(base+0x8291770C);
  if(!original||ctx.r7.u32!=original||selected==motogp::prompts::Style::Xbox)return false;
  if(selected==motogp::prompts::Style::Keyboard){
    const auto color=rex::memory::load_and_swap<uint32_t>(base+ctx.r3.u32+12);
    // Keep the native localized action text visible: it is drawn before the
    // diagram, so opaque row backgrounds must not cover its outer columns.
    static thread_local std::array<std::string,18> cached_labels;
    static thread_local uint32_t cached_color=0;
    static thread_local motogp::prompts::Geometry cached_geometry;
    const auto labels=KeyboardRows();
    if(labels!=cached_labels||color!=cached_color||cached_geometry.quads.empty()){
      cached_labels=labels;cached_color=color;cached_geometry=KeyboardDiagram(labels,color);
    }
    motogp::prompts::EmitGeometry(ctx,base,cached_geometry);return true;
  }
  const auto replacement=motogp::prompts::PS3ControlTexture(original,base);if(!replacement)return false;
  const std::array vertices{ctx.r3.u32,ctx.r4.u32,ctx.r5.u32,ctx.r6.u32};std::array<std::array<uint8_t,8>,4> saved;
  for(unsigned i=0;i<4;++i){std::memcpy(saved[i].data(),base+vertices[i]+16,8);rex::memory::store_and_swap<float>(base+vertices[i]+16,i==1||i==2?1.f:0.f);rex::memory::store_and_swap<float>(base+vertices[i]+20,i>=2?1.f:0.f);}
  ctx.r7.u64=replacement;motogp::wide::DrawUiQuad(ctx,base);
  for(unsigned i=0;i<4;++i)std::memcpy(base+vertices[i]+16,saved[i].data(),8);
  return true;
}
}
REX_HOOK_RAW(sub_82236A50){
  const auto old_drawing=motogp::controls::drawing;const auto old_style=motogp::controls::style;
  motogp::controls::drawing=true;motogp::controls::style=motogp::prompts::CurrentStyle();
  struct Restore {bool previous;motogp::prompts::Style previous_style;~Restore(){motogp::controls::drawing=previous;motogp::controls::style=previous_style;}} restore{old_drawing,old_style};
  __imp__sub_82236A50(ctx,base);
}
REX_HOOK_RAW(sub_822011C8){
  if(motogp::controls::drawing)motogp::controls::PrepareLabels(ctx,base,motogp::controls::style);
  __imp__sub_822011C8(ctx,base);
}
