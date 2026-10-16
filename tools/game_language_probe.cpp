// Exercise the shipped selector with the real title's localization mapping.
#include "motogp0910_init.h"
#include "../src/localization.h"
#include <rex/cvar.h>
#include <rex/kernel/init.h>
#include <rex/memory.h>
#include <rex/runtime.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc,char** argv) {
  try {
    if(argc!=2)throw std::runtime_error("Usage: game_language_probe PATCHED_GAME_ROOT");
    rex::Runtime runtime(std::filesystem::canonical(argv[1]));
    rex::RuntimeConfig config{.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true};
    if(XFAILED(runtime.Setup(PPCImageConfig,std::move(config))) ||
       XFAILED(runtime.LoadXexImage("game:\\default.xex")))throw std::runtime_error("Game setup failed");
    auto* memory=runtime.memory();auto* base=runtime.virtual_membase();
    const auto allocation=memory->SystemHeapAlloc(4096);
    if(!allocation)throw std::runtime_error("Fixture allocation failed");
    constexpr uint32_t property_slot=0x83374448;
    const auto saved=rex::memory::load_and_swap<uint32_t>(base+property_slot);
    rex::memory::store_and_swap<uint32_t>(base+property_slot,allocation);
    int checks=0;
    auto call=[&](const char* expected) {
      PPCContext ctx{};ctx.r1.u64=allocation+2048;ctx.lr=0x11223344;
      ctx.r31.u64=0x5566778899AABBCC;
      sub_8236DC30(ctx,base);
      const auto found=std::string(reinterpret_cast<const char*>(base+ctx.r3.u32));
      if(found!=expected)throw std::runtime_error("Wrong native localization: "+found+", expected "+expected);
      if(ctx.r1.u32!=allocation+2048 || ctx.lr!=0x11223344 || ctx.r31.u64!=0x5566778899AABBCC)
        throw std::runtime_error("Language selection damaged PPC calling convention");
      if(rex::memory::load_and_swap<uint32_t>(base+allocation)!=4)
        throw std::runtime_error("The engine language override was not restored");
      ++checks;
    };
    // Unconfigured direct launch keeps the original engine's French override.
    rex::memory::store_and_swap<uint32_t>(base+allocation,4);
    call("french");
    for(auto [id,name]:{std::pair{1,"english"},std::pair{3,"german"},std::pair{4,"french"},
                       std::pair{5,"spanish"},std::pair{6,"italian"}}) {
      if(!rex::cvar::SetFlagByName("user_language",std::to_string(id)))throw std::runtime_error("Language flag not registered");
      call(name);
    }
    rex::cvar::SetFlagByName("user_language","4");
    rex::cvar::SetFlagByName("game_text_language","turkish");
    motogp::localization::ApplyStartupLanguage();call("english");
    rex::memory::store_and_swap<uint32_t>(base+property_slot,saved);
    memory->SystemHeapFree(allocation);
    std::cout<<"Passed "<<checks<<" native localization, override preservation and ABI checks\n";
    return 0;
  } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
