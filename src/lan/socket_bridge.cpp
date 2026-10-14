#include "socket_bridge.h"
#include <rex/memory.h>
#include <rex/system/util/object_table.h>
#include <rex/system/xevent.h>
#include <rex/system/xthread.h>
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <map>
#include <set>
#include <thread>
namespace motogp::lan {
namespace {
constexpr uint32_t kPending=0x103,kFault=10014,kInvalid=10022,kNotSocket=10038,
    kTooLarge=10040,kNotSupported=10045,kWouldBlock=10035,kAborted=995,kIncomplete=996;
thread_local uint32_t socket_error=0;
uint32_t U32(const uint8_t* p){return rex::memory::load_and_swap<uint32_t>(p);}
uint16_t U16(const uint8_t* p){return rex::memory::load_and_swap<uint16_t>(p);}
uint64_t U64(const uint8_t* p){return rex::memory::load_and_swap<uint64_t>(p);}
void W32(uint8_t* p,uint32_t v){rex::memory::store_and_swap<uint32_t>(p,v);}
void W16(uint8_t* p,uint16_t v){rex::memory::store_and_swap<uint16_t>(p,v);}
void Error(uint32_t value){socket_error=value;if(rex::system::XThread::IsInThread())rex::system::XThread::SetLastError(value);}
struct Buffer {uint32_t address=0,size=0;};
struct Request {
  std::vector<Buffer> buffers;
  uint32_t from=0,from_size=0,overlapped=0,flags=0;
  rex::system::object_ref<rex::system::XEvent> event;
  ~Request(){if(event)event->ReleaseHandle();}
};
struct Completed {uint32_t error=0,bytes=0,flags=0;};
struct GuestSocket final:rex::system::XObject {
  static constexpr Type kObjectType=Type::Socket;
  uint16_t port=0,connected_port=0;
  uint32_t connected_ip=0;
  bool nonblocking=false,closed=false;
  std::deque<Datagram> queue;
  size_t queued=0;
  std::deque<std::unique_ptr<Request>> pending;
  std::map<uint32_t,Completed> complete;
  explicit GuestSocket(rex::system::KernelState* kernel):XObject(kernel,kObjectType){}
  bool Initialize(){return CreateNative<rex::system::X_DISPATCH_HEADER>()!=nullptr;}
};
}
struct SocketBridge::Impl {
  GuestBridge& guest;
  rex::system::KernelState* kernel;
  rex::memory::Memory* memory;
  mutable std::mutex mutex;
  std::condition_variable changed;
  std::atomic<bool> stopping=false;
  std::thread receiver;
  std::map<uint32_t,rex::system::object_ref<GuestSocket>> sockets;
  std::set<uint32_t> closed;
  std::map<uint32_t,uint64_t> aliases;
  std::map<uint64_t,uint32_t> reverse;
  uint32_t next_alias=0x0AFE0001;
  std::set<uint32_t> qos_allocations;
  bool received_packet=false,sent_packet=false;
  explicit Impl(GuestBridge& bridge):guest(bridge),kernel(bridge.Kernel()),memory(kernel->memory()){
    receiver=std::thread([this]{ReceiveLoop();});
  }
  ~Impl(){
    Shutdown();
    std::lock_guard lock(mutex);
    for(auto& [id,socket]:sockets){Cancel(*socket);socket->ReleaseHandle();}
    sockets.clear();for(auto address:qos_allocations)memory->SystemHeapFree(address);
  }
  void Shutdown(){
    if(stopping.exchange(true))return;
    if(receiver.joinable())receiver.join();
    std::lock_guard lock(mutex);for(auto& [id,socket]:sockets)Cancel(*socket);changed.notify_all();
  }
  uint8_t* Ptr(uint32_t address){return memory->TranslateVirtual(address);}
  bool Valid(uint32_t address,size_t bytes,bool write=false){return guest.Valid(address,bytes,write);}
  uint32_t Arg(PPCContext& ctx,unsigned index){
    if(index<8){const std::array values{ctx.r3.u32,ctx.r4.u32,ctx.r5.u32,ctx.r6.u32,ctx.r7.u32,ctx.r8.u32,ctx.r9.u32,ctx.r10.u32};return values[index];}
    const auto address=uint64_t(ctx.r1.u32)+84+8*(index-8);
    if(address>UINT32_MAX || !Valid(static_cast<uint32_t>(address),4))throw std::runtime_error("Bad network stack argument");
    return U32(Ptr(static_cast<uint32_t>(address)));
  }
  uint64_t Fail(uint32_t value){Error(value);return UINT64_MAX;}
  GuestSocket* Socket(uint32_t handle){auto found=sockets.find(handle);return found==sockets.end() || found->second->closed?nullptr:found->second.get();}
  uint32_t Alias(uint64_t identity){
    if(auto it=reverse.find(identity);it!=reverse.end())return it->second;
    if(reverse.size()>=128)throw std::runtime_error("Too many LAN addresses");
    const auto value=next_alias++;aliases[value]=identity;reverse[identity]=value;return value;
  }
  uint64_t Peer(uint32_t ip){
    if(auto it=aliases.find(ip);it!=aliases.end())return it->second;
    for(auto id:guest.Network().ConnectedPeers())if(auto ep=guest.Network().Peer(id);ep && ep->ip==ip)return id;
    return 0;
  }
  bool Buffers(uint32_t address,uint32_t count,bool write,std::vector<Buffer>& result){
    if(!count || count>16 || !Valid(address,count*8))return false;
    uint64_t total=0;
    for(unsigned i=0;i<count;++i){auto* p=Ptr(address+i*8);Buffer b{U32(p+4),U32(p)};total+=b.size;
      if(total>1024*1024 || (b.size && !Valid(b.address,b.size,write)))return false;result.push_back(b);}
    return true;
  }
  std::unique_ptr<Request> MakeRequest(uint32_t buffers,uint32_t count,uint32_t from,
      uint32_t from_size,uint32_t ov,uint32_t flags,uint32_t routine){
    if(routine)throw uint32_t(kNotSupported);
    if(flags&~2u)throw uint32_t(kNotSupported);
    auto r=std::make_unique<Request>();r->from=from;r->from_size=from_size;r->overlapped=ov;r->flags=flags;
    if(!Buffers(buffers,count,true,r->buffers))throw uint32_t(kFault);
    if(from && (!Valid(from,16,true) || !Valid(from_size,4,true) || U32(Ptr(from_size))<16))throw uint32_t(kFault);
    if(ov){
      if(!Valid(ov,20,true))throw uint32_t(kFault);
      const auto event=U32(Ptr(ov+16))&~1u;
      if(event){r->event=kernel->object_table()->LookupObject<rex::system::XEvent>(event);if(!r->event)throw uint32_t(kInvalid);r->event->RetainHandle();r->event->Reset();}
    }
    return r;
  }
  Completed Copy(Request& request,const Datagram& packet){
    Completed c;
    for(const auto& b:request.buffers)if(b.size && !Valid(b.address,b.size,true)){c.error=kFault;return c;}
    if(request.from && (!Valid(request.from,16,true) || !Valid(request.from_size,4,true))){c.error=kFault;return c;}
    for(const auto& b:request.buffers){const auto count=std::min<size_t>(b.size,packet.bytes.size()-c.bytes);if(count)std::memcpy(Ptr(b.address),packet.bytes.data()+c.bytes,count);c.bytes+=static_cast<uint32_t>(count);}
    if(request.from){auto* out=Ptr(request.from);std::memset(out,0,16);W16(out,2);W16(out+2,packet.source_port);W32(out+4,Alias(packet.peer));W32(Ptr(request.from_size),16);}
    if(c.bytes<packet.bytes.size())c.error=kTooLarge;
    return c;
  }
  void Complete(GuestSocket& socket,Request& request,Completed value){
    if(request.overlapped){
      if(socket.complete.size()>=128 && !socket.complete.contains(request.overlapped))socket.complete.erase(socket.complete.begin());
      socket.complete[request.overlapped]=value;
      if(Valid(request.overlapped,20,true)){
        auto* ov=Ptr(request.overlapped);W32(ov+4,value.bytes);W32(ov+8,value.flags);
        std::atomic_thread_fence(std::memory_order_release);
        W32(ov,value.error==kAborted?0xC0000120:value.error==kTooLarge?0x80000005:value.error?0xC0000001:0);
      }
    }
    if(request.event)request.event->Set(0,false);
    changed.notify_all();
  }
  void Cancel(GuestSocket& socket){for(auto& r:socket.pending)Complete(socket,*r,{kAborted,0,0});socket.pending.clear();socket.queue.clear();socket.queued=0;}
  void ReceiveLoop(){
    while(!stopping.load()){
      auto packet=guest.Network().ReceiveData(std::chrono::milliseconds(20));if(!packet)continue;
      std::lock_guard lock(mutex);
      if(!received_packet){received_packet=true;REXLOG_INFO("LAN_PACKET: first receive peer={:016X} guest_port={} bytes={}",packet->peer,packet->destination_port,packet->bytes.size());}
      for(auto& [id,socket]:sockets){
        if(socket->closed || socket->port!=packet->destination_port)continue;
        bool consumed=false;
        while(!socket->pending.empty()){
          auto request=std::move(socket->pending.front());socket->pending.pop_front();
          const auto result=Copy(*request,*packet);Complete(*socket,*request,result);
          if(!(request->flags&2)){consumed=true;break;}
        }
        if(!consumed && socket->queue.size()<128 && socket->queued+packet->bytes.size()<=1024*1024){socket->queued+=packet->bytes.size();socket->queue.push_back(*packet);}
        break;
      }
      changed.notify_all();
    }
  }
  uint64_t Receive(PPCContext& ctx,bool from){
    auto* socket=Socket(ctx.r4.u32);if(!socket)return Fail(kNotSocket);
    const auto bytes=ctx.r7.u32,flags=ctx.r8.u32;
    if((bytes && !Valid(bytes,4,true)) || !Valid(flags,4,true))return Fail(kFault);
    const auto ov=from?Arg(ctx,8):ctx.r9.u32,routine=from?Arg(ctx,9):ctx.r10.u32;
    auto request=MakeRequest(ctx.r5.u32,ctx.r6.u32,from?ctx.r9.u32:0,from?ctx.r10.u32:0,ov,U32(Ptr(flags)),routine);
    if(ov && socket->pending.size()>=64)return Fail(10055);
    if(ov && std::any_of(socket->pending.begin(),socket->pending.end(),[&](const auto& p){return p->overlapped==ov;}))return Fail(kInvalid);
    if(!socket->queue.empty()){
      auto result=Copy(*request,socket->queue.front());
      if(!(request->flags&2)){socket->queued-=socket->queue.front().bytes.size();socket->queue.pop_front();}
      Complete(*socket,*request,result);if(bytes)W32(Ptr(bytes),result.bytes);W32(Ptr(flags),result.flags);
      return result.error?Fail(result.error):0;
    }
    if(bytes)W32(Ptr(bytes),0);W32(Ptr(flags),0);
    if(!ov)return Fail(kWouldBlock);
    // lpNumberOfBytesRecvd/lpFlags may be temporary stack outputs: only buffers,
    // sockaddr and OVERLAPPED survive submission. GetOverlappedResult supplies counts.
    W32(Ptr(ov+4),0);W32(Ptr(ov),kPending);socket->complete.erase(ov);
    socket->pending.push_back(std::move(request));return Fail(997);
  }
  uint32_t Transmit(GuestSocket& socket,uint32_t ip,uint16_t port,std::span<const uint8_t> data){
    if(!socket.port || !port)return kInvalid;if(data.size()>kMaxPacket-56)return kTooLarge;
    if(!sent_packet){sent_packet=true;REXLOG_INFO("LAN_PACKET: first send guest_port={} destination={}:{} bytes={}",socket.port,IPv4(ip),port,data.size());}
    if(ip==UINT32_MAX){bool sent=false;for(auto id:guest.Network().ConnectedPeers())sent|=guest.Network().SendData(id,socket.port,port,data);return sent?0:10065;}
    const auto id=Peer(ip);return id && guest.Network().Connected(id) && guest.Network().SendData(id,socket.port,port,data)?0:10065;
  }
  uint64_t Send(PPCContext& ctx,bool to){
    auto* socket=Socket(ctx.r4.u32);if(!socket)return Fail(kNotSocket);
    if(ctx.r8.u32)return Fail(kNotSupported);
    std::vector<Buffer> buffers;if(!Buffers(ctx.r5.u32,ctx.r6.u32,false,buffers))return Fail(kFault);
    std::vector<uint8_t> data;
    for(const auto& b:buffers){if(data.size()+b.size>kMaxPacket-56)return Fail(kTooLarge);if(b.size)data.insert(data.end(),Ptr(b.address),Ptr(b.address)+b.size);}
    uint32_t ip=socket->connected_ip;uint16_t port=socket->connected_port;
    if(to && ctx.r9.u32){if(ctx.r10.u32<16 || !Valid(ctx.r9.u32,16) || U16(Ptr(ctx.r9.u32))!=2)return Fail(kFault);ip=U32(Ptr(ctx.r9.u32+4));port=U16(Ptr(ctx.r9.u32+2));}
    const auto ov=to?Arg(ctx,8):ctx.r9.u32,routine=to?Arg(ctx,9):ctx.r10.u32;
    if(routine)return Fail(kNotSupported);
    if((ctx.r7.u32 && !Valid(ctx.r7.u32,4,true)) || (ov && !Valid(ov,20,true)))return Fail(kFault);
    Request request;request.overlapped=ov;
    if(ov){const auto event=U32(Ptr(ov+16))&~1u;if(event){request.event=kernel->object_table()->LookupObject<rex::system::XEvent>(event);if(!request.event)return Fail(kInvalid);request.event->RetainHandle();request.event->Reset();}}
    const auto error=Transmit(*socket,ip,port,data);if(error)return Fail(error);
    if(ctx.r7.u32)W32(Ptr(ctx.r7.u32),static_cast<uint32_t>(data.size()));
    if(ov)Complete(*socket,request,{0,static_cast<uint32_t>(data.size()),0});return 0;
  }
  uint64_t Overlapped(PPCContext& ctx,std::unique_lock<std::mutex>& lock){
    auto* socket=Socket(ctx.r4.u32);if(!socket){Error(kNotSocket);return 0;}
    if(!Valid(ctx.r5.u32,20) || !Valid(ctx.r6.u32,4,true) || !Valid(ctx.r8.u32,4,true)){Error(kFault);return 0;}
    const auto handle=ctx.r4.u32,ov=ctx.r5.u32;
    if(ctx.r7.u32)changed.wait(lock,[&]{auto* s=Socket(handle);return !s || s->complete.contains(ov) || U32(Ptr(ov))!=kPending || stopping.load();});
    socket=Socket(handle);if(!socket){Error(kAborted);return 0;}
    auto found=socket->complete.find(ov);
    if(found==socket->complete.end()){Error(U32(Ptr(ov))==kPending?kIncomplete:kInvalid);return 0;}
    W32(Ptr(ctx.r6.u32),found->second.bytes);W32(Ptr(ctx.r8.u32),found->second.flags);
    Error(found->second.error);return found->second.error?0:1;
  }
  uint32_t Qos(PPCContext& ctx,bool service){
    const auto count=service?1:ctx.r4.u32;
    const auto output=service?ctx.r6.u32:Arg(ctx,12),event=service?ctx.r5.u32:Arg(ctx,11);
    if(!count || count>20 || !Valid(output,4,true) || (!service && (!Valid(ctx.r5.u32,count*4) || !Valid(ctx.r6.u32,count*4) || !Valid(ctx.r7.u32,count*4))))return kInvalid;
    rex::system::object_ref<rex::system::XEvent> notify;
    if(event){notify=kernel->object_table()->LookupObject<rex::system::XEvent>(event);if(!notify)return kInvalid;}
    std::vector<std::optional<Room>> results(count);
    if(!service){auto rooms=guest.KnownRooms();
      for(unsigned i=0;i<count;++i){const auto xnaddr=U32(Ptr(ctx.r5.u32+i*4)),kid=U32(Ptr(ctx.r6.u32+i*4)),key=U32(Ptr(ctx.r7.u32+i*4));
        if(!Valid(xnaddr,36) || !Valid(kid,8) || !Valid(key,16))return kFault;
        const auto peer=U64(Ptr(xnaddr+16)),id=U64(Ptr(kid));
        for(const auto& room:rooms)if(room.host==peer && room.id==id && std::equal(room.key.begin(),room.key.end(),Ptr(key))){results[i]=room;break;}
      }
    }
    uint32_t size=8+count*24;for(const auto& room:results)if(room)size+=static_cast<uint32_t>((room->qos.size()+3)&~size_t(3));
    const auto allocation=memory->SystemHeapAlloc(size);if(!allocation)return 10055;
    qos_allocations.insert(allocation);auto* base=Ptr(allocation);std::memset(base,0,size);W32(base,count);uint32_t cursor=allocation+8+count*24;
    for(unsigned i=0;i<count;++i){auto* q=base+8+i*24;const auto& room=results[i];q[0]=service?3:room?3:5;
      W16(q+2,service?0:1);W16(q+4,room?1:0);
      if(room && !room->qos.empty()){q[0]|=8;W16(q+6,static_cast<uint16_t>(room->qos.size()));W32(q+8,cursor);std::copy(room->qos.begin(),room->qos.end(),Ptr(cursor));cursor+=static_cast<uint32_t>((room->qos.size()+3)&~size_t(3));}
    }
    W32(Ptr(output),allocation);if(notify)notify->Set(0,false);return 0;
  }
};
SocketBridge::SocketBridge(GuestBridge& guest):impl_(std::make_unique<Impl>(guest)){}
SocketBridge::~SocketBridge()=default;
bool SocketBridge::Owns(uint32_t handle)const{std::lock_guard lock(impl_->mutex);return impl_->sockets.contains(handle);}
void SocketBridge::CancelSessionReceives(){std::lock_guard lock(impl_->mutex);for(auto& [id,s]:impl_->sockets)impl_->Cancel(*s);}
void SocketBridge::Shutdown(){impl_->Shutdown();}
bool SocketBridge::Handle(std::string_view name,PPCContext& ctx,uint8_t* base){
  (void)base;auto& p=*impl_;std::unique_lock lock(p.mutex);
  try{
    if(p.stopping.load() && name!="NetDll_WSAGetOverlappedResult" && name!="NetDll_WSAGetLastError"){ctx.r3.u64=p.Fail(kAborted);return true;}
    if(name=="NetDll_WSAGetLastError"){ctx.r3.u64=rex::system::XThread::IsInThread()?rex::system::XThread::GetLastError():socket_error;return true;}
    if(name=="NetDll_WSASetLastError"){Error(ctx.r3.u32);return true;}
    if(name=="NetDll_XNetRandom"){
      if(ctx.r5.u32>65536 || (ctx.r5.u32 && !p.Valid(ctx.r4.u32,ctx.r5.u32,true)))ctx.r3.u64=kFault;
      else {if(ctx.r5.u32)RandomBytes({p.Ptr(ctx.r4.u32),ctx.r5.u32});ctx.r3.u64=0;}return true;
    }
    if(name=="NetDll_XNetGetTitleXnAddr"){
      if(!p.Valid(ctx.r4.u32,36,true)){ctx.r3.u64=0;return true;}p.guest.WriteAddress(ctx.r4.u32,p.guest.Network().Identity(),p.guest.Network().Address());ctx.r3.u64=0x94;return true;
    }
    if(name=="NetDll_XNetGetEthernetLinkStatus"){ctx.r3.u64=0x0B;return true;}
    if(name=="NetDll_XNetXnAddrToInAddr"){
      if(!p.Valid(ctx.r4.u32,36) || !p.Valid(ctx.r5.u32,8) || !p.Valid(ctx.r6.u32,4,true)){ctx.r3.u64=kFault;return true;}
      const auto peer=U64(p.Ptr(ctx.r4.u32+16)),id=U64(p.Ptr(ctx.r5.u32));bool known=false;
      auto rooms=p.guest.KnownRooms();
      for(const auto& room:rooms)if(room.id==id && (room.host==peer || p.guest.Network().Connected(peer) || peer==p.guest.Network().Identity()))known=true;
      if(!known){ctx.r3.u64=kInvalid;return true;}W32(p.Ptr(ctx.r6.u32),p.Alias(peer));ctx.r3.u64=0;return true;
    }
    if(name=="NetDll_XNetConnect"){
      const auto peer=p.Peer(ctx.r4.u32);if(!peer){ctx.r3.u64=kInvalid;return true;}
      if(!p.guest.Network().Connected(peer)){auto rooms=p.guest.KnownRooms();auto found=std::find_if(rooms.begin(),rooms.end(),[&](const Room& r){return r.host==peer;});
        if(found==rooms.end() || !p.guest.Network().Join(*found)){ctx.r3.u64=10060;return true;}}
      ctx.r3.u64=0;return true;
    }
    if(name=="NetDll_XNetGetConnectStatus"){const auto peer=p.Peer(ctx.r4.u32);ctx.r3.u64=peer?(p.guest.Network().Connected(peer)?2:1):0;return true;}
    if(name=="NetDll_XNetUnregisterInAddr"){auto found=p.aliases.find(ctx.r4.u32);if(found!=p.aliases.end()){p.reverse.erase(found->second);p.aliases.erase(found);}ctx.r3.u64=0;return true;}
    if(name=="NetDll_XNetQosListen"){
      const auto flags=ctx.r8.u32;if((flags&4) && (ctx.r6.u32>kMaxQos || (ctx.r6.u32 && !p.Valid(ctx.r5.u32,ctx.r6.u32)))){ctx.r3.u64=kFault;return true;}
      if(flags&4)p.guest.Network().SetQos(ctx.r6.u32?std::span<const uint8_t>(p.Ptr(ctx.r5.u32),ctx.r6.u32):std::span<const uint8_t>{});
      if(flags&0x12)p.guest.Network().SetQos({});ctx.r3.u64=0;return true;
    }
    if(name=="NetDll_XNetQosLookup" || name=="NetDll_XNetQosServiceLookup"){ctx.r3.u64=p.Qos(ctx,name=="NetDll_XNetQosServiceLookup");return true;}
    if(name=="NetDll_XNetQosRelease"){const auto address=ctx.r4.u32;if(p.qos_allocations.erase(address))p.memory->SystemHeapFree(address);else if(address){ctx.r3.u64=kInvalid;return true;}ctx.r3.u64=0;return true;}
    if(name=="NetDll_socket"){
      if(ctx.r5.u32!=2)return false;
      if(ctx.r4.u32!=2 || (ctx.r6.u32!=0 && ctx.r6.u32!=17 && ctx.r6.u32!=254)){ctx.r3.u64=p.Fail(kNotSupported);return true;}
      if(p.sockets.size()>=64){ctx.r3.u64=p.Fail(10055);return true;}
      auto socket=rex::system::object_ref<GuestSocket>(new GuestSocket(p.kernel));if(!socket->Initialize()){socket->ReleaseHandle();ctx.r3.u64=p.Fail(10055);return true;}
      const auto handle=socket->handle();p.closed.erase(handle);p.sockets[handle]=std::move(socket);ctx.r3.u64=handle;return true;
    }
    const auto handle=name=="NtClose"?ctx.r3.u32:ctx.r4.u32;
    auto* socket=p.Socket(handle);if(!socket){
      if(p.closed.contains(handle) && !p.kernel->object_table()->LookupObject<rex::system::XObject>(handle)){ctx.r3.u64=p.Fail(kNotSocket);return true;}
      return false;
    }
    if(name=="NetDll_WSARecvFrom" || name=="NetDll_WSARecv"){ctx.r3.u64=p.Receive(ctx,name=="NetDll_WSARecvFrom");return true;}
    if(name=="NetDll_WSASendTo" || name=="NetDll_WSASend"){ctx.r3.u64=p.Send(ctx,name=="NetDll_WSASendTo");return true;}
    if(name=="NetDll_WSAGetOverlappedResult"){ctx.r3.u64=p.Overlapped(ctx,lock);return true;}
    if(name=="NetDll_closesocket" || name=="NtClose"){
      p.Cancel(*socket);socket->closed=true;socket->ReleaseHandle();p.sockets.erase(handle);if(p.closed.size()>=256)p.closed.erase(p.closed.begin());p.closed.insert(handle);p.changed.notify_all();ctx.r3.u64=0;return true;
    }
    if(name=="NetDll_shutdown"){
      if(ctx.r5.u32>2){ctx.r3.u64=p.Fail(kInvalid);return true;}
      if(ctx.r5.u32!=1)p.Cancel(*socket);ctx.r3.u64=0;return true;
    }
    if(name=="NetDll_WSACancelOverlappedIO"){p.Cancel(*socket);ctx.r3.u64=0;return true;}
    if(name=="NetDll_bind"){
      if(ctx.r6.u32<16 || !p.Valid(ctx.r5.u32,16) || U16(p.Ptr(ctx.r5.u32))!=2){ctx.r3.u64=p.Fail(kFault);return true;}
      auto port=U16(p.Ptr(ctx.r5.u32+2));if(!port)port=static_cast<uint16_t>(40000+(handle&0xFFF));
      if(socket->port || std::any_of(p.sockets.begin(),p.sockets.end(),[&](const auto& s){return s.second->port==port;})){ctx.r3.u64=p.Fail(10048);return true;}
      socket->port=port;ctx.r3.u64=0;return true;
    }
    if(name=="NetDll_ioctlsocket"){
      if(!p.Valid(ctx.r6.u32,4,true)){ctx.r3.u64=p.Fail(kFault);return true;}
      if(ctx.r5.u32==0x8004667E){socket->nonblocking=U32(p.Ptr(ctx.r6.u32))!=0;ctx.r3.u64=0;return true;}
      if(ctx.r5.u32==0x4004667F){W32(p.Ptr(ctx.r6.u32),socket->queue.empty()?0:static_cast<uint32_t>(socket->queue.front().bytes.size()));ctx.r3.u64=0;return true;}
      ctx.r3.u64=p.Fail(kNotSupported);return true;
    }
    if(name=="NetDll_connect"){
      if(ctx.r6.u32<16 || !p.Valid(ctx.r5.u32,16)){ctx.r3.u64=p.Fail(kFault);return true;}
      socket->connected_ip=U32(p.Ptr(ctx.r5.u32+4));socket->connected_port=U16(p.Ptr(ctx.r5.u32+2));ctx.r3.u64=0;return true;
    }
    if(name=="NetDll_recv" || name=="NetDll_recvfrom"){
      const auto from=name=="NetDll_recvfrom"?ctx.r8.u32:0,from_len=name=="NetDll_recvfrom"?ctx.r9.u32:0;
      if((ctx.r6.u32 && !p.Valid(ctx.r5.u32,ctx.r6.u32,true)) || (from && (!p.Valid(from,16,true) || !p.Valid(from_len,4,true) || U32(p.Ptr(from_len))<16))){ctx.r3.u64=p.Fail(kFault);return true;}
      if(socket->queue.empty()){ctx.r3.u64=p.Fail(kWouldBlock);return true;}
      Request r;r.buffers.push_back({ctx.r5.u32,ctx.r6.u32});r.from=from;r.from_size=from_len;r.flags=ctx.r7.u32;
      auto result=p.Copy(r,socket->queue.front());if(!(r.flags&2)){socket->queued-=socket->queue.front().bytes.size();socket->queue.pop_front();}
      ctx.r3.u64=result.error?p.Fail(result.error):result.bytes;return true;
    }
    if(name=="NetDll_send" || name=="NetDll_sendto"){
      if(ctx.r7.u32 || (ctx.r6.u32 && !p.Valid(ctx.r5.u32,ctx.r6.u32))){ctx.r3.u64=p.Fail(kFault);return true;}
      uint32_t ip=socket->connected_ip;uint16_t port=socket->connected_port;
      if(name=="NetDll_sendto"){if(ctx.r9.u32<16 || !p.Valid(ctx.r8.u32,16)){ctx.r3.u64=p.Fail(kFault);return true;}ip=U32(p.Ptr(ctx.r8.u32+4));port=U16(p.Ptr(ctx.r8.u32+2));}
      const auto error=p.Transmit(*socket,ip,port,{p.Ptr(ctx.r5.u32),ctx.r6.u32});ctx.r3.u64=error?p.Fail(error):ctx.r6.u32;return true;
    }
    if(name=="NetDll_listen" || name=="NetDll_accept"){ctx.r3.u64=p.Fail(kNotSupported);return true;}
    // Never pass one of our virtual objects to SDK code that expects XSocket.
    ctx.r3.u64=p.Fail(kNotSupported);return true;
  }catch(uint32_t error){ctx.r3.u64=p.Fail(error);return true;}
   catch(const std::exception&){ctx.r3.u64=p.Fail(kInvalid);return true;}
}
}
