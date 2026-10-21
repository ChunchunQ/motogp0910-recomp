#include "user_data_paths.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <map>
#include <mutex>
#include <random>
#include <stdexcept>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#endif

namespace motogp::user_data {
namespace {
namespace fs=std::filesystem;
bool Present(const fs::path& path){return fs::symlink_status(path).type()!=fs::file_type::not_found;}
bool Ready(const fs::path& target){
  if(!Present(target))return false;
  if(!fs::is_directory(target))throw std::runtime_error("The MotoGP 09-10 user-data path is not a directory; existing data was preserved.");
  return true;
}
void CheckLink(const fs::path& path){
  if(fs::is_symlink(fs::symlink_status(path)))throw std::runtime_error("Cannot migrate linked user-data files; existing data was preserved.");
#ifdef _WIN32
  const auto attributes=GetFileAttributesW(path.c_str());
  if(attributes==INVALID_FILE_ATTRIBUTES)throw fs::filesystem_error("Cannot inspect user data",path,std::error_code(GetLastError(),std::system_category()));
  if(attributes&FILE_ATTRIBUTE_REPARSE_POINT){
    WIN32_FIND_DATAW data{};const auto search=FindFirstFileW(path.c_str(),&data);
    if(search==INVALID_HANDLE_VALUE)throw fs::filesystem_error("Cannot inspect user-data link",path,std::error_code(GetLastError(),std::system_category()));
    FindClose(search);
    // OneDrive cloud placeholders are allowed; junctions and symlinks are not.
    if(IsReparseTagNameSurrogate(data.dwReserved0))throw std::runtime_error("Cannot migrate linked user-data directories; existing data was preserved.");
  }
#endif
}
struct Entry {
  bool directory{};
  uintmax_t size{};
  fs::file_time_type modified{};
  bool operator==(const Entry&)const=default;
};
using Snapshot=std::map<fs::path,Entry>;
Snapshot Inspect(const fs::path& root){
  CheckLink(root);
  if(!fs::is_directory(root))throw std::runtime_error("The old MotoGP user-data path is not a directory; existing data was preserved.");
  Snapshot result;
  for(const auto& item:fs::recursive_directory_iterator(root)){
    CheckLink(item.path());
    const bool directory=item.is_directory();
    if(!directory&&!item.is_regular_file())throw std::runtime_error("Cannot migrate unsupported user-data entries; existing data was preserved.");
    result.emplace(item.path().lexically_relative(root),Entry{directory,directory?0:item.file_size(),item.last_write_time()});
  }
  return result;
}
void Verify(const fs::path& source,const fs::path& copy){
  std::ifstream original(source,std::ios::binary),duplicate(copy,std::ios::binary);
  if(!original||!duplicate)throw std::runtime_error("Cannot verify copied user data; the original folder was preserved.");
  std::array<char,64*1024> a{},b{};
  do{
    original.read(a.data(),a.size());duplicate.read(b.data(),b.size());
    if((original.fail()&&!original.eof())||(duplicate.fail()&&!duplicate.eof())||original.gcount()!=duplicate.gcount()||
        !std::equal(a.begin(),a.begin()+original.gcount(),b.begin()))
      throw std::runtime_error("User-data verification failed; the original folder was preserved.");
  }while(original.gcount());
}
#ifdef _WIN32
struct MigrationLock {
  HANDLE handle{};
  explicit MigrationLock(const fs::path& target){
    auto key=fs::absolute(target).lexically_normal().wstring();
    CharLowerBuffW(key.data(),static_cast<DWORD>(key.size()));
    uint64_t hash=14695981039346656037ull;for(const auto c:key){hash^=static_cast<uint16_t>(c);hash*=1099511628211ull;}
    const auto name=L"Local\\MotoGP0910UserDataMigration-"+std::to_wstring(hash);
    handle=CreateMutexW(nullptr,FALSE,name.c_str());
    if(!handle)throw std::runtime_error("Cannot lock user-data migration; existing data was preserved.");
    const auto wait=WaitForSingleObject(handle,INFINITE);
    if(wait!=WAIT_OBJECT_0&&wait!=WAIT_ABANDONED){CloseHandle(handle);handle=nullptr;throw std::runtime_error("Cannot wait for user-data migration; existing data was preserved.");}
  }
  ~MigrationLock(){ReleaseMutex(handle);CloseHandle(handle);}
};
void RequireGameClosed(){
  const auto processes=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
  if(processes==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot check whether MotoGP is running; existing data was preserved.");
  struct Close{HANDLE handle;~Close(){CloseHandle(handle);}} close{processes};
  PROCESSENTRY32W process{sizeof(process)};
  if(!Process32FirstW(processes,&process))throw std::runtime_error("Cannot read running processes; existing data was preserved.");
  do{
    if(process.th32ProcessID!=GetCurrentProcessId()&&!_wcsicmp(process.szExeFile,L"motogp0910.exe"))
      throw std::runtime_error("Close other MotoGP 09/10 game instances, then restart to migrate your saved data. The original folder was preserved.");
  }while(Process32NextW(processes,&process));
  if(GetLastError()!=ERROR_NO_MORE_FILES)throw std::runtime_error("Cannot finish checking running processes; existing data was preserved.");
}
#else
std::mutex migration_mutex;
struct MigrationLock {std::unique_lock<std::mutex> lock{migration_mutex};explicit MigrationLock(const fs::path&){};};
void RequireGameClosed(){}
#endif
// One-time cleanup also covers users migrated by a previous launcher version.
// A conflicting old folder is retained; normal launches never rescan it.
const char* const cleanup_marker=".legacy-migration-cleanup";
void FinishLegacyCleanup(const fs::path& source,const fs::path& target){
  bool removed=false;
  try{
    if(!Present(source))removed=true;
    else{
      RequireGameClosed();
      const auto before=Inspect(source);
      CheckLink(target);
      for(const auto& [relative,entry]:before){
        auto current=target;
        for(const auto& part:relative){current/=part;CheckLink(current);}
        const auto copy=target/relative;
        if(entry.directory){if(!fs::is_directory(copy))throw std::runtime_error("User-data directory conflict");}
        else{
          if(!fs::is_regular_file(copy)||fs::file_size(copy)!=entry.size)throw std::runtime_error("User-data file conflict");
          Verify(source/relative,copy);
        }
      }
      if(Inspect(source)!=before)throw std::runtime_error("Old user data changed before cleanup");
      // The new tree holds a verified copy of every old file before deletion.
      fs::remove_all(source);removed=true;
    }
  }catch(const std::exception&){
    // Cleanup must never prevent using the successfully published destination.
    // Conflicts, links, a running older game or deletion failures retain old data.
  }
  try{
    const auto marker=target/cleanup_marker;
    if(!Present(marker)){
      std::ofstream output(marker,std::ios::binary);output<<(removed?"complete\n":"preserved\n");
    }
  }catch(const std::exception&){} // A cleanup marker cannot invalidate migrated saves.
}
struct Staging {
  fs::path path;
  ~Staging(){std::error_code error;if(!path.empty())fs::remove_all(path,error);}
};
}
fs::path DefaultRoot(const fs::path& documents){
  if(documents.empty())throw std::runtime_error("Could not locate the Documents folder; existing saved data was preserved.");
  const auto target=documents/"MotoGP 09-10";
  // After one cleanup attempt, normal startup never scans the legacy tree.
  if(Ready(target)&&Present(target/cleanup_marker))return target;
  MigrationLock lock(target);
  const auto source=documents/"motogp0910";
  if(Ready(target)){
    if(!Present(target/cleanup_marker))FinishLegacyCleanup(source,target);
    return target;
  }
  if(!Present(source)){fs::create_directories(target);FinishLegacyCleanup(source,target);return target;}
  RequireGameClosed();
  const auto before=Inspect(source);
  Staging staging;
  std::random_device random;
  for(int attempt=0;attempt<32&&staging.path.empty();++attempt){
    const auto candidate=documents/(".MotoGP 09-10.migrate-"+std::to_string(random())+"-"+std::to_string(random()));
    if(fs::create_directory(candidate))staging.path=candidate;
  }
  if(staging.path.empty())throw std::runtime_error("Cannot stage user-data migration; the original folder was preserved.");
  for(const auto& [relative,entry]:before){
    const auto from=source/relative,to=staging.path/relative;
    if(entry.directory){fs::create_directories(to);continue;}
    CheckLink(from);fs::create_directories(to.parent_path());
    fs::copy_file(from,to,fs::copy_options::none);
    Verify(from,to);fs::last_write_time(to,entry.modified);
  }
  if(Inspect(source)!=before)throw std::runtime_error("Saved data changed during migration. Close the game and try again; the original folder was preserved.");
  // Publish the complete verified copy in one operation, then clean up the
  // legacy tree. Never merge into or overwrite an existing destination.
#ifdef _WIN32
  if(!MoveFileExW(staging.path.c_str(),target.c_str(),MOVEFILE_WRITE_THROUGH)){
    const auto error=GetLastError();
    if(Ready(target))return target;
    throw fs::filesystem_error("Cannot publish migrated user data; the original folder was preserved",target,std::error_code(error,std::system_category()));
  }
#else
  if(Ready(target))return target;
  fs::rename(staging.path,target);
#endif
  staging.path.clear();
  FinishLegacyCleanup(source,target);
  return target;
}
}
