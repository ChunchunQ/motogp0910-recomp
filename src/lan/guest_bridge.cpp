#include "guest_bridge.h"
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
  std::map<uint64_t,bool> local,remote;
  explicit Session(rex::system::KernelState* kernel,std::weak_ptr<Node> owner)
      :XObject(kernel,kObjectType),network(std::move(owner)){}
  bool Initialize(){return CreateNative<rex::system::X_DISPATCH_HEADER>()!=nullptr;}
  void Close(){
    if(auto node=network.lock())if(auto current=node->CurrentRoom();current && current->id==room){
      if(flags&1)node->CloseRoom();else node->Leave();
    }
    room=0;local.clear();remote.clear();
  }
  ~Session(){Close();}
};
}
GuestBridge::GuestBridge(rex::system::KernelState* kernel,std::shared_ptr<Node> node,
                         std::string direct,std::string name)
    :kernel_(kernel),node_(std::move(node)),direct_ip_(std::move(direct)),room_name_(std::move(name)){
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
std::optional<uint32_t> GuestBridge::Dispatch(uint32_t message,uint32_t buffer,uint32_t size){
  const unsigned needed=message==0xB0006?24:message==0xB0007?28:message==0xB0010?28:
      message==0xB0012||message==0xB0013?20:message==0xB001C?36:
      message==0xB0016?32:message==0xB001D?24:
      message==0xB0011||message==0xB0014||message==0xB0015||message==0xB0018?16:0;
  if(!needed)return {};
  std::lock_guard lock(mutex_);
  if((size && size!=needed) || !Valid(buffer,needed))return kInvalid;
  auto* b=kernel_->memory()->TranslateVirtual(buffer);
  const auto mem=kernel_->memory();
  try{
    if(message==0xB0007)return Error(50); // Property serialization is a later stage.
    if(message==0xB0006){
      if(Load32(b)!=0 || (contexts_.size()>=32 && !contexts_.contains(Load32(b+16))))return kInvalid;
      contexts_[Load32(b+16)]=Load32(b+20);
      if(auto room=node_->CurrentRoom();room && room->host==node_->Identity()){
        Writer w;w.U16(static_cast<uint16_t>(contexts_.size()));for(auto [id,value]:contexts_){w.U32(id);w.U32(value);}
        room->properties=std::move(w.bytes);node_->UpdateRoom(*room);
      }
      return 0;
    }
    if(message==0xB001C || message==0xB0016){
      const auto maximum=Load32(b+8),bytes=Load32(b+24),output=Load32(b+28);
      const auto num_contexts=rex::memory::load_and_swap<uint16_t>(b+14);
      if(rex::memory::load_and_swap<uint16_t>(b+12))return Error(50);
      const auto context_array=Load32(b+20);
      if(Load32(b+4)!=0 || !maximum || maximum>64 || num_contexts>32 ||
         (num_contexts && !Valid(context_array,num_contexts*8)))return kInvalid;
      // Each record fits the original title's own 1326-byte-per-result query.
      if(bytes<8+maximum*92)return kInsufficient;
      if(!Valid(output,bytes,true))return kInvalid;
      node_->Search(direct_ip_);std::this_thread::sleep_for(std::chrono::milliseconds(300));
      auto rooms=node_->Rooms();
      std::vector<std::map<uint32_t,uint32_t>> contexts;
      std::vector<std::u16string> names;
      std::vector<Room> selected;
      for(const auto& room:rooms){
        std::map<uint32_t,uint32_t> values;
        try{if(!room.properties.empty()){
          Reader r(room.properties);const auto count=r.U16();if(count>32)continue;
          for(unsigned n=0;n<count;++n){auto id=r.U32();values[id]=r.U32();}if(!r.End())continue;
        }}catch(const std::exception&){continue;}
        bool match=true;
        for(unsigned i=0;i<num_contexts;++i){const auto* c=mem->TranslateVirtual(context_array+i*8);
          const auto id=Load32(c);const auto v=values.find(id);if(v==values.end() || v->second!=Load32(c+4))match=false;}
        if(!match || (message==0xB001C && Load32(b+32)>room.capacity-room.members))continue;
        std::u16string name;
        try{name=rex::string::to_utf16(room.name);}catch(const std::exception&){continue;}
        selected.push_back(room);names.push_back(std::move(name));contexts.push_back(std::move(values));if(selected.size()==maximum)break;
      }
      size_t total=8+selected.size()*92;
      for(size_t i=0;i<selected.size();++i)total+=24+(((names[i].size()+1)*2+3)&~size_t(3))+contexts[i].size()*8;
      if(total>bytes)return kInsufficient;
      auto* out=mem->TranslateVirtual(output);std::memset(out,0,total);
      Store32(out,static_cast<uint32_t>(selected.size()));Store32(out+4,selected.empty()?0:output+8);
      uint32_t cursor=output+8+static_cast<uint32_t>(selected.size())*92;
      for(size_t i=0;i<selected.size();++i){
        const auto row=output+8+static_cast<uint32_t>(i)*92;WriteInfo(row,selected[i]);
        auto* p=mem->TranslateVirtual(row);Store32(p+60,selected[i].capacity-selected[i].members);Store32(p+68,selected[i].members);
        Store32(p+76,1);Store32(p+80,static_cast<uint32_t>(contexts[i].size()));Store32(p+84,cursor);
        auto* property=mem->TranslateVirtual(cursor);Store32(property,0x40008109);property[8]=4;
        const auto chars=static_cast<uint32_t>(names[i].size());Store32(property+16,(chars+1)*2);Store32(property+20,cursor+24);
        for(unsigned n=0;n<chars;++n)rex::memory::store_and_swap<uint16_t>(property+24+n*2,names[i][n]);
        cursor+=24+(((chars+1)*2+3)&~uint32_t(3));
        if(!contexts[i].empty()){Store32(p+88,cursor);for(auto [id,value]:contexts[i]){
          auto* c=mem->TranslateVirtual(cursor);Store32(c,id);Store32(c+4,value);cursor+=8;}}
      }
      return 0;
    }
    const auto pointer=Load32(b);
    if(!Valid(pointer,16))return kBadHandle;
    auto* header=mem->TranslateVirtual<rex::system::X_DISPATCH_HEADER*>(pointer);
    if(header->wait_list_flink!=rex::system::kXObjSignature)return kBadHandle;
    auto session=kernel_->object_table()->LookupObject<Session>(header->wait_list_blink);
    if(!session || session->guest_object()!=pointer)return kBadHandle;
    if(message==0xB0010){
      const auto flags=Load32(b+4),pub=Load32(b+8),priv=Load32(b+12),info=Load32(b+20),nonce=Load32(b+24);
      if(session->room || Load32(b+16)!=0 || !pub || pub>20 || priv>20-pub ||
         !Valid(info,60,true) || !Valid(nonce,8,true))return kInvalid;
      if(priv)return Error(50); // Only public lobby slots are advertised by this preview.
      const auto session_nonce=RandomId();
      Room room;
      if(flags&1){
        Writer w;w.U16(static_cast<uint16_t>(contexts_.size()));for(auto [id,value]:contexts_){w.U32(id);w.U32(value);}
        room=node_->Host(room_name_,pub+priv,flags,std::move(w.bytes));
      }else{
        auto* data=mem->TranslateVirtual(info);auto rooms=node_->Rooms();
        auto found=std::find_if(rooms.begin(),rooms.end(),[&](const Room& r){return r.id==Load64(data) &&
          std::equal(r.key.begin(),r.key.end(),data+44);});
        if(found==rooms.end() || !node_->Join(*found))return Error(1168);
        room=*node_->CurrentRoom();
      }
      session->room=room.id;session->flags=flags;session->public_slots=pub;session->private_slots=priv;
      session->nonce=session_nonce;WriteInfo(info,room);Store64(mem->TranslateVirtual(nonce),session->nonce);return 0;
    }
    if(!session->room)return kBadHandle;
    if(message==0xB0011){session->Close();return 0;}
    if(message==0xB0014 || message==0xB0015){
      auto room=node_->CurrentRoom();if(!room || room->id!=session->room)return Error(1168);
      if(message==0xB0014 && session->state==2)return kInvalid;
      session->state=message==0xB0014?2:3;
      if(session->flags&1){room->state=message==0xB0014?1:0;node_->UpdateRoom(*room);}return 0;
    }
    if(message==0xB0012 || message==0xB0013){
      const auto count=Load32(b+4),xuids=Load32(b+8),indices=Load32(b+12),private_slots=Load32(b+16);
      if(!count || count>20 || !Valid(private_slots,count*4) ||
          (xuids?!Valid(xuids,count*8):!Valid(indices,count*4)))return kInvalid;
      auto updated=xuids?session->remote:session->local;
      for(unsigned i=0;i<count;++i){
        const auto index=xuids?0:Load32(mem->TranslateVirtual(indices+i*4));if(index!=0)return kInvalid;
        const auto id=xuids?Load64(mem->TranslateVirtual(xuids+i*8)):node_->Identity();
        if(!id)return kInvalid;
        const auto private_value=Load32(mem->TranslateVirtual(private_slots+i*4));if(private_value>1)return kInvalid;
        if(message==0xB0012){if(xuids && !node_->Connected(id))return Error(1168);updated[id]=private_value!=0;}else updated.erase(id);
      }
      unsigned used_public=0,used_private=0;
      for(const auto* list:{&updated,xuids?&session->local:&session->remote})for(auto [id,priv]:*list){if(priv)++used_private;else ++used_public;}
      if(used_public>session->public_slots || used_private>session->private_slots)return Error(112);
      (xuids?session->remote:session->local)=std::move(updated);return 0;
    }
    unsigned used_public=0,used_private=0;
    for(const auto* list:{&session->local,&session->remote})for(auto [id,priv]:*list){if(priv)++used_private;else ++used_public;}
    if(message==0xB0018){
      const auto pub=Load32(b+8),priv=Load32(b+12);
      if(!pub || pub>20 || priv>20-pub || pub<used_public || priv<used_private)return kInvalid;
      if(priv)return Error(50);
      auto room=node_->CurrentRoom();if(!room || room->id!=session->room)return Error(1168);
      auto flags=(Load32(b+4)&~1u)|(session->flags&1);
      if(session->flags&1){room->capacity=pub+priv;room->flags=flags;node_->UpdateRoom(*room);}
      session->public_slots=pub;session->private_slots=priv;session->flags=flags;return 0;
    }
    if(message==0xB001D){
      const auto bytes=Load32(b+4),output=Load32(b+8),count=used_public+used_private;
      if(bytes<128+count*16)return kInsufficient;
      if(!Valid(output,128+count*16,true))return kInvalid;
      auto room=node_->CurrentRoom();if(!room || room->id!=session->room)return Error(1168);
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
