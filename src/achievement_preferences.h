#pragma once
// Shared by the launcher and F7 overlay. Unlock state is never written here.
#include <toml++/toml.hpp>
#include <algorithm>
#include <cwctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace motogp::achievements {
struct Options {bool popup=true,sound=true;int volume=25;bool operator==(const Options&)const=default;};
inline Options ReadOptions(const std::filesystem::path& path) {
  if(!std::filesystem::exists(path))return {};
  auto table=toml::parse_file(path.string());
  return {table["popup_enabled"].value_or(true),table["sound_enabled"].value_or(true),std::clamp(table["sound_volume"].value_or(25),0,100)};
}
// Only changed fields are merged into the latest file, preserving the other
// process's preferences and unknown future fields. A corrupt file isn't replaced.
inline bool WriteOptions(const std::filesystem::path& path,const Options& before,const Options& after) {
  if(path.empty()||before==after)return true;
#ifdef _WIN32
  uint64_t hash=14695981039346656037ull;
  for(auto c:std::filesystem::absolute(path).lexically_normal().wstring()){hash^=static_cast<uint16_t>(towlower(c));hash*=1099511628211ull;}
  const auto name=L"Local\\MotoGPAchievementPreferences-"+std::to_wstring(hash);
  HANDLE lock=CreateMutexW(nullptr,FALSE,name.c_str());if(!lock)return false;
  const auto wait=WaitForSingleObject(lock,3000);
  if(wait!=WAIT_OBJECT_0&&wait!=WAIT_ABANDONED){CloseHandle(lock);return false;}
  struct Unlock {HANDLE lock;~Unlock(){ReleaseMutex(lock);CloseHandle(lock);}} unlock{lock};
#endif
  auto temp=path;temp+=".tmp";
  try {
    auto table=std::filesystem::exists(path)?toml::parse_file(path.string()):toml::table{};
    if(before.popup!=after.popup)table.insert_or_assign("popup_enabled",after.popup);
    if(before.sound!=after.sound)table.insert_or_assign("sound_enabled",after.sound);
    if(before.volume!=after.volume)table.insert_or_assign("sound_volume",std::clamp(after.volume,0,100));
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(temp,std::ios::binary|std::ios::trunc);file<<table;file.close();if(!file)return false;
#ifdef _WIN32
    if(!MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return false;
#else
    std::filesystem::rename(temp,path);
#endif
    return true;
  }catch(const std::exception&){std::error_code ec;std::filesystem::remove(temp,ec);return false;}
}
}
