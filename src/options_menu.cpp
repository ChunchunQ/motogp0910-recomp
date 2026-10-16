#include "options_menu.h"
#include <rex/hook.h>
#include <rex/memory.h>
REX_EXTERN(__imp__sub_8223B4E0);
REX_EXTERN(__imp__sub_8223B950);
REX_EXTERN(__imp__sub_8223BA68);
namespace motogp::options {
bool RemoveMarketplace(uint8_t* base,uint32_t menu,uint32_t screen){
  const auto read=[&](uint32_t at){return rex::memory::load_and_swap<uint32_t>(base+at);};
  const auto write=[&](uint32_t at,uint32_t value){rex::memory::store_and_swap<uint32_t>(base+at,value);};
  if(!menu||read(menu)!=kVtable)return false;
  for(unsigned i=0;i<5;++i)if(read(menu+32+4*i)!=i)return false;
  if(read(menu+56)!=UINT32_MAX||(read(menu+52)!=5&&read(menu+52)!=UINT32_MAX))return false;
  write(menu+52,UINT32_MAX);
  if(screen&&read(screen+24)==5)write(screen+24,4);
  return true;
}
}
REX_HOOK_RAW(sub_8223B4E0){motogp::options::RemoveMarketplace(base,ctx.r3.u32,ctx.r4.u32);__imp__sub_8223B4E0(ctx,base);}
REX_HOOK_RAW(sub_8223B950){motogp::options::RemoveMarketplace(base,ctx.r3.u32,ctx.r4.u32);__imp__sub_8223B950(ctx,base);}
REX_HOOK_RAW(sub_8223BA68){motogp::options::RemoveMarketplace(base,ctx.r3.u32,ctx.r4.u32);__imp__sub_8223BA68(ctx,base);}
