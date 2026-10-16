#include "core.h"
#include "achievements.h"
#include "../turkish_achievements.h"
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/system/util/xdbf_utils.h>
#include <shlobj.h>
#include <charconv>
namespace launcher::achievements {
namespace {
fs::path Root(const fs::path& exe,const std::string& configured,const wchar_t* environment) {
  fs::path value;
  if(!configured.empty())value=fs::path(Wide(configured));
  else {const auto count=GetEnvironmentVariableW(environment,nullptr,0);if(count){std::wstring text(count,L'\0');const auto size=GetEnvironmentVariableW(environment,text.data(),count);if(size&&size<count){text.resize(size);value=text;}}}
  return value.empty()?value:fs::absolute(value.is_absolute()?value:exe/value).lexically_normal();
}
std::vector<uint8_t> Image(rex::Runtime& runtime,const rex::system::AchievementInfo& info) {
  for(const auto& relative:{fs::path(Wide(info.icon_path)),fs::path("icons")/(std::to_string(info.image_id)+".png")}){
    if(relative.empty())continue;
    if(auto path=runtime.FindMetadataPath(relative)){
      std::ifstream file(*path,std::ios::binary|std::ios::ate);const auto length=file.tellg();
      if(file&&length>0&&length<=4*1024*1024){std::vector<uint8_t> bytes(static_cast<size_t>(length));file.seekg(0);file.read(reinterpret_cast<char*>(bytes.data()),length);if(file)return bytes;}
    }
  }
  const auto block=runtime.kernel_state()->title_xdbf().GetEntry(rex::system::util::XdbfSection::kImage,info.image_id);
  return block&&block.size<=4*1024*1024?std::vector<uint8_t>(block.buffer,block.buffer+block.size):std::vector<uint8_t>{};
}
}
fs::path UserRoot(const fs::path& exe,const std::string& configured) {
  auto root=Root(exe,configured,L"REX_USER_DATA_ROOT");if(!root.empty())return root;
  PWSTR documents=nullptr;if(FAILED(SHGetKnownFolderPath(FOLDERID_Documents,KF_FLAG_DEFAULT,nullptr,&documents)))throw std::runtime_error("Could not locate Windows Documents");
  root=fs::path(documents)/"motogp0910";CoTaskMemFree(documents);return root;
}
fs::path MetadataRoot(const fs::path& exe,const std::string& configured){return Root(exe,configured,L"REX_METADATA_ROOT");}
Catalog LoadCatalog(const fs::path& game,const fs::path& metadata,int language) {
  if(!fs::is_regular_file(game/"default.xex"))throw std::runtime_error("Select the game folder in General to load achievements");
  if(Sha256(game/"default.xex")!=kXexHash)throw std::runtime_error("Unsupported default.xex; select the original MotoGP 09/10 game folder");
  // No user root: the SDK must not load, migrate or save the player's unlocks.
  rex::Runtime runtime(game,{},{},{},metadata);
  rex::cvar::SetFlagByName("user_language",std::to_string(language==1000?1:language));
  if(XFAILED(runtime.Setup(rex::RuntimeConfig{.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true})))throw std::runtime_error("Could not initialize the achievement reader");
  if(XFAILED(runtime.LoadXexImage("game:\\default.xex")))throw std::runtime_error("Could not read the game's achievement catalog");
  if(runtime.kernel_state()->title_id()!=0x434307F0)throw std::runtime_error("Unexpected game title ID");
  Catalog result;
  for(auto& info:runtime.kernel_state()->loaded_achievements()){if(language==1000)motogp::localization::TranslateAchievement(info);auto icon=Image(runtime,info);result.entries.push_back({std::move(info),std::move(icon)});}
  if(result.entries.empty())throw std::runtime_error("The game has no available achievement catalog");
  return result;
}
std::map<uint32_t,uint64_t> ReadUnlocks(const fs::path& file) {
  if(!fs::exists(file))throw std::runtime_error("Achievement save is temporarily unavailable");
  if(fs::file_size(file)>1024*1024)throw std::runtime_error("Achievement save is too large");
  const auto table=toml::parse_file(file.string());std::map<uint32_t,uint64_t> result;
  if(auto* legacy=table["unlocked"].as_array()){
    for(const auto& node:*legacy){auto id=node.value<int64_t>();if(!id||*id<=0||*id>UINT32_MAX)throw std::runtime_error("Invalid legacy achievement ID");result[static_cast<uint32_t>(*id)]=0;}
  }else if(auto* modern=table["unlocked"].as_table()){
    for(const auto& [key,node]:*modern){uint32_t id=0;const auto text=key.str();auto [end,error]=std::from_chars(text.data(),text.data()+text.size(),id);
      if(error!=std::errc{}||end!=text.data()+text.size()||!id||!node.is_table())throw std::runtime_error("Invalid achievement save entry");
      auto timestamp=(*node.as_table())["filetime"].value<int64_t>();if(!timestamp||*timestamp<0)throw std::runtime_error("Invalid achievement timestamp");result[id]=static_cast<uint64_t>(*timestamp);
    }
  }else if(table.contains("unlocked"))throw std::runtime_error("Invalid achievement save format");
  return result;
}
}
