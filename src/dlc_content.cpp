// Portable marketplace-content routing using the pinned ReXGlue content ABI.
// Enumeration and completion behavior follows ReXGlue's xam_content.cpp
// (Xenia/ReXGlue, BSD-3-Clause; notices retained with the runtime).
#include "dlc_content.h"
#include "dlc_paths.h"
#include "launcher/dlc_packages.h"
#include <rex/hook.h>
#include <rex/runtime.h>
#include <rex/filesystem/vfs.h>
#include <rex/system/xam/content_device.h>
#include <rex/system/xam/content_manager.h>
#include <rex/system/xenumerator.h>
#include <rex/system/xthread.h>
#include <rex/system/xio.h>
#include <fstream>
#include <map>
#include <mutex>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

using rex::X_RESULT;
using rex::X_HRESULT;
namespace motogp::dlc {
namespace {
namespace fs=std::filesystem;
using rex::system::xam::XCONTENT_DATA;
using rex::system::xam::XCONTENT_AGGREGATE_DATA;
using rex::system::xam::ContentPackage;
struct Store {
  rex::system::KernelState* kernel;
  fs::path root;
  std::mutex mutex;
  bool active=true;
  std::map<std::string,std::unique_ptr<ContentPackage>> mounts;
};
std::mutex store_mutex;
std::shared_ptr<Store> store;
std::shared_ptr<Store> Current(){
  std::lock_guard lock(store_mutex);
  return store&&store->kernel==rex::system::kernel_state()?store:nullptr;
}
bool Valid(rex::system::KernelState* kernel,uint32_t address,size_t length,bool write=false){
  if(!address||!length||uint64_t(address)+length>0x100000000ull)return false;
  for(uint64_t at=address,end=at+length;at<end;){
    auto* heap=kernel->memory()->LookupHeap(static_cast<uint32_t>(at));rex::memory::HeapAllocationInfo info{};
    if(!heap||!heap->QueryRegionInfo(static_cast<uint32_t>(at),&info)||!(info.state&rex::memory::kMemoryAllocationCommit)||
        !(info.protect&rex::memory::kMemoryProtectRead)||(write&&!(info.protect&rex::memory::kMemoryProtectWrite)))return false;
    const auto next=uint64_t(info.base_address)+info.region_size;if(next<=at)return false;at=next;
  }
  return true;
}
std::string Name(const XCONTENT_DATA& data){
  const auto* end=std::find(std::begin(data.file_name_raw),std::end(data.file_name_raw),'\0');
  return {std::begin(data.file_name_raw),end};
}
bool NoLinks(const fs::path& root,const fs::path& path){
  for(auto at=path;!at.empty()&&at!=root.parent_path();at=at.parent_path()){
#ifdef _WIN32
    const auto flags=GetFileAttributesW(at.c_str());
    if(flags!=INVALID_FILE_ATTRIBUTES&&(flags&FILE_ATTRIBUTE_REPARSE_POINT))return false;
#else
    if(fs::is_symlink(fs::symlink_status(at)))return false;
#endif
  }
  return true;
}
bool Metadata(const Store& content,const launcher::dlc::Package& spec,XCONTENT_AGGREGATE_DATA& data){
  const auto package=Packages(content.root)/spec.id,header=Headers(content.root)/(std::string(spec.id)+".header");
  if(!NoLinks(content.root,package)||!NoLinks(content.root,header)||!fs::is_directory(package)||!fs::is_regular_file(header)||
      fs::file_size(header)!=sizeof(data)+(spec.license?sizeof(uint32_t):0))return false;
  std::ifstream input(header,std::ios::binary);input.read(reinterpret_cast<char*>(&data),sizeof(data));
  if(!input||data.title_id!=0x434307F0||data.xuid!=0||data.content_type!=rex::system::XContentType::kMarketplaceContent||Name(data)!=spec.id)return false;
  uint32_t mask=0;if(spec.license)input.read(reinterpret_cast<char*>(&mask),sizeof(mask));
  if(!input||mask!=spec.license)return false;
  const auto marker=package/("DLCPack0"+std::to_string(spec.marker)+".meta");
  if(!NoLinks(content.root,marker)||!fs::is_regular_file(marker)||fs::file_size(marker)!=15)return false;
  std::ifstream mark(marker,std::ios::binary);std::string text((std::istreambuf_iterator<char>(mark)),{});return text=="DLCPACK1PADDING";
}
std::string RootName(rex::system::KernelState* kernel,uint8_t* base,uint32_t address){
  std::string result;
  for(unsigned i=0;i<64;++i){
    if(uint64_t(address)+i>UINT32_MAX||!Valid(kernel,address+i,1))return {};
    const char c=static_cast<char>(base[address+i]);if(!c)return result;
    if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'))return {};
    result+=c>='A'&&c<='Z'?c+32:c;
  }
  return {};
}
void Original(const char* name,PPCContext& ctx,uint8_t* base){
#ifdef _WIN32
  const auto module=GetModuleHandleW(L"rexruntime.dll");const auto fn=module?reinterpret_cast<PPCFunc*>(GetProcAddress(module,name)):nullptr;
#else
  const auto fn=reinterpret_cast<PPCFunc*>(dlsym(RTLD_NEXT,name));
#endif
  if(!fn)throw std::runtime_error(std::string("Missing runtime content import: ")+name);
  fn(ctx,base);
}
void Complete(Store& content,PPCContext& ctx,uint32_t overlapped,uint32_t result,uint32_t disposition=0){
  if(!overlapped){ctx.r3.u64=result;return;}
  if(rex::system::XThread::IsInThread())content.kernel->CompleteOverlappedImmediateEx(overlapped,result,X_HRESULT_FROM_WIN32(result),disposition);
  else content.kernel->CompleteOverlappedEx(overlapped,result,X_HRESULT_FROM_WIN32(result),disposition);
  ctx.r3.u64=X_ERROR_IO_PENDING;
}
bool Marketplace(Store& content,uint8_t* base,uint32_t address){
  if(!Valid(content.kernel,address,sizeof(XCONTENT_DATA)))return false;
  return rex::memory::load_and_swap<uint32_t>(base+address+4)==uint32_t(rex::system::XContentType::kMarketplaceContent);
}
void CloseMount(Store& content,const std::string& name){
  std::string target;
  // The SDK helper closes files by the live VFS alias. Guard against its
  // empty-prefix case before detaching our package/device.
  if(content.kernel->file_system()->FindSymbolicLink(name+":",target)&&!target.empty())
    content.kernel->content_manager()->CloseOpenedFilesFromContent(name);
  content.mounts.erase(name);
}
}
void Initialize(rex::Runtime* runtime,const fs::path& application){
  auto next=std::make_shared<Store>();next->kernel=runtime->kernel_state();next->root=fs::absolute(ContentRoot(application)).lexically_normal();
  std::lock_guard lock(store_mutex);store=std::move(next);
}
void Stop(){
  std::shared_ptr<Store> old;{std::lock_guard lock(store_mutex);old=std::move(store);}
  if(!old)return;std::lock_guard lock(old->mutex);old->active=false;
  while(!old->mounts.empty())CloseMount(*old,old->mounts.begin()->first);
}
}

REX_HOOK_RAW(__imp__XamContentCreateEnumerator){
  using namespace motogp::dlc;
  auto content=Current();
  if(!content||ctx.r5.u32!=uint32_t(rex::system::XContentType::kMarketplaceContent))return Original("__imp__XamContentCreateEnumerator",ctx,base);
  const auto device=ctx.r4.u32,count=ctx.r7.u32,size=ctx.r8.u32,handle=ctx.r9.u32;
  if(!Valid(content->kernel,handle,4,true)||(size&&!Valid(content->kernel,size,4,true))||!count||count>4096){ctx.r3.u64=X_E_INVALIDARG;return;}
  rex::memory::store_and_swap<uint32_t>(base+handle,0);
  if(size)rex::memory::store_and_swap<uint32_t>(base+size,0);
  const auto* info=device?rex::system::xam::GetDummyDeviceInfo(device):nullptr;
  if(device&&!info){ctx.r3.u64=X_E_INVALIDARG;return;}
  std::lock_guard lock(content->mutex);if(!content->active){ctx.r3.u64=X_ERROR_ACCESS_DENIED;return;}
  try{
    auto e=rex::system::make_object<rex::system::XStaticEnumerator<XCONTENT_DATA>>(content->kernel,count);
    const auto status=e->Initialize(0xFF,0xFE,0x20005,0x20007,0);if(XFAILED(status)){ctx.r3.u64=status;return;}
    if(!info||info->device_id==rex::system::xam::DummyDeviceId::HDD)for(const auto& spec:launcher::dlc::kPackages){
      XCONTENT_AGGREGATE_DATA metadata{};if(Metadata(*content,spec,metadata))*e->AppendItem()=metadata;
    }
    if(size)rex::memory::store_and_swap<uint32_t>(base+size,static_cast<uint32_t>(sizeof(XCONTENT_DATA)*count));
    rex::memory::store_and_swap<uint32_t>(base+handle,e->handle());ctx.r3.u64=X_ERROR_SUCCESS;
  }catch(const std::exception& error){REXLOG_WARN("DLC enumeration failed: {}",error.what());ctx.r3.u64=X_ERROR_ACCESS_DENIED;}
}
REX_HOOK_RAW(__imp__XamContentCreateEx){
  using namespace motogp::dlc;
  auto content=Current();if(!content||!Marketplace(*content,base,ctx.r5.u32))return Original("__imp__XamContentCreateEx",ctx,base);
  const auto stack=uint64_t(ctx.r1.u32)+0x54;
  if(stack>UINT32_MAX||!Valid(content->kernel,static_cast<uint32_t>(stack),4)){ctx.r3.u64=X_ERROR_INVALID_PARAMETER;return;}
  const auto overlapped=rex::memory::load_and_swap<uint32_t>(base+stack),disposition=ctx.r7.u32,license=ctx.r8.u32;
  if((overlapped&&!Valid(content->kernel,overlapped,sizeof(rex::system::XAM_OVERLAPPED),true))||
      (disposition&&!Valid(content->kernel,disposition,4,true))||(license&&!Valid(content->kernel,license,4,true))){ctx.r3.u64=X_ERROR_INVALID_PARAMETER;return;}
  const auto name=RootName(content->kernel,base,ctx.r4.u32);
  XCONTENT_DATA requested{};std::memcpy(&requested,base+ctx.r5.u32,sizeof(requested));
  const auto* spec=launcher::dlc::ByName(Name(requested));uint32_t result=X_ERROR_PATH_NOT_FOUND,opened=0;
  std::lock_guard lock(content->mutex);
  try{
    XCONTENT_AGGREGATE_DATA metadata{};const bool exists=content->active&&spec&&Metadata(*content,*spec,metadata);
    const auto mode=ctx.r6.u32&15;
    if(name.empty())result=X_ERROR_INVALID_PARAMETER;
    else if(mode==1)result=exists?X_ERROR_ALREADY_EXISTS:X_ERROR_ACCESS_DENIED;
    else if(mode!=3&&mode!=4)result=X_ERROR_ACCESS_DENIED;
    else if(exists){
      opened=2;std::string alias;
      if(content->mounts.contains(name)||!content->kernel->content_manager()->GetOpenPackagePath(name).empty()||
          content->kernel->file_system()->FindSymbolicLink(name+":",alias))result=X_ERROR_ALREADY_EXISTS;
      else{
        auto package=std::make_unique<ContentPackage>(content->kernel,name,metadata,Packages(content->root)/spec->id);
        package->LoadPackageLicenseMask(Headers(content->root)/(std::string(spec->id)+".header"));
        if(license)rex::memory::store_and_swap<uint32_t>(base+license,package->GetPackageLicense());
        content->mounts.emplace(name,std::move(package));result=X_ERROR_SUCCESS;
      }
    }
  }catch(const std::exception& error){REXLOG_WARN("DLC open failed: {}",error.what());result=X_ERROR_ACCESS_DENIED;}
  if(disposition)rex::memory::store_and_swap<uint32_t>(base+disposition,opened);
  Complete(*content,ctx,overlapped,result,opened);
}
REX_HOOK_RAW(__imp__XamContentClose){
  using namespace motogp::dlc;auto content=Current();
  if(!content)return Original("__imp__XamContentClose",ctx,base);
  const auto name=RootName(content->kernel,base,ctx.r3.u32);
  const auto overlapped=ctx.r4.u32;
  std::unique_lock lock(content->mutex);
  if(!content->mounts.contains(name)){lock.unlock();return Original("__imp__XamContentClose",ctx,base);}
  if(overlapped&&!Valid(content->kernel,overlapped,sizeof(rex::system::XAM_OVERLAPPED),true)){ctx.r3.u64=X_ERROR_INVALID_PARAMETER;return;}
  CloseMount(*content,name);Complete(*content,ctx,overlapped,X_ERROR_SUCCESS);
}
REX_HOOK_RAW(__imp__XamContentDelete){
  using namespace motogp::dlc;auto content=Current();
  if(!content||!Marketplace(*content,base,ctx.r4.u32))return Original("__imp__XamContentDelete",ctx,base);
  const auto overlapped=ctx.r5.u32;
  if(overlapped&&!Valid(content->kernel,overlapped,sizeof(rex::system::XAM_OVERLAPPED),true)){ctx.r3.u64=X_ERROR_INVALID_PARAMETER;return;}
  // Imported marketplace assets are managed by the installer, never a save-
  // content delete request. In particular, do not fall back to Documents DLC.
  Complete(*content,ctx,overlapped,X_ERROR_ACCESS_DENIED);
}
