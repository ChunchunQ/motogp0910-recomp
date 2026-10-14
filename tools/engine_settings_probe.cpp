#include "motogp0910_init.h"
#include "../src/engine_settings.h"
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/cvar.h>
#include <rex/hook.h>
#include <array>
#include <cstring>
#include <iostream>
#include <filesystem>
REX_EXTERN(sub_82386158);
namespace {int checks=0;void Check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);++checks;}}
int main(int argc,char** argv){
  try{
    if(argc!=2)throw std::runtime_error("Usage: engine_settings_probe PATCHED_GAME_ROOT");
    rex::Runtime runtime(std::filesystem::canonical(argv[1]));
    rex::RuntimeConfig config{.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true};
    Check(!XFAILED(runtime.Setup(PPCImageConfig,std::move(config))) && !XFAILED(runtime.LoadXexImage("game:\\default.xex")),"Actual title update loaded");
    auto* memory=runtime.memory();auto* base=runtime.virtual_membase();auto stack=memory->SystemHeapAlloc(4096);
    Check(stack!=0,"Native stack allocated");
    const auto write=[&](uint32_t a,uint32_t v){rex::memory::store_and_swap<uint32_t>(base+a,v);};
    const auto read=[&](uint32_t a){return rex::memory::load_and_swap<uint32_t>(base+a);};
    using namespace motogp::engine;
    // Native FSFX registry overrides are disabled so the title keeps its camera
    // parameters. Exercise the real settings copier immediately before drawing.
    constexpr uint32_t dof_mofo=0x8337BC28,blur_mofo=0x8337BC68;
    for(auto object:{dof_mofo,blur_mofo}){write(object+12,object+16);base[object+16]=0;}
    for(bool blur:{false,true})for(bool dof:{false,true}){
      rex::cvar::SetFlagByName("engine_speed_blur",blur?"off":"game");
      rex::cvar::SetFlagByName("engine_depth_of_field",dof?"off":"game");
      std::memset(base+kDofSettings,0x5A,64);base[kDofSettings]=1;base[kSpeedBlurSettings]=1;
      PPCContext ctx{};ctx.r1.u32=stack+2048;ctx.lr=0x11223344;ctx.r30.u64=0x1122334455667788;ctx.r31.u64=0x8877665544332211;
      sub_82386158(ctx,base);
      Check(base[kDofSettings]==(dof?0:1),"DOF flag reaches native renderer settings");
      Check(base[kSpeedBlurSettings]==(blur?0:1),"Speed blur flag reaches native renderer settings");
      for(int offset=1;offset<64;++offset)if(offset!=32)Check(base[kDofSettings+offset]==0x5A,"Focus/blur parameters are preserved");
      Check(ctx.r1.u32==stack+2048 && ctx.lr==0x11223344 && ctx.r30.u64==0x1122334455667788 && ctx.r31.u64==0x8877665544332211,"Native FSFX hook preserves calling convention");
    }
    // Check actual title registry locations and byte-versus-float ownership.
    write(kShadowEnabled+12,kShadowEnabled+16);write(kShadowEnabled+16,0x01ABCDEF);
    {PropertyOverride outer(base,kShadowEnabled,0,true,true);
      Check(read(kShadowEnabled+16)==0x00ABCDEF,"Shadow override writes exactly one byte");
      {PropertyOverride nested(base,kShadowEnabled,1,true,true);Check(read(kShadowEnabled+16)==0x01ABCDEF,"Nested override takes effect");}
      Check(read(kShadowEnabled+16)==0x00ABCDEF,"Nested return restores outer override");}
    Check(read(kShadowEnabled+16)==0x01ABCDEF,"Shadow flag fully restored");
    write(kShadowDistance+12,kShadowDistance+16);const auto original=std::bit_cast<uint32_t>(90.f);write(kShadowDistance+16,original);
    for(int distance:{45,90,180}){
      const auto desired=std::bit_cast<uint32_t>(static_cast<float>(distance));
      {PropertyOverride changed(base,kShadowDistance,desired,false,true);Check(read(kShadowDistance+16)==desired,"Big-endian float distance installed");}
      Check(read(kShadowDistance+16)==original,"Original track distance restored");
    }
    try{PropertyOverride changed(base,kShadowDistance,0,false,true);throw 1;}catch(int){}
    Check(read(kShadowDistance+16)==original,"Exception restores track distance");
    write(kShadowDistance+12,0);
    {PropertyOverride uninitialized(base,kShadowDistance,0,false,true);Check(read(kShadowDistance+16)==original,"Uninitialized registry is not modified");}
    Check(!rex::cvar::SetFlagByName("engine_shadow_distance","181"),"Distance range enforced");
    Check(!rex::cvar::SetFlagByName("engine_speed_blur","on"),"Unsupported forced-enable mode rejected");
    // Verify the native shadow renderer reads the identified registry value.
    Check(read(0x8238D084)==0x817A000C && read(0x8238D08C)==0xC1AB0000,"Native shadow distance consumer matches the pinned title");
    memory->SystemHeapFree(stack);
    std::cout<<checks<<" native engine flag, parameter preservation, scope and ABI checks passed\n";return 0;
  }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<'\n';return 1;}
}
