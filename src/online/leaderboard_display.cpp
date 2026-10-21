#include "leaderboard_display.h"
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/memory.h>
#include <rex/runtime.h>
#include <atomic>
#include <cstring>
#include <stdexcept>
#include <string>

REX_EXTERN(__imp__sub_8223CF40);
REX_EXTERN(__imp__sub_8223F340);

namespace motogp::online::lap_times {
namespace {
thread_local bool drawing=false;
std::atomic<uint32_t> label_bank{0};
uint32_t LabelAddress(uint32_t mode){
 const auto label=ModeLabel(mode);
 const auto bank=label_bank.load(std::memory_order_acquire);
 if(label==Label::Native || !bank)return 0;
 const unsigned language=rex::cvar::Query<std::string>("game_text_language")=="turkish"?1:0;
 uint32_t offset=0;
 for(unsigned lang=0;lang<2;++lang)for(size_t i=0;i<kLabels[lang].size();++i){
  if(lang==language && i==static_cast<size_t>(label))return bank+offset;
  offset+=static_cast<uint32_t>(kLabels[lang][i].size()+1);
 }
 return 0;
}
}
void Initialize(rex::Runtime* runtime){
 uint32_t size=0;
 for(const auto& language:kLabels)for(auto text:language)size+=static_cast<uint32_t>(text.size()+1);
 const auto address=runtime->memory()->SystemHeapAlloc(size);
 if(!address)throw std::runtime_error("Could not allocate Lap Times mode labels");
 auto* output=runtime->virtual_membase()+address;
 for(const auto& language:kLabels)for(auto text:language){
  std::memcpy(output,text.data(),text.size());output[text.size()]=0;output+=text.size()+1;
 }
 label_bank.store(address,std::memory_order_release);
}
}

REX_HOOK_RAW(sub_8223CF40){
 // Base leaderboard draw: +24 selects Lap Times (0), Arcade (1),
 // All Time Ghost (2) or Monthly Ghost (3). All timed rows share the getter.
 // Scope the override to this call, so another board/thread cannot inherit it.
 const bool previous=motogp::online::lap_times::drawing;
 const auto category=ctx.r3.u32?rex::memory::load_and_swap<uint32_t>(base+ctx.r3.u32+24):UINT32_MAX;
 motogp::online::lap_times::drawing=category==0||category==2||category==3;
 struct Restore {bool value;~Restore(){motogp::online::lap_times::drawing=value;}} restore{previous};
 __imp__sub_8223CF40(ctx,base);
}
REX_HOOK_RAW(sub_8223F340){
 // This call site draws the row's mode text, after its native column setup.
 if(motogp::online::lap_times::drawing && ctx.lr==0x8223DD14 && ctx.r3.u32){
  const auto mode=rex::memory::load_and_swap<uint32_t>(base+ctx.r3.u32+124);
  if(const auto label=motogp::online::lap_times::LabelAddress(mode)){
   ctx.r3.u64=label;return;
  }
 }
 __imp__sub_8223F340(ctx,base);
}
