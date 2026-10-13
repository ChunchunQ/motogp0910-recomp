// Exercise the real packaged GPU plugin's scope API and production guest hook.
// This checks lifetime/ABI/flag preservation; it does not simulate GPU pixels.
#include "../src/livery_readback.h"
#include <rex/cvar.h>
#include <rex/hook.h>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#if REX_PLATFORM_WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

REX_EXTERN(sub_82370A60);
namespace {
using ScopeFunction=uint32_t(*)(int32_t);
ScopeFunction scope_api;
int calls=0;
void Require(bool condition,const char* message) {
  if(!condition)throw std::runtime_error(message);
}
}
REX_HOOK_RAW(__imp__sub_82370A60) {
  ++calls;
  Require(scope_api(0)==(ctx.r3.u32==0?0:ctx.r3.u32==2?2:1),"Wrong readback scope inside original guest call");
  if(ctx.r3.u32==3)throw std::runtime_error("Fixture exception");
  if(ctx.r3.u32==1) {
    auto nested=ctx;nested.r3.u64=2;sub_82370A60(nested,base);
    Require(scope_api(0)==1,"Nested call released its parent scope");
  }
  ctx.r3.u64=0xAABBCCDD;ctx.r4.u64+=13;
}
int main(int argc,char** argv) {
  try {
    Require(argc==2,"Usage: livery_readback_probe GPU_PLUGIN_PATH");
#if REX_PLATFORM_WIN32
    const auto module=LoadLibraryA(argv[1]);
    scope_api=module?reinterpret_cast<ScopeFunction>(GetProcAddress(module,"Motogp_LiveryReadbackScope")):nullptr;
#else
    const auto module=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
    scope_api=module?reinterpret_cast<ScopeFunction>(dlsym(module,"Motogp_LiveryReadbackScope")):nullptr;
#endif
    Require(scope_api,"Packaged GPU plugin has no livery scope API");
    motogp::livery::Initialize();
    Require(scope_api(0)==0 && scope_api(-1)==0,"Unbalanced initial readback scope");
    int checks=1;
    uint8_t base[16]{};
    for(const char* value:{"none","fast","some","full"}) {
      Require(rex::cvar::SetFlagByName("readback_resolve",value),"Readback flag missing");
      const auto source=rex::cvar::GetFlagSource("readback_resolve");
      for(unsigned input:{0u,1u,2u,3u}) {
        if(input==2)continue; // Only the original fixture issues nested calls.
        PPCContext ctx{};ctx.r3.u64=input;ctx.r4.u64=10;ctx.lr=0x12345678;ctx.r1.u64=0x1000;
        bool threw=false;
        try {sub_82370A60(ctx,base);} catch(const std::runtime_error&) {threw=true;}
        Require(threw==(input==3),"Unexpected guest-call exception");
        Require(scope_api(0)==0,"Readback scope leaked after guest call");
        Require(rex::cvar::Query<std::string>("readback_resolve")==value &&
                rex::cvar::GetFlagSource("readback_resolve")==source,"User GPU setting was modified");
        if(!threw)Require(ctx.r3.u32==0xAABBCCDD && ctx.r4.u32==23 &&
                          ctx.lr==0x12345678 && ctx.r1.u32==0x1000,"Guest calling convention changed");
        ++checks;
      }
    }
    Require(calls==16,"Guest fixture checks did not all execute");
    std::cout<<"Passed "<<checks<<" packaged livery scope checks (16 guest calls)\n";
    return 0;
  } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
