#include "../src/config_file.h"
#include <rex/cvar.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
REXCVAR_DEFINE_INT32(config_probe_integer,1,"Test","JSON adapter precedence regression");
REXCVAR_DEFINE_BOOL(config_probe_boolean,true,"Test","JSON adapter type regression");
namespace fs=std::filesystem;
int main(){try{
  unsigned checks=0;const auto check=[&](bool value,const char* message){if(!value)throw std::runtime_error(message);++checks;};
  const auto write=[](const fs::path& path,std::string_view value){std::ofstream f(path,std::ios::binary);f<<value;if(!f)throw std::runtime_error("Cannot write test fixture");};
  const auto root=fs::current_path()/fs::path(u8"config-probe Türkçe");check(!fs::exists(root),"A fresh configuration fixture is required");fs::create_directory(root);
  const std::string legacy="# Exact original backup\nconfig_probe_integer=42\nconfig_probe_boolean=false\nuser_data_root='Saved Games Türkçe'\nunknown=[1,2,3]\nlarge_integer=9007199254740993\n[window]\nwidth=1920\nheight=1080\n";
  const std::string preferences="language='tr'\ncrash_monitor=false\nfuture_preference='keep'\n";
  write(root/"motogp0910.toml",legacy);write(root/"motogp0910-launcher.toml",preferences);
  const auto path=motogp::config::Ensure(root);const auto original=motogp::config::Read(path);
  check(path==root/"config.json"&&original&&original->find('{')==0,"Migration creates an actual JSON object in config.json");
  check(!fs::exists(root/"motogp0910.toml")&&!fs::exists(root/"motogp0910-launcher.toml"),"Legacy active configs are replaced by one canonical file");
  check(motogp::config::Read(root/"motogp0910.toml.legacy.bak")==legacy&&motogp::config::Read(root/"motogp0910-launcher.toml.legacy.bak")==preferences,"Both legacy backups preserve exact original bytes");
  auto table=motogp::config::Load(path);
  check(table["window"]["width"].value_or(0)==1920&&table["unknown"].as_array()->size()==3,"Nested game settings and unknown arrays survive migration");
  check(table["launcher"]["language"].value_or(std::string{})=="tr"&&!table["launcher"]["crash_monitor"].value_or(true)&&table["launcher"]["future_preference"].value_or(std::string{})=="keep","Launcher preferences share the JSON file without losing unknown keys");
  check(table["large_integer"].value_or(int64_t{})==9007199254740993LL,"JSON preserves integer precision beyond JavaScript doubles");
  check(table["user_data_root"].value_or(std::string{})=="Saved Games Türkçe","Unicode save paths survive JSON encoding");
  check(motogp::config::Ensure(root)==path&&motogp::config::Read(path)==original,"Repeated startup does not rewrite migrated settings");
  rex::cvar::SetFlagFromCommandLine("config_probe_integer","77");
  motogp::config::SdkBridge bridge;const auto temporary=bridge.Prepare(table);
  check(fs::is_regular_file(temporary)&&temporary.parent_path()!=root&&!motogp::config::Load(temporary).contains("launcher"),"SDK adapter is private and excludes launcher-only preferences");
  rex::cvar::LoadConfig(temporary);
  check(rex::cvar::Query<int32_t>("config_probe_integer")==77&&rex::cvar::GetFlagSource("config_probe_integer")==rex::cvar::Source::kCommandLine,"JSON settings preserve real SDK command-line priority");
  check(!rex::cvar::Query<bool>("config_probe_boolean")&&rex::cvar::GetFlagSource("config_probe_boolean")==rex::cvar::Source::kConfig,"SDK receives typed JSON settings with config priority");
  bridge.Clear();check(!fs::exists(temporary),"SDK adapter file is removed after loading");
  auto roundtrip=motogp::config::Parse(motogp::config::Serialize(table,path),path);
  check(roundtrip==table,"Complete JSON table survives serialization and parsing");
  for(const auto* invalid:{"[]","{\"launcher\":false}","{\"a\":1,\"a\":2}","{\"a\":{\"b\":1,\"b\":2}}","{\"a\":null}","{\"a\":18446744073709551615}","{\"a\":1,}"}){
    bool rejected=false;try{motogp::config::Parse(invalid,path);}catch(const std::exception&){rejected=true;}check(rejected,"Malformed or unsupported JSON values are rejected instead of silently rewritten");
  }
  write(path,"{ broken JSON");bool rejected=false;try{motogp::config::Ensure(root);}catch(const std::exception&){rejected=true;}
  check(rejected&&motogp::config::Read(path)=="{ broken JSON","Invalid canonical JSON is preserved and never replaced with defaults or legacy values");
  std::cout<<checks<<" shared JSON config, migration, preservation and native SDK priority checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
