#pragma once
#include <rex/system/achievement_store.h>
#include <filesystem>
#include <map>
#include <string>
#include <vector>
namespace launcher::achievements {
struct Entry {rex::system::AchievementInfo info;std::vector<uint8_t> icon;};
struct Catalog {std::vector<Entry> entries;};
Catalog LoadCatalog(const std::filesystem::path& game,const std::filesystem::path& metadata,int language);
// Read-only: accepts current timestamped and legacy unlock files, without the
// SDK manager's automatic legacy migration / saving behavior.
std::map<uint32_t,uint64_t> ReadUnlocks(const std::filesystem::path& file);
std::filesystem::path UserRoot(const std::filesystem::path& exe,const std::string& configured);
std::filesystem::path MetadataRoot(const std::filesystem::path& exe,const std::string& configured);
}
