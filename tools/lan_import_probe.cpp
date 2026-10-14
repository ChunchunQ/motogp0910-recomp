#include "motogp0910_init.h"
#include "lan/runtime.h"
#include "../src/launcher/core.h"
#include <rex/kernel/init.h>
#include <rex/runtime.h>
#include <rex/system/xio.h>
#include <rex/system/xevent.h>
#include <fstream>
#include <iostream>
#include <thread>
REX_EXTERN(__imp__XamSessionCreateHandle);
REX_EXTERN(__imp__XamSessionRefObjByHandle);
REX_EXTERN(__imp__XMsgInProcessCall);
REX_EXTERN(__imp__XMsgStartIORequest);
REX_EXTERN(sub_825D9668);
REX_EXTERN(sub_825D9DA8);
REX_EXTERN(sub_825D9A28);
REX_EXTERN(sub_825D9AD0);
namespace {
unsigned checks=0;
void Check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);++checks;}
struct Fixture {
  rex::Runtime runtime;
  uint32_t allocation=0,handle=0;
  rex::system::object_ref<rex::system::XEvent> event;
  Fixture(const char* root):runtime(std::filesystem::canonical(root)){
    if(XFAILED(runtime.Setup(PPCImageConfig,{.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true})))throw std::runtime_error("Runtime setup failed");
    allocation=runtime.memory()->SystemHeapAlloc(65536);Check(allocation!=0,"Guest allocation");
    std::memset(runtime.virtual_membase()+allocation,0,65536);
    event=rex::system::object_ref<rex::system::XEvent>(new rex::system::XEvent(runtime.kernel_state()));
    event->Initialize(true,false);
  }
  ~Fixture(){motogp::lan::Stop();if(event)event->ReleaseHandle();}
  uint8_t* Base(){return runtime.virtual_membase();}
  void Put(unsigned offset,uint32_t value){rex::memory::store_and_swap<uint32_t>(Base()+allocation+offset,value);}
  uint32_t Get(unsigned offset){return rex::memory::load_and_swap<uint32_t>(Base()+allocation+offset);}
  PPCContext Context(){PPCContext ctx{};ctx.r1.u64=allocation+60000;return ctx;}
  void Start(const char* direct=""){
    rex::cvar::SetFlagByName("lan_enabled","true");rex::cvar::SetFlagByName("lan_bind_ip","127.0.0.1");
    rex::cvar::SetFlagByName("lan_host_ip",direct);rex::cvar::SetFlagByName("lan_room_name","Two-process MotoGP guest");
    motogp::lan::Start(runtime.kernel_state());
  }
  void Create(bool host){
    event->Reset();Put(140,event->handle());
    auto ctx=Context();ctx.r3.u64=host?0x21:0x20;ctx.r4.u64=0;ctx.r5.u64=2;ctx.r6.u64=0;
    ctx.r7.u64=allocation+576;ctx.r8.u64=allocation+512;ctx.r9.u64=allocation+128;ctx.r10.u64=allocation;
    sub_825D9668(ctx,Base());
    Check(ctx.r3.u32==997,"Actual title XSessionCreate returns IO_PENDING");
    Check(Get(128)==0 && Get(152)==0,"Actual title create completes real XAM overlapped successfully");
    uint32_t type=0,state=0;event->Query(&type,&state);Check(state==1,"Real XAM create completion signals actual kernel event");
    handle=Get(0);Check(handle!=0 && handle!=0xCAFEDEAD,"Actual title gets real kernel handle");
    auto room=motogp::lan::CurrentRoom();Check(room && room->id==rex::memory::load_and_swap<uint64_t>(Base()+allocation+512),"Title output equals actual network room");
  }
  void Search(){
    event->Reset();Put(172,event->handle());
    Put(256,1326*4+8);
    auto ctx=Context();ctx.r3.u64=0;ctx.r4.u64=0;ctx.r5.u64=4;ctx.r6.u64=1;
    rex::memory::store_and_swap<uint32_t>(Base()+ctx.r1.u32+84,allocation+256);
    rex::memory::store_and_swap<uint32_t>(Base()+ctx.r1.u32+92,allocation+4096);
    rex::memory::store_and_swap<uint32_t>(Base()+ctx.r1.u32+100,allocation+160);
    sub_825D9DA8(ctx,Base());
    Check(ctx.r3.u32==997 && Get(160)==0,"Actual title SearchEx completes real XAM overlapped");
    uint32_t type=0,state=0;event->Query(&type,&state);Check(state==1,"Real XAM search completion signals actual kernel event");
    Check(Get(4096)==1 && Get(4100)==allocation+4104,"Actual title SearchEx finds actual separate host");
    std::memcpy(Base()+allocation+512,Base()+allocation+4104,60);
  }
  void Delete(){
    auto ctx=Context();ctx.r3.u64=handle;ctx.r4.u64=allocation+8;
    __imp__XamSessionRefObjByHandle(ctx,Base());Check(ctx.r3.u32==0,"Actual import references created session");
    Put(256,Get(8));Put(260,0);Put(264,0);Put(268,0);
    ctx=Context();ctx.r3.u64=0xFB;ctx.r4.u64=0xB0011;ctx.r5.u64=allocation+256;ctx.r6.u64=16;
    __imp__XMsgInProcessCall(ctx,Base());Check(ctx.r3.u32==0 && !motogp::lan::CurrentRoom(),"Actual XGI delete closes session");
    runtime.kernel_state()->object_table()->ReleaseHandle(handle);runtime.kernel_state()->object_table()->ReleaseHandle(handle);
  }
  void LeaveMembers(){
    motogp::lan::Options options;options.bind_ip="127.0.0.1";
    motogp::lan::Node client(options);
    Check(client.Join(*motogp::lan::CurrentRoom()),"Leave fixture performs real remote join");
    auto ctx=Context();ctx.r3.u64=handle;ctx.r4.u64=allocation+8;
    __imp__XamSessionRefObjByHandle(ctx,Base());Check(ctx.r3.u32==0,"Reference session for membership regression");
    const auto object=Get(8);
    rex::memory::store_and_swap<uint64_t>(Base()+allocation+2048,client.Identity());Put(2064,0);Put(2068,0);
    for(bool remote:{false,true}){
      Put(256,object);Put(260,1);Put(264,remote?allocation+2048:0);Put(268,remote?0:allocation+2064);Put(272,allocation+2068);
      ctx=Context();ctx.r3.u64=0xFB;ctx.r4.u64=0xB0012;ctx.r5.u64=allocation+256;ctx.r6.u64=20;
      __imp__XMsgInProcessCall(ctx,Base());Check(ctx.r3.u32==0,"Admit real local/remote fixture member");
    }
    for(bool remote:{true,false}){
      event->Reset();Put(140,event->handle());
      ctx=Context();ctx.r3.u64=handle;ctx.r4.u64=1;ctx.r5.u64=remote?allocation+2048:allocation+2064;ctx.r6.u64=allocation+128;
      if(remote)sub_825D9AD0(ctx,Base());else sub_825D9A28(ctx,Base());
      Check(ctx.r3.u32==997 && Get(128)==0,"Original generated leave wrapper accepts its null private-slot field");
      uint32_t type=0,state=0;event->Query(&type,&state);Check(state==1,"Original leave signals real completion event");
      Put(256,object);Put(260,512);Put(264,allocation+12000);Put(268,0);Put(272,0);Put(276,0);
      ctx=Context();ctx.r3.u64=0xFB;ctx.r4.u64=0xB001D;ctx.r5.u64=allocation+256;ctx.r6.u64=24;
      __imp__XMsgInProcessCall(ctx,Base());Check(ctx.r3.u32==0 && Get(12000+32)==(remote?1:0),"Original leave updates actual session membership");
    }
    runtime.kernel_state()->object_table()->ReleaseHandle(handle);
    client.Leave();
  }
};
}
int main(int argc,char** argv){try{
 if(argc<2)throw std::runtime_error("Usage: lan_import_probe GAME_ROOT [host READY STOP | client automatic|direct]");
 if(argc==3 && std::string(argv[2])=="startup-auto"){
   launcher::Settings settings;settings.lan_enabled=true;settings.lan_player_name="inflames";settings.lan_room_name="test";
   settings.bindings[0]=""; // Intentional unbinding must survive the empty-CLI fix.
   toml::table config;settings.Store(config);
   const auto file=std::filesystem::temp_directory_path()/("motogp-lan-startup-"+std::to_string(GetCurrentProcessId())+".toml");
   struct Cleanup{std::filesystem::path file;~Cleanup(){std::error_code error;std::filesystem::remove(file,error);}} cleanup{file};
   {std::ofstream output(file);output<<toml::toml_formatter{config};if(!output)throw std::runtime_error("Cannot write startup fixture");}
   auto command=launcher::Arguments(std::filesystem::canonical(argv[1]),settings,config);
   std::vector<std::string> arguments{"probe"};for(const auto& argument:command)arguments.push_back(launcher::Utf8(argument));
   std::vector<char*> pointers;for(auto& argument:arguments)pointers.push_back(argument.data());
   rex::cvar::Init(static_cast<int>(pointers.size()),pointers.data());rex::cvar::LoadConfig(file);
   Check(rex::cvar::Query<bool>("lan_enabled"),"Actual launcher command preserves LAN enabled");
   Check(rex::cvar::Query<std::string>("lan_bind_ip").empty(),"Auto local IP stays empty after real SDK CLI parsing");
   Check(rex::cvar::Query<std::string>("lan_host_ip").empty(),"Auto host IP stays empty after real SDK CLI parsing");
   Check(rex::cvar::Query<std::string>("lan_player_name")=="inflames","Player name is not consumed as the previous argument");
   Check(rex::cvar::Query<std::string>("lan_room_name")=="test","Room name survives actual launcher and SDK parser");
   Check(rex::cvar::Query<std::string>("keybind_a").empty(),"Intentional unbound key loads from saved TOML");
   Fixture fixture(argv[1]);Check(!XFAILED(fixture.runtime.LoadXexImage("game:\\default.xex")),"Load real TU before automatic LAN start");
   for(unsigned i=0;i<3;++i){motogp::lan::Start(fixture.runtime.kernel_state());motogp::lan::Stop();}
   rex::cvar::SetFlagFromCommandLine("lan_bind_ip","--lan_enabled=true");bool rejected=false;
   try{motogp::lan::Start(fixture.runtime.kernel_state());}catch(const std::exception& error){rejected=std::string(error.what())=="Invalid LAN local IPv4";}
   Check(rejected,"Reported malformed CLI IP is rejected without publishing a partial bridge");motogp::lan::Stop();
   auto ctx=fixture.Context();ctx.r3.u64=fixture.allocation+32;__imp__XamSessionCreateHandle(ctx,fixture.Base());
   Check(ctx.r3.u32==0 && fixture.Get(32)==0xCAFEDEAD,"Failed startup restores original offline imports");
   rex::cvar::SetFlagFromCommandLine("lan_bind_ip","");motogp::lan::Start(fixture.runtime.kernel_state());fixture.Create(true);fixture.Delete();
   std::cout<<checks<<" actual launcher/SDK auto-LAN startup checks passed\n";return 0;
 }
 Fixture fixture(argv[1]);
 if(argc==2){
   auto ctx=fixture.Context();ctx.r3.u64=fixture.allocation+32;__imp__XamSessionCreateHandle(ctx,fixture.Base());
   Check(ctx.r3.u32==0 && fixture.Get(32)==0xCAFEDEAD,"Disabled LAN preserves original SDK import behavior");
   fixture.Start();fixture.Create(true);fixture.LeaveMembers();fixture.Delete();motogp::lan::Stop();
   ctx=fixture.Context();ctx.r3.u64=fixture.allocation+32;__imp__XamSessionCreateHandle(ctx,fixture.Base());
   Check(ctx.r3.u32==0 && fixture.Get(32)==0xCAFEDEAD,"Stopped bridge restores original imports");
 }else if(argc==5 && std::string(argv[2])=="host"){
   fixture.Start();fixture.Create(true);{std::ofstream file(argv[3]);file<<fixture.handle<<'\n';}
   const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(20);bool joined=false;
   while(std::chrono::steady_clock::now()<deadline && !std::filesystem::exists(argv[4])){
     if(auto room=motogp::lan::CurrentRoom())joined|=room->members==2;
     std::this_thread::sleep_for(std::chrono::milliseconds(10));
   }
   Check(joined,"Actual guest host observed real client membership");fixture.Delete();
 }else if(argc==4 && std::string(argv[2])=="client"){
   fixture.Start(std::string(argv[3])=="direct"?"127.0.0.1":"");fixture.Search();
   if(std::string(argv[3])=="delayed")std::this_thread::sleep_for(std::chrono::milliseconds(3800));
   fixture.Create(false);
   std::this_thread::sleep_for(std::chrono::milliseconds(100));fixture.Delete();
 }else throw std::runtime_error("Unknown test mode");
 std::cout<<checks<<" actual MotoGP generated-wrapper/LAN import checks passed\n";return 0;
}catch(const std::exception& error){std::cerr<<"FAIL after "<<checks<<": "<<error.what()<<'\n';return 1;}}
