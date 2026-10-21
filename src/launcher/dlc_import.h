#pragma once
#include "dlc_archives.h"
#include "../dlc_installation.h"
#include <rex/system/xam/content_manager.h>
#include <rex/system/xam/content_device.h>

namespace launcher::dlc {
struct Progress {std::atomic<uint64_t> done{0},total{0};std::atomic<unsigned> phase{0};};
struct Result {unsigned installed=0,existing=0;};
struct Stage {
  fs::path path;
  ~Stage(){std::error_code ec;fs::remove_all(path,ec);}
};
struct ContentLock {
  HANDLE handle{};
  explicit ContentLock(const fs::path& root){
    uint64_t hash=14695981039346656037ull;
    for(auto c:root.wstring()){hash^=static_cast<uint16_t>(std::towlower(c));hash*=1099511628211ull;}
    const auto name=L"Local\\MotoGP0910Dlc-"+std::to_wstring(hash);
    handle=CreateMutexW(nullptr,TRUE,name.c_str());
    if(!handle)throw std::runtime_error("Could not lock DLC storage");
    if(GetLastError()==ERROR_ALREADY_EXISTS){CloseHandle(handle);handle=nullptr;throw Error(L"Another launcher is already importing DLC into this installation.",L"Başka bir Launcher bu kurulumun DLC klasörüne zaten içerik kuruyor.");}
  }
  ~ContentLock(){if(handle){ReleaseMutex(handle);CloseHandle(handle);}}
  ContentLock(const ContentLock&)=delete;ContentLock& operator=(const ContentLock&)=delete;
};
inline fs::path CreateStage(const fs::path& root){
  fs::create_directories(root);
  for(unsigned attempt=0;attempt<16;++attempt){
    std::array<uint8_t,16> random{};
    if(BCryptGenRandom(nullptr,random.data(),random.size(),BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0)throw std::runtime_error("Could not create DLC staging token");
    std::wstring name=L".motogp-dlc-";for(auto c:random){name+=L"0123456789abcdef"[c>>4];name+=L"0123456789abcdef"[c&15];}
    const auto path=root/name;if(fs::create_directory(path))return path;
  }
  throw std::runtime_error("Could not create DLC staging folder");
}
inline void CheckDestination(const fs::path& root,const fs::path& target){
  auto current=root;
  for(const auto& component:target.lexically_relative(root)){
    if(component==L"..")throw std::runtime_error("DLC path is outside the content folder");
    current/=component;
    if(iso::Reparse(current))throw Error(L"A DLC destination folder is a filesystem link. No files were changed.",L"DLC hedefindeki bir klasör dosya sistemi bağlantısı. Hiçbir dosya değiştirilmedi.");
    if(fs::exists(current)&&!fs::is_directory(current)&&current!=target)throw std::runtime_error("A file blocks the DLC destination");
  }
}
inline void CopyPackage(const fs::path& source,const fs::path& destination,Cancel cancel){
  iso::Handle input(CreateFileW(source.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));
  if(input.value==INVALID_HANDLE_VALUE)throw std::runtime_error("Could not open the selected DLC package");
  std::ofstream output(destination,std::ios::binary);if(!output)throw std::runtime_error("Could not stage the DLC package");
  std::vector<uint8_t> buffer(1024*1024);DWORD read=0;
  for(;;){Check(cancel);if(!::ReadFile(input.value,buffer.data(),buffer.size(),&read,nullptr))throw std::runtime_error("Could not read the DLC package");if(!read)break;output.write(reinterpret_cast<char*>(buffer.data()),read);if(!output)throw std::runtime_error("Could not write staged DLC package");}
  output.close();if(!output)throw std::runtime_error("Could not finish staged DLC package");
}
inline std::vector<const Package*> CopyFolder(const fs::path& folder,const fs::path& stage,Cancel cancel,std::set<std::string>& found){
  std::vector<const Package*> result;size_t entries=0;
  for(auto it=fs::recursive_directory_iterator(folder);it!=fs::recursive_directory_iterator();++it){
    Check(cancel);if(++entries>20000||it.depth()>32)throw Error(L"Select the extracted DLC folder, rather than a drive or your entire game library.",L"Bir sürücü veya tüm oyun arşivin yerine çıkarılmış DLC klasörünü seç.");
    if(it->path()==stage||iso::Reparse(it->path())){it.disable_recursion_pending();continue;}
    if(!it->is_regular_file())continue;
    auto name=Utf8(it->path().filename().wstring());for(auto& c:name)if(c>='a'&&c<='f')c-=32;
    const auto* p=ByName(name);if(!p)continue;
    if(fs::file_size(it->path())!=p->bytes||!found.insert(p->id).second)throw Error(L"A DLC package is incomplete or appears more than once in this folder.",L"Bu klasörde eksik bir DLC paketi veya aynı paketten birden fazla var.");
    if(fs::space(stage).available<p->bytes+p->payload_bytes+96*1024*1024)throw Error(L"There is not enough free space to import the DLC.",L"DLC kurulumu için yeterli boş alan yok.");
    CopyPackage(it->path(),stage/L"packages"/p->id,cancel);result.push_back(p);
  }
  if(result.empty())throw Error(L"No supported DLC package was found. Select the folder containing the original Xbox 360 DLC files.",L"Desteklenen DLC paketi bulunamadı. Özgün Xbox 360 DLC dosyalarını içeren klasörü seç.");
  return result;
}
struct PayloadFile {fs::path path;rex::filesystem::Entry* entry;};
inline std::vector<PayloadFile> InspectPayload(rex::filesystem::Entry* root,const Package& spec,Cancel cancel){
  struct Directory{rex::filesystem::Entry* entry;fs::path path;unsigned depth;};
  std::vector<Directory> pending{{root,{},0}};std::vector<PayloadFile> files;std::set<std::wstring> names;uint64_t bytes=0;size_t entries=0;
  while(!pending.empty()){
    Check(cancel);auto item=std::move(pending.back());pending.pop_back();if(item.depth>32)throw std::runtime_error("DLC directory nesting is too deep");
    for(const auto& child:item.entry->children()){
      if(++entries>4096)throw std::runtime_error("DLC has too many entries");
      const auto& name=child->name();auto path=item.path/iso::Name(reinterpret_cast<const uint8_t*>(name.data()),name.size());
      if(!names.insert(iso::Key(path.generic_wstring())).second)throw std::runtime_error("Duplicate DLC path");
      if(child->attributes()&rex::filesystem::kFileAttributeDirectory)pending.push_back({child.get(),path,item.depth+1});
      else {if(child->size()>spec.payload_bytes-bytes)throw std::runtime_error("Invalid DLC payload size");bytes+=child->size();files.push_back({std::move(path),child.get()});}
    }
  }
  if(bytes!=spec.payload_bytes||files.size()!=spec.files)throw std::runtime_error("DLC payload does not match the supported package");
  const auto marker=L"DLCPack0"+std::to_wstring(spec.marker)+L".meta";
  if(std::none_of(files.begin(),files.end(),[&](const auto& file){return file.path==marker&&file.entry->size()==15;}))throw std::runtime_error("DLC recognition marker is missing");
  return files;
}
inline void ExtractPayload(const fs::path& source,const fs::path& target,const fs::path& header,
    const Package& spec,Cancel cancel,Progress& progress){
  Check(cancel);
  if(fs::file_size(source)!=spec.bytes||Sha256(source)!=spec.sha256)
    throw Error(L"A DLC package is damaged or unsupported. Nothing was installed.",L"DLC paketlerinden biri bozuk veya desteklenmiyor. Kurulum yapılmadı.");
  iso::Handle lock(CreateFileW(source.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));
  if(lock.value==INVALID_HANDLE_VALUE)throw std::runtime_error("Could not lock the staged DLC package");
  rex::filesystem::StfsContainerDevice device("dlc-import:",source);
  if(!device.Initialize())throw std::runtime_error("Could not open DLC content");
  const auto& metadata=device.header().metadata;
  if(metadata.execution_info.title_id!=0x434307F0||metadata.content_type!=rex::system::XContentType::kMarketplaceContent)
    throw std::runtime_error("DLC belongs to a different title or content type");
  uint32_t mask=0;for(const auto& license:device.header().header.licenses)if(license.license_flags)mask|=uint32_t(license.license_bits);
  if(mask!=spec.license)throw std::runtime_error("DLC license metadata does not match the original package");
  auto* root=device.ResolvePath("");if(!root)throw std::runtime_error("DLC root is missing");
  const auto files=InspectPayload(root,spec,cancel);fs::create_directory(target);std::vector<uint8_t> buffer(1024*1024);
  for(const auto& item:files){
    Check(cancel);const auto output=target/item.path;fs::create_directories(output.parent_path());
    rex::filesystem::File* raw=nullptr;
    if(XFAILED(item.entry->Open(rex::filesystem::FileAccess::kGenericRead,&raw))||!raw)throw std::runtime_error("Could not read a DLC asset");
    struct Close{rex::filesystem::File* file;~Close(){file->Destroy();}} close{raw};
    std::ofstream file(output,std::ios::binary);if(!file)throw std::runtime_error("Could not create a staged DLC asset");
    size_t offset=0;
    while(offset<item.entry->size()){
      Check(cancel);const auto count=std::min<uint64_t>(buffer.size(),item.entry->size()-offset);size_t read=0;
      if(XFAILED(raw->ReadSync(std::span<uint8_t>(buffer.data(),count),offset,&read))||read!=count)throw std::runtime_error("Short DLC asset read");
      file.write(reinterpret_cast<char*>(buffer.data()),read);if(!file)throw std::runtime_error("DLC asset write failed");offset+=read;progress.done.fetch_add(read);
    }
    file.close();if(!file||fs::file_size(output)!=item.entry->size())throw std::runtime_error("DLC asset write was incomplete");
  }
  const auto marker=target/(L"DLCPack0"+std::to_wstring(spec.marker)+L".meta");
  {std::ifstream input(marker,std::ios::binary);std::string text((std::istreambuf_iterator<char>(input)),{});if(text!="DLCPACK1PADDING")throw std::runtime_error("Invalid DLC recognition marker");}
  // Use the pinned SDK's actual content-header ABI. Preserve the supplied
  // package's license bits; do not manufacture unlocks or global licenses.
  rex::system::xam::XCONTENT_AGGREGATE_DATA data{};
  data.device_id=uint32_t(rex::system::xam::DummyDeviceId::HDD);data.content_type=rex::system::XContentType::kMarketplaceContent;
  data.xuid=0;data.title_id=0x434307F0;data.set_file_name(spec.id);data.set_display_name(metadata.display_name(rex::system::XLanguage::kEnglish));
  std::ofstream out(header,std::ios::binary);out.write(reinterpret_cast<const char*>(&data),sizeof(data));
  if(mask)out.write(reinterpret_cast<const char*>(&mask),sizeof(mask));out.close();if(!out)throw std::runtime_error("Could not prepare DLC content metadata");
}
inline bool SameFile(const fs::path& a,const fs::path& b){return !iso::Reparse(a)&&fs::is_regular_file(a)&&fs::file_size(a)==fs::file_size(b)&&Sha256(a)==Sha256(b);}
inline void RequireSamePayload(const fs::path& existing,const fs::path& staged,Cancel cancel){
  if(iso::Reparse(existing)||!fs::is_directory(existing))throw std::runtime_error("An existing DLC path is not a regular folder");
  for(const auto& entry:fs::recursive_directory_iterator(staged)){
    Check(cancel);const auto path=existing/entry.path().lexically_relative(staged);
    if(iso::Reparse(path)||(entry.is_directory()? !fs::is_directory(path):!SameFile(path,entry.path())))
      throw Error(L"An existing DLC installation differs and was kept. No game or save files were changed.",L"Mevcut DLC kurulumu farklı olduğu için korundu. Oyun ve kayıt dosyaları değiştirilmedi.");
  }
}
inline Result Import(const std::vector<fs::path>& sources,const fs::path& app,Cancel cancel,Progress& progress){
  if(sources.empty()||sources.size()>64)throw std::runtime_error("Select between one and 64 DLC sources");
  const auto destination=motogp::dlc::ContentRoot(app);
  if(iso::Reparse(destination))throw Error(L"The DLC folder cannot be a filesystem link.",L"DLC klasörü dosya sistemi bağlantısı olamaz.");
  const auto root=fs::weakly_canonical(destination);ContentLock storage_lock(root);Stage stage{CreateStage(root)};progress.phase=0;
  fs::create_directory(stage.path/L"packages");
  std::vector<const Package*> selected;std::set<std::string> found;unsigned index=0;
  for(const auto& source:sources){
    Check(cancel);
    if(fs::is_directory(source)){
      const auto packages=CopyFolder(source,stage.path,cancel,found);selected.insert(selected.end(),packages.begin(),packages.end());
    }else{
      const auto archive_stage=stage.path/(L"archive-"+std::to_wstring(index++));fs::create_directory(archive_stage);
      for(const auto& entry:UnpackArchive(source,app,archive_stage,cancel)){
        if(!found.insert(entry.package->id).second)throw Error(L"The selection contains the same DLC package more than once. Select each package only once.",L"Seçimde aynı DLC paketi birden fazla kez bulunuyor. Her paketi yalnızca bir kez seç.");
        fs::rename(archive_stage/L"packages"/entry.package->id,stage.path/L"packages"/entry.package->id);selected.push_back(entry.package);
      }
    }
  }
  for(const auto* p:selected)progress.total.fetch_add(p->payload_bytes);
  if(fs::space(stage.path).available<progress.total.load()+96*1024*1024)throw Error(L"There is not enough free space to extract all selected DLC packages.",L"Seçilen tüm DLC paketlerini çıkarmak için yeterli boş alan yok.");
  fs::create_directory(stage.path/L"payload");fs::create_directory(stage.path/L"headers");progress.phase=1;
  for(const auto* p:selected){
    ExtractPayload(stage.path/L"packages"/p->id,stage.path/L"payload"/p->id,stage.path/L"headers"/(std::string(p->id)+".header"),*p,cancel,progress);
    fs::remove(stage.path/L"packages"/p->id);
  }
  const auto packages=motogp::dlc::Packages(root),headers=motogp::dlc::Headers(root);
  struct Move{fs::path from,to;bool directory;};std::vector<Move> moves;Result result;
  for(const auto* p:selected){
    Check(cancel);const auto target=packages/p->id,header=headers/(std::string(p->id)+".header");
    CheckDestination(root,target);CheckDestination(root,header);
    if(fs::exists(target)){RequireSamePayload(target,stage.path/L"payload"/p->id,cancel);++result.existing;}
    else {moves.push_back({stage.path/L"payload"/p->id,target,true});++result.installed;}
    if(fs::exists(header)){
      if(!SameFile(header,stage.path/L"headers"/header.filename()))throw Error(L"Existing DLC metadata differs and was kept.",L"Mevcut DLC bilgileri farklı olduğu için korundu.");
    }else moves.push_back({stage.path/L"headers"/header.filename(),header,false});
  }
  Check(cancel);
  if(updates::GameRunning(app))throw Error(L"Close the game before installing DLC.",L"DLC kurmadan önce oyunu kapat.");
  progress.phase=2;std::vector<const Move*> installed;installed.reserve(moves.size());
  try{
    for(const auto& move:moves){
      Check(cancel);CheckDestination(root,move.to);fs::create_directories(move.to.parent_path());
      if(fs::exists(move.to)||!MoveFileExW(move.from.c_str(),move.to.c_str(),MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Could not publish DLC content");
      installed.push_back(&move);
    }
    Check(cancel);
  }catch(...){
    // Only paths created by this transaction are removed. Pre-existing game,
    // update, save and DLC paths are never overwritten or deleted.
    std::error_code ec;
    for(auto it=installed.rbegin();it!=installed.rend();++it){if((*it)->directory)fs::remove_all((*it)->to,ec);else fs::remove((*it)->to,ec);}
    throw;
  }
  return result;
}
}
