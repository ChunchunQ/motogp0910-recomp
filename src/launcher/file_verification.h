#pragma once
#include "core.h"
#include <atomic>
#include <functional>

namespace launcher {
inline std::string_view EmbeddedBytes(int id) {
  const auto module=GetModuleHandleW(nullptr);
  const auto resource=FindResourceW(module,MAKEINTRESOURCEW(id),MAKEINTRESOURCEW(10));
  if(!resource)throw std::runtime_error("Embedded launcher resource is missing");
  const auto handle=LoadResource(module,resource);
  const auto bytes=static_cast<const char*>(LockResource(handle));
  const auto size=SizeofResource(module,resource);
  if(!bytes || !size)throw std::runtime_error("Embedded launcher resource is empty");
  return {bytes,size};
}
struct GameFileReference {fs::path path;uintmax_t size;std::string sha256;};
inline std::vector<GameFileReference> GameFileReferences(std::string_view data) {
  std::vector<GameFileReference> result;
  std::istringstream stream{std::string(data)};std::string line;
  while(std::getline(stream,line)) {
    if(!line.empty() && line.back()=='\r')line.pop_back();
    if(line.empty())continue;
    const auto first=line.find('\t'),second=line.find('\t',first==std::string::npos?0:first+1);
    if(first!=64 || second==std::string::npos || line.find_first_not_of("0123456789abcdef",0)<first)
      throw std::runtime_error("Invalid game reference manifest");
    const auto relative=fs::path(Wide(std::string_view(line).substr(second+1)));
    if(relative.empty() || relative.is_absolute() || relative.has_root_name())throw std::runtime_error("Invalid game reference path");
    for(const auto& part:relative)if(part==L".." || part==L".")throw std::runtime_error("Invalid game reference traversal");
    const auto number=line.substr(first+1,second-first-1);size_t used=0;
    const auto size=std::stoull(number,&used);
    if(number.empty() || used!=number.size() || number.find_first_not_of("0123456789")!=std::string::npos)
      throw std::runtime_error("Invalid game reference size");
    result.push_back({relative,size,line.substr(0,first)});
  }
  if(result.empty())throw std::runtime_error("No game reference files");
  return result;
}
struct GameVerification {
  size_t checked=0,matched=0,missing=0,changed=0,unreadable=0;
  bool cancelled=false;
  std::vector<std::wstring> problems;
  bool Passed()const{return !cancelled && !missing && !changed && !unreadable;}
};
// This routine reads only the user's disk files; it never repairs, deletes,
// creates a trust baseline from their files, or touches game saves/profiles.
inline GameVerification VerifyGameData(const fs::path& root,const std::vector<GameFileReference>& files,
    const std::atomic<bool>& cancel,std::atomic<size_t>& progress) {
  GameVerification result;
  for(const auto& expected:files) {
    if(cancel.load()){result.cancelled=true;break;}
    const auto file=root/expected.path;
    try {
      if(!fs::is_regular_file(file)){++result.missing;result.problems.push_back(L"MISSING | "+expected.path.wstring());}
      else if(fs::file_size(file)!=expected.size || Sha256(file)!=expected.sha256){++result.changed;result.problems.push_back(L"MISMATCH | "+expected.path.wstring());}
      else ++result.matched;
    } catch(const std::exception&) {++result.unreadable;result.problems.push_back(L"UNREADABLE | "+expected.path.wstring());}
    progress.store(++result.checked);
  }
  return result;
}
} // namespace launcher
