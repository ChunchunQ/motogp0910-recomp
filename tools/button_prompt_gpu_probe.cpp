// Real-game diagnostic sheet. Optional target; never packaged with the game.
#include "motogp0910_init.h"
#include "../src/motogp0910_app.h"
#define sub_82388C38 production_ps3_quad_hook
#include "../src/button_prompts.cpp"
#undef sub_82388C38

REX_HOOK_RAW(sub_82388C38) {
  const auto table=rex::memory::load_and_swap<uint32_t>(base+0x8292CCB8);
  const bool anchor=table && ctx.r7.u32==rex::memory::load_and_swap<uint32_t>(base+table) &&
      ctx.lr==0x82358C6C;
  if(!anchor) {production_ps3_quad_hook(ctx,base);return;}
  const auto pointers=std::array{ctx.r3.u32,ctx.r4.u32,ctx.r5.u32,ctx.r6.u32};
  std::array<std::array<uint8_t,24>,4> saved;
  for(unsigned v=0;v<4;++v)std::memcpy(saved[v].data(),base+pointers[v],24);
  for(unsigned i=0;i<21;++i) {
    const float x=.06f+float(i%7)*.13f,y=.24f+float(i/7)*.2f;
    for(unsigned v=0;v<4;++v) {
      rex::memory::store_and_swap<float>(base+pointers[v],x+(v==1||v==2?.082f:0));
      rex::memory::store_and_swap<float>(base+pointers[v]+4,y+(v>=2?.146f:0));
      rex::memory::store_and_swap<uint32_t>(base+pointers[v]+12,0xFFFFFFFF);
    }
    auto draw=ctx;
    draw.r7.u64=rex::memory::load_and_swap<uint32_t>(base+table+4*i);
    if(!motogp::prompts::DrawReplacement(draw,base,motogp::prompts::Style::PlayStation))
      __imp__sub_82388C38(draw,base);
  }
  for(unsigned v=0;v<4;++v)std::memcpy(base+pointers[v],saved[v].data(),24);
  production_ps3_quad_hook(ctx,base);
}
REX_DEFINE_APP(motogp0910, Motogp0910App::Create)
