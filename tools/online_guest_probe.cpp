#include "motogp0910_init.h"
#include "lan/runtime.h"
#include <rex/kernel/init.h>
#include <rex/runtime.h>
#include <rex/system/xam/user_profile.h>
#include <rex/system/xevent.h>
#include <rex/system/xthread.h>
#include <iostream>
#include <cstring>
REX_EXTERN(sub_825D9668);
REX_EXTERN(sub_825DA018);
REX_EXTERN(sub_825D9F70);
REX_EXTERN(sub_8239C808);
REX_EXTERN(sub_8239C868);
REX_EXTERN(sub_8239D468);
REX_EXTERN(__imp__XamUserGetXUID);
REX_EXTERN(__imp__XamEnumerate);
REX_EXTERN(__imp__XamSessionRefObjByHandle);
REX_EXTERN(__imp__XMsgInProcessCall);
namespace {
unsigned checks=0;
void Check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);++checks;}
struct Stop {~Stop(){motogp::lan::Stop();}};
}
int main(int argc,char** argv){try{
 if(argc!=3)throw std::runtime_error("Usage: online_guest_probe GAME_ROOT SERVER_URL");
 rex::Runtime runtime(std::filesystem::canonical(argv[1]));
 if(XFAILED(runtime.Setup(PPCImageConfig,{.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true})))throw std::runtime_error("Runtime setup failed");
 if(XFAILED(runtime.LoadXexImage("game:\\default.xex")))throw std::runtime_error("Real patched title load failed");
 std::exception_ptr failure;
 auto task=[&]()->int{try{
 const auto profile_xuid=runtime.kernel_state()->user_profile()->xuid();
 rex::cvar::SetFlagByName("network_mode","online");rex::cvar::SetFlagByName("online_server",argv[2]);rex::cvar::SetFlagByName("lan_player_name","GuestProbe");
 motogp::lan::Start(runtime.kernel_state());Stop stop;
 const auto allocation=runtime.memory()->SystemHeapAlloc(65536);Check(allocation!=0,"Real guest heap allocation");auto* base=runtime.virtual_membase();std::memset(base+allocation,0,65536);
 auto put=[&](unsigned off,uint32_t v){rex::memory::store_and_swap<uint32_t>(base+allocation+off,v);};auto get=[&](unsigned off){return rex::memory::load_and_swap<uint32_t>(base+allocation+off);};
 auto context=[&](){PPCContext ctx{};ctx.r1.u64=allocation+60000;return ctx;};
 auto ctx=context();ctx.r3.u64=0;ctx.r4.u64=2;ctx.r5.u64=allocation+64;__imp__XamUserGetXUID(ctx,base);
 Check(ctx.r3.u32==0,"Online XUID import succeeds");const auto online_xuid=rex::memory::load_and_swap<uint64_t>(base+allocation+64);
 Check(online_xuid!=profile_xuid,"Persistent online identity is distinct from offline save identity");
 ctx=context();ctx.r3.u64=0x21;ctx.r4.u64=0;ctx.r5.u64=2;ctx.r6.u64=0;ctx.r7.u64=allocation+576;ctx.r8.u64=allocation+512;ctx.r9.u64=allocation+128;ctx.r10.u64=allocation;
 sub_825D9668(ctx,base);Check(ctx.r3.u32==997 && get(128)==0 && motogp::lan::CurrentRoom().has_value(),"Actual generated XSessionCreate opens a relayed room");const auto room=motogp::lan::CurrentRoom()->id;
 ctx=context();ctx.r3.u64=0x707;ctx.r4.u64=0;ctx.r5.u64=1;ctx.r6.u64=0;ctx.r7.u64=allocation+576;ctx.r8.u64=allocation+512;ctx.r9.u64=allocation+128;ctx.r10.u64=allocation+4;
 sub_825D9668(ctx,base);Check(ctx.r3.u32==997 && get(128)==0 && motogp::lan::CurrentRoom()->id==room,"Original statistics-only session coexists without advertising or replacing a race lobby");const auto session=get(4);
 put(1024,1);put(1028,2);put(1032,allocation+1100);
 put(1100,0x20000003);base[allocation+1108]=2;rex::memory::store_and_swap<uint64_t>(base+allocation+1116,static_cast<uint64_t>(int64_t(-9000000)));
 put(1124,0x10000002);base[allocation+1132]=1;put(1140,90000);
 auto event=rex::system::object_ref<rex::system::XEvent>(new rex::system::XEvent(runtime.kernel_state()));event->Initialize(true,false);put(204,event->handle());
 ctx=context();ctx.r3.u64=session;ctx.r4.u64=online_xuid;ctx.r5.u64=1;ctx.r6.u64=allocation+1024;ctx.r7.u64=allocation+192;
 sub_825DA018(ctx,base);
 Check(ctx.r3.u32==997,"Actual native statistics write returns asynchronously");
 auto flushed=rex::system::object_ref<rex::system::XEvent>(new rex::system::XEvent(runtime.kernel_state()));flushed->Initialize(true,false);put(268,flushed->handle());
 ctx=context();ctx.r3.u64=session;ctx.r4.u64=allocation+256;sub_825D9F70(ctx,base);Check(ctx.r3.u32==997,"Original XSessionFlushStats queues a barrier after the upload");
 std::memset(base+allocation+1024,0,200);uint64_t timeout=static_cast<uint64_t>(int64_t(-100000000));
 Check(event->Wait(0,0,0,&timeout)==0 && get(192)==0,"Original 24-byte statistics ABI copies stack inputs and completes real kernel event");event->ReleaseHandle();
 Check(flushed->Wait(0,0,0,&timeout)==0 && get(256)==0,"Native flush completes after prior statistics writes without blocking the worker");flushed->ReleaseHandle();
 put(2048,1);put(2052,3);for(unsigned i=0;i<3;++i)rex::memory::store_and_swap<uint16_t>(base+allocation+2056+i*2,i==0?3:i==1?2:0xFFFA);
 ctx=context();ctx.r3.u64=0;ctx.r4.u64=1;ctx.r5.u64=10;ctx.r6.u64=1;ctx.r7.u64=allocation+2048;ctx.r8.u64=allocation+2200;ctx.r9.u64=allocation+2204;
 sub_8239C808(ctx,base);Check(ctx.r3.u32==0 && get(2200)>24 && get(2204)!=0,"Actual by-rank wrapper creates a real kernel statistics enumerator");const auto handle=get(2204),size=get(2200);
 put(2300,0x11223344);ctx=context();ctx.r3.u64=handle;ctx.r4.u64=0;ctx.r5.u64=allocation+4096;ctx.r6.u64=size-1;ctx.r7.u64=allocation+2300;
 __imp__XamEnumerate(ctx,base);Check(ctx.r3.u32==122 && get(2300)==0x11223344,"Undersized statistics output is rejected without writes");
 ctx=context();ctx.r3.u64=handle;ctx.r4.u64=allocation+4096;ctx.r5.u64=size;ctx.r6.u64=allocation+2300;ctx.r7.u64=0;
 sub_8239D468(ctx,base);Check(ctx.r3.u32==0 && get(2300)==1 && get(4096)==1,"Original XEnumerate wrapper reads persistent server results");
 const auto view=get(4100);Check(rex::memory::load_and_swap<uint32_t>(base+view)==1 && rex::memory::load_and_swap<uint32_t>(base+view+8)>0,"Native leaderboard view has a guest-relative row pointer");
 const auto row=rex::memory::load_and_swap<uint32_t>(base+view+12);Check(rex::memory::load_and_swap<uint64_t>(base+row)==online_xuid && static_cast<int64_t>(rex::memory::load_and_swap<uint64_t>(base+row+16))==-9000000,"Native 48-byte row preserves XUID and signed rating");
 const auto columns=rex::memory::load_and_swap<uint32_t>(base+row+44);Check(base[columns+8]==2 && rex::memory::load_and_swap<uint32_t>(base+columns+24+16)==90000 && rex::memory::load_and_swap<uint32_t>(base+columns+48+16)>0,"Native 24-byte columns preserve race time and rank pseudo-column");
 ctx=context();ctx.r3.u64=handle;ctx.r4.u64=allocation+4096;ctx.r5.u64=size;ctx.r6.u64=allocation+2300;sub_8239D468(ctx,base);Check(ctx.r3.u32==18,"Consumed real statistics enumerator terminates normally");
 runtime.kernel_state()->object_table()->ReleaseHandle(handle);
 ctx=context();ctx.r3.u64=session;ctx.r4.u64=allocation+3000;__imp__XamSessionRefObjByHandle(ctx,base);Check(ctx.r3.u32==0,"Statistics session can be referenced independently");
 put(3100,get(3000));put(3104,0);put(3108,0);put(3112,0);
 ctx=context();ctx.r3.u64=0xFB;ctx.r4.u64=0xB0011;ctx.r5.u64=allocation+3100;ctx.r6.u64=16;__imp__XMsgInProcessCall(ctx,base);
 Check(ctx.r3.u32==0 && motogp::lan::CurrentRoom()->id==room,"Deleting a solo statistics session preserves the active online lobby");
 Check(runtime.kernel_state()->user_profile()->xuid()==profile_xuid,"Online statistics never change offline profile/save XUID");
 motogp::lan::Stop();rex::cvar::SetFlagByName("lan_enabled","true");rex::cvar::SetFlagByName("network_mode","offline");motogp::lan::Start(runtime.kernel_state());
 ctx=context();ctx.r3.u64=0;ctx.r4.u64=1;ctx.r5.u64=allocation+64;__imp__XamUserGetXUID(ctx,base);Check(rex::memory::load_and_swap<uint64_t>(base+allocation+64)==profile_xuid,"Explicit Offline overrides a stale legacy LAN flag");
 std::cout<<checks<<" real Windows generated-title online statistics checks passed\n";return 0;
 }catch(...){failure=std::current_exception();motogp::lan::Stop();return 1;}};
 auto thread=rex::system::object_ref<rex::system::XHostThread>(new rex::system::XHostThread(runtime.kernel_state(),128*1024,0,task));
 if(XFAILED(thread->Create()))throw std::runtime_error("Cannot create actual kernel test thread");
 uint64_t wait=static_cast<uint64_t>(int64_t(-300000000));const auto status=thread->Wait(0,0,0,&wait);
 if(status!=0)throw std::runtime_error("Kernel online probe thread timed out");thread->ReleaseHandle();if(failure)std::rethrow_exception(failure);return 0;
}catch(const std::exception& error){std::cerr<<"FAIL after "<<checks<<": "<<error.what()<<'\n';motogp::lan::Stop();return 1;}}
