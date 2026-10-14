#include "node.h"
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <map>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <utility>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <MSWSock.h>
#include <iphlpapi.h>
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
namespace motogp::lan {
namespace {
using Clock=std::chrono::steady_clock;
#ifdef _WIN32
using Socket=SOCKET;constexpr Socket kInvalid=INVALID_SOCKET;
struct NetworkScope {
 NetworkScope(){WSADATA data{};if(WSAStartup(MAKEWORD(2,2),&data))throw std::runtime_error("LAN Winsock initialization failed");}
 ~NetworkScope(){WSACleanup();}
};
void Close(Socket s){closesocket(s);}
#else
using Socket=int;constexpr Socket kInvalid=-1;
struct NetworkScope {};
void Close(Socket s){close(s);}
#endif
struct SocketOwner {Socket value=kInvalid;~SocketOwner(){if(value!=kInvalid)Close(value);}};
sockaddr_in Native(Endpoint ep){sockaddr_in out{};out.sin_family=AF_INET;out.sin_port=htons(ep.port);out.sin_addr.s_addr=htonl(ep.ip);return out;}
bool Same(Endpoint a,Endpoint b){return a==b;}
}
std::vector<Adapter> Adapters(){
 std::vector<Adapter> result;
#ifdef _WIN32
 ULONG bytes=16384;std::vector<uint8_t> buffer(bytes);ULONG status=0;
 for(unsigned attempt=0;attempt<3;++attempt){status=GetAdaptersAddresses(AF_INET,GAA_FLAG_SKIP_ANYCAST|GAA_FLAG_SKIP_MULTICAST|GAA_FLAG_SKIP_DNS_SERVER,nullptr,reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()),&bytes);if(status!=ERROR_BUFFER_OVERFLOW)break;buffer.resize(bytes);}
 if(status!=NO_ERROR)return result;
 for(auto* item=reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());item;item=item->Next){
  if(item->OperStatus!=IfOperStatusUp || item->IfType==IF_TYPE_SOFTWARE_LOOPBACK)continue;
  std::wstring wide=item->Description?item->Description:L"";
  std::string name;
  const int n=WideCharToMultiByte(CP_UTF8,0,wide.c_str(),-1,nullptr,0,nullptr,nullptr);
  if(n>0){name.resize(n);WideCharToMultiByte(CP_UTF8,0,wide.c_str(),-1,name.data(),n,nullptr,nullptr);name.resize(n-1);}
  auto lower=wide;std::transform(lower.begin(),lower.end(),lower.begin(),[](wchar_t c){return c>=L'A'&&c<=L'Z'?c+32:c;});
  const bool radmin=lower.find(L"radmin")!=std::wstring::npos;
  for(auto* address=item->FirstUnicastAddress;address;address=address->Next){
   if(!address->Address.lpSockaddr || address->Address.lpSockaddr->sa_family!=AF_INET)continue;
   const auto ip=ntohl(reinterpret_cast<sockaddr_in*>(address->Address.lpSockaddr)->sin_addr.s_addr);
   const unsigned prefix=address->OnLinkPrefixLength;
   if(!ip || (ip>>16)==0xA9FE || prefix>32)continue;
   result.push_back({name,ip,prefix?UINT32_MAX<<(32-prefix):0,radmin});
  }
 }
#else
 ifaddrs* list=nullptr;if(getifaddrs(&list))return result;
 for(auto* item=list;item;item=item->ifa_next){
  if(!item->ifa_addr || !item->ifa_netmask || item->ifa_addr->sa_family!=AF_INET || !(item->ifa_flags&IFF_UP) || (item->ifa_flags&IFF_LOOPBACK))continue;
  const auto ip=ntohl(reinterpret_cast<sockaddr_in*>(item->ifa_addr)->sin_addr.s_addr);
  result.push_back({item->ifa_name,ip,ntohl(reinterpret_cast<sockaddr_in*>(item->ifa_netmask)->sin_addr.s_addr),false});
 }
 freeifaddrs(list);
#endif
 std::stable_sort(result.begin(),result.end(),[](const Adapter& a,const Adapter& b){return a.radmin && !b.radmin;});return result;
}
struct Node::Impl {
 NetworkScope network;SocketOwner socket;Options options;Adapter adapter;Endpoint address;uint64_t identity=RandomId();
 mutable std::mutex mutex;std::condition_variable changed;std::atomic<bool> stopping{false};std::thread worker;
 struct Seen {Room room;Clock::time_point seen;};
 using RoomKey=std::pair<uint64_t,uint64_t>; // session + host identity, not reply interface
 std::map<RoomKey,Seen> rooms;
 struct Member {Endpoint endpoint;Clock::time_point seen;};
 std::map<uint64_t,Member> members;
 std::map<uint64_t,Endpoint> peers;
 std::optional<Room> hosted,joined;
 std::deque<Datagram> datagrams;size_t queued_bytes=0;
 uint32_t pending_request=0;uint64_t pending_session=0,pending_peer=0;Endpoint pending_endpoint;bool pending_done=false,pending_ok=false;
 Clock::time_point joined_seen{},last_heartbeat{};
 uint32_t next_request=static_cast<uint32_t>(RandomId());
 uint32_t search_request=0,search_target=0;
 explicit Impl(Options v):options(std::move(v)){
  if(options.expiry<std::chrono::milliseconds(300))throw std::runtime_error("LAN expiry is too short");
  const auto adapters=Adapters();
  if(!options.bind_ip.empty()){
   const auto ip=ParseIPv4(options.bind_ip);if(!ip || !*ip || *ip==UINT32_MAX)throw std::runtime_error("Invalid LAN local IPv4");
   adapter={"Explicit adapter",*ip,0xFFFFFF00,false};
   bool found=false;
   for(const auto& item:adapters)if(item.ip==*ip){adapter=item;found=true;break;}
   if(*ip==0x7F000001){adapter.mask=0xFF000000;found=true;}
   if(!found)throw std::runtime_error("LAN local IPv4 is not assigned to an active adapter");
  }else adapter=adapters.empty()?Adapter{"Loopback",0x7F000001,0xFF000000,false}:adapters.front();
  socket.value=::socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);if(socket.value==kInvalid)throw std::runtime_error("Cannot create LAN UDP socket");
  int enabled=1;if(setsockopt(socket.value,SOL_SOCKET,SO_BROADCAST,reinterpret_cast<const char*>(&enabled),sizeof(enabled)))throw std::runtime_error("Cannot enable LAN discovery");
#ifdef _WIN32
  BOOL reset=FALSE;DWORD returned=0;WSAIoctl(socket.value,SIO_UDP_CONNRESET,&reset,sizeof(reset),nullptr,0,&returned,nullptr,nullptr);
#endif
  const unsigned count=options.port?1:kPortCount;bool bound=false;
  // A socket bound to a unicast address may miss subnet broadcasts. Receive on
  // all local addresses; the selected adapter determines discovery destinations
  // and the public endpoint. Directed broadcast/direct-IP sends follow its route.
  for(unsigned i=0;i<count;++i){const uint16_t port=options.port?options.port:static_cast<uint16_t>(kFirstPort+i);auto native=Native({0,port});if(!bind(socket.value,reinterpret_cast<sockaddr*>(&native),sizeof(native))){address={adapter.ip,port};bound=true;break;}}
  if(!bound)throw std::runtime_error("No free LAN port; choose a different port or local adapter");
  worker=std::thread([this]{Run();});
 }
 ~Impl(){stopping.store(true);if(worker.joinable())worker.join();}
 bool Send(Endpoint to,const Packet& packet){
  const auto bytes=Encode(packet);const auto native=Native(to);
  return sendto(socket.value,reinterpret_cast<const char*>(bytes.data()),static_cast<int>(bytes.size()),0,reinterpret_cast<const sockaddr*>(&native),sizeof(native))==static_cast<int>(bytes.size());
 }
 Room PublicRoom()const{auto room=*hosted;room.members=static_cast<uint32_t>(members.size()+1);room.endpoint=address;return room;}
 void Advert(Endpoint to,uint32_t request){if(hosted)Send(to,{Kind::Advert,request,identity,hosted->id,EncodeRoom(PublicRoom())});}
 void Process(const Packet& p,Endpoint from){
  if(p.peer==identity || !from.ip || !from.port)return;
  std::lock_guard lock(mutex);
  if(p.kind==Kind::Discover){if(p.payload.empty())Advert(from,p.request);return;}
  if(p.kind==Kind::Advert){
   if(!search_request || p.request!=search_request || (search_target && from.ip!=search_target))return;
   auto room=DecodeRoom(p.payload);if(!room || room->host!=p.peer || room->id!=p.session)return;
   room->endpoint=from;const RoomKey key{room->id,p.peer};
   if(rooms.size()>=128 && !rooms.contains(key))return;
   if(peers.size()>=128 && !peers.contains(p.peer))return;
   if(auto old=rooms.find(key);old!=rooms.end()){
    if(old->second.room.key!=room->key)return;
    const auto preferred=[&](Endpoint ep){return (ep.ip & adapter.mask)==(adapter.ip & adapter.mask);};
    if(!search_target && preferred(old->second.room.endpoint) && !preferred(from))return;
   }
   rooms[key]={*room,Clock::now()};peers[p.peer]=from;changed.notify_all();return;
  }
  if(p.kind==Kind::Join){
   bool ok=hosted && p.session==hosted->id && p.payload.size()==16 &&
       std::equal(p.payload.begin(),p.payload.end(),hosted->key.begin()) && hosted->state==0;
   if(ok && members.contains(p.peer) && !Same(members[p.peer].endpoint,from))ok=false;
   if(ok && !members.contains(p.peer) && members.size()+1>=hosted->capacity)ok=false;
   if(ok){members[p.peer]={from,Clock::now()};peers[p.peer]=from;Send(from,{Kind::Joined,p.request,identity,hosted->id,EncodeRoom(PublicRoom())});}
   else Send(from,{Kind::Reject,p.request,identity,p.session,{}});
   changed.notify_all();return;
  }
  if(p.kind==Kind::Joined || p.kind==Kind::Reject){
   if(!pending_request || p.request!=pending_request || p.session!=pending_session || p.peer!=pending_peer || from!=pending_endpoint)return;
   auto room=p.kind==Kind::Joined?DecodeRoom(p.payload):std::nullopt;
   pending_ok=room && room->id==pending_session && room->host==pending_peer;
   if(pending_ok){room->endpoint=from;joined=*room;joined_seen=Clock::now();peers[p.peer]=from;datagrams.clear();queued_bytes=0;}
   pending_done=true;changed.notify_all();return;
  }
  if(p.kind==Kind::Leave){
   if(hosted && p.session==hosted->id){auto found=members.find(p.peer);if(found!=members.end() && found->second.endpoint==from){members.erase(found);peers.erase(p.peer);Send(from,{Kind::Left,p.request,identity,p.session,{}});}}
   return;
  }
  if(p.kind==Kind::Left){if(joined && p.session==joined->id && p.peer==joined->host && from==joined->endpoint){peers.erase(joined->host);joined.reset();datagrams.clear();queued_bytes=0;changed.notify_all();}return;}
  if(p.kind==Kind::Heartbeat){
   if(!p.payload.empty())return;
   if(hosted && p.session==hosted->id){auto found=members.find(p.peer);if(found!=members.end() && found->second.endpoint==from){found->second.seen=Clock::now();Send(from,{Kind::Heartbeat,0,identity,p.session,{}});}}
   else if(joined && p.session==joined->id && p.peer==joined->host && from==joined->endpoint)joined_seen=Clock::now();return;
  }
  if(p.kind==Kind::Data){
   const auto found=peers.find(p.peer);if(found==peers.end() || found->second!=from)return;
   const auto session=hosted?hosted->id:joined?joined->id:0;if(!session || p.session!=session || (hosted && !members.contains(p.peer)) || (joined && joined->host!=p.peer))return;
   try{Reader r(p.payload);Datagram d;d.peer=p.peer;d.source_port=r.U16();d.destination_port=r.U16();const auto bytes=r.Blob(kMaxPacket-56);if(!r.End() || !d.source_port || !d.destination_port)return;
    if(datagrams.size()>=512 || queued_bytes+bytes.size()>1024*1024)return;
    d.bytes=bytes;queued_bytes+=d.bytes.size();datagrams.push_back(std::move(d));changed.notify_all();
   }catch(const std::exception&){}
  }
 }
 void Tick(){
  std::lock_guard lock(mutex);const auto now=Clock::now();
  std::erase_if(rooms,[&](const auto& entry){return now-entry.second.seen>options.expiry;});
  std::erase_if(members,[&](const auto& entry){if(now-entry.second.seen<=options.expiry)return false;peers.erase(entry.first);return true;});
  std::erase_if(peers,[&](const auto& entry){
   if(members.contains(entry.first) || (joined && joined->host==entry.first))return false;
   return std::none_of(rooms.begin(),rooms.end(),[&](const auto& room){return room.second.room.host==entry.first;});
  });
  std::erase_if(datagrams,[&](const auto& packet){
   if((hosted && members.contains(packet.peer)) || (joined && joined->host==packet.peer))return false;
   queued_bytes-=packet.bytes.size();return true;
  });
  if(joined && now-joined_seen>options.expiry){peers.erase(joined->host);joined.reset();datagrams.clear();queued_bytes=0;changed.notify_all();}
  if(joined && now-last_heartbeat>std::min(std::chrono::milliseconds(500),options.expiry/3)){Send(joined->endpoint,{Kind::Heartbeat,0,identity,joined->id,{}});last_heartbeat=now;}
 }
 void Run(){
  std::array<uint8_t,kMaxPacket+1> buffer{};
  while(!stopping.load()){
   fd_set fds;FD_ZERO(&fds);FD_SET(socket.value,&fds);timeval timeout{0,20000};
   const int ready=select(static_cast<int>(socket.value)+1,&fds,nullptr,nullptr,&timeout);
   if(ready>0){sockaddr_in from{};
#ifdef _WIN32
    int length=sizeof(from);
#else
    socklen_t length=sizeof(from);
#endif
    const int n=recvfrom(socket.value,reinterpret_cast<char*>(buffer.data()),static_cast<int>(buffer.size()),0,reinterpret_cast<sockaddr*>(&from),&length);
    if(n>0)if(auto packet=Decode({buffer.data(),static_cast<size_t>(n)}))Process(*packet,{ntohl(from.sin_addr.s_addr),ntohs(from.sin_port)});
   }
   Tick();
  }
 }
};
Node::Node(Options options):impl_(std::make_unique<Impl>(std::move(options))){}
Node::~Node(){CloseRoom();Leave();}
Endpoint Node::Address()const{return impl_->address;}
uint64_t Node::Identity()const{return impl_->identity;}
std::string Node::AdapterName()const{return impl_->adapter.name;}
Room Node::Host(std::string name,uint32_t capacity,uint32_t flags,std::vector<uint8_t> properties,std::vector<uint8_t> qos){
 Room room;room.id=RandomId();room.host=Identity();room.endpoint=Address();room.name=std::move(name);room.capacity=capacity;room.flags=flags;room.properties=std::move(properties);room.qos=std::move(qos);RandomBytes(room.key);EncodeRoom(room);
 std::lock_guard lock(impl_->mutex);if(impl_->hosted || impl_->joined)throw std::runtime_error("Already in a LAN room");impl_->hosted=room;return room;
}
void Node::CloseRoom(){std::lock_guard lock(impl_->mutex);if(impl_->hosted){for(const auto& [id,member]:impl_->members)impl_->Send(member.endpoint,{Kind::Left,0,Identity(),impl_->hosted->id,{}});impl_->hosted.reset();impl_->members.clear();impl_->peers.clear();impl_->datagrams.clear();impl_->queued_bytes=0;}}
void Node::UpdateRoom(const Room& room){EncodeRoom(room);std::lock_guard lock(impl_->mutex);if(!impl_->hosted || impl_->hosted->id!=room.id || room.host!=Identity() || room.capacity<impl_->members.size()+1)throw std::runtime_error("Invalid room update");impl_->hosted=room;}
void Node::Search(const std::string& direct_ip){
 std::vector<uint32_t> targets;
 if(!direct_ip.empty()){const auto ip=ParseIPv4(direct_ip);if(!ip || !*ip || *ip==UINT32_MAX)throw std::runtime_error("Invalid host IPv4");targets.push_back(*ip);}
 else if(!impl_->options.discovery_targets.empty())targets=impl_->options.discovery_targets;
 else {targets.push_back(impl_->adapter.ip | ~impl_->adapter.mask);targets.push_back(UINT32_MAX);targets.push_back(impl_->adapter.ip);}
 std::lock_guard lock(impl_->mutex);uint32_t request=++impl_->next_request;if(!request)request=++impl_->next_request;
 impl_->search_request=request;impl_->search_target=direct_ip.empty()?0:targets.front();
 if(impl_->search_target)impl_->rooms.clear();
 for(auto ip:targets)for(uint16_t i=0;i<kPortCount;++i)impl_->Send({ip,static_cast<uint16_t>(kFirstPort+i)},{Kind::Discover,request,Identity(),0,{}});
}
std::vector<Room> Node::Rooms()const{std::lock_guard lock(impl_->mutex);std::vector<Room> result;const auto now=Clock::now();for(const auto& [key,seen]:impl_->rooms)if(now-seen.seen<=impl_->options.expiry && !seen.room.state && seen.room.members<seen.room.capacity)result.push_back(seen.room);return result;}
std::optional<Room> Node::CurrentRoom()const{std::lock_guard lock(impl_->mutex);if(impl_->hosted)return impl_->PublicRoom();return impl_->joined;}
bool Node::Join(const Room& room,std::chrono::milliseconds timeout){
 if(!room.id || !room.host || !room.endpoint.ip || !room.endpoint.port || timeout<=std::chrono::milliseconds(0) || timeout>std::chrono::seconds(10))return false;
 std::unique_lock lock(impl_->mutex);if(impl_->hosted || impl_->joined || impl_->pending_request)return false;
 impl_->pending_request=++impl_->next_request;if(!impl_->pending_request)impl_->pending_request=++impl_->next_request;
 impl_->pending_session=room.id;impl_->pending_peer=room.host;impl_->pending_endpoint=room.endpoint;impl_->pending_done=false;impl_->pending_ok=false;
 Packet packet{Kind::Join,impl_->pending_request,Identity(),room.id,{room.key.begin(),room.key.end()}};
 const auto deadline=Clock::now()+timeout;
 do {impl_->Send(room.endpoint,packet);impl_->changed.wait_for(lock,std::chrono::milliseconds(150),[&]{return impl_->pending_done;});}while(!impl_->pending_done && Clock::now()<deadline);
 const bool ok=impl_->pending_done && impl_->pending_ok;impl_->pending_request=0;return ok;
}
void Node::Leave(){std::lock_guard lock(impl_->mutex);if(impl_->joined){impl_->Send(impl_->joined->endpoint,{Kind::Leave,0,Identity(),impl_->joined->id,{}});impl_->peers.erase(impl_->joined->host);impl_->joined.reset();impl_->datagrams.clear();impl_->queued_bytes=0;}}
uint32_t Node::Members()const{std::lock_guard lock(impl_->mutex);return impl_->hosted?static_cast<uint32_t>(impl_->members.size()+1):impl_->joined?impl_->joined->members:0;}
std::optional<Endpoint> Node::Peer(uint64_t peer)const{std::lock_guard lock(impl_->mutex);const auto found=impl_->peers.find(peer);return found==impl_->peers.end()?std::nullopt:std::optional(found->second);}
bool Node::Connected(uint64_t peer)const{std::lock_guard lock(impl_->mutex);return impl_->members.contains(peer) || (impl_->joined && impl_->joined->host==peer);}
void Node::RememberPeer(uint64_t peer,Endpoint endpoint){std::lock_guard lock(impl_->mutex);if(peer && endpoint.ip && endpoint.port && impl_->peers.size()<128)impl_->peers[peer]=endpoint;}
bool Node::SendData(uint64_t peer,uint16_t source,uint16_t destination,std::span<const uint8_t> bytes){
 if(bytes.size()>kMaxPacket-56 || !source || !destination)return false;
 std::lock_guard lock(impl_->mutex);const auto found=impl_->peers.find(peer);const auto session=impl_->hosted?impl_->hosted->id:impl_->joined?impl_->joined->id:0;if(found==impl_->peers.end() || !session)return false;
 Writer w;w.U16(source);w.U16(destination);w.Blob(bytes);return impl_->Send(found->second,{Kind::Data,0,Identity(),session,std::move(w.bytes)});
}
std::optional<Datagram> Node::ReceiveData(){std::lock_guard lock(impl_->mutex);if(impl_->datagrams.empty())return {};auto result=std::move(impl_->datagrams.front());impl_->datagrams.pop_front();impl_->queued_bytes-=result.bytes.size();return result;}
}
