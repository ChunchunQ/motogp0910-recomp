#pragma once
#include "file_verification.h"
#include <cstdint>
#include <cstring>
#include <set>
#include <rex/filesystem/devices/stfs_container_device.h>
#include <rex/filesystem/entry.h>
#include <rex/filesystem/file.h>

namespace launcher::iso {
struct Cancelled {};
struct Progress {
  std::atomic<uint64_t> copied{0},total{0};
  std::atomic<size_t> checked{0};
  std::atomic<bool> verifying{false};
};
inline void CheckCancel(const std::atomic<bool>& cancel){if(cancel.load())throw Cancelled{};}
struct Handle {
  HANDLE value=INVALID_HANDLE_VALUE;
  explicit Handle(HANDLE handle):value(handle){}
  ~Handle(){if(value!=INVALID_HANDLE_VALUE)CloseHandle(value);}
  Handle(const Handle&)=delete;Handle& operator=(const Handle&)=delete;
};
// Read a supported raw XDVDFS image without mounting it or running external tools.
// All offsets and directory records are bounded before reading or creating files.
class Image {
 public:
  explicit Image(const fs::path& path):file_(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,
      nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr)){
    LARGE_INTEGER bytes{};
    if(file_.value==INVALID_HANDLE_VALUE||!GetFileSizeEx(file_.value,&bytes)||bytes.QuadPart<2048)
      throw Error(L"Could not read the selected ISO.",L"Seçilen ISO okunamadı.");
    size_=static_cast<uint64_t>(bytes.QuadPart);
  }
  void Read(uint64_t offset,void* output,size_t bytes){
    if(offset>size_||bytes>size_-offset||bytes>MAXDWORD)Bad();
    LARGE_INTEGER where{};where.QuadPart=static_cast<LONGLONG>(offset);DWORD count=0;
    if(!SetFilePointerEx(file_.value,where,nullptr,FILE_BEGIN)||
       !::ReadFile(file_.value,output,static_cast<DWORD>(bytes),&count,nullptr)||count!=bytes)Bad();
  }
  uint64_t size()const{return size_;}
  [[noreturn]] static void Bad(){throw Error(L"The ISO has an invalid or unsupported Xbox 360 filesystem.",
      L"ISO geçersiz veya desteklenmeyen bir Xbox 360 dosya sistemi içeriyor.");}
 private:
  Handle file_;uint64_t size_=0;
};
inline uint32_t U32(const uint8_t* p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
inline uint16_t U16(const uint8_t* p){return uint16_t(p[0])|(uint16_t(p[1])<<8);}
inline std::wstring Key(std::wstring name){for(auto& c:name)if(c>=L'a'&&c<=L'z')c-=L'a'-L'A';return name;}
inline std::wstring Name(const uint8_t* bytes,size_t size){
  if(!size)Image::Bad();std::wstring result;
  for(size_t i=0;i<size;++i){const auto c=bytes[i];
    if(c<32||c>126||std::strchr("<>:\"/\\|?*",c))Image::Bad();result+=wchar_t(c);
  }
  if(result==L"."||result==L".."||result.back()==L'.'||result.back()==L' ')Image::Bad();
  auto stem=Key(result.substr(0,result.find(L'.')));while(!stem.empty()&&stem.back()==L' ')stem.pop_back();
  if(stem==L"CON"||stem==L"PRN"||stem==L"AUX"||stem==L"NUL"||
      (stem.size()==4&&(stem.starts_with(L"COM")||stem.starts_with(L"LPT"))&&stem[3]>=L'1'&&stem[3]<=L'9'))Image::Bad();
  return result;
}
struct File {fs::path path;uint64_t offset=0,size=0;};
struct Plan {std::vector<File> files;std::vector<fs::path> directories;uint64_t bytes=0;};
inline Plan Inspect(Image& image,const std::atomic<bool>& cancel){
  constexpr uint64_t sector_size=2048;
  std::array<uint8_t,2048> descriptor{};uint64_t partition=0;bool found=false;
  for(const uint64_t offset:{0ULL,0xFB20ULL,0x20600ULL,0x2080000ULL,0xFD90000ULL}){
    CheckCancel(cancel);const auto position=offset+32*sector_size;
    if(position>image.size()||descriptor.size()>image.size()-position)continue;
    image.Read(position,descriptor.data(),descriptor.size());
    if(!std::memcmp(descriptor.data(),"MICROSOFT*XBOX*MEDIA",20)&&
        !std::memcmp(descriptor.data()+descriptor.size()-20,"MICROSOFT*XBOX*MEDIA",20)){
      partition=offset;found=true;break;
    }
  }
  if(!found)Image::Bad();
  struct Directory {fs::path path;uint32_t sector,size,depth;};
  std::vector<Directory> pending{{{},U32(descriptor.data()+20),U32(descriptor.data()+24),0}};
  std::set<uint64_t> tables;std::set<std::wstring> names;Plan plan;size_t entries=0,path_characters=0;
  while(!pending.empty()){
    CheckCancel(cancel);auto directory=std::move(pending.back());pending.pop_back();
    if(directory.depth>64||directory.size>32*1024*1024)Image::Bad();
    if(!directory.size){if(directory.path.empty())Image::Bad();continue;}
    const auto start=partition+uint64_t(directory.sector)*sector_size;
    if(!tables.insert(start).second||start>image.size()||directory.size>image.size()-start)Image::Bad();
    std::vector<uint8_t> table(directory.size);image.Read(start,table.data(),table.size());
    std::vector<uint32_t> nodes{0};std::set<uint32_t> visited;
    while(!nodes.empty()){
      CheckCancel(cancel);const auto offset=nodes.back();nodes.pop_back();
      if(++entries>100000||!visited.insert(offset).second||offset>table.size()||table.size()-offset<14)Image::Bad();
      const auto* node=table.data()+offset;const auto length=node[13];
      if(length>table.size()-offset-14)Image::Bad();
      auto path=directory.path/Name(node+14,length);
      path_characters+=path.native().size();
      if(path.native().size()>4096||path_characters>8*1024*1024||!names.insert(Key(path.generic_wstring())).second)Image::Bad();
      const auto sector=U32(node+4),bytes=U32(node+8);const auto position=partition+uint64_t(sector)*sector_size;
      if(position>image.size()||bytes>image.size()-position)Image::Bad();
      if(node[12]&0x10){plan.directories.push_back(path);pending.push_back({path,sector,bytes,directory.depth+1});}
      else {
        if(bytes>image.size()-plan.bytes)Image::Bad();plan.bytes+=bytes;
        plan.files.push_back({std::move(path),position,bytes});
      }
      if(const auto left=U16(node))nodes.push_back(uint32_t(left)*4);
      if(const auto right=U16(node+2))nodes.push_back(uint32_t(right)*4);
    }
  }
  const auto executable=std::find_if(plan.files.begin(),plan.files.end(),[](const File& f){return Key(f.path.generic_wstring())==L"DEFAULT.XEX";});
  if(executable==plan.files.end())throw Error(L"This ISO does not contain MotoGP's default.xex.",L"Bu ISO MotoGP'nin default.xex dosyasını içermiyor.");
  if(executable->size>128*1024*1024)Image::Bad();
  std::iter_swap(plan.files.begin(),executable);return plan;
}
inline bool Reparse(const fs::path& path){const auto attributes=GetFileAttributesW(path.c_str());return attributes!=INVALID_FILE_ATTRIBUTES&&(attributes&FILE_ATTRIBUTE_REPARSE_POINT);}
inline void Publish(const fs::path& stage,const fs::path& target,const std::atomic<bool>& cancel){
  if(!fs::exists(target)){
    CheckCancel(cancel);
    if(MoveFileExW(stage.c_str(),target.c_str(),MOVEFILE_WRITE_THROUGH))return;
    if(!fs::exists(target))throw std::runtime_error("Could not install the extracted game folder");
  }
  if(Reparse(target)||!fs::is_directory(target))throw Error(L"The game destination must be a normal folder.",L"Oyun dosyalarının çıkarılacağı hedef normal bir klasör olmalı.");
  std::vector<fs::path> directories,files;
  for(const auto& entry:fs::recursive_directory_iterator(stage)){
    CheckCancel(cancel);const auto relative=entry.path().lexically_relative(stage);
    if(entry.is_directory())directories.push_back(relative);else files.push_back(relative);
  }
  std::sort(directories.begin(),directories.end());
  std::vector<fs::path> created_directories,created_files;
  created_directories.reserve(directories.size());created_files.reserve(files.size());
  try{
    for(const auto& relative:directories){
      CheckCancel(cancel);const auto path=target/relative;
      if(Reparse(path))throw std::runtime_error("A game subfolder is a filesystem link");
      if(fs::create_directory(path))created_directories.push_back(path);
      else if(!fs::is_directory(path))throw std::runtime_error("A game file conflicts with an extracted directory");
    }
    // Preflight every conflict before moving any new files. Existing matching
    // files and separately supplied updates remain untouched.
    for(const auto& relative:files){
      CheckCancel(cancel);const auto path=target/relative;
      if(fs::exists(path)&&(Reparse(path)||!fs::is_regular_file(path)||
          fs::file_size(path)!=fs::file_size(stage/relative)||Sha256(path)!=Sha256(stage/relative)))
        throw Error(L"An existing game file differs from the ISO and was kept: "+relative.wstring(),
            L"Mevcut oyun dosyası ISO'daki dosyadan farklı olduğu için korundu: "+relative.wstring());
    }
    for(const auto& relative:files){
      CheckCancel(cancel);const auto path=target/relative;
      if(fs::exists(path))continue;
      if(!MoveFileExW((stage/relative).c_str(),path.c_str(),MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Could not install an extracted game file");
      created_files.push_back(path);
    }
    CheckCancel(cancel);
  }catch(...){
    std::error_code error;
    for(auto i=created_files.rbegin();i!=created_files.rend();++i)fs::remove(*i,error);
    for(auto i=created_directories.rbegin();i!=created_directories.rend();++i)fs::remove(*i,error);
    throw;
  }
}
inline fs::path Import(const fs::path& source,const fs::path& target,const fs::path& previous,
    const std::vector<GameFileReference>& references,const std::atomic<bool>& cancel,Progress& progress){
  const auto parent=target.parent_path();
  Image image(source);auto plan=Inspect(image,cancel);progress.total=plan.bytes;
  if(fs::space(parent).available<plan.bytes+64*1024*1024)
    throw Error(L"There is not enough free space to extract the ISO.",L"ISO'yu çıkarmak için yeterli boş alan yok.");
  fs::path stage;
  for(int i=0;i<100;++i){auto candidate=parent/(L".motogp-iso-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64())+L"-"+std::to_wstring(i));
    if(fs::create_directory(candidate)){stage=std::move(candidate);break;}
  }
  if(stage.empty())throw std::runtime_error("Could not create an ISO staging folder");
  struct Cleanup {fs::path path;~Cleanup(){if(!path.empty()){std::error_code error;fs::remove_all(path,error);}}} cleanup{stage};
  for(const auto& directory:plan.directories){CheckCancel(cancel);fs::create_directories(stage/directory);}
  std::vector<uint8_t> buffer(1024*1024);
  for(size_t index=0;index<plan.files.size();++index){
    CheckCancel(cancel);const auto& file=plan.files[index];
    Handle output(CreateFileW((stage/file.path).c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr));
    if(output.value==INVALID_HANDLE_VALUE)throw std::runtime_error("Could not create an extracted file");
    for(uint64_t done=0;done<file.size;){
      CheckCancel(cancel);const auto bytes=static_cast<DWORD>(std::min<uint64_t>(buffer.size(),file.size-done));DWORD written=0;
      image.Read(file.offset+done,buffer.data(),bytes);
      if(!WriteFile(output.value,buffer.data(),bytes,&written,nullptr)||written!=bytes)throw std::runtime_error("Could not write an extracted file");
      done+=bytes;progress.copied.fetch_add(bytes);
    }
    if(!FlushFileBuffers(output.value))throw std::runtime_error("Could not flush an extracted file");
    CloseHandle(output.value);output.value=INVALID_HANDLE_VALUE;
    if(index==0&&Sha256(stage/file.path)!=kXexHash)
      throw Error(L"This is not the supported original MotoGP 09/10 ISO.",L"Bu ISO, desteklenen özgün MotoGP 09/10 sürümüyle eşleşmiyor.");
  }
  progress.verifying=true;
  const auto checked=VerifyGameData(stage,references,cancel,progress.checked);CheckCancel(cancel);
  if(!checked.Passed())throw Error(L"Extracted game files failed verification. Use a complete, unmodified supported ISO.",
      L"Çıkarılan oyun dosyaları doğrulanamadı. Desteklenen, eksiksiz ve değiştirilmemiş bir ISO kullan.");
  // Preserve a separately supplied, verified TU from the previously selected
  // game folder. Disc images are not assumed to contain this update.
  for(const auto& item:{std::pair{fs::path("default.xexp"),kPatchHash},std::pair{fs::path(kUpdate),kUpdateHash}}){
    CheckCancel(cancel);const auto from=previous/item.first,to=stage/item.first;
    if(!fs::exists(to)&&fs::is_regular_file(from)&&Sha256(from)==item.second)fs::copy_file(from,to);
  }
  CheckCancel(cancel);
  Publish(stage,target,cancel);return target;
}
struct TitleUpdateTemporary {
  fs::path path;bool owned=false;
  ~TitleUpdateTemporary(){if(owned){std::error_code error;fs::remove(path,error);}}
};
inline void ExtractTitleUpdatePatch(const fs::path& source,TitleUpdateTemporary& output){
  // Use the same pinned STFS reader as the game's startup extraction path.
  rex::filesystem::StfsContainerDevice device("launcher-update:",source);
  if(!device.Initialize())throw Error(L"Could not read the title-update package.",L"Oyun güncellemesi paketi okunamadı.");
  auto* entry=device.ResolvePath("default.xexp");rex::filesystem::File* file=nullptr;
  if(!entry||entry->size()!=1032192||XFAILED(entry->Open(rex::filesystem::FileAccess::kGenericRead,&file))||!file)
    throw Error(L"The title update does not contain the supported default.xexp patch.",L"Oyun güncellemesi desteklenen default.xexp yamasını içermiyor.");
  struct CloseFile {rex::filesystem::File* file;~CloseFile(){file->Destroy();}} close{file};
  std::vector<uint8_t> bytes(static_cast<size_t>(entry->size()));size_t read=0;
  if(XFAILED(file->ReadSync(bytes,0,&read))||read!=bytes.size())throw std::runtime_error("Could not extract the title-update patch");
  Handle destination(CreateFileW(output.path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr));
  if(destination.value==INVALID_HANDLE_VALUE)throw std::runtime_error("Could not stage default.xexp");
  output.owned=true;DWORD written=0;
  if(!WriteFile(destination.value,bytes.data(),static_cast<DWORD>(bytes.size()),&written,nullptr)||written!=bytes.size()||!FlushFileBuffers(destination.value))
    throw std::runtime_error("Could not write the extracted title-update patch");
  CloseHandle(destination.value);destination.value=INVALID_HANDLE_VALUE;
  if(Sha256(output.path)!=kPatchHash)throw Error(L"The extracted default.xexp failed verification.",L"Çıkarılan default.xexp doğrulanamadı.");
}
inline void InstallTitleUpdate(const fs::path& source,const fs::path& root){
  if(fs::file_size(source)>64*1024*1024)throw Error(L"Select the supported title-update file or default.xexp patch.",
      L"Desteklenen oyun güncellemesi dosyasını veya default.xexp yamasını seç.");
  const auto hash=Sha256(source);const bool patch=hash==kPatchHash;
  if(!patch&&hash!=kUpdateHash)throw Error(L"This file is not the supported MotoGP 09/10 title update or patch.",
      L"Bu dosya desteklenen MotoGP 09/10 oyun güncellemesi veya yama dosyası değil.");
  if(Reparse(root)||!fs::is_directory(root))throw std::runtime_error("The game destination must be a normal folder");
  const auto stamp=std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());
  TitleUpdateTemporary incoming{root/(L".title-update-"+stamp+L".tmp")};
  TitleUpdateTemporary extracted{root/(L".title-update-patch-"+stamp+L".tmp")};
  if(!CopyFileW(source.c_str(),incoming.path.c_str(),TRUE))throw std::runtime_error("Could not stage the title update");
  incoming.owned=true;
  if(Sha256(incoming.path)!=hash)throw std::runtime_error("The selected title update changed while copying");
  if(!patch)ExtractTitleUpdatePatch(incoming.path,extracted);
  struct Change {fs::path staged,target,backup;std::string_view expected;bool skip=false,installed=false;};
  std::vector<Change> changes;
  if(!patch)changes.push_back({incoming.path,root/kUpdate,{},kUpdateHash});
  changes.push_back({patch?incoming.path:extracted.path,root/L"default.xexp",{},kPatchHash});
  // Both inputs are verified before replacing anything. Preserve old update
  // files and roll back earlier replacements if installing the pair fails.
  for(auto& change:changes){
    if(Reparse(change.target))throw std::runtime_error("The title-update destination is a filesystem link");
    if(fs::exists(change.target)){
      if(!fs::is_regular_file(change.target))throw std::runtime_error("The title-update destination is not a regular file");
      if(Sha256(change.target)==change.expected){change.skip=true;continue;}
      change.backup=root/(change.target.filename().wstring()+L".previous-"+stamp);
      if(!CopyFileW(change.target.c_str(),change.backup.c_str(),TRUE))throw std::runtime_error("Could not preserve the previous title update");
    }
  }
  try{
    for(auto& change:changes){
      if(change.skip)continue;
      if(!MoveFileExW(change.staged.c_str(),change.target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("Could not install the title update and extracted patch");
      change.installed=true;
    }
  }catch(...){
    for(auto i=changes.rbegin();i!=changes.rend();++i)if(i->installed){
      if(i->backup.empty())DeleteFileW(i->target.c_str());
      else MoveFileExW(i->backup.c_str(),i->target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
    }
    throw;
  }
}
} // namespace launcher::iso
