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

REXCVAR_DEFINE_STRING(game_text_language,"original","Language","Written language: original or turkish; Turkish keeps existing English voices")
    .allowed({"original","turkish"}).lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
namespace motogp::localization {
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
  struct Mapping{std::string original,target;};std::vector<Mapping> mappings;
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
    mappings.push_back({original->absolute_path(),std::string(mount)+"\\"+relative});
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
  auto device=std::make_unique<rex::filesystem::HostPathDevice>(mount,resources,true);
  if(!device->Initialize()||!vfs->RegisterDevice(std::move(device)))throw std::runtime_error("Could not mount Turkish text resources");
  // Route resolved physical filenames, rather than overlapping game: prefixes.
  // SDK symbolic links use first-prefix matching, not longest-prefix matching.
  for(const auto& map:mappings)vfs->RegisterSymbolicLink(map.original,map.target);
  auto achievements=runtime->kernel_state()->loaded_achievements();
  for(auto& entry:achievements)TranslateAchievement(entry);
  runtime->kernel_state()->SetLoadedAchievements(std::move(achievements));
  REXLOG_INFO("TURKISH_TEXT_READY messages=2102 overlay_files={} voice_language=english",files.size());
}
}
