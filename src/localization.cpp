#include "localization.h"
#include "turkish_achievements.h"
#include <rex/cvar.h>
#include <rex/runtime.h>
#include <rex/logging.h>
#include <rex/filesystem/vfs.h>
#include <rex/filesystem/devices/host_path_device.h>
#include <array>
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <unordered_map>

REXCVAR_DEFINE_STRING(game_text_language,"original","Language","Written language: original or turkish; Turkish keeps existing English voices")
    .allowed({"original","turkish"}).lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
namespace motogp::localization {
namespace {
using namespace rex::filesystem;
using rex::X_STATUS;
// VFS::OpenFile resolves the parent then calls GetChild, and relative opens
// walk the directory handle directly. Leaf symlinks do not cover either path.
// Keep a merged read-only entry tree so all three operations see the same file.
class OverlayFile final:public File {
 public:
  OverlayFile(uint32_t access,Entry* entry,File* backing):File(access,entry),backing_(backing){}
  ~OverlayFile() override {backing_->Destroy();}
  void Destroy() override {delete this;}
  X_STATUS ReadSync(std::span<uint8_t> bytes,size_t offset,size_t* read) override {return backing_->ReadSync(bytes,offset,read);}
  X_STATUS ReadAsync(std::span<uint8_t> bytes,size_t offset,size_t* read) override {return backing_->ReadAsync(bytes,offset,read);}
  X_STATUS WriteSync(std::span<const uint8_t>,size_t,size_t* written) override {if(written)*written=0;return X_STATUS_ACCESS_DENIED;}
 private:
  File* backing_;
};
using Replacements=std::unordered_map<Entry*,Entry*>;
class OverlayEntry final:public Entry {
 public:
  OverlayEntry(Device* device,Entry* parent,Entry* original,const Replacements& replacements)
      :Entry(device,parent,original->path()),backing_(replacements.contains(original)?replacements.at(original):original){
    attributes_=backing_->attributes()|kFileAttributeReadOnly;size_=backing_->size();allocation_size_=backing_->allocation_size();
    if(!original->children().empty())attributes_=(attributes_|kFileAttributeDirectory)&~kFileAttributeNormal;
    create_timestamp_=backing_->create_timestamp();access_timestamp_=backing_->access_timestamp();write_timestamp_=backing_->write_timestamp();
    for(const auto& child:original->children())children_.push_back(std::make_unique<OverlayEntry>(device,this,child.get(),replacements));
  }
  X_STATUS Open(uint32_t access,File** output) override {
    if(access&(FileAccess::kFileWriteData|FileAccess::kFileAppendData))return X_STATUS_ACCESS_DENIED;
    File* file=nullptr;const auto status=backing_->Open(access,&file);
    if(!XFAILED(status))*output=new OverlayFile(access,this,file);
    return status;
  }
  bool can_map() const override {return backing_->can_map();}
  std::unique_ptr<rex::memory::MappedMemory> OpenMapped(rex::memory::MappedMemory::Mode mode,size_t offset=0,size_t length=0) override {
    return mode==rex::memory::MappedMemory::Mode::kRead?backing_->OpenMapped(mode,offset,length):nullptr;
  }
 private:
  Entry* backing_;
};
class OverlayDevice final:public HostPathDevice {
 public:
  OverlayDevice(std::string_view mount,const std::filesystem::path& resources,Entry* original,
                std::vector<std::pair<Entry*,std::string>> files)
      :HostPathDevice(mount,resources,true),original_(original),files_(std::move(files)){}
  bool Initialize() override {
    if(!HostPathDevice::Initialize())return false;
    // The SDK lazily discovers entries beneath host directory symlinks. Prime
    // those before taking our snapshot, or only the ten translated paths would
    // be visible and unrelated scripts/voices could disappear from the tree.
    if(auto* host=dynamic_cast<HostPathDevice*>(original_->device())){
      const auto& folder=host->host_path();
      for(auto it=std::filesystem::recursive_directory_iterator(folder,std::filesystem::directory_options::follow_directory_symlink);
          it!=std::filesystem::recursive_directory_iterator();++it){
        if(it.depth()>=64){it.disable_recursion_pending();continue;}
        auto path=it->path().lexically_relative(folder).generic_string();std::replace(path.begin(),path.end(),'/','\\');
        if(!host->ResolvePath(path))return false;
      }
    }
    Replacements replacements;
    for(const auto& [original,path]:files_){
      auto* translated=HostPathDevice::ResolvePath(path);
      if(!translated)return false;
      replacements.emplace(original,translated);
    }
    root_=std::make_unique<OverlayEntry>(this,nullptr,original_,replacements);return true;
  }
  Entry* ResolvePath(std::string_view path) override {return root_->ResolvePath(path);}
  void Dump(rex::string::StringBuffer* buffer) override {root_->Dump(buffer,0);}
 private:
  Entry* original_;
  std::vector<std::pair<Entry*,std::string>> files_;
  std::unique_ptr<OverlayEntry> root_;
};
}
bool Turkish(){return rex::cvar::Query<std::string>("game_text_language")=="turkish";}
void ApplyStartupLanguage(){
  if(Turkish()&&!rex::cvar::SetFlagByName("user_language","1"))throw std::runtime_error("Could not select the base voice language");
}
void Install(rex::Runtime* runtime,const std::filesystem::path& resources){
  if(!Turkish())return;
  constexpr std::array files={"text/english.msg","scripts/bikes.english.lua","scripts/tracks.english.lua",
      "scripts/upgrades.english.lua","scripts/unlockables.english.lua","scripts/credits.english.lua",
      "fonts/impact_17pt.font","fonts/impact_small.font","fonts/arial_bold_17pt.font","fonts/arial_bold_small.font"};
  auto* vfs=runtime->file_system();
  constexpr const char* mount="\\Device\\MotoGPTurkishText";
  auto* original_root=vfs->ResolvePath("game:\\");
  if(!original_root)throw std::runtime_error("Turkish localization requires the game data root");
  std::vector<std::pair<rex::filesystem::Entry*,std::string>> mappings;
  for(const char* file:files){
    if(!std::filesystem::is_regular_file(resources/file))throw std::runtime_error("Turkish text resources are missing. Extract the complete build including resources/localization.");
    if(std::string_view(file).starts_with("fonts/")){
      std::ifstream font(resources/file,std::ios::binary);std::array<uint8_t,20> header{};
      font.read(reinterpret_cast<char*>(header.data()),header.size());
      const auto read32=[&](unsigned at){return uint32_t(header[at])<<24|uint32_t(header[at+1])<<16|uint32_t(header[at+2])<<8|header[at+3];};
      const auto height=uint32_t(header[18])<<8|header[19];
      if(!font||read32(0)!=1||read32(12)!=0x201||header[16]!=4||header[17]!=0||
         (height!=512&&height!=2048)||read32(8)!=1024*height+8||
         std::filesystem::file_size(resources/file)!=20ull+1024*height+4+368*32)
        throw std::runtime_error("Invalid Turkish font resource: "+std::string(file));
    }
    std::string relative=file;std::replace(relative.begin(),relative.end(),'/','\\');
    const auto guest="game:\\xenondata\\"+relative;
    auto* original=vfs->ResolvePath(guest);
    if(!original)throw std::runtime_error("Turkish localization requires the complete game's xenondata folder: "+guest);
    mappings.emplace_back(original,relative);
  }
  // Validate all files before changing VFS routing. A truncated bank must never
  // be handed to the title's unchecked pointer relocation loop.
  std::ifstream bank(resources/"text/english.msg",std::ios::binary|std::ios::ate);
  const auto length=bank.tellg();if(length<8412||length>1024*1024)throw std::runtime_error("Invalid Turkish message bank");
  std::vector<uint8_t> bytes(static_cast<size_t>(length));bank.seekg(0);bank.read(reinterpret_cast<char*>(bytes.data()),length);
  const auto u32=[&](size_t at){return uint32_t(bytes[at])<<24|uint32_t(bytes[at+1])<<16|uint32_t(bytes[at+2])<<8|bytes[at+3];};
  if(!bank||u32(0)!=2102)throw std::runtime_error("Invalid Turkish message count");
  for(unsigned i=0;i<2102;++i){const auto offset=uint64_t(u32(4+4*i))+4;
    if(offset<8412||offset>=bytes.size()||std::find(bytes.begin()+offset,bytes.end(),0)==bytes.end())throw std::runtime_error("Invalid Turkish message offset");
  }
  auto device=std::make_unique<OverlayDevice>(mount,resources,original_root,std::move(mappings));
  if(!device->Initialize()||!vfs->RegisterDevice(std::move(device)))throw std::runtime_error("Could not mount Turkish text resources");
  // Route the root once. Original files remain backing entries for everything
  // outside the ten whitelisted replacements, including every voice bank.
  vfs->RegisterSymbolicLink(original_root->absolute_path(),mount);
  auto achievements=runtime->kernel_state()->loaded_achievements();
  for(auto& entry:achievements)TranslateAchievement(entry);
  runtime->kernel_state()->SetLoadedAchievements(std::move(achievements));
  REXLOG_DEBUG("TURKISH_TEXT_READY messages=2102 overlay_files={} voice_language=english",files.size());
}
}
