#include "options_menu.h"
#include <rex/hook.h>
#include <rex/memory.h>
#include <rex/runtime.h>
#include <array>
#include <atomic>
#include <cstring>
#include <stdexcept>
REX_EXTERN(__imp__sub_8223B4E0);
REX_EXTERN(__imp__sub_8223B950);
REX_EXTERN(__imp__sub_8223BA68);
REX_EXTERN(__imp__sub_8223A5C8);
REX_EXTERN(__imp__sub_8223B120);
REX_EXTERN(__imp__sub_8223A688);
REX_EXTERN(__imp__sub_8223AEE8);
REX_EXTERN(__imp__sub_822483E0);
REX_EXTERN(__imp__sub_82248630);
REX_EXTERN(__imp__sub_82248840);
REX_EXTERN(__imp__sub_8224DF60);
REX_EXTERN(__imp__sub_8224EAA0);
REX_EXTERN(__imp__sub_8224CFB8);
REX_EXTERN(__imp__sub_8224D130);
REX_EXTERN(__imp__sub_826EDB38);
namespace motogp::options {
namespace {
std::atomic<uint32_t> online_label{0};
uint32_t Read(uint8_t* base,uint32_t at){return rex::memory::load_and_swap<uint32_t>(base+at);}
void Write(uint8_t* base,uint32_t at,uint32_t value){rex::memory::store_and_swap<uint32_t>(base+at,value);}
bool IsMatchSettings(uint8_t* base,uint32_t menu){
  if(!menu)return false;const auto type=Read(base,menu);
  return type==kCreateVtable||type==kCustomVtable||type==kOnlineCreateVtable;
}
// Keep native action IDs, compact only their display-order array. Both native
// navigation and accept handlers continue resolving the remaining original IDs.
bool Filter(uint8_t* base,uint32_t menu,uint32_t screen,unsigned capacity,unsigned max_id,uint32_t removed,bool update_count=false){
  std::array<uint32_t,12> before{},after{};unsigned count=0,kept=0;uint32_t seen=0;
  for(;count<capacity;++count){const auto id=Read(base,menu+32+4*count);if(id==UINT32_MAX)break;
    if(id>max_id||(seen&(1u<<id)))return false;seen|=1u<<id;before[count]=id;if(id!=removed)after[kept++]=id;}
  if(!count||count==capacity||!kept)return false;
  const auto selected=screen?Read(base,screen+24):UINT32_MAX;
  for(unsigned i=0;i<kept;++i)Write(base,menu+32+4*i,after[i]);Write(base,menu+32+4*kept,UINT32_MAX);
  if(update_count)Write(base,menu+24,kept+1);
  if(screen&&selected!=UINT32_MAX){unsigned next=selected<kept?selected:kept-1;
    if(selected<count)for(unsigned i=0;i<kept;++i)if(after[i]==before[selected]){next=i;break;}
    Write(base,screen+24,next);}
  return true;
}
}
void Initialize(rex::Runtime* runtime){
  const auto address=runtime->memory()->SystemHeapAlloc(8);if(!address)throw std::runtime_error("Could not allocate the PC Online label");
  std::memcpy(runtime->virtual_membase()+address,"ONLINE",7);online_label.store(address,std::memory_order_release);
}
uint32_t OnlineLabel(){return online_label.load(std::memory_order_acquire);}
bool RemoveStorage(uint8_t* base,uint32_t menu,uint32_t screen){return menu&&Read(base,menu)==kGameSettingsVtable&&Filter(base,menu,screen,7,5,5,true);}
bool RemoveParty(uint8_t* base,uint32_t menu,uint32_t screen){return menu&&Read(base,menu)==kMultiplayerVtable&&Filter(base,menu,screen,5,3,1);}
bool RemovePremium(uint8_t* base,uint32_t menu,uint32_t screen){return IsMatchSettings(base,menu)&&Filter(base,menu,screen,12,10,7);}
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
REX_HOOK_RAW(sub_8223A5C8){const auto menu=ctx.r3.u32;__imp__sub_8223A5C8(ctx,base);motogp::options::RemoveStorage(base,menu,0);}
REX_HOOK_RAW(sub_8223B120){motogp::options::RemoveStorage(base,ctx.r3.u32,ctx.r4.u32);__imp__sub_8223B120(ctx,base);}
REX_HOOK_RAW(sub_8223A688){motogp::options::RemoveStorage(base,ctx.r3.u32,ctx.r4.u32);__imp__sub_8223A688(ctx,base);}
REX_HOOK_RAW(sub_8223AEE8){motogp::options::RemoveStorage(base,ctx.r3.u32,ctx.r4.u32);__imp__sub_8223AEE8(ctx,base);}
REX_HOOK_RAW(sub_822483E0){motogp::options::RemoveParty(base,ctx.r3.u32,ctx.r4.u32);__imp__sub_822483E0(ctx,base);}
REX_HOOK_RAW(sub_82248630){motogp::options::RemoveParty(base,ctx.r3.u32,ctx.r4.u32);__imp__sub_82248630(ctx,base);}
REX_HOOK_RAW(sub_82248840){motogp::options::RemoveParty(base,ctx.r3.u32,ctx.r4.u32);__imp__sub_82248840(ctx,base);}
REX_HOOK_RAW(sub_8224DF60){motogp::options::RemovePremium(base,ctx.r3.u32,ctx.r4.u32);__imp__sub_8224DF60(ctx,base);}
REX_HOOK_RAW(sub_8224EAA0){motogp::options::RemovePremium(base,ctx.r3.u32,ctx.r4.u32);__imp__sub_8224EAA0(ctx,base);}
REX_HOOK_RAW(sub_8224CFB8){motogp::options::RemovePremium(base,ctx.r3.u32,ctx.r4.u32);__imp__sub_8224CFB8(ctx,base);}
REX_HOOK_RAW(sub_8224D130){const auto menu=ctx.r3.u32;__imp__sub_8224D130(ctx,base);
  if(motogp::options::IsMatchSettings(base,menu))motogp::options::Write(base,menu+128,0); // Off for hosting, Any for search.
}
REX_HOOK_RAW(sub_826EDB38){const auto label=motogp::options::OnlineLabel();
  if(label&&ctx.r3.u32==motogp::options::kOnlineHandle&&motogp::options::Read(base,ctx.r3.u32)==731){ctx.r3.u64=label;return;}
  __imp__sub_826EDB38(ctx,base);
}
