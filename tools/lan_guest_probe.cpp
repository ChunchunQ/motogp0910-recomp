#include "motogp0910_init.h"
#include "lan/guest_bridge.h"
#include "lan/socket_bridge.h"
#include <rex/kernel/init.h>
#include <rex/runtime.h>
#include <rex/system/xam/user_profile.h>
#include <rex/system/util/object_table.h>
#include <cstring>
#include <iostream>
#include <stdexcept>
using namespace motogp::lan;
namespace {
unsigned checks=0;
void Check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);++checks;}
Options Local(){Options result;result.bind_ip="127.0.0.1";result.discovery_targets={0x7F000001};return result;}
}
int main(int argc,char** argv){try{
 if(argc!=2)throw std::runtime_error("Usage: lan_guest_probe GAME_ROOT");
 rex::Runtime runtime(std::filesystem::canonical(argv[1]));
 if(XFAILED(runtime.Setup(PPCImageConfig,{.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true})))throw std::runtime_error("Runtime setup failed");
 const auto saved_xuid=runtime.kernel_state()->user_profile()->xuid();
 auto host=std::make_shared<Node>(Local()),client=std::make_shared<Node>(Local());
 GuestBridge h(runtime.kernel_state(),host,{},"Qatar \xC5\x9E\xC4\x9F"),c(runtime.kernel_state(),client);
 const uint32_t allocation=runtime.memory()->SystemHeapAlloc(32768);
 Check(allocation!=0,"Guest test allocation");auto* base=runtime.virtual_membase();
 std::memset(base+allocation,0,32768);
 auto word=[&](unsigned offset,uint32_t value){rex::memory::store_and_swap<uint32_t>(base+allocation+offset,value);};
 auto read=[&](unsigned offset){return rex::memory::load_and_swap<uint32_t>(base+allocation+offset);};
 auto call=[&](GuestBridge& bridge,uint32_t message,uint32_t bytes){auto result=bridge.Dispatch(message,allocation+256,bytes);Check(result.has_value(),"Message implemented by guest bridge");return *result;};
 Check(!h.Valid(0,4) && !h.Valid(0xFFFFFFF0,32) && !h.Valid(allocation,0),"Invalid guest ranges rejected");
 Check(h.CreateHandle(allocation)==0 && c.CreateHandle(allocation+4)==0,"Actual kernel session handles created");
 const auto host_handle=read(0),client_handle=read(4);
 Check(host_handle!=client_handle && host_handle!=0xCAFEDEAD,"Handles are unique, not SDK sentinels");
 Check(h.Reference(host_handle,allocation+8)==0 && c.Reference(client_handle,allocation+12)==0,"Native session objects referenced");
 const auto host_object=read(8),client_object=read(12);
 runtime.kernel_state()->object_table()->ReleaseHandle(host_handle);runtime.kernel_state()->object_table()->ReleaseHandle(client_handle);
 word(16,0xAABBCCDD);Check(h.Reference(0xDEADBEEF,allocation+16)!=0 && read(16)==0xAABBCCDD,"Invalid handle leaves output intact");
 word(256,0);word(272,0x8000000B);word(276,1);Check(call(h,0xB0006,24)==0,"Host game-mode context stored");
 word(256,host_object);word(260,0x21);word(264,2);word(268,0);word(272,0);word(276,allocation+512);word(280,allocation+576);
 Check(call(h,0xB0010,28)==0 && host->CurrentRoom().has_value(),"Guest session create publishes actual room");
 Check(rex::memory::load_and_swap<uint64_t>(base+allocation+512)==host->CurrentRoom()->id,"Guest session ID equals real advertised room");
 Check(read(520)==host->Address().ip && rex::memory::load_and_swap<uint16_t>(base+allocation+528)==host->Address().port,"Guest XNADDR contains actual host endpoint");
 Check(std::memcmp(base+allocation+556,host->CurrentRoom()->key.data(),16)==0,"Guest session key equals actual host key");
 word(256,0);word(260,0);word(264,4);word(268,0);word(272,0);word(276,0);word(280,8192);word(284,allocation+4096);word(288,1);
 Check(call(c,0xB001C,36)==0 && read(4096)==1 && read(4100)==allocation+4104,"Guest SearchEx discovers real host and returns guest-relative array");
 Check(read(4104+60)==1 && read(4104+68)==1 && read(4104+76)==1 && read(4104+80)==1,"Search slot counts/name/context metadata are populated");
 const auto property=read(4104+84);const auto name=rex::memory::load_and_swap<uint32_t>(base+property+20);
 Check(rex::memory::load_and_swap<uint32_t>(base+property+16)==18,"UTF-16 host name byte length includes terminator");
 constexpr std::array<uint16_t,8> letters{'Q','a','t','a','r',' ',0x15E,0x11F};
 for(unsigned i=0;i<letters.size();++i)Check(rex::memory::load_and_swap<uint16_t>(base+name+i*2)==letters[i],"Turkish room name uses guest big-endian UTF-16");
 std::memcpy(base+allocation+1024,base+allocation+4104,60);
 std::this_thread::sleep_for(std::chrono::milliseconds(3800));
 Check(client->Rooms().empty(),"Discovery cache really expires while title search screen remains open");
 Check(c.KnownRooms().size()==1,"Copied guest search candidate survives discovery expiry");
 {
   SocketBridge sockets(c);
   PPCContext ctx{};ctx.r3.u64=1;ctx.r4.u64=allocation+1032;ctx.r5.u64=allocation+1024;ctx.r6.u64=allocation+2096;
   Check(sockets.Handle("NetDll_XNetXnAddrToInAddr",ctx,base) && ctx.r3.u32==0 && read(2096)!=0,"Expired search candidate still resolves its XNet peer address");
 }
 word(256,client_object);word(260,0x20);word(264,2);word(268,0);word(272,0);word(276,allocation+1024);word(280,allocation+1088);
 base[allocation+1068]^=1;Check(call(c,0xB0010,28)!=0 && host->Members()==1,"Wrong guest session key cannot create fake membership");
 base[allocation+1068]^=1;Check(call(c,0xB0010,28)==0 && host->Members()==2,"Guest client create performs actual remote handshake");
 auto member=[&](GuestBridge& bridge,uint32_t object,bool remote,uint64_t id){
   word(256,object);word(260,1);word(264,remote?allocation+2048:0);word(268,remote?0:allocation+2064);word(272,allocation+2068);
   rex::memory::store_and_swap<uint64_t>(base+allocation+2048,id);word(2064,0);word(2068,0);return call(bridge,0xB0012,20);
 };
 Check(member(h,host_object,false,0)==0 && member(c,client_object,false,0)==0,"Actual local members stored");
 Check(member(h,host_object,true,client->Identity())==0,"Host admits the joined remote peer");
 word(256,host_object);word(260,512);word(264,allocation+12000);word(268,0);word(272,0);word(276,0);
 Check(call(h,0xB001D,24)==0 && read(12000+32)==2 && read(12000+24)==0,"Guest details include real members and occupied slots");
 auto leave=[&](bool remote,uint32_t array){
   word(256,host_object);word(260,1);word(264,remote?array:0);word(268,remote?0:array);word(272,0);
   return call(h,0xB0013,20);
 };
 auto details=[&](){word(256,host_object);word(260,512);word(264,allocation+12000);return call(h,0xB001D,24);};
 Check(leave(true,0xFFFFFFF0)!=0 && details()==0 && read(12000+32)==2,"Invalid leave input cannot mutate membership");
 Check(leave(true,allocation+2048)==0 && details()==0 && read(12000+32)==1,"Remote leave accepts null private slots and removes only remote member");
 Check(leave(false,allocation+2064)==0 && details()==0 && read(12000+32)==0,"Local leave accepts null private slots and removes local member");
 Check(member(h,host_object,false,0)==0 && member(h,host_object,true,client->Identity())==0,"Local and remote members can rejoin after leaving");
 word(256,host_object);word(260,0);word(264,0);word(268,0);Check(call(h,0xB0014,16)==0 && host->CurrentRoom()->state==1,"Session start changes discoverability");
 Check(call(h,0xB0014,16)!=0,"Repeated start rejected");Check(call(h,0xB0015,16)==0 && host->CurrentRoom()->state==0,"Session end restores lobby state");
 word(256,client_object);Check(call(c,0xB0011,16)==0 && !client->CurrentRoom(),"Delete releases client membership");
 word(256,host_object);Check(call(h,0xB0011,16)==0 && !host->CurrentRoom(),"Delete removes actual room");
 Check(c.CreateHandle(allocation+24)==0 && c.Reference(read(24),allocation+28)==0,"Fresh client session handle for stale-result rejection");
 const auto retry_handle=read(24),retry_object=read(28);
 runtime.kernel_state()->object_table()->ReleaseHandle(retry_handle);
 word(256,retry_object);word(260,0x20);word(264,2);word(268,0);word(272,0);word(276,allocation+1024);word(280,allocation+1088);
 Check(call(c,0xB0010,28)!=0 && !client->CurrentRoom(),"Retained search result cannot fabricate a join after the host closes");
 runtime.kernel_state()->object_table()->ReleaseHandle(retry_handle);
 Check(!h.Dispatch(0xB0008,0,0),"Offline achievements are not intercepted");
 Check(runtime.kernel_state()->user_profile()->xuid()==saved_xuid,"Offline profile/save identity preserved");
 runtime.kernel_state()->object_table()->ReleaseHandle(host_handle);runtime.kernel_state()->object_table()->ReleaseHandle(client_handle);
 Check(h.Reference(host_handle,allocation+16)!=0,"Closed session cannot be referenced");
 runtime.memory()->SystemHeapFree(allocation);
 std::cout<<checks<<" actual Windows kernel/guest LAN session checks passed\n";return 0;
}catch(const std::exception& error){std::cerr<<"FAIL after "<<checks<<": "<<error.what()<<'\n';return 1;}}
