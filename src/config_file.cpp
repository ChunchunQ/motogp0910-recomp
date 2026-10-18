#include "config_file.h"
#include "../third_party/nlohmann/json.hpp"
#include <fstream>
#include <sstream>
#include <set>
#include <random>
#include <limits>
#include <cmath>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace motogp::config {
namespace {
namespace fs=std::filesystem;
using Json=nlohmann::json;
void Insert(toml::table& table,const std::string& key,const Json& value);
template<class Container,class Key> void Decode(Container& out,const Key& key,const Json& value){
  const auto add=[&](auto&& v){if constexpr(std::is_same_v<Container,toml::table>)out.insert(key,std::forward<decltype(v)>(v));else out.push_back(std::forward<decltype(v)>(v));};
  if(value.is_object()){toml::table child;for(auto it=value.begin();it!=value.end();++it)Insert(child,it.key(),it.value());add(std::move(child));}
  else if(value.is_array()){toml::array child;for(const auto& element:value)Decode(child,0,element);add(std::move(child));}
  else if(value.is_string())add(value.get<std::string>());
  else if(value.is_boolean())add(value.get<bool>());
  else if(value.is_number_unsigned()){
    const auto number=value.get<uint64_t>();if(number>uint64_t(std::numeric_limits<int64_t>::max()))throw std::runtime_error("config.json: integer is too large");add(static_cast<int64_t>(number));
  }else if(value.is_number_integer())add(value.get<int64_t>());
  else if(value.is_number_float()){const auto number=value.get<double>();if(!std::isfinite(number))throw std::runtime_error("config.json: non-finite number");add(number);}
  else throw std::runtime_error("config.json: use a typed value instead of null");
}
void Insert(toml::table& table,const std::string& key,const Json& value){Decode(table,key,value);}
void WriteNew(const fs::path& path,std::string_view bytes){
#ifdef _WIN32
  const auto file=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(file==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot create configuration temporary file");
  DWORD written=0;const bool ok=bytes.size()<=MAXDWORD&&WriteFile(file,bytes.data(),static_cast<DWORD>(bytes.size()),&written,nullptr)&&written==bytes.size()&&FlushFileBuffers(file);CloseHandle(file);
#else
  const int file=open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL,0600);
  if(file<0)throw std::runtime_error("Cannot create configuration temporary file");
  const bool ok=write(file,bytes.data(),bytes.size())==static_cast<ssize_t>(bytes.size())&&fsync(file)==0;close(file);
#endif
  if(!ok){std::error_code error;fs::remove(path,error);throw std::runtime_error("Cannot write configuration temporary file");}
}
}
std::optional<std::string> Read(const std::filesystem::path& path){
  if(!std::filesystem::exists(path))return {};
  if(std::filesystem::file_size(path)>16*1024*1024)throw std::runtime_error("Configuration file exceeds 16 MiB");
  std::ifstream file(path,std::ios::binary);if(!file)throw std::runtime_error("Cannot read configuration file");
  std::string bytes((std::istreambuf_iterator<char>(file)),{});if(file.bad())throw std::runtime_error("Cannot read configuration file");return bytes;
}
toml::table Parse(std::string_view bytes,const std::filesystem::path& path){
  if(path.extension()!=".json")return toml::parse(bytes);
  std::vector<std::set<std::string>> objects;
  auto parsed=Json::parse(bytes,[&](int depth,Json::parse_event_t event,Json& value){
    if(depth>64)throw std::runtime_error("config.json: excessive nesting");
    if(event==Json::parse_event_t::object_start)objects.emplace_back();
    else if(event==Json::parse_event_t::key){if(!objects.back().insert(value.get<std::string>()).second)throw std::runtime_error("config.json: duplicate property");}
    else if(event==Json::parse_event_t::object_end)objects.pop_back();return true;
  });
  if(!parsed.is_object())throw std::runtime_error("config.json must contain a JSON object");
  if(parsed.contains("launcher")&&!parsed["launcher"].is_object())throw std::runtime_error("config.json: launcher must be a JSON object");
  toml::table result;for(auto it=parsed.begin();it!=parsed.end();++it)Insert(result,it.key(),it.value());return result;
}
toml::table Load(const std::filesystem::path& path){auto bytes=Read(path);return bytes?Parse(*bytes,path):toml::table{};}
const toml::node* Find(const toml::table& table,std::string_view key,std::string prefix){
  for(const auto& [name,value]:table){auto full=prefix.empty()?std::string(name.str()):prefix+"_"+std::string(name.str());
    if(full==key&&!value.is_table())return &value;
    if(value.is_table())if(auto* result=Find(*value.as_table(),key,full))return result;
  }return nullptr;
}
std::string Serialize(const toml::table& table,const std::filesystem::path& path){
  std::ostringstream stream;
  if(path.extension()==".json")stream<<toml::json_formatter{table};else stream<<toml::toml_formatter{table};
  return stream.str()+'\n';
}
std::filesystem::path Ensure(const std::filesystem::path& root){
  const auto path=root/"config.json";
  if(std::filesystem::exists(path)){Load(path);return path;}
  const auto game=root/"motogp0910.toml",launcher=root/"motogp0910-launcher.toml";
  auto table=Load(game);auto preferences=Load(launcher);
  if(!table.contains("launcher"))table.insert("launcher",std::move(preferences));
  else if(auto* nested=table["launcher"].as_table()){
    for(auto& [key,node]:preferences)if(!nested->contains(key))nested->insert(key,node);
  }
  else throw std::runtime_error("Legacy launcher settings must be a table");
  auto temporary=path;temporary+=".migrate.tmp";
  WriteNew(temporary,Serialize(table,path));
  std::error_code error;
#ifdef _WIN32
  const bool installed=MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_WRITE_THROUGH)!=FALSE;
#else
  std::filesystem::create_hard_link(temporary,path,error);const bool installed=!error;std::filesystem::remove(temporary,error);
#endif
  if(!installed){std::filesystem::remove(temporary,error);if(!std::filesystem::exists(path))throw std::runtime_error("Cannot create config.json; original settings preserved");Load(path);return path;}
  for(const auto& old:{game,launcher})if(std::filesystem::exists(old)){
    auto backup=old;backup+=".legacy.bak";
    if(!std::filesystem::exists(backup))std::filesystem::rename(old,backup,error);
  }
  return path;
}
std::filesystem::path SdkBridge::Prepare(toml::table table){
  Clear();table.erase("launcher");
  std::random_device random;
  auto path=std::filesystem::temp_directory_path()/("motogp-config-"+std::to_string(random())+"-"+std::to_string(random())+".tmp");
  WriteNew(path,Serialize(table,path));path_=std::move(path);return path_;
}
void SdkBridge::Clear()noexcept{std::error_code error;if(!path_.empty())std::filesystem::remove(path_,error);path_.clear();}
}
