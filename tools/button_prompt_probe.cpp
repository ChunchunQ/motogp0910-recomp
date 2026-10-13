// Exercise replacement geometry through the real generated native UI appender.
// The fixture starts with an empty untextured batch, so no GPU is required.
#include "motogp0910_init.h"
#include "../src/button_prompts.h"
#include "../src/button_prompt_geometry.h"
#include "../src/button_prompt_mapping.h"
#include <rex/cvar.h>
#include <rex/kernel/init.h>
#include <rex/memory.h>
#include <rex/runtime.h>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>

int main(int argc,char** argv) {
  try {
    if(argc!=2) {std::cerr<<"Usage: button_prompt_probe GAME_ROOT\n";return 2;}
    rex::Runtime runtime(std::filesystem::canonical(argv[1]));
    rex::RuntimeConfig config{.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true};
    if(XFAILED(runtime.Setup(PPCImageConfig,std::move(config))) ||
       XFAILED(runtime.LoadXexImage("game:\\default.xex"))) throw std::runtime_error("Patched game setup failed");
    auto* memory=runtime.memory();auto* base=runtime.virtual_membase();
    const auto allocation=memory->SystemHeapAlloc(0x40000);
    if(!allocation)throw std::runtime_error("Fixture allocation failed");
    const uint32_t count_address=uint32_t((-31941*65536)-17184);
    const uint32_t buffer_address=count_address+4,texture_address=count_address+8;
    const uint32_t slots[]={count_address,buffer_address,texture_address,0x8292CCB8};
    std::array<uint32_t,4> saved_slots;
    std::array<uint8_t,88> saved_codes;
    for(unsigned i=0;i<4;++i)saved_slots[i]=rex::memory::load_and_swap<uint32_t>(base+slots[i]);
    std::memcpy(saved_codes.data(),base+0x8292CC60,88);
    auto write=[&](uint32_t address,uint32_t value){rex::memory::store_and_swap<uint32_t>(base+address,value);};
    auto restore=[&] {
      std::memcpy(base+0x8292CC60,saved_codes.data(),88);
      for(unsigned i=0;i<4;++i)write(slots[i],saved_slots[i]);
      memory->SystemHeapFree(allocation);
    };
    int checks=0;
    try {
      const std::string glyphs="ABYXT oORLrlSP1342zZQq";
      std::string codes;for(char c:glyphs)if(c!=' ')codes+=c;
      write(0x8292CCB8,allocation+0x1800);
      for(unsigned i=0;i<22;++i) {
        write(0x8292CC60+4*i,i<codes.size()?0x700+uint8_t(codes[i]):0);
        write(allocation+0x1800+4*i,i<codes.size()?0x44440000+4*i:0);
      }
      const uint32_t vertices[]={allocation+0x1000,allocation+0x1020,allocation+0x1040,allocation+0x1060};
      for(unsigned i=0;i<4;++i) {
        const float position[3]={float(i==1||i==2?1.5:1),float(i>=2?2.25:2),.75f};
        for(unsigned c=0;c<3;++c)rex::memory::store_and_swap<float>(base+vertices[i]+4*c,position[c]);
        write(vertices[i]+12,0x7FFFFFFF);write(vertices[i]+16,0x3F800000);write(vertices[i]+20,0x3F000000);
      }
      std::array<std::array<uint8_t,24>,4> saved_vertices;
      for(unsigned v=0;v<4;++v)std::memcpy(saved_vertices[v].data(),base+vertices[v],24);
      for(unsigned i=0;i<codes.size();++i)for(auto style:{motogp::prompts::Style::PlayStation,motogp::prompts::Style::Keyboard}) {
        write(count_address,0);write(buffer_address,allocation+0x4000);write(texture_address,0);
        PPCContext context{};context.r1.u64=allocation+0x3000;context.lr=0xABCD1234;
        context.r3.u64=vertices[0];context.r4.u64=vertices[1];context.r5.u64=vertices[2];context.r6.u64=vertices[3];context.r7.u64=0x44440000+4*i;
        context.r30.u64=0x123456789ABCDEF0;context.r31.u64=0xFEDCBA9876543210;
        const bool replaced=motogp::prompts::DrawReplacement(context,base,style);
        const bool shared=style==motogp::prompts::Style::PlayStation && motogp::prompts::FindGlyph(codes[i])->shared;
        if(replaced==shared)throw std::runtime_error(std::string("Wrong replacement/passthrough: ")+codes[i]);
        if(shared)rex::runtime::ResolveIndirectFunction(0x82388C38)(context,base);
        const auto count=rex::memory::load_and_swap<uint32_t>(base+count_address);
        if(!count || count%4 || count>8192 || context.r1.u32!=allocation+0x3000 || context.lr!=0xABCD1234 ||
           context.r30.u64!=0x123456789ABCDEF0 || context.r31.u64!=0xFEDCBA9876543210)
          throw std::runtime_error("Native batching or calling convention mismatch");
        for(unsigned v=0;v<4;++v)if(std::memcmp(saved_vertices[v].data(),base+vertices[v],24))throw std::runtime_error("Original guest vertices were changed");
        for(unsigned v=0;v<count;++v) {
          const auto point=allocation+0x4000+v*24;
          const float x=rex::memory::load_and_swap<float>(base+point),y=rex::memory::load_and_swap<float>(base+point+4),z=rex::memory::load_and_swap<float>(base+point+8);
          const auto color=rex::memory::load_and_swap<uint32_t>(base+point+12);
          if(!std::isfinite(x)||!std::isfinite(y)||x<.9999f||x>1.5001f||y<1.9999f||y>2.2501f||std::abs(z-.75f)>.0001f||(color>>24)!=0x7F)
            throw std::runtime_error("Replacement escaped the native sprite or lost alpha");
        }
        ++checks;
      }
      for(auto style:{motogp::prompts::Style::Xbox,motogp::prompts::Style::Keyboard}) {
        PPCContext context{};context.r7.u64=style==motogp::prompts::Style::Xbox?0x44440000:0x11112222;
        if(motogp::prompts::DrawReplacement(context,base,style))throw std::runtime_error("Passthrough texture was replaced");
        ++checks;
      }
    } catch(...) {restore();throw;}
    restore();
    std::cout<<"Passed "<<checks<<" native button replacement checks\n";
    return 0;
  } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
