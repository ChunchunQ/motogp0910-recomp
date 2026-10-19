// Execute the production wrappers and real recompiled title consumers. CPU
// observers stop unrelated lighting/GPU/allocation work at named boundaries;
// these checks do not execute shaders or replace the consumers under test.
#include "motogp0910_init.h"
#include "../src/engine_settings.h"
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/cvar.h>
#include <rex/hook.h>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>

REX_EXTERN(sub_8237FA00);
REX_EXTERN(sub_82385428);
REX_EXTERN(sub_8237B370);
REX_EXTERN(sub_8238CFD8);
REX_EXTERN(sub_82274570);
REX_EXTERN(sub_823908C0);
REX_EXTERN(__imp__sub_823921B0);
REX_EXTERN(__imp__sub_822D4718);
REX_EXTERN(__imp__sub_82272370);
REX_EXTERN(__imp__sub_823996A0);
REX_EXTERN(__imp__sub_8237AD90);
REX_EXTERN(__imp__sub_8237AE80);
REX_EXTERN(__imp__sub_8238CAB0);
REX_EXTERN(__imp__sub_8238CD30);
REX_EXTERN(__imp__sub_8236E360);
namespace {
enum class Phase {Idle,Vignette,Bloom,Particles,DrawDistance,Filter,SoftShadow};
Phase phase=Phase::Idle;
struct Boundary {};
int checks=0,soft_calls=0;uint32_t observed=0;
void Check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);++checks;}
uint32_t Read(uint8_t* b,uint32_t a){return rex::memory::load_and_swap<uint32_t>(b+a);}
void Write(uint8_t* b,uint32_t a,uint32_t v){rex::memory::store_and_swap<uint32_t>(b+a,v);}
void Float(uint8_t* b,uint32_t a,float v){Write(b,a,std::bit_cast<uint32_t>(v));}
float Real(uint8_t* b,uint32_t a){return std::bit_cast<float>(Read(b,a));}
void Property(uint8_t* b,uint32_t object,uint32_t bits){Write(b,object+12,object+16);Write(b,object+16,bits);}
template<class F> void StopAtBoundary(F action){bool stopped=false;try{action();}catch(const Boundary&){stopped=true;}Check(stopped,"Real consumer reached the declared CPU observation boundary");}
}
REX_HOOK_RAW(sub_823921B0){if(phase==Phase::Bloom)throw Boundary{};__imp__sub_823921B0(ctx,base);}
REX_HOOK_RAW(sub_822D4718){if(phase==Phase::DrawDistance){observed=std::bit_cast<uint32_t>(float(ctx.f2.f64));throw Boundary{};}__imp__sub_822D4718(ctx,base);}
REX_HOOK_RAW(sub_82272370){if(phase==Phase::Particles)throw Boundary{};__imp__sub_82272370(ctx,base);}
REX_HOOK_RAW(sub_823996A0){if(phase==Phase::Filter){observed=base[motogp::engine::kShadowFilter+16];throw Boundary{};}__imp__sub_823996A0(ctx,base);}
REX_HOOK_RAW(sub_8237AD90){if(phase!=Phase::SoftShadow)__imp__sub_8237AD90(ctx,base);}
REX_HOOK_RAW(sub_8237AE80){if(phase!=Phase::SoftShadow)__imp__sub_8237AE80(ctx,base);}
REX_HOOK_RAW(sub_8238CAB0){if(phase!=Phase::SoftShadow)__imp__sub_8238CAB0(ctx,base);}
REX_HOOK_RAW(sub_8238CD30){if(phase==Phase::SoftShadow){observed=ctx.r7.u32;++soft_calls;return;}__imp__sub_8238CD30(ctx,base);}
REX_HOOK_RAW(sub_8236E360){if(phase!=Phase::Vignette)__imp__sub_8236E360(ctx,base);}

int main(int argc,char** argv){try{
  std::cout<<std::unitbuf;
  Check(argc==2,"Usage: engine_advanced_probe PATCHED_GAME_ROOT");
  rex::Runtime runtime(std::filesystem::canonical(argv[1]));
  rex::RuntimeConfig config{.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true};
  Check(!XFAILED(runtime.Setup(PPCImageConfig,std::move(config)))&&!XFAILED(runtime.LoadXexImage("game:\\default.xex")),"Real patched game and production runtime loaded");
  using namespace motogp::engine;
  auto* b=runtime.virtual_membase();const auto allocation=runtime.memory()->SystemHeapAlloc(65536);
  Check(allocation!=0,"Isolated native input and stack allocated");
  const auto context=[&]{PPCContext c{};c.r1.u32=allocation+49152;c.lr=0x11223344;return c;};
  const auto set=[](const char* key,const char* value){Check(rex::cvar::SetFlagByName(key,value),"Production CVar accepts the tested setting");};

  // FSFX publishes vignette alpha to its real renderer state. The leading
  // graphics matrix/setup call is bypassed; the parameter publisher runs.
  phase=Phase::Vignette;
  constexpr uint32_t vignette_output=0x83377280;
  Float(b,kVignetteAlpha,.8f);Float(b,kSpeedBlurSettings+4,.65f);
  Float(b,kSpeedBlurSettings+8,.4f);Float(b,kSpeedBlurSettings+12,.2f);b[kSpeedBlurSettings]=1;
  for(bool blur_off:{false,true}){
   set("engine_speed_blur",blur_off?"off":"game");
   for(auto [choice,factor]:std::array<std::pair<const char*,float>,6>{{{"game",1},{"off",0},{"25",.25f},{"50",.5f},{"75",.75f},{"100",1}}}){
    set("engine_vignette",choice);
    for(int frame=0;frame<2;++frame){auto c=context();sub_8237FA00(c,b);
      Check(std::abs(Real(b,vignette_output)-.8f*factor)<.00001f,"Real FSFX publisher receives the selected vignette alpha");
      Check(std::abs(Real(b,0x8337728C)-(blur_off?0.f:.65f))<.00001f && std::abs(Real(b,0x83377288)-(blur_off?0.f:-.4f))<.00001f,"Motion blur switches off independently while the requested vignette remains active");
      Check(Real(b,kVignetteAlpha)==.8f && Real(b,kSpeedBlurSettings+4)==.65f && Real(b,kSpeedBlurSettings+8)==.4f,"Each pass preserves the camera's original blur and vignette parameters");
      Check(c.r1.u32==allocation+49152 && c.lr==0x11223344,"Complete FSFX consumer preserves stack and return address");}
   }
  }
  b[kSpeedBlurSettings]=0;set("engine_vignette","50");auto disabled=context();sub_8237FA00(disabled,b);
  Check(b[0x83377290]==0,"Adjusting vignette never forces on a speed effect disabled by the game camera");
  b[kSpeedBlurSettings]=1;set("engine_speed_blur","game");
  set("engine_vignette","game");std::cout<<"Vignette and independent motion blur: native FSFX publication PASS\n";

  // Stop only after the native publisher has emitted all common bloom floats,
  // before its unrelated per-material lighting color updates begin.
  phase=Phase::Bloom;
  constexpr uint32_t bloom_passes=0x8337A3B8,bloom_output=0x8337A390;
  Property(b,bloom_passes,1);Float(b,kBloomCurrentIntensity,.8f);
  for(auto [choice,factor]:std::array<std::pair<const char*,float>,6>{{{"game",1},{"off",0},{"25",.25f},{"50",.5f},{"75",.75f},{"100",1}}}){
    set("engine_bloom",choice);
    for(int frame=0;frame<2;++frame){auto c=context();c.r3.u32=kBloomObject;
      StopAtBoundary([&]{sub_82385428(c,b);});
      Check(std::abs(Real(b,bloom_output)-.8f*factor)<.00001f,"Real bloom publisher emits the selected strength");
      Check(Real(b,kBloomCurrentIntensity)==.8f,"Bloom scope restores the interpolated camera value after the native call");}
  }
  set("engine_bloom","game");std::cout<<"Bloom: native parameter publication PASS\n";

  // The native particle preparation multiplies its template count by the
  // density and stores the result before the allocation/timing boundary.
  phase=Phase::Particles;
  const auto instance=allocation+4096,definition=allocation+8192,amount=allocation+12000,type=amount+4;
  constexpr uint32_t particle_mode=0x829270E0;
  Property(b,particle_mode,0);Property(b,kParticleDensity,60);
  Write(b,instance,definition);Write(b,instance+20,1);Write(b,definition+528,amount);Write(b,amount,200);
  Write(b,definition+2048,type);Write(b,type,0);
  for(auto [choice,expected]:std::array<std::pair<const char*,int>,6>{{{"game",120},{"off",0},{"25",50},{"50",100},{"75",150},{"100",200}}}){
    set("engine_particle_density",choice);auto c=context();c.r3.u32=instance;c.r4.u32=11;c.r5.u32=0;
    StopAtBoundary([&]{sub_82274570(c,b);});
    Check(Read(b,instance+32)-Read(b,instance+28)==static_cast<uint32_t>(expected),"Real particle code calculates the requested spawn count");
    Check(Read(b,kParticleDensity+16)==60,"Particle preparation restores the original density");
  }
  set("engine_particle_density","game");std::cout<<"Particles: native count calculation PASS\n";

  // The real LOD consumer reads the scoped far-clip value and passes it to
  // frustum construction; observe that argument before camera math starts.
  phase=Phase::DrawDistance;Property(b,kFarClipScale,std::bit_cast<uint32_t>(.8f));
  Property(b,kFarClipScale+36,std::bit_cast<uint32_t>(.1f));
  Float(b,kFarClipScale+20,0);Float(b,kFarClipScale+24,1);
  for(auto [choice,expected]:std::array<std::pair<const char*,float>,6>{{{"game",.8f},{"50",.4f},{"75",.6f},{"100",.8f},{"125",1},{"150",1}}}){
    set("engine_draw_distance",choice);auto c=context();c.r3.u32=instance;
    StopAtBoundary([&]{sub_823908C0(c,b);});
    Check(std::abs(std::bit_cast<float>(observed)-expected)<.00001f,"Real LOD consumer passes the requested bounded scale to frustum construction");
    Check(Real(b,kFarClipScale+16)==.8f,"LOD scope restores the original track value");
  }
  set("engine_draw_distance","game");std::cout<<"Draw distance: native frustum input PASS\n";

  // Execute the real soft/hard-shadow distance branch. GPU setup/draw and
  // teardown are observation-only boundaries in this isolated CPU fixture.
  phase=Phase::SoftShadow;constexpr uint32_t riders=0x83488018;
  b[0x828928F8]=0;Write(b,0x829952A8,1);Write(b,riders+960,1);
  Float(b,riders+20,6000);Property(b,kShadowDistance,std::bit_cast<uint32_t>(180.f));
  Property(b,kSoftShadowDistance,std::bit_cast<uint32_t>(90.f));
  for(auto [choice,expected]:std::array<std::pair<const char*,uint32_t>,4>{{{"game",1},{"45",0},{"90",1},{"180",1}}}){
    set("engine_soft_shadow_distance",choice);set("engine_shadow_distance","0");soft_calls=0;auto c=context();c.r3.u32=instance;sub_8238CFD8(c,b);
    Check(soft_calls==1&&observed==expected,"Real distance comparison selects the expected soft/hard-shadow draw flag");
    Check(Real(b,kSoftShadowDistance+16)==90.f,"Soft shadow scope restores the track threshold");
  }
  set("engine_soft_shadow_distance","game");std::cout<<"Soft shadows: native distance branch PASS\n";

  // Filtering needs a GPU to render. Verify the production scope is visible
  // when the real native pipeline enters GPU setup; do not emulate a GPU.
  phase=Phase::Filter;Property(b,kShadowFilter,0x01ABCDEF);
  for(auto [choice,expected]:std::array<std::pair<const char*,uint32_t>,3>{{{"game",1},{"off",0},{"on",1}}}){
    set("engine_shadow_filter",choice);Write(b,0x833748D4,1);auto c=context();c.r3.u32=instance;
    StopAtBoundary([&]{sub_8237B370(c,b);});
    Check(observed==expected,"Native shadow pipeline sees the requested filter flag at GPU entry");
    Check(Read(b,kShadowFilter+16)==0x01ABCDEF,"Filter changes preserve neighboring bytes and restore the original flag");
  }
  set("engine_shadow_filter","game");phase=Phase::Idle;
  runtime.memory()->SystemHeapFree(allocation);
  std::cout<<checks<<" advanced native consumer checks passed (GPU boundaries excluded)\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
