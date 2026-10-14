#include "motogp0910_init.h"
#include "../src/engine_settings.h"
#include "../src/ultrawide.h"
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/cvar.h>
#include <rex/hook.h>
#include <array>
#include <cstring>
#include <iostream>
#include <filesystem>
REX_EXTERN(sub_82386158);
REX_EXTERN(sub_821E2108);
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
    const auto source=stack+512;
    constexpr uint32_t projection=0x82994870;
    std::array<float,16> native_projection{1.1f,0,0,0,0,1.9555556f,0,0,0,0,1.0001f,1,0,0,-.1f,0};
    for(auto ratio:{std::pair{16u,9u},std::pair{21u,9u},std::pair{32u,9u}}){
      motogp::wide::SetRatio(ratio.first,ratio.second);
      for(unsigned i=0;i<16;++i)write(source+i*4,std::bit_cast<uint32_t>(native_projection[i]));
      PPCContext ctx{};ctx.r3.u32=source;ctx.r1.u32=stack+2048;ctx.lr=0x821CD0A0;
      sub_821E2108(ctx,base);
      const auto scale=motogp::wide::HorizontalScale();
      for(unsigned i=0;i<16;++i)Check(std::abs(std::bit_cast<float>(read(projection+i*4))-native_projection[i]*(i%4==0?scale:1))<.00001f,"Native perspective setter preserves vertical/depth axes and expands horizontal coverage");
      Check(ctx.r1.u32==stack+2048 && ctx.lr==0x821CD0A0,"Projection hook preserves native ABI");
      Check(read(source)==std::bit_cast<uint32_t>(native_projection[0]),"Source camera matrix is preserved");
      const auto x_clip=std::bit_cast<float>(read(projection));
      Check(ratio.first==16?x_clip>1:x_clip<1,"A world point outside 16:9 becomes visible in wide modes");
      ctx.r3.u32=projection;ctx.lr=0x8236BB78;sub_821E2108(ctx,base);
      Check(std::bit_cast<float>(read(projection))==x_clip,"State restoration never applies the aspect twice");
    }
    constexpr uint32_t batch_count=uint32_t((-31941*65536)-17184);
    const auto old_count=read(batch_count),old_buffer=read(batch_count+4),old_texture=read(batch_count+8);
    for(auto ratio:{std::pair{16u,9u},std::pair{21u,9u},std::pair{32u,9u}})for(bool alias:{false,true}){
      motogp::wide::SetRatio(ratio.first,ratio.second);
      std::array<uint32_t,4> vertices{stack+1024,stack+1048,stack+1072,stack+1096};if(alias)vertices[3]=vertices[0];
      for(unsigned i=0;i<4;++i)for(unsigned word=0;word<6;++word)write(vertices[i]+word*4,std::bit_cast<uint32_t>(float((i+word)%2)));
      std::array<std::array<uint32_t,6>,4> saved{};for(unsigned i=0;i<4;++i)for(unsigned word=0;word<6;++word)saved[i][word]=read(vertices[i]+word*4);
      write(batch_count,0);write(batch_count+4,stack+256);write(batch_count+8,0);
      PPCContext ctx{};ctx.r1.u32=stack+2048;ctx.lr=0x12345678;ctx.r3.u32=vertices[0];ctx.r4.u32=vertices[1];ctx.r5.u32=vertices[2];ctx.r6.u32=vertices[3];
      motogp::wide::DrawUiQuad(ctx,base);
      Check(read(batch_count)==4,"Four HUD vertices reach actual native batch");
      for(unsigned i=0;i<4;++i)for(unsigned word=0;word<6;++word){
        const auto batched=read(stack+256+i*24+word*4);
        const auto expected=word==0?std::bit_cast<uint32_t>(.5f+(std::bit_cast<float>(saved[i][word])-.5f)*motogp::wide::HorizontalScale()):saved[i][word];
        Check(batched==expected,"Central HUD corrects horizontal aspect and preserves Y/Z/color/UV");
        Check(read(vertices[i]+word*4)==saved[i][word],"Native HUD source vertices fully restored, including aliases");
      }
      Check(ctx.r1.u32==stack+2048 && ctx.lr==0x12345678,"HUD quad wrapper preserves native ABI");
    }
    write(batch_count,old_count);write(batch_count+4,old_buffer);write(batch_count+8,old_texture);
    motogp::wide::SetRatio(16,9);
    memory->SystemHeapFree(stack);
    std::cout<<checks<<" native engine flag, parameter preservation, scope and ABI checks passed\n";return 0;
  }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<'\n';return 1;}
}
