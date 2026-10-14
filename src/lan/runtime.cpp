#include "runtime.h"
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/system/xevent.h>
#include <rex/system/xio.h>
#include <rex/system/xthread.h>
#include <shared_mutex>
#ifdef _WIN32
#include <Windows.h>
#endif
REXCVAR_DEFINE_BOOL(lan_enabled,false,"LAN","Experimental native LAN lobby bridge; offline is the default")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
REXCVAR_DEFINE_STRING(lan_bind_ip,"","LAN","Local adapter IPv4; empty automatically prefers Radmin VPN")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
REXCVAR_DEFINE_STRING(lan_host_ip,"","LAN","Optional fallback host IPv4; empty uses automatic room discovery")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
REXCVAR_DEFINE_STRING(lan_room_name,"MotoGP room","LAN","Advertised room name, maximum 31 UTF-8 bytes")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
namespace motogp::lan {
namespace {
std::shared_mutex bridge_mutex;
std::unique_ptr<GuestBridge> bridge;
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
  const auto result=bridge->Dispatch(ctx.r4.u32,async?ctx.r6.u32:ctx.r5.u32,async?ctx.r7.u32:ctx.r6.u32);
  if(!result)return false;
  if(overlapped){
    if(rex::system::XThread::IsInThread())kernel->CompleteOverlappedImmediate(overlapped,*result);
    else kernel->CompleteOverlappedEx(overlapped,*result,*result,*result?UINT32_MAX:0);
    ctx.r3.u64=997;
  }else ctx.r3.u64=*result;
  if(rex::system::XThread::IsInThread())rex::system::XThread::SetLastError(*result?(*result&0xFFFF):0);
  return true;
}
#endif
}
void Start(rex::system::KernelState* kernel){
  std::unique_lock lock(bridge_mutex);
  bridge.reset();
  if(!rex::cvar::Query<bool>("lan_enabled"))return;
#ifdef _WIN32
  Options options;options.bind_ip=rex::cvar::Query<std::string>("lan_bind_ip");
  auto node=std::make_shared<Node>(options);
  bridge=std::make_unique<GuestBridge>(kernel,node,rex::cvar::Query<std::string>("lan_host_ip"),rex::cvar::Query<std::string>("lan_room_name"));
  REXLOG_INFO("LAN_LOBBY: adapter={} endpoint={}:{} discovery={} race_transport=unfinished",node->AdapterName(),IPv4(node->Address().ip),node->Address().port,
      rex::cvar::Query<std::string>("lan_host_ip").empty()?"automatic":"direct-IP fallback");
#else
  (void)kernel;throw std::runtime_error("The LAN preview targets Windows");
#endif
}
void Stop(){std::unique_lock lock(bridge_mutex);bridge.reset();}
std::optional<Room> CurrentRoom(){std::shared_lock lock(bridge_mutex);return bridge?bridge->Network().CurrentRoom():std::nullopt;}
#ifdef _WIN32
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
