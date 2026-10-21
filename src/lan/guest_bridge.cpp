#include "guest_bridge.h"
#include "online/stats.h"
#include <rex/memory.h>
#include <rex/string/utf8.h>
#include <rex/system/util/object_table.h>
#include <rex/system/xobject.h>
#include <algorithm>
#include <cstring>
#include <set>
#include <thread>
namespace motogp::lan {
namespace {
constexpr uint32_t Error(uint32_t code){return 0x80070000u|code;}
constexpr uint32_t kInvalid=Error(87),kBadHandle=Error(6),kInsufficient=Error(122);
uint32_t Load32(const uint8_t* p){return rex::memory::load_and_swap<uint32_t>(p);}
uint64_t Load64(const uint8_t* p){return rex::memory::load_and_swap<uint64_t>(p);}
void Store32(uint8_t* p,uint32_t v){rex::memory::store_and_swap<uint32_t>(p,v);}
void Store64(uint8_t* p,uint64_t v){rex::memory::store_and_swap<uint64_t>(p,v);}
class Session final:public rex::system::XObject {
 public:
  static constexpr Type kObjectType=Type::Session;
  std::weak_ptr<Node> network;
  uint64_t room=0,nonce=0;
  uint32_t flags=0,public_slots=0,private_slots=0,state=0;
  std::optional<Room> statistics_room;
  std::map<uint64_t,bool> local,remote;
  explicit Session(rex::system::KernelState* kernel,std::weak_ptr<Node> owner)
      :XObject(kernel,kObjectType),network(std::move(owner)){}
  bool Initialize(){return CreateNative<rex::system::X_DISPATCH_HEADER>()!=nullptr;}
  void Close(){
    if(auto node=network.lock();node && !statistics_room)if(auto current=node->CurrentRoom();current && current->id==room){
      if(flags&1)node->CloseRoom();else node->Leave();
    }
    room=0;statistics_room.reset();local.clear();remote.clear();
  }
  ~Session(){Close();}
};
}
GuestBridge::GuestBridge(rex::system::KernelState* kernel,std::shared_ptr<Node> node,
                         std::string direct,std::string name,std::shared_ptr<online::Client> service)
    :kernel_(kernel),node_(std::move(node)),online_(std::move(service)),direct_ip_(std::move(direct)),room_name_(std::move(name)){
  if(!kernel_ || !node_ || room_name_.empty() || room_name_.size()>31 ||
     room_name_.find('\0')!=std::string::npos || (!direct_ip_.empty() && !ParseIPv4(direct_ip_)))
    throw std::runtime_error("Invalid guest LAN bridge options");
}
bool GuestBridge::Valid(uint32_t address,size_t size,bool write)const{
  if(!address || !size || size>UINT32_MAX || uint64_t(address)+size>0x100000000ull)return false;
  const auto end=uint64_t(address)+size;uint64_t cursor=address;
  while(cursor<end){
    auto* heap=kernel_->memory()->LookupHeap(static_cast<uint32_t>(cursor));
    rex::memory::HeapAllocationInfo info{};
    if(!heap || !heap->QueryRegionInfo(static_cast<uint32_t>(cursor),&info) ||
       !(info.state&rex::memory::kMemoryAllocationCommit) ||
       !(info.protect&rex::memory::kMemoryProtectRead) ||
       (write && !(info.protect&rex::memory::kMemoryProtectWrite)))return false;
    const uint64_t next=uint64_t(info.base_address)+info.region_size;
    if(next<=cursor)return false;cursor=next;
  }
  return true;
}
uint32_t GuestBridge::CreateHandle(uint32_t output){
  if(!Valid(output,4,true))return kInvalid;
  auto* session=new Session(kernel_,node_);
  if(!session->Initialize()){session->ReleaseHandle();session->Release();return Error(8);}
  Store32(kernel_->memory()->TranslateVirtual(output),session->handle());
  session->Release();return 0;
}
uint32_t GuestBridge::Reference(uint32_t handle,uint32_t output){
  if(!Valid(output,4,true))return kInvalid;
  const auto session=kernel_->object_table()->LookupObject<Session>(handle);
  if(!session)return kBadHandle;
  session->RetainHandle();Store32(kernel_->memory()->TranslateVirtual(output),session->guest_object());return 0;
}
void GuestBridge::WriteAddress(uint32_t output,uint64_t identity,Endpoint endpoint)const{
  if(!Valid(output,36,true))throw std::runtime_error("Invalid XNADDR output");
  auto* p=kernel_->memory()->TranslateVirtual(output);std::memset(p,0,36);
  Store32(p,endpoint.ip);Store32(p+4,endpoint.ip);
  rex::memory::store_and_swap<uint16_t>(p+8,endpoint.port);
  p[10]=0x02;for(unsigned i=0;i<5;++i)p[11+i]=static_cast<uint8_t>(identity>>(i*8));
  Store64(p+16,identity);
}
void GuestBridge::WriteInfo(uint32_t output,const Room& room)const{
  if(!Valid(output,60,true))throw std::runtime_error("Invalid session info output");
  auto* p=kernel_->memory()->TranslateVirtual(output);Store64(p,room.id);
  WriteAddress(output+8,room.host,room.endpoint);std::copy(room.key.begin(),room.key.end(),p+44);
}
std::vector<Room> GuestBridge::KnownRooms()const{
  std::lock_guard lock(mutex_);
  auto rooms=node_->Rooms();
  if(auto current=node_->CurrentRoom())rooms.push_back(*current);
  for(const auto& cached:search_results_){
    if(std::none_of(rooms.begin(),rooms.end(),[&](const Room& room){
      return room.id==cached.id && room.host==cached.host;
    }))rooms.push_back(cached);
  }
  return rooms;
}
std::function<uint32_t()> GuestBridge::StatisticsOperation(uint32_t message,uint32_t buffer,uint32_t size){
  if(!online_)return {};
  if(message==0xB0021)return online::PrepareRead(*this,buffer,size);
  if(message!=0xB0025 && message!=0xB0026)return {};
  std::lock_guard lock(mutex_);
  const auto fail=[](uint32_t error){return [error](){return error;};};
  if((size && size!=24) || !Valid(buffer,24))return fail(kInvalid);
  const auto pointer=Load32(kernel_->memory()->TranslateVirtual(buffer));if(!Valid(pointer,16))return fail(kBadHandle);
  auto* header=kernel_->memory()->TranslateVirtual<rex::system::X_DISPATCH_HEADER*>(pointer);
  if(header->wait_list_flink!=rex::system::kXObjSignature)return fail(kBadHandle);
  auto session=kernel_->object_table()->LookupObject<Session>(header->wait_list_blink);
  if(!session || session->guest_object()!=pointer || !session->room)return fail(kBadHandle);
  // The kernel executes this barrier after earlier queued writes. Waiting on
  // future writes here would block the same worker that must upload them.
  if(message==0xB0026)return [](){return 0u;};
  return online::PrepareWrite(*this,buffer);
}
std::optional<uint32_t> GuestBridge::Dispatch(uint32_t message,uint32_t buffer,uint32_t size){
  if(online_ && message==0xB0021)return online::ReadStats(*this,buffer,size);
  const unsigned needed=message==0xB0006?24:message==0xB0007?28:message==0xB0010?28:
      online_ && (message==0xB0025 || message==0xB0026)?24:
      message==0xB0012||message==0xB0013?20:message==0xB001C?36:
      message==0xB0016?32:message==0xB001D?24:
      message==0xB0011||message==0xB0014||message==0xB0015||message==0xB0018?16:0;
  if(!needed)return {};
  std::lock_guard lock(mutex_);
  if((size && size!=needed) || !Valid(buffer,needed))return kInvalid;
  auto* b=kernel_->memory()->TranslateVirtual(buffer);
  const auto mem=kernel_->memory();
  try{
    if(message==0xB0006 || message==0xB0007){
      if(Load32(b)!=0)return kInvalid;
      auto next=metadata_;const auto id=Load32(b+16);
      if(message==0xB0006)next.contexts[id]=Load32(b+20);
      else{
        const auto bytes=Load32(b+20),value=Load32(b+24);
        if(!bytes)next.properties.erase(id);
        else{
          if(bytes>512 || !Valid(value,bytes))return kInvalid;
          std::span<const uint8_t> data(mem->TranslateVirtual(value),bytes);
          if(!ValidProperty(id,data))return kInvalid;
          next.properties[id]={data.begin(),data.end()};
        }
      }
      auto encoded=EncodeMetadata(next);
      if(auto room=node_->CurrentRoom();room && room->host==node_->Identity()){
        room->properties=std::move(encoded);node_->UpdateRoom(*room);
      }
      metadata_=std::move(next);return 0;
    }
    if(message==0xB001C || message==0xB0016){
      const auto maximum=Load32(b+8),bytes=Load32(b+24),output=Load32(b+28);
      const auto num_properties=rex::memory::load_and_swap<uint16_t>(b+12);
      const auto num_contexts=rex::memory::load_and_swap<uint16_t>(b+14);
      const auto property_array=Load32(b+16),context_array=Load32(b+20);
      if(Load32(b+4)!=0 || !maximum || maximum>64 || num_properties>32 || num_contexts>32 ||
         (num_contexts && !Valid(context_array,num_contexts*8)) ||
         (num_properties && !Valid(property_array,num_properties*24)))return kInvalid;
      Metadata filters;
      for(unsigned i=0;i<num_contexts;++i){auto* c=mem->TranslateVirtual(context_array+i*8);filters.contexts[Load32(c)]=Load32(c+4);}
      for(unsigned i=0;i<num_properties;++i){auto* p=mem->TranslateVirtual(property_array+i*24);const auto id=Load32(p),type=id>>28;
        if(p[8]!=type)return kInvalid;
        uint32_t address=property_array+i*24+16,length=type==1 || type==5?4:8;
        if(type==4 || type==6){length=Load32(p+16);address=Load32(p+20);}
        if(length>512 || (length && !Valid(address,length)))return kInvalid;
        std::span<const uint8_t> value(length?mem->TranslateVirtual(address):nullptr,length);
        if(!ValidProperty(id,value))return kInvalid;
        filters.properties[id]={value.begin(),value.end()};
      }
      if(bytes<8+maximum*92)return kInsufficient;
      if(!Valid(output,bytes,true))return kInvalid;
      node_->Search(direct_ip_);std::this_thread::sleep_for(std::chrono::milliseconds(online_?800:300));
      auto rooms=node_->Rooms();
      std::vector<Metadata> fields;std::vector<Room> selected;
      for(const auto& room:rooms){
        auto data=DecodeMetadata(room.properties);if(!data)continue;bool match=true;
        for(auto [id,value]:filters.contexts){auto found=data->contexts.find(id);if(value!=UINT32_MAX && (found==data->contexts.end() || found->second!=value))match=false;}
        for(const auto& [id,value]:filters.properties){auto found=data->properties.find(id);
          const bool wildcard=(id>>28)==1 && value==std::vector<uint8_t>(4,255);
          if(!wildcard && (found==data->properties.end() || found->second!=value))match=false;}
        const auto public_free=room.capacity-room.private_slots-(room.members-room.private_members);
        if(!match || (message==0xB001C && Load32(b+32)>public_free))continue;
        const auto name=rex::string::to_utf16(room.name);std::vector<uint8_t> host_name((name.size()+1)*2,0);
        for(size_t i=0;i<name.size();++i)rex::memory::store_and_swap<uint16_t>(host_name.data()+i*2,name[i]);
        data->properties[0x40008109]=std::move(host_name);
        selected.push_back(room);fields.push_back(std::move(*data));if(selected.size()==maximum)break;
      }
      size_t total=8+selected.size()*92;
      for(const auto& data:fields){total+=data.properties.size()*24+data.contexts.size()*8;
        for(const auto& [id,value]:data.properties)if((id>>28)==4 || (id>>28)==6)total+=(value.size()+3)&~size_t(3);}
      if(total>bytes)return kInsufficient;
      auto* out=mem->TranslateVirtual(output);std::memset(out,0,total);
      Store32(out,static_cast<uint32_t>(selected.size()));Store32(out+4,selected.empty()?0:output+8);
      uint32_t cursor=output+8+static_cast<uint32_t>(selected.size())*92;
      for(size_t i=0;i<selected.size();++i){
        const auto row=output+8+static_cast<uint32_t>(i)*92;WriteInfo(row,selected[i]);
        const auto& room=selected[i];const auto& data=fields[i];auto* p=mem->TranslateVirtual(row);
        Store32(p+60,room.capacity-room.private_slots-(room.members-room.private_members));Store32(p+64,room.private_slots-room.private_members);
        Store32(p+68,room.members-room.private_members);Store32(p+72,room.private_members);
        Store32(p+76,static_cast<uint32_t>(data.properties.size()));Store32(p+80,static_cast<uint32_t>(data.contexts.size()));
        if(!data.properties.empty())Store32(p+84,cursor);
        uint32_t property_cursor=cursor;cursor+=static_cast<uint32_t>(data.properties.size())*24;
        for(const auto& [id,value]:data.properties){auto* property=mem->TranslateVirtual(property_cursor);const auto type=id>>28;Store32(property,id);property[8]=static_cast<uint8_t>(type);
          if(type==4 || type==6){Store32(property+16,static_cast<uint32_t>(value.size()));Store32(property+20,value.empty()?0:cursor);
            if(!value.empty())std::copy(value.begin(),value.end(),mem->TranslateVirtual(cursor));cursor+=static_cast<uint32_t>((value.size()+3)&~size_t(3));}
          else std::copy(value.begin(),value.end(),property+16);
          property_cursor+=24;
        }
        if(!data.contexts.empty()){Store32(p+88,cursor);for(auto [id,value]:data.contexts){auto* c=mem->TranslateVirtual(cursor);Store32(c,id);Store32(c+4,value);cursor+=8;}}
      }
      search_results_=std::move(selected);
      return 0;
    }
    const auto pointer=Load32(b);
    if(!Valid(pointer,16))return kBadHandle;
    auto* header=mem->TranslateVirtual<rex::system::X_DISPATCH_HEADER*>(pointer);
    if(header->wait_list_flink!=rex::system::kXObjSignature)return kBadHandle;
    auto session=kernel_->object_table()->LookupObject<Session>(header->wait_list_blink);
    if(!session || session->guest_object()!=pointer)return kBadHandle;
    if(message==0xB0010){
      const auto flags=Load32(b+4),info=Load32(b+20),nonce=Load32(b+24);
      auto pub=Load32(b+8),priv=Load32(b+12);
      if(session->room || Load32(b+16)!=0 || (!pub && !priv) || pub>20 || priv>20-pub ||
         !Valid(info,60,true) || !Valid(nonce,8,true))return kInvalid;
      if(flags&16)return Error(50); // Ranked arbitration requires Xbox Live services.
      // PC-created rooms always expose their full capacity as public. Retain
      // the wire fields and joining behavior for compatibility with other hosts.
      if(flags&1){pub+=priv;priv=0;}
      const auto session_nonce=RandomId();
      Room room;
      if(online_ && !(flags&0x20)){
        // The title also creates a 0x707 statistics-only session while playing
        // solo. It must neither advertise a lobby nor occupy the race node.
        room.id=RandomId();room.host=node_->Identity();room.endpoint=node_->Address();
        room.name=room_name_;room.flags=flags;room.capacity=pub+priv;room.private_slots=priv;room.private_members=pub?0:1;
        RandomBytes(room.key);session->statistics_room=room;
      }else if(flags&1){
        room=node_->Host(room_name_,pub+priv,flags,EncodeMetadata(metadata_),{},priv);
      }else{
        auto* data=mem->TranslateVirtual(info);auto rooms=node_->Rooms();if(auto current=node_->CurrentRoom())rooms.push_back(*current);
        // Results on the guest search screen remain valid join candidates after
        // the short discovery cache expires. A real keyed handshake still
        // decides whether the selected host is alive and accepts membership.
        rooms.insert(rooms.end(),search_results_.begin(),search_results_.end());
        auto found=std::find_if(rooms.begin(),rooms.end(),[&](const Room& r){return r.id==Load64(data) &&
          std::equal(r.key.begin(),r.key.end(),data+44);});
        if(found==rooms.end() || !node_->Join(*found,std::chrono::milliseconds(online_?4000:1200)))return Error(1168);
        room=*node_->CurrentRoom();
      }
      session->room=room.id;session->flags=flags;session->public_slots=pub;session->private_slots=priv;
      session->nonce=session_nonce;WriteInfo(info,room);Store64(mem->TranslateVirtual(nonce),session->nonce);return 0;
    }
    if(!session->room)return kBadHandle;
    if(online_ && message==0xB0025)return online::WriteStats(*this,buffer);
    if(online_ && message==0xB0026){try{online_->Flush();return 0;}catch(const std::exception&){return Error(1460);}}
    if(message==0xB0011){session->Close();return 0;}
    if(message==0xB0014 || message==0xB0015){
      auto room=session->statistics_room?session->statistics_room:node_->CurrentRoom();if(!room || room->id!=session->room)return Error(1168);
      if(message==0xB0014 && session->state==2)return kInvalid;
      session->state=message==0xB0014?2:3;
      if(session->flags&1){room->state=message==0xB0014?1:0;if(session->statistics_room)session->statistics_room=room;else node_->UpdateRoom(*room);}return 0;
    }
    if(message==0xB0012 || message==0xB0013){
      const auto count=Load32(b+4),xuids=Load32(b+8),indices=Load32(b+12),private_slots=Load32(b+16);
      const bool joining=message==0xB0012;
      // The original local/remote leave wrappers pass NULL for private slots.
      // That array belongs only to the join request, despite the shared layout.
      if(!count || count>20 || (joining && !Valid(private_slots,count*4)) ||
          (xuids?!Valid(xuids,count*8):!Valid(indices,count*4)))return kInvalid;
      auto updated=xuids?session->remote:session->local;
      for(unsigned i=0;i<count;++i){
        const auto index=xuids?0:Load32(mem->TranslateVirtual(indices+i*4));if(index!=0)return kInvalid;
        const auto id=xuids?Load64(mem->TranslateVirtual(xuids+i*8)):node_->Identity();
        if(!id)return kInvalid;
        const auto private_value=joining?Load32(mem->TranslateVirtual(private_slots+i*4)):0;if(private_value>1)return kInvalid;
        if(message==0xB0012){if(xuids && !node_->Connected(id))return Error(1168);updated[id]=private_value!=0;}else updated.erase(id);
      }
      unsigned used_public=0,used_private=0;
      for(const auto* list:{&updated,xuids?&session->local:&session->remote})for(auto [id,priv]:*list){if(priv)++used_private;else ++used_public;}
      if(used_public>session->public_slots || used_private>session->private_slots)return Error(112);
      if(!session->statistics_room && !xuids && message==0xB0012 && updated.contains(node_->Identity()))node_->SetPrivateSlot(updated.at(node_->Identity()));
      (xuids?session->remote:session->local)=std::move(updated);return 0;
    }
    unsigned used_public=0,used_private=0;
    for(const auto* list:{&session->local,&session->remote})for(auto [id,priv]:*list){if(priv)++used_private;else ++used_public;}
    if(message==0xB0018){
      auto pub=Load32(b+8),priv=Load32(b+12);
      if((!pub && !priv) || pub>20 || priv>20-pub)return kInvalid;
      if(session->flags&1){pub+=priv;priv=0;}
      if(pub<used_public || priv<used_private)return kInvalid;
      auto room=session->statistics_room?session->statistics_room:node_->CurrentRoom();if(!room || room->id!=session->room)return Error(1168);
      auto flags=(Load32(b+4)&~1u)|(session->flags&1);
      if(session->flags&1){room->capacity=pub+priv;room->private_slots=priv;room->flags=flags;if(session->statistics_room)session->statistics_room=room;else node_->UpdateRoom(*room);}
      session->public_slots=pub;session->private_slots=priv;session->flags=flags;return 0;
    }
    if(message==0xB001D){
      const auto bytes=Load32(b+4),output=Load32(b+8),count=used_public+used_private;
      if(bytes<128+count*16)return kInsufficient;
      if(!Valid(output,128+count*16,true))return kInvalid;
      auto room=session->statistics_room?session->statistics_room:node_->CurrentRoom();if(!room || room->id!=session->room)return Error(1168);
      auto* p=mem->TranslateVirtual(output);std::memset(p,0,128+count*16);
      Store32(p,0);Store32(p+12,session->flags);Store32(p+16,session->public_slots);Store32(p+20,session->private_slots);
      Store32(p+24,session->public_slots-used_public);Store32(p+28,session->private_slots-used_private);
      Store32(p+32,count);Store32(p+36,count);Store32(p+40,session->state);Store64(p+48,session->nonce);WriteInfo(output+56,*room);
      Store32(p+124,count?output+128:0);uint32_t cursor=output+128;
      for(const auto* list:{&session->local,&session->remote})for(auto [id,priv]:*list){
        auto* member=mem->TranslateVirtual(cursor);Store64(member,id);Store32(member+8,list==&session->local?0:UINT32_MAX);
        Store32(member+12,priv?1:0);cursor+=16;
      }
      return 0;
    }
    return Error(50);
  }catch(const std::exception&){return kInvalid;}
}
}
