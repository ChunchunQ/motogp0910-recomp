#include "stats.h"
#include "lan/guest_bridge.h"
#include <rex/system/xenumerator.h>
#include <rex/system/xam/user_profile.h>
#include <algorithm>
#include <cstring>
#include <map>
#include <set>
#include <atomic>

namespace motogp::online {
namespace {
uint32_t U32(const uint8_t* p){return rex::memory::load_and_swap<uint32_t>(p);}
uint64_t U64(const uint8_t* p){return rex::memory::load_and_swap<uint64_t>(p);}
void Put32(uint8_t* p,uint32_t value){rex::memory::store_and_swap<uint32_t>(p,value);}
void Put64(uint8_t* p,uint64_t value){rex::memory::store_and_swap<uint64_t>(p,value);}
constexpr uint32_t Error(uint32_t code){return 0x80070000u|code;}
std::mutex enumerator_mutex;std::map<const rex::system::XEnumerator*,uint32_t> enumerators;
struct WriteReservation {
 std::shared_ptr<Client> client;std::atomic<bool> finished{false};
 explicit WriteReservation(std::shared_ptr<Client> value):client(std::move(value)){client->BeginWrite();}
 void Finish(){if(!finished.exchange(true))client->EndWrite();}
 ~WriteReservation(){Finish();}
};
bool Valid(rex::system::KernelState* kernel,uint32_t address,size_t length){
 if(!address || !length || uint64_t(address)+length>0x100000000ull)return false;
 const auto end=uint64_t(address)+length;uint64_t cursor=address;
 while(cursor<end){auto* heap=kernel->memory()->LookupHeap(static_cast<uint32_t>(cursor));rex::memory::HeapAllocationInfo info{};
  if(!heap || !heap->QueryRegionInfo(static_cast<uint32_t>(cursor),&info) || !(info.state&rex::memory::kMemoryAllocationCommit) || (info.protect&(rex::memory::kMemoryProtectRead|rex::memory::kMemoryProtectWrite))!=(rex::memory::kMemoryProtectRead|rex::memory::kMemoryProtectWrite))return false;
  const uint64_t next=uint64_t(info.base_address)+info.region_size;if(next<=cursor)return false;cursor=next;
 }return true;
}
std::vector<Spec> Specs(lan::GuestBridge& bridge,uint32_t count,uint32_t address){
 if(!count || count>16 || !bridge.Valid(address,count*136))throw std::runtime_error("Invalid statistics specifications");
 std::vector<Spec> result;const auto* base=bridge.Kernel()->memory()->virtual_membase();
 for(unsigned i=0;i<count;++i){const auto* p=base+address+i*136;Spec spec;spec.view=U32(p);const auto columns=U32(p+4);if(!spec.view || spec.view>378 || columns>64)throw std::runtime_error("Unsupported statistics specification");
  std::set<uint16_t> distinct;for(unsigned j=0;j<columns;++j){const auto id=rex::memory::load_and_swap<uint16_t>(p+8+j*2);if(!distinct.insert(id).second)throw std::runtime_error("Duplicate statistics column");spec.columns.push_back(id);}result.push_back(std::move(spec));
 }return result;
}
class Enumerator final:public rex::system::XEnumerator {
 rex::system::KernelState* kernel_;std::shared_ptr<Client> client_;std::vector<Spec> specs_;
 uint32_t count_,mode_,size_;uint64_t first_;bool done_=false;std::mutex mutex_;
 public:
 Enumerator(rex::system::KernelState* kernel,std::shared_ptr<Client> client,std::vector<Spec> specs,uint32_t count,uint32_t mode,uint64_t first)
  :XEnumerator(kernel,1,ResultsSize(specs,count)),kernel_(kernel),client_(std::move(client)),specs_(std::move(specs)),count_(count),mode_(mode),size_(ResultsSize(specs_,count)),first_(first){std::lock_guard lock(enumerator_mutex);enumerators[this]=size_;}
 ~Enumerator(){std::lock_guard lock(enumerator_mutex);enumerators.erase(this);}
 uint32_t WriteItems(uint32_t output,uint8_t* data,uint32_t* written)override{
  std::lock_guard lock(mutex_);if(written)*written=0;if(done_)return 18;
  if(!Valid(kernel_,output,size_))return 87;
  try{
   std::vector<Page> pages;for(const auto& spec:specs_)pages.push_back(client_->Read(spec.view,mode_==1?static_cast<uint32_t>(first_):1,count_,{},mode_==0?first_:0));
   // Recheck after the network wait; do not retain a writable guest pointer.
   if(!Valid(kernel_,output,size_))return 87;
   (void)data;WriteResults(kernel_->memory()->virtual_membase(),output,specs_,pages);done_=true;if(written)*written=1;return 0;
  }catch(const std::exception& error){REXLOG_WARN("ONLINE_STATS: enumeration failed: {}",error.what());return 1231;}
 }
};
}
uint32_t ResultsSize(const std::vector<Spec>& specs,uint32_t rows){
 if(specs.empty() || specs.size()>16 || !rows || rows>100)throw std::runtime_error("Invalid statistics result capacity");
 size_t size=8+specs.size()*16;for(const auto& spec:specs){if(spec.columns.size()>64)throw std::runtime_error("Invalid statistics column capacity");size+=rows*(48+spec.columns.size()*24);}
 if(size>1024*1024)throw std::runtime_error("Statistics result capacity too large");return static_cast<uint32_t>(size);
}
void WriteResults(uint8_t* base,uint32_t output,const std::vector<Spec>& specs,const std::vector<Page>& pages){
 if(specs.size()!=pages.size())throw std::runtime_error("Statistics view count mismatch");
 size_t size=8+specs.size()*16;for(size_t i=0;i<specs.size();++i){if(pages[i].rows.size()>100)throw std::runtime_error("Statistics row count too large");size+=pages[i].rows.size()*(48+specs[i].columns.size()*24);}
 std::memset(base+output,0,size);Put32(base+output,static_cast<uint32_t>(specs.size()));Put32(base+output+4,output+8);
 uint32_t cursor=output+8+static_cast<uint32_t>(specs.size())*16;
 for(size_t i=0;i<specs.size();++i){const auto& page=pages[i];const auto& spec=specs[i];auto* view=base+output+8+i*16;
  Put32(view,spec.view);Put32(view+4,page.total);Put32(view+8,static_cast<uint32_t>(page.rows.size()));Put32(view+12,page.rows.empty()?0:cursor);
  const auto rows=cursor;cursor+=static_cast<uint32_t>(page.rows.size())*48;
  for(size_t j=0;j<page.rows.size();++j){const auto& source=page.rows[j];auto* row=base+rows+j*48;Put64(row,source.peer);Put32(row+8,source.rank);
   const auto primary=std::find_if(source.columns.begin(),source.columns.end(),[](const auto& c){return c.id==3;});if(primary!=source.columns.end())Put64(row+16,primary->bits);
   std::memcpy(row+24,source.name.data(),std::min<size_t>(15,source.name.size()));Put32(row+40,static_cast<uint32_t>(spec.columns.size()));Put32(row+44,spec.columns.empty()?0:cursor);
   for(auto id:spec.columns){auto* column=base+cursor;rex::memory::store_and_swap<uint16_t>(column,id);auto found=std::find_if(source.columns.begin(),source.columns.end(),[&](const auto& c){return c.id==id;});
    const Column value=id==0xFFFA?Column{id,1,source.rank}:found!=source.columns.end()?*found:Column{id,1,0};column[8]=value.type;
    if(value.type==1 || value.type==5)Put32(column+16,static_cast<uint32_t>(value.bits));else Put64(column+16,value.bits);cursor+=24;
   }
  }
 }
}
uint32_t CreateEnumerator(lan::GuestBridge& bridge,uint32_t mode,uint64_t first,uint32_t count,uint32_t num_specs,uint32_t input,uint32_t size_out,uint32_t handle_out){
 if(!bridge.Online() || mode>1 || !count || count>100 || !bridge.Valid(size_out,4,true) || !bridge.Valid(handle_out,4,true))return 87;
 try{
  auto specs=Specs(bridge,num_specs,input);const auto size=ResultsSize(specs,count);
  auto* object=new Enumerator(bridge.Kernel(),bridge.Online(),std::move(specs),count,mode,first);
  const auto status=object->Initialize(0,0xFB,0xB0023,0xB0024,0);
  if(XFAILED(status)){object->ReleaseHandle();object->Release();return 8;}
  auto* base=bridge.Kernel()->memory()->virtual_membase();Put32(base+size_out,size);Put32(base+handle_out,object->handle());object->Release();return 0;
 }catch(const std::exception& error){REXLOG_WARN("ONLINE_STATS: enumerator rejected: {}",error.what());return 87;}
}
std::optional<uint32_t> EnumeratorSize(rex::system::KernelState* kernel,uint32_t handle){
 auto object=kernel->object_table()->LookupObject<rex::system::XEnumerator>(handle);if(!object)return {};
 std::lock_guard lock(enumerator_mutex);const auto found=enumerators.find(object.get());return found==enumerators.end()?std::nullopt:std::optional(found->second);
}
uint32_t ReadStats(lan::GuestBridge& bridge,uint32_t buffer,uint32_t size){
 return PrepareRead(bridge,buffer,size)();
}
std::function<uint32_t()> PrepareRead(lan::GuestBridge& bridge,uint32_t buffer,uint32_t size){
 const auto fail=[](uint32_t error){return [error](){return error;};};
 if((size && size!=28) || !bridge.Valid(buffer,28))return fail(Error(87));auto* base=bridge.Kernel()->memory()->virtual_membase();auto* b=base+buffer;
 const auto title=U32(b),count=U32(b+4),users=U32(b+8),num_specs=U32(b+12),input=U32(b+16),bytes=U32(b+20),output=U32(b+24);
 if((title && title!=lan::kTitle) || count>100 || (count && !bridge.Valid(users,count*8)))return fail(Error(87));
 try{
  auto specs=Specs(bridge,num_specs,input);const auto required=ResultsSize(specs,std::max(1u,count));if(bytes<required)return fail(Error(122));if(!bridge.Valid(output,bytes,true))return fail(Error(87));
  std::vector<uint64_t> identities;for(unsigned i=0;i<count;++i){auto id=U64(base+users+i*8);if(id==bridge.Kernel()->user_profile()->xuid())id=bridge.Network().Identity();identities.push_back(id);}
  return [client=bridge.Online(),kernel=bridge.Kernel(),specs=std::move(specs),identities=std::move(identities),count,output,bytes](){
   try{std::vector<Page> pages;for(const auto& spec:specs)pages.push_back(client->Read(spec.view,1,std::max(1u,count),identities));
    if(!Valid(kernel,output,bytes))return Error(87);WriteResults(kernel->memory()->virtual_membase(),output,specs,pages);return 0u;
   }catch(const std::exception& error){REXLOG_WARN("ONLINE_STATS: read failed: {}",error.what());return Error(1231);}
  };
 }catch(const std::exception& error){REXLOG_WARN("ONLINE_STATS: read rejected: {}",error.what());return fail(Error(87));}
}
uint32_t WriteStats(lan::GuestBridge& bridge,uint32_t buffer){
 return PrepareWrite(bridge,buffer)();
}
std::function<uint32_t()> PrepareWrite(lan::GuestBridge& bridge,uint32_t buffer){
 const auto fail=[](uint32_t error){return [error](){return error;};};
 auto* base=bridge.Kernel()->memory()->virtual_membase();const auto* b=base+buffer;
 const auto id=U64(b+8);if(id!=bridge.Network().Identity() && id!=bridge.Kernel()->user_profile()->xuid())return fail(Error(5));
 const auto count=U32(b+16),input=U32(b+20);if(!count || count>16 || !bridge.Valid(input,count*12))return fail(Error(87));
 try{
  std::vector<View> views;
  for(unsigned i=0;i<count;++i){const auto* p=base+input+i*12;View view;view.id=U32(p);const auto columns=U32(p+4),properties=U32(p+8);
   if(!view.id || view.id>378 || !columns || columns>64 || !bridge.Valid(properties,columns*24))return fail(Error(87));
   for(unsigned j=0;j<columns;++j){const auto* property=base+properties+j*24;const auto prop=U32(property),type=prop>>28;if(type!=property[8] || (type!=1 && type!=2 && type!=3 && type!=5))return fail(Error(87));
    const auto bits=type==1?static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(U32(property+16)))):type==5?uint64_t(U32(property+16)):U64(property+16);view.columns.push_back({static_cast<uint16_t>(prop),static_cast<uint8_t>(type),bits});
   }views.push_back(std::move(view));
  }
  std::array<uint8_t,16> nonce{};lan::RandomBytes(nonce);std::string key;constexpr char hex[]="0123456789abcdef";for(auto byte:nonce){key+=hex[byte>>4];key+=hex[byte&15];}
  auto reservation=std::make_shared<WriteReservation>(bridge.Online());
  return [reservation,views=std::move(views),key=std::move(key)](){
   uint32_t result=0;
   try{reservation->client->Write(views,key);REXLOG_INFO("ONLINE_STATS: {} native view(s) stored for this player",views.size());}
   catch(const std::exception& error){REXLOG_WARN("ONLINE_STATS: write failed: {}",error.what());result=Error(1231);}
   reservation->Finish();return result;
  };
 }catch(const std::exception& error){REXLOG_WARN("ONLINE_STATS: write rejected: {}",error.what());return fail(Error(87));}
}
}
