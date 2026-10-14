// Exercise replacement geometry through the real generated native UI appender.
// The fixture starts with an empty untextured batch, so no GPU is required.
#include "motogp0910_init.h"
#include "../src/button_prompts.h"
#include "../src/button_prompt_geometry.h"
#include "../src/button_prompt_mapping.h"
#include "../src/ps3_prompt_textures.h"
#include <rex/filesystem.h>
#include <rex/cvar.h>
#include <rex/kernel/init.h>
#include <rex/memory.h>
#include <rex/runtime.h>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
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
    const auto artwork_path=rex::filesystem::GetExecutableFolder()/"resources/controller/ps3-prompts.bin";
    motogp::prompts::LoadPS3Artwork(memory,artwork_path);
    std::ifstream artwork_stream(artwork_path,std::ios::binary);
    const std::vector<uint8_t> artwork((std::istreambuf_iterator<char>(artwork_stream)),{});
    const auto allocation=memory->SystemHeapAlloc(0x40000);
    if(!allocation)throw std::runtime_error("Fixture allocation failed");
    const uint32_t count_address=uint32_t((-31941*65536)-17184);
    const uint32_t buffer_address=count_address+4,texture_address=count_address+8;
    const uint32_t slots[]={count_address,buffer_address,texture_address,0x8292CCB8,0x8294679C};
    std::array<uint32_t,5> saved_slots;
    std::array<uint8_t,88> saved_codes;
    for(unsigned i=0;i<saved_slots.size();++i)saved_slots[i]=rex::memory::load_and_swap<uint32_t>(base+slots[i]);
    std::memcpy(saved_codes.data(),base+0x8292CC60,88);
    auto write=[&](uint32_t address,uint32_t value){rex::memory::store_and_swap<uint32_t>(base+address,value);};
    auto restore=[&] {
      std::memcpy(base+0x8292CC60,saved_codes.data(),88);
      for(unsigned i=0;i<saved_slots.size();++i)write(slots[i],saved_slots[i]);
      memory->SystemHeapFree(allocation);
    };
    int checks=0;
    try {
      const std::string glyphs="ABYXT oORLrlSP1342zZQq";
      std::string codes;for(char c:glyphs)if(c!=' ')codes+=c;
      write(0x8292CCB8,allocation+0x1800);
      for(unsigned i=0;i<22;++i) {
        write(0x8292CC60+4*i,i<codes.size()?0x700+uint8_t(codes[i]):0);
        const auto wrapper=allocation+0x2000+64*i;
        write(allocation+0x1800+4*i,i<codes.size()?wrapper:0);
        write(wrapper,allocation+0x2E00);
        write(wrapper+8,64);write(wrapper+12,64);
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
        context.r3.u64=vertices[0];context.r4.u64=vertices[1];context.r5.u64=vertices[2];context.r6.u64=vertices[3];context.r7.u64=allocation+0x2000+64*i;
        context.r30.u64=0x123456789ABCDEF0;context.r31.u64=0xFEDCBA9876543210;
        const bool replaced=motogp::prompts::DrawReplacement(context,base,style);
        if(!replaced)throw std::runtime_error(std::string("Missing button replacement: ")+codes[i]);
        const auto count=rex::memory::load_and_swap<uint32_t>(base+count_address);
        if(!count || count%4 || count>8192 || context.r1.u32!=allocation+0x3000 || context.lr!=0xABCD1234 ||
           context.r30.u64!=0x123456789ABCDEF0 || context.r31.u64!=0xFEDCBA9876543210)
          throw std::runtime_error("Native batching or calling convention mismatch");
        const auto batched_texture=rex::memory::load_and_swap<uint32_t>(base+texture_address);
        if(style==motogp::prompts::Style::PlayStation) {
          const auto expected=motogp::prompts::PS3Texture(codes[i],allocation+0x2000+64*i,base);
          if(count!=4 || batched_texture!=expected)throw std::runtime_error("PS3 bitmap was not emitted as one native textured sprite");
          const auto descriptor=rex::memory::load_and_swap<uint32_t>(base+expected);
          const auto d0=rex::memory::load_and_swap<uint32_t>(base+descriptor+28);
          const auto d1=rex::memory::load_and_swap<uint32_t>(base+descriptor+32);
          if(d0!=0x01000002 || (d1&0xFFF)!=0x14 ||
             rex::memory::load_and_swap<uint32_t>(base+descriptor+36)!=0x0007E03F)
            throw std::runtime_error("PS3 linear BC3 descriptor lost dimensions, endian mode or row pitch");
          // Read the texture exactly as the GPU does: linear BC3 rows are
          // aligned to 32 compressed blocks, even when only 16 contain pixels.
          // Compare every visible block to the source pack. This rejects the
          // previous tightly packed upload that passed descriptor-only checks.
          constexpr std::array<unsigned,21> source_indices{
            106,107,108,109,110,110,112,113,114,115,116,117,135,
            136,137,138,139,131,132,133,134};
          const auto source=artwork.data()+12+(source_indices[i]-102)*(16+4096)+16;
          const auto* pixels=memory->TranslatePhysical(d1&0xFFFFF000);
          for(unsigned row=0;row<16;++row) {
            if(std::memcmp(pixels+row*512,source+row*256,256))
              throw std::runtime_error("GPU-visible BC3 pixels differ from original PS3 artwork");
            for(unsigned byte=256;byte<512;++byte)if(pixels[row*512+byte])
              throw std::runtime_error("BC3 row padding is not initialized");
          }
          ++checks;
        } else if(batched_texture)throw std::runtime_error("Keyboard cap unexpectedly used a texture");
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
      // Real saved bindings, including punctuation and long modifier names,
      // must fit the native prompt width without losing the game's fade.
      const auto previous_binding=rex::cvar::Query<std::string>("keybind_a");
      for(const char* binding:{"Space","Quote","Semicolon","Ctrl+Shift+PageDown","Alt+Backspace"})
        for(uint32_t alpha:{0u,0x40u,0xFFu}){
          rex::cvar::SetFlagByName("keybind_a",binding);
          const auto label=motogp::prompts::PrimaryLabel(binding);
          const auto width=.25f*motogp::prompts::LabelAspect(label);
          for(unsigned v=0;v<4;++v){
            rex::memory::store_and_swap<float>(base+vertices[v],1.f+((v==1||v==2)?width:0));
            write(vertices[v]+12,(alpha<<24)|0xFFFFFF);
          }
          std::array<std::array<uint8_t,24>,4> original;
          for(unsigned v=0;v<4;++v)std::memcpy(original[v].data(),base+vertices[v],24);
          write(count_address,0);write(buffer_address,allocation+0x4000);write(texture_address,0);
          PPCContext ctx{};ctx.r1.u64=allocation+0x3000;ctx.lr=0xABCD1234;
          ctx.r3.u64=vertices[0];ctx.r4.u64=vertices[1];ctx.r5.u64=vertices[2];ctx.r6.u64=vertices[3];ctx.r7.u64=allocation+0x2000;
          if(!motogp::prompts::DrawReplacement(ctx,base,motogp::prompts::Style::Keyboard))throw std::runtime_error("Custom keyboard prompt not drawn");
          const auto count=rex::memory::load_and_swap<uint32_t>(base+count_address);
          if(!count || count>=8192 || ctx.r1.u32!=allocation+0x3000 || ctx.lr!=0xABCD1234)throw std::runtime_error("Long keyboard prompt lost native batch/ABI");
          for(unsigned v=0;v<count;++v){
            const auto p=allocation+0x4000+v*24;
            const auto x=rex::memory::load_and_swap<float>(base+p),y=rex::memory::load_and_swap<float>(base+p+4);
            if(!std::isfinite(x)||!std::isfinite(y)||x<.9999f||x>1.f+width+.0001f||y<1.9999f||y>2.2501f||
                (rex::memory::load_and_swap<uint32_t>(base+p+12)>>24)!=alpha)throw std::runtime_error("Custom keyboard cap escaped layout/fade");
          }
          for(unsigned v=0;v<4;++v)if(std::memcmp(original[v].data(),base+vertices[v],24))throw std::runtime_error("Custom cap changed native source vertices");
          ++checks;
        }
      rex::cvar::SetFlagByName("keybind_a",previous_binding);
      for(unsigned v=0;v<4;++v)std::memcpy(base+vertices[v],saved_vertices[v].data(),24);
      for(auto style:{motogp::prompts::Style::Xbox,motogp::prompts::Style::Keyboard}) {
        PPCContext context{};context.r7.u64=style==motogp::prompts::Style::Xbox?0x44440000:0x11112222;
        if(motogp::prompts::DrawReplacement(context,base,style))throw std::runtime_error("Passthrough texture was replaced");
        ++checks;
      }
      // Cropped replay sprites have a separate bank from inline font icons.
      // Check both replay variants, and ensure media/timeline callers cannot
      // trigger replacement even with the same underlying texture atlas.
      const auto bank=allocation+0x1900;
      write(0x8294679C,bank);
      const std::array<std::pair<unsigned,char>,8> replay{{{19,'L'},{20,'l'},{21,'R'},{22,'r'},
                                                                      {23,'A'},{24,'B'},{25,'X'},{26,'Y'}}};
      for(unsigned i=0;i<45;++i)write(bank+4*i,allocation+0x2000+16*i);
      struct Row {uint32_t begin,end;bool controller;unsigned expected;};
      for(auto row:{Row{0x8233A348,0x8233A420,true,5},
                    Row{0x8233A420,0x8233A4F8,false,5},
                    Row{0x8233A880,0x8233A99C,true,8},
                    Row{0x8233A99C,0x8233AB0C,false,8}}) {
        unsigned last_id=0,found=0;
        for(uint32_t pc=row.begin;pc<row.end;pc+=4) {
          const auto instruction=rex::memory::load_and_swap<uint32_t>(base+pc);
          if((instruction&0xFFFF0000u)==0x38600000u)last_id=instruction&0xFFFF;
          if((instruction&0xFC000003u)!=0x48000001u)continue;
          const int32_t displacement=int32_t((instruction&0x03FFFFFCu)<<6)>>6;
          if(pc+displacement!=0x822B1198)continue;
          if(last_id>=45)throw std::runtime_error("Unexpected native replay sprite ID");
          const auto sprite=rex::memory::load_and_swap<uint32_t>(base+bank+4*last_id);
          const auto glyph=motogp::prompts::ReplaySpriteGlyph(row.end,sprite,base);
          if(bool(glyph)!=row.controller)
            throw std::runtime_error("Real game replay row was misclassified");
          ++found;++checks;
        }
        if(found!=row.expected)throw std::runtime_error("Native replay sprite sequence changed");
      }
      for(auto caller:{0x8233A420u,0x8233A99Cu})for(auto [index,glyph]:replay) {
        const auto sprite=rex::memory::load_and_swap<uint32_t>(base+bank+4*index);
        if(motogp::prompts::ReplaySpriteGlyph(caller,sprite,base)!=glyph ||
           motogp::prompts::ReplaySpriteGlyph(0x8233A4F8,sprite,base) ||
           motogp::prompts::ReplaySpriteGlyph(0x8233AB0C,sprite,base))
          throw std::runtime_error("Replay controller row / media row mismatch");
        ++checks;
      }
      for(unsigned index=0;index<19;++index) {
        if(motogp::prompts::ReplaySpriteGlyph(0x8233A420,allocation+0x2000+16*index,base) ||
           motogp::prompts::ReplaySpriteGlyph(0x8233A99C,allocation+0x2000+16*index,base))
          throw std::runtime_error("Replay media icon was classified as a controller");
        ++checks;
      }
      for(auto caller:{0x8233A420u,0x8233A99Cu})for(auto [index,glyph]:replay)
        for(auto style:{motogp::prompts::Style::PlayStation,motogp::prompts::Style::Keyboard})
          for(unsigned state=0;state<3;++state) {
            const auto sprite=allocation+0x2000+16*index;
            const auto regions=allocation+0x2800;
            const auto atlas=allocation+0x2F00;
            write(atlas,allocation+0x2E00);write(atlas+8,64);write(atlas+12,64);
            write(sprite,atlas);write(sprite+4,3);write(sprite+8,regions);
            for(unsigned n=0;n<3;++n)for(unsigned v=0;v<4;++v)
              rex::memory::store_and_swap<float>(base+regions+32*n+4*v,float(v)*.1f+.05f);
            const auto positions=allocation+0x1C00;
            for(unsigned v=0;v<4;++v)
              rex::memory::store_and_swap<float>(base+positions+4*v,float(v<2?1:1.25));
            const auto color=positions+32;write(color,0xFFFFFFFF);
            write(count_address,0);write(buffer_address,allocation+0x4000);write(texture_address,0);
            PPCContext context{};context.r1.u64=allocation+0x3800;context.lr=caller;
            context.r3.u64=positions;context.r4.u64=positions+8;context.r5.u64=sprite;
            context.r6.u64=state;context.r7.u64=color;context.r8.u64=0;
            context.r30.u64=0x1122334455667788;context.r31.u64=0x8877665544332211;
            motogp::prompts::DrawReplaySprite(context,base,style);
            const auto count=rex::memory::load_and_swap<uint32_t>(base+count_address);
            if(!count || count>8192 || count%4 || context.lr!=caller ||
               context.r1.u32!=allocation+0x3800 || context.r30.u64!=0x1122334455667788 ||
               context.r31.u64!=0x8877665544332211 ||
               (style==motogp::prompts::Style::Keyboard && rex::memory::load_and_swap<uint32_t>(base+texture_address)!=0) ||
               (style==motogp::prompts::Style::PlayStation && (count!=4 ||
                rex::memory::load_and_swap<uint32_t>(base+texture_address)!=motogp::prompts::PS3Texture(glyph,atlas,base))))
              throw std::runtime_error("Cropped replay sprite did not reach native replacement batching");
            if(style==motogp::prompts::Style::PlayStation)for(unsigned v=0;v<4;++v) {
              const auto p=allocation+0x4000+24*v;
              const float u=rex::memory::load_and_swap<float>(base+p+16),t=rex::memory::load_and_swap<float>(base+p+20);
              if(u!=(v==1||v==2?1.f:0.f) || t!=(v>=2?1.f:0.f))
                throw std::runtime_error("Replay atlas crop leaked into the PS3 texture");
            }
            ++checks;
          }
    } catch(...) {restore();throw;}
    restore();
    std::cout<<"Passed "<<checks<<" native button replacement checks\n";
    return 0;
  } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
