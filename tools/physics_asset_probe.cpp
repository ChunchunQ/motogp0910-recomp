// Verify the real game's collision-record endian conversion against the raw
// serialized bits. Finite coordinates can look like NaNs before byte reversal.
#include "motogp0910_init.h"
#include <rex/runtime.h>
#include <rex/hook.h>
REX_EXTERN(__imp__sub_823559C8);
REX_EXTERN(__imp__sub_82355AA8);
#include <rex/kernel/init.h>
#include <rex/memory.h>
#include <bit>
#include <array>
#include <cstring>
#include <iostream>
#include <filesystem>

int main(int argc,char** argv){
 if(argc<2 || argc>3)return 2;
 const bool original=argc==3 && std::string(argv[2])=="--original";
 try{
  rex::Runtime runtime(std::filesystem::canonical(argv[1]));
  rex::RuntimeConfig config{.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true};
  if(XFAILED(runtime.Setup(PPCImageConfig,std::move(config))) || XFAILED(runtime.LoadXexImage("game:\\default.xex")))return 2;
  auto* memory=runtime.memory();auto* base=runtime.virtual_membase();const auto fixture=memory->SystemHeapAlloc(0x3000);
  unsigned mismatches=0,checks=0;
  constexpr std::array<uint32_t,8> values{0x00000000,0x3F800000,0xC2480000,0x43AB807F,0x43AB80FF,0x00800000,0x00000001,0x42FF807F};
  for(auto routine:{0x823559C8u,0x82355AA8u})for(auto bits:values){
   const std::array<unsigned,14> triangle{8,12,20,24,32,36,44,48,52,56,60,64,68,72};
   const std::array<unsigned,4> shape{24,28,32,36};
   std::memset(base+fixture,0,128);
   const auto size=routine==0x823559C8?76u:40u;
   for(unsigned offset=0;offset<size;offset+=4)rex::memory::store_and_swap<uint32_t>(base+fixture+offset,0x11220000u+offset);
   auto write=[&](unsigned offset){rex::memory::store_and_swap<uint32_t>(base+fixture+offset,std::byteswap(bits));};
   if(routine==0x823559C8)for(auto offset:triangle)write(offset);else for(auto offset:shape)write(offset);
   std::array<uint8_t,76> expected_record{};
   std::memcpy(expected_record.data(),base+fixture,size);
   for(unsigned offset=0;offset<size;offset+=4){
    const bool halves=routine==0x823559C8 && (offset==4 || offset==16 || offset==28 || offset==40);
    if(halves){std::swap(expected_record[offset],expected_record[offset+1]);std::swap(expected_record[offset+2],expected_record[offset+3]);}
    else {std::swap(expected_record[offset],expected_record[offset+3]);std::swap(expected_record[offset+1],expected_record[offset+2]);}
   }
   const auto return_offset=routine==0x823559C8?0u:20u;
   uint32_t expected_return;std::memcpy(&expected_return,expected_record.data()+return_offset,4);expected_return=std::byteswap(expected_return);
   PPCContext ctx{};ctx.r1.u64=fixture+0x2800;ctx.r3.u64=fixture;ctx.lr=0x12345678;
   ctx.r29.u64=0xAABBCCDDEEFF0011;ctx.r30.u64=0x1122334455667788;ctx.r31.u64=0xFFEEDDCCBBAA9988;
   if(original)(routine==0x823559C8?__imp__sub_823559C8:__imp__sub_82355AA8)(ctx,base);
   else rex::runtime::ResolveIndirectFunction(routine)(ctx,base);
   auto verify=[&](unsigned offset){const auto actual=rex::memory::load_and_swap<uint32_t>(base+fixture+offset);++checks;
    if(actual!=bits){if(mismatches<5)std::cerr<<"record="<<std::hex<<routine<<" offset="<<offset<<" expected="<<bits<<" actual="<<actual<<std::dec<<'\n';++mismatches;}};
   if(routine==0x823559C8)for(auto offset:triangle)verify(offset);else for(auto offset:shape)verify(offset);
   ++checks;if(std::memcmp(expected_record.data(),base+fixture,size))++mismatches;
   ++checks;if(ctx.r1.u32!=fixture+0x2800 || ctx.lr!=0x12345678 || ctx.r29.u64!=0xAABBCCDDEEFF0011 || ctx.r30.u64!=0x1122334455667788 || ctx.r31.u64!=0xFFEEDDCCBBAA9988)++mismatches;
   ++checks;if(ctx.r3.u32!=expected_return)++mismatches;
  }
  memory->SystemHeapFree(fixture);std::cout<<checks<<" collision field/record/ABI checks; "<<mismatches<<" mismatches\n";
  return mismatches?1:0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
