#include "runtime.h"
#include "socket_bridge.h"
#include "online/client.h"
#include "online/stats.h"
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/system/xevent.h>
#include <rex/system/xio.h>
#include <rex/system/xthread.h>
#include <rex/system/xam/user_profile.h>
#include <shared_mutex>
#include <atomic>
#ifdef _WIN32
#include <Windows.h>
#endif
REXCVAR_DEFINE_BOOL(lan_enabled,false,"LAN","Experimental LAN multiplayer; offline is the default")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
REXCVAR_DEFINE_STRING(network_mode,"legacy","Network","offline, lan or online; legacy respects the existing lan_enabled setting")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
REXCVAR_DEFINE_STRING(online_server,"https://motogp.nobkonline.com","Network","Community server HTTPS origin; used only in online mode")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
REXCVAR_DEFINE_STRING(lan_bind_ip,"","LAN","Local adapter IPv4; empty automatically prefers Radmin VPN")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
REXCVAR_DEFINE_STRING(lan_host_ip,"","LAN","Optional fallback host IPv4; empty uses automatic room discovery")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
REXCVAR_DEFINE_STRING(lan_room_name,"MotoGP room","LAN","Advertised room name, maximum 31 UTF-8 bytes")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
REXCVAR_DEFINE_STRING(lan_player_name,"Player","LAN","LAN player name, maximum 15 UTF-8 bytes")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
namespace motogp::lan {
namespace {
std::shared_mutex bridge_mutex;
std::unique_ptr<GuestBridge> bridge;
std::unique_ptr<SocketBridge> sockets;
#ifdef _WIN32
void Original(const char* name,PPCContext& ctx,uint8_t* base){
  const auto module=GetModuleHandleW(L"rexruntime.dll");
  const auto target=module?reinterpret_cast<PPCFunc*>(GetProcAddress(module,name)):nullptr;
  if(!target)throw std::runtime_error(std::string("Missing original runtime import: ")+name);
  target(ctx,base);
}
bool Message(PPCContext& ctx,uint8_t* base,bool async){
  if(!bridge || ctx.r3.u32!=0xFB)return false;
  const auto kernel=rex::system::kernel_state();
  const auto overlapped=async?ctx.r5.u32:0;
  rex::system::object_ref<rex::system::XEvent> completion_event;
  struct EventHandle {
    rex::system::XEvent* value=nullptr;
    ~EventHandle(){if(value)value->ReleaseHandle();}
  } retained_event;
  if(overlapped){
    if(!bridge->Valid(overlapped,sizeof(rex::system::XAM_OVERLAPPED),true)){
      ctx.r3.u64=0x80070057;return true;
    }
    auto* p=kernel->memory()->TranslateVirtual(overlapped);
    const auto event=rex::system::XOverlappedGetEvent(p);
    if(event){
      completion_event=kernel->object_table()->LookupObject<rex::system::XEvent>(event);
      if(!completion_event){ctx.r3.u64=0x80070006;return true;}
      completion_event->RetainHandle();retained_event.value=completion_event.get();
    }
    if(!rex::system::XThread::IsInThread() && rex::system::XOverlappedGetCompletionRoutine(p)){
      ctx.r3.u64=0x80070057;return true;
    }
  }
  if(overlapped && bridge->Online())if(auto operation=bridge->StatisticsOperation(ctx.r4.u32,ctx.r6.u32,ctx.r7.u32)){
    if(!rex::system::XThread::IsInThread()){
      const auto result=operation();kernel->CompleteOverlappedEx(overlapped,result,result,result?UINT32_MAX:0);ctx.r3.u64=997;return true;
    }
    // Input structs are stack-backed in the generated XDK wrappers. Copy them
    // before returning; HTTPS runs on the kernel completion worker, never on
    // the racing thread. Keep the completion event handle valid until finished.
    std::shared_ptr<rex::system::XEvent> event_hold;
    if(retained_event.value){event_hold={retained_event.value,[](auto* event){event->ReleaseHandle();}};retained_event.value=nullptr;}
    kernel->CompleteOverlappedDeferredEx([operation=std::move(operation),event_hold](uint32_t& extended,uint32_t& length){
      (void)event_hold;
      const auto result=operation();extended=result;length=result?UINT32_MAX:0;return result;
    },overlapped);
    ctx.r3.u64=997;return true;
  }
  const auto before_delete=ctx.r4.u32==0xB0011?bridge->Network().CurrentRoom():std::nullopt;
  const auto result=bridge->Dispatch(ctx.r4.u32,async?ctx.r6.u32:ctx.r5.u32,async?ctx.r7.u32:ctx.r6.u32);
  if(!result)return false;
  if(ctx.r4.u32!=0xB0006 && ctx.r4.u32!=0xB0007){
    const auto room=bridge->Network().CurrentRoom();
    REXLOG_DEBUG("LAN_SESSION: message={:08X} result={:08X} room={:016X} members={}",ctx.r4.u32,*result,room?room->id:0,room?room->members:0);
  }
  if(ctx.r4.u32==0xB0011 && *result==0 && sockets && before_delete && !bridge->Network().CurrentRoom())sockets->CancelSessionReceives();
  if(overlapped){
    if(rex::system::XThread::IsInThread())kernel->CompleteOverlappedImmediate(overlapped,*result);
    else kernel->CompleteOverlappedEx(overlapped,*result,*result,*result?UINT32_MAX:0);
    ctx.r3.u64=997;
  }else ctx.r3.u64=*result;
  if(rex::system::XThread::IsInThread())rex::system::XThread::SetLastError(*result?(*result&0xFFFF):0);
  return true;
}
uint32_t Parent(PPCContext& ctx,uint8_t* base){
  const auto address=uint64_t(ctx.r1.u32)+88;
  return address<=UINT32_MAX && bridge->Valid(static_cast<uint32_t>(address),4)?rex::memory::load_and_swap<uint32_t>(base+address):0;
}
bool NetworkXuidCaller(PPCContext& ctx,uint8_t* base){
  if(ctx.r4.u32==2 || ctx.r4.u32==4)return true;
  if(ctx.lr!=0x8239CA18)return false;
  const auto parent=Parent(ctx,base);
  // The title's session host/join, local-member predicate and online refresh.
  // Profile/save caller 0x8224A838 and all other callers keep the original XUID.
  return parent==0x823042D0 || parent==0x82305844 || parent==0x822FE560 || parent==0x822FF960;
}
std::string PlayerName(){
  auto name=rex::cvar::Query<std::string>("lan_player_name");
  if(name=="Player"){const auto id=bridge->Network().Identity();static constexpr char hex[]="0123456789ABCDEF";name+=" ";for(int shift=12;shift>=0;shift-=4)name+=hex[(id>>shift)&15];}
  return name;
}
#endif
}
void Start(rex::system::KernelState* kernel){
  std::unique_lock lock(bridge_mutex);
  sockets.reset();
  bridge.reset();
  auto mode=rex::cvar::Query<std::string>("network_mode");
  if(mode=="legacy")mode=rex::cvar::Query<bool>("lan_enabled")?"lan":"offline";
  if(mode=="offline")return;
  if(mode!="lan" && mode!="online")throw std::runtime_error("Invalid network_mode; choose offline, lan or online");
#ifdef _WIN32
  Options options;options.bind_ip=rex::cvar::Query<std::string>("lan_bind_ip");
  options.log=[](const std::string& line){REXLOG_WARN("{}",line);};
  const auto player=rex::cvar::Query<std::string>("lan_player_name");
  if(player.empty() || player.size()>15 || player.find('\0')!=std::string::npos)throw std::runtime_error("LAN player name must contain 1-15 UTF-8 bytes");
  std::shared_ptr<online::Client> service;
  if(mode=="online"){
    service=std::make_shared<online::Client>(rex::cvar::Query<std::string>("online_server"),player);
    options.bind_ip.clear();options.relay=service->Connection();
    REXLOG_DEBUG("ONLINE_STARTUP: authenticated community connection; relay port={}",options.relay->endpoint.port);
  }
  REXLOG_DEBUG("LAN_STARTUP: initializing adapter={}",options.bind_ip.empty()?"automatic":options.bind_ip);
  auto node=std::make_shared<Node>(options);
  auto next_bridge=std::make_unique<GuestBridge>(kernel,node,mode=="online"?"":rex::cvar::Query<std::string>("lan_host_ip"),rex::cvar::Query<std::string>("lan_room_name"),service);
  auto next_sockets=std::make_unique<SocketBridge>(*next_bridge);
  bridge=std::move(next_bridge);sockets=std::move(next_sockets);
  REXLOG_INFO("NETWORK_READY: mode={} adapter={} endpoint={}:{} discovery={}",mode,node->AdapterName(),IPv4(node->Address().ip),node->Address().port,
      rex::cvar::Query<std::string>("lan_host_ip").empty()?"automatic":"direct-IP fallback");
#else
  (void)kernel;throw std::runtime_error("The LAN preview targets Windows");
#endif
}
void Stop(){
  {std::shared_lock lock(bridge_mutex);if(sockets)sockets->Shutdown();}
  std::unique_lock lock(bridge_mutex);sockets.reset();bridge.reset();
}
std::optional<Room> CurrentRoom(){std::shared_lock lock(bridge_mutex);return bridge?bridge->Network().CurrentRoom():std::nullopt;}
#ifdef _WIN32
REX_HOOK_RAW(__imp__NetDll_XNetGetOpt){
  // XNetGetOpt(caller, id, value, size). The race/lobby code polls option 6,
  // XNET_OPTID_CALLER_XMIT_BYTES (ULONGLONG bytes sent by the title), about
  // four times a second for its bandwidth meter. The pinned runtime does not
  // implement it, so every poll logged an error and returned WSAEINVAL.
  // Answer it with the bytes the title has sent through the LAN bridge.
  if(ctx.r3.u32==1 && ctx.r4.u32==6){
    std::shared_lock lock(bridge_mutex);
    const auto value=ctx.r5.u32,size=ctx.r6.u32;
    if(!bridge || !size || !bridge->Valid(size,4,true)){ctx.r3.u64=10022;return;}
    auto* size_ptr=base+size;
    const auto available=rex::memory::load_and_swap<uint32_t>(size_ptr);
    rex::memory::store_and_swap<uint32_t>(size_ptr,8);
    if(available<8){ctx.r3.u64=10040;return;}  // WSAEMSGSIZE
    if(!value || !bridge->Valid(value,8,true)){ctx.r3.u64=10014;return;}  // WSAEFAULT
    rex::memory::store_and_swap<uint64_t>(base+value,TitleBytesSent());
    ctx.r3.u64=0;return;
  }
  Original("__imp__NetDll_XNetGetOpt",ctx,base);
}
REX_HOOK_RAW(__imp__XamUserCreateStatsEnumerator){
  std::shared_lock lock(bridge_mutex);
  if(!bridge || !bridge->Online()){Original("__imp__XamUserCreateStatsEnumerator",ctx,base);return;}
  if(ctx.r3.u32 && ctx.r3.u32!=kTitle){ctx.r3.u64=87;return;}
  auto first=ctx.r5.u64;
  if(ctx.r4.u32==0 && first==bridge->Kernel()->user_profile()->xuid())first=bridge->Network().Identity();
  ctx.r3.u64=online::CreateEnumerator(*bridge,ctx.r4.u32,first,ctx.r6.u32,ctx.r7.u32,ctx.r8.u32,ctx.r9.u32,ctx.r10.u32);
}
REX_HOOK_RAW(__imp__XamEnumerate){
  std::shared_lock lock(bridge_mutex);
  if(bridge && bridge->Online())if(const auto size=online::EnumeratorSize(bridge->Kernel(),ctx.r3.u32)){
    // XamEnumerate ABI: handle, flags, buffer, length, items, overlapped.
    if(ctx.r4.u32 || ctx.r6.u32<*size || !bridge->Valid(ctx.r5.u32,*size,true) ||
       (ctx.r7.u32 && !bridge->Valid(ctx.r7.u32,4,true)) ||
       (ctx.r8.u32 && !bridge->Valid(ctx.r8.u32,sizeof(rex::system::XAM_OVERLAPPED),true))){ctx.r3.u64=ctx.r6.u32<*size?122:87;return;}
  }
  Original("__imp__XamEnumerate",ctx,base);
}
REX_HOOK_RAW(__imp__XamUserGetSigninState){
  std::shared_lock lock(bridge_mutex);if(bridge && ctx.r3.u32==0)ctx.r3.u64=2;else Original("__imp__XamUserGetSigninState",ctx,base);
}
REX_HOOK_RAW(__imp__XamUserCheckPrivilege){
  std::shared_lock lock(bridge_mutex);
  if(bridge && (ctx.r3.u32==0 || ctx.r3.u32==0xFF) && (ctx.r4.u32==254 || ctx.r4.u32==252 || ctx.r4.u32==251 || ctx.r4.u32==247)){
    if(!bridge->Valid(ctx.r5.u32,4,true)){ctx.r3.u64=87;return;}
    rex::memory::store_and_swap<uint32_t>(base+ctx.r5.u32,1);ctx.r3.u64=0;
  }else Original("__imp__XamUserCheckPrivilege",ctx,base);
}
REX_HOOK_RAW(__imp__XamUserGetXUID){
  std::shared_lock lock(bridge_mutex);
  if(bridge && ctx.r3.u32==0 && NetworkXuidCaller(ctx,base)){
    if(!bridge->Valid(ctx.r5.u32,8,true)){ctx.r3.u64=0x80070057;return;}
    rex::memory::store_and_swap<uint64_t>(base+ctx.r5.u32,bridge->Network().Identity());ctx.r3.u64=0;
  }else Original("__imp__XamUserGetXUID",ctx,base);
}
REX_HOOK_RAW(__imp__XamUserGetSigninInfo){
  std::shared_lock lock(bridge_mutex);const auto user=ctx.r3.u32,flags=ctx.r4.u32,info=ctx.r5.u32;
  if(bridge && !bridge->Valid(info,40,true)){ctx.r3.u64=0x80070057;return;}
  const bool network=bridge && ((flags&2) || (ctx.lr==0x8239C480 && Parent(ctx,base)==0x8230382C));
  Original("__imp__XamUserGetSigninInfo",ctx,base);
  if(bridge && user==0 && ctx.r3.u32==0){
    rex::memory::store_and_swap<uint32_t>(base+info+8,1);rex::memory::store_and_swap<uint32_t>(base+info+12,2);
    if(network){rex::memory::store_and_swap<uint64_t>(base+info,bridge->Network().Identity());const auto name=PlayerName();std::memset(base+info+24,0,16);std::copy(name.begin(),name.end(),base+info+24);}
  }
}
REX_HOOK_RAW(__imp__XamUserGetName){
  std::shared_lock lock(bridge_mutex);
  if(bridge && ctx.r3.u32==0){const auto name=PlayerName();if(ctx.r5.u32<=name.size() || !bridge->Valid(ctx.r4.u32,ctx.r5.u32,true)){ctx.r3.u64=87;return;}std::copy(name.begin(),name.end(),base+ctx.r4.u32);base[ctx.r4.u32+name.size()]=0;ctx.r3.u64=0;}
  else Original("__imp__XamUserGetName",ctx,base);
}
REX_HOOK_RAW(__imp__XNetLogonGetTitleID){std::shared_lock lock(bridge_mutex);if(bridge)ctx.r3.u64=kTitle;else Original("__imp__XNetLogonGetTitleID",ctx,base);}
REX_HOOK_RAW(__imp__XamUserGetMembershipTierFromXUID){
  std::shared_lock lock(bridge_mutex);if(bridge)ctx.r3.u64=(ctx.r3.u64==bridge->Network().Identity() || bridge->Network().Connected(ctx.r3.u64))?6:0;else Original("__imp__XamUserGetMembershipTierFromXUID",ctx,base);
}
#define MOTOGP_NET_IMPORT(name) \
REX_HOOK_RAW(__imp__##name){ \
  std::shared_lock lock(bridge_mutex); \
  if(!sockets || !sockets->Handle(#name,ctx,base))Original("__imp__" #name,ctx,base); \
}
MOTOGP_NET_IMPORT(NetDll_socket)
MOTOGP_NET_IMPORT(NetDll_bind)
MOTOGP_NET_IMPORT(NetDll_ioctlsocket)
MOTOGP_NET_IMPORT(NetDll_closesocket)
MOTOGP_NET_IMPORT(NetDll_shutdown)
MOTOGP_NET_IMPORT(NetDll_connect)
MOTOGP_NET_IMPORT(NetDll_listen)
MOTOGP_NET_IMPORT(NetDll_accept)
MOTOGP_NET_IMPORT(NetDll_send)
MOTOGP_NET_IMPORT(NetDll_sendto)
MOTOGP_NET_IMPORT(NetDll_recv)
MOTOGP_NET_IMPORT(NetDll_recvfrom)
MOTOGP_NET_IMPORT(NetDll_WSASendTo)
MOTOGP_NET_IMPORT(NetDll_WSASend)
MOTOGP_NET_IMPORT(NetDll_WSARecvFrom)
MOTOGP_NET_IMPORT(NetDll_WSARecv)
MOTOGP_NET_IMPORT(NetDll_WSAGetOverlappedResult)
MOTOGP_NET_IMPORT(NetDll_WSACancelOverlappedIO)
MOTOGP_NET_IMPORT(NetDll_WSAGetLastError)
MOTOGP_NET_IMPORT(NetDll_WSASetLastError)
MOTOGP_NET_IMPORT(NetDll_XNetRandom)
MOTOGP_NET_IMPORT(NetDll_XNetGetTitleXnAddr)
MOTOGP_NET_IMPORT(NetDll_XNetGetEthernetLinkStatus)
MOTOGP_NET_IMPORT(NetDll_XNetXnAddrToInAddr)
MOTOGP_NET_IMPORT(NetDll_XNetConnect)
MOTOGP_NET_IMPORT(NetDll_XNetGetConnectStatus)
MOTOGP_NET_IMPORT(NetDll_XNetUnregisterInAddr)
MOTOGP_NET_IMPORT(NetDll_XNetQosListen)
MOTOGP_NET_IMPORT(NetDll_XNetQosLookup)
MOTOGP_NET_IMPORT(NetDll_XNetQosServiceLookup)
MOTOGP_NET_IMPORT(NetDll_XNetQosRelease)
MOTOGP_NET_IMPORT(NtClose)
#undef MOTOGP_NET_IMPORT
REX_HOOK_RAW(__imp__XamSessionCreateHandle){
  std::shared_lock lock(bridge_mutex);
  if(bridge)ctx.r3.u64=bridge->CreateHandle(ctx.r3.u32);else Original("__imp__XamSessionCreateHandle",ctx,base);
}
REX_HOOK_RAW(__imp__XamSessionRefObjByHandle){
  std::shared_lock lock(bridge_mutex);
  if(bridge)ctx.r3.u64=bridge->Reference(ctx.r3.u32,ctx.r4.u32);else Original("__imp__XamSessionRefObjByHandle",ctx,base);
}
REX_HOOK_RAW(__imp__XMsgInProcessCall){
  std::shared_lock lock(bridge_mutex);
  if(!Message(ctx,base,false))Original("__imp__XMsgInProcessCall",ctx,base);
}
REX_HOOK_RAW(__imp__XMsgStartIORequest){
  std::shared_lock lock(bridge_mutex);
  if(!Message(ctx,base,true))Original("__imp__XMsgStartIORequest",ctx,base);
}
REX_HOOK_RAW(__imp__XMsgStartIORequestEx){
  std::shared_lock lock(bridge_mutex);
  if(!Message(ctx,base,true))Original("__imp__XMsgStartIORequestEx",ctx,base);
}
#endif
}
