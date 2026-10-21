// Steam integration adapted from ChunchunQ/fifa09-recomp (MIT).
// See third_party/fifa09-launcher-LICENSE.txt and embedded launcher notices.
#pragma once
#include "core.h"
#include "steam_vdf.h"
#include <tlhelp32.h>
#include <shlwapi.h>
#include <gdiplus.h>
#include <chrono>
#include <array>
#include <map>
#include <memory>
#include <cwchar>
#include <thread>

namespace launcher::steam {
inline fs::path RegistryPath(HKEY hive,const wchar_t* key,const wchar_t* value){
 DWORD bytes=0;
 if(RegGetValueW(hive,key,value,RRF_RT_REG_SZ,nullptr,nullptr,&bytes)!=ERROR_SUCCESS || bytes>65536)return {};
 std::wstring text(bytes/sizeof(wchar_t),L'\0');
 if(RegGetValueW(hive,key,value,RRF_RT_REG_SZ,nullptr,text.data(),&bytes)!=ERROR_SUCCESS)return {};
 text.resize(wcsnlen(text.data(),text.size()));return text;
}
inline fs::path FindSteam(){
 for(const auto& path:{RegistryPath(HKEY_CURRENT_USER,L"Software\\Valve\\Steam",L"SteamPath"),
  RegistryPath(HKEY_LOCAL_MACHINE,L"SOFTWARE\\WOW6432Node\\Valve\\Steam",L"InstallPath"),
  RegistryPath(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Valve\\Steam",L"InstallPath")})
  if(!path.empty()&&fs::is_regular_file(path/L"steam.exe"))return fs::absolute(path);
 return {};
}
inline bool Running(){
 const auto snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
 if(snapshot==INVALID_HANDLE_VALUE)throw Error(L"Could not check whether Steam is running.",L"Steam'in çalışıp çalışmadığı kontrol edilemedi.");
 PROCESSENTRY32W process{};process.dwSize=sizeof(process);bool found=false;
 if(Process32FirstW(snapshot,&process))do{if(_wcsicmp(process.szExeFile,L"steam.exe")==0){found=true;break;}}while(Process32NextW(snapshot,&process));
 CloseHandle(snapshot);return found;
}
inline bool Launch(const fs::path& steam,const wchar_t* options){
 const auto executable=steam/L"steam.exe";std::wstring command=L"\""+executable.wstring()+L"\" "+options;
 STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION process{};
 if(!CreateProcessW(executable.c_str(),command.data(),nullptr,nullptr,FALSE,0,nullptr,steam.c_str(),&startup,&process))return false;
 CloseHandle(process.hThread);CloseHandle(process.hProcess);return true;
}
class LoginUsers {
 std::string bytes;size_t at=0;
 void Space(){
  while(at<bytes.size()){
   if(bytes[at]==' '||bytes[at]=='\t'||bytes[at]=='\r'||bytes[at]=='\n'){++at;continue;}
   if(bytes.compare(at,2,"//")==0){while(at<bytes.size()&&bytes[at]!='\n')++at;continue;}break;
  }
 }
 std::string Token(){
  Space();if(at>=bytes.size())throw std::runtime_error("Invalid Steam loginusers file");
  if(bytes[at]!='\"')throw std::runtime_error("Invalid Steam loginusers token");
  ++at;std::string out;
  while(at<bytes.size()){
   char c=bytes[at++];if(c=='\"')return out;
   if(c=='\\'){if(at>=bytes.size())break;c=bytes[at++];}
   out+=c;
  }
  throw std::runtime_error("Truncated Steam loginusers token");
 }
 std::vector<Value> Object(unsigned depth,bool nested){
  if(depth>16)throw std::runtime_error("Invalid Steam loginusers nesting");
  std::vector<Value> out;
  for(;;){
   Space();if(at==bytes.size()){if(nested)throw std::runtime_error("Truncated Steam loginusers object");return out;}
   if(bytes[at]=='}'){if(!nested)throw std::runtime_error("Invalid Steam loginusers object");++at;return out;}
   auto key=Token();Space();
   if(at<bytes.size()&&bytes[at]=='{'){++at;Value value{0,std::move(key)};value.children=Object(depth+1,true);out.push_back(std::move(value));}
   else SetText(out,std::move(key),Token());
  }
 }
 public:
 explicit LoginUsers(std::string value):bytes(std::move(value)){if(bytes.size()>4*1024*1024)throw std::runtime_error("Steam loginusers file is too large");}
 std::vector<Value> Read(){return Object(0,false);}
};
inline std::vector<uint8_t> ReadBinary(const fs::path& path){
 if(!fs::exists(path))return {};
 if(!fs::is_regular_file(path)||fs::file_size(path)>4*1024*1024)throw Error(L"Steam's shortcuts file cannot be read safely.",L"Steam kısayol dosyası okunamıyor.");
 std::ifstream file(path,std::ios::binary);if(!file)throw Error(L"Could not read Steam's shortcuts file.",L"Steam kısayol dosyası okunamadı.");
 std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)),{});
 if(file.bad())throw std::runtime_error("Steam shortcuts read failed");return bytes;
}
inline fs::path UserDirectory(const fs::path& steam){
 const auto users=steam/L"userdata";
 if(!fs::is_directory(users))return {};
 DWORD active=0,size=sizeof(active);
 if(RegGetValueW(HKEY_CURRENT_USER,L"Software\\Valve\\Steam\\ActiveProcess",L"ActiveUser",RRF_RT_REG_DWORD,nullptr,&active,&size)==ERROR_SUCCESS && active && fs::is_directory(users/std::to_wstring(active)))return users/std::to_wstring(active);
 const auto login=steam/L"config"/L"loginusers.vdf";
 if(fs::is_regular_file(login)){
  try{
   const auto data=ReadBinary(login);auto root=LoginUsers(std::string(data.begin(),data.end())).Read();
   if(auto* list=Find(root,"users"))for(auto& user:list->children){
    const auto* recent=Find(user.children,"MostRecent");
    if(!recent||Text(*recent)!="1"||user.key.size()!=17||!std::all_of(user.key.begin(),user.key.end(),[](auto c){return c>='0'&&c<='9';}))continue;
    const auto id=std::stoull(user.key);constexpr uint64_t base=76561197960265728ull;
    if(id>base && id-base<=UINT32_MAX && fs::is_directory(users/std::to_wstring(id-base)))return users/std::to_wstring(id-base);
   }
  }catch(const std::exception&){/* A single unambiguous userdata folder is still usable. */}
 }
 fs::path only;
 for(const auto& folder:fs::directory_iterator(users)){
  const auto name=folder.path().filename().wstring();
  if(!folder.is_directory()||name.empty()||name==L"0"||!std::all_of(name.begin(),name.end(),[](auto c){return c>=L'0'&&c<=L'9';}))continue;
  if(!only.empty())return {};only=folder.path();
 }
 return only;
}
inline void CommitShortcuts(const fs::path& path,std::span<const uint8_t> before,std::span<const uint8_t> after){
 const auto suffix=std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());
 const fs::path temporary(path.wstring()+L".motogp-"+suffix+L".tmp"),backup(path.wstring()+L".motogp-"+suffix+L".bak");
 HANDLE file=CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_WRITE_THROUGH,nullptr);
 if(file==INVALID_HANDLE_VALUE)throw Error(L"Could not write Steam's shortcuts file.",L"Steam kısayol dosyası yazılamadı.");
 DWORD written=0;const bool ok=WriteFile(file,after.data(),static_cast<DWORD>(after.size()),&written,nullptr)&&written==after.size()&&FlushFileBuffers(file);CloseHandle(file);
 struct Cleanup{fs::path path;~Cleanup(){std::error_code ec;fs::remove(path,ec);}} cleanup{temporary};
 if(!ok)throw std::runtime_error("Steam shortcuts write failed");
 if(Running()||ReadBinary(path)!=std::vector<uint8_t>(before.begin(),before.end()))throw Error(L"Steam or another app changed its shortcuts. Close Steam and try again.",L"Steam veya başka bir uygulama kısayolları değiştirdi. Steam'i kapatıp yeniden dene.");
 const bool replaced=fs::exists(path)?ReplaceFileW(path.c_str(),temporary.c_str(),backup.c_str(),0,nullptr,nullptr)!=FALSE:
  MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_WRITE_THROUGH)!=FALSE;
 if(!replaced){
  // ReplaceFile may have moved the original to the backup before failing.
  // Restore only a missing destination; never replace somebody else's change.
  if(!fs::exists(path)&&fs::exists(backup)&&!MoveFileExW(backup.c_str(),path.c_str(),MOVEFILE_WRITE_THROUGH))
   throw Error(L"Steam shortcuts could not be restored automatically. Your original file is backed up at: "+backup.wstring(),L"Steam kısayolları otomatik geri yüklenemedi. Özgün dosyanın yedeği: "+backup.wstring());
  throw Error(L"Could not update Steam's shortcuts. Your existing file was kept.",L"Steam kısayolları güncellenemedi. Mevcut dosyan korundu.");
 }
}
// All Steam artwork is read from editable loose files. Never crop, render or
// fall back to the embedded launcher background or executable icon.
inline constexpr std::array<const wchar_t*,5> kArtworkFiles{
 L"icon.png",L"logo.png",L"banner.png",L"cover.png",L"wide.png"};
inline fs::path ArtworkFolder(const fs::path& root){return root/L"resources"/L"steam";}
inline void RequireArtwork(const fs::path& folder){
 for(const auto* name:kArtworkFiles){
  const auto path=folder/name;
  if(!fs::exists(path)&&std::wstring_view(name)!=L"icon.png")continue;
  if(!fs::is_regular_file(path)||!fs::file_size(path)||fs::file_size(path)>32*1024*1024)
   throw Error(L"Steam artwork is missing or invalid. Keep icon.png, logo.png, banner.png, cover.png and wide.png in resources/steam.",
       L"Steam görselleri eksik veya geçersiz. icon.png, logo.png, banner.png, cover.png ve wide.png dosyalarını resources/steam klasöründe tut.");
 }
}
inline bool Artwork(const fs::path& grid,const fs::path& folder,uint32_t id){
 try{
  RequireArtwork(folder);fs::create_directories(grid);const auto prefix=std::to_wstring(id);
  const std::array<std::pair<const wchar_t*,std::wstring>,5> art{{
   {L"icon.png",prefix+L"_icon.png"},{L"logo.png",prefix+L"_logo.png"},
   {L"banner.png",prefix+L"_hero.png"},{L"cover.png",prefix+L"p.png"},{L"wide.png",prefix+L".png"}}};
  bool complete=true;
  for(const auto& [source,destination]:art){
   if(!fs::is_regular_file(folder/source)){complete=false;continue;}
   const auto target=grid/destination;
   const auto temp=grid/(destination+L".motogp-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64())+L".tmp");
   struct Cleanup{fs::path path;~Cleanup(){std::error_code ec;fs::remove(path,ec);}} cleanup{temp};
   fs::copy_file(folder/source,temp);
   // An explicit Add to Steam refreshes this shortcut's five pictures so edits
   // in resources/steam take effect. Other games' artwork is never touched.
   if(!MoveFileExW(temp.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return false;
  }
  return complete;
 }catch(const std::exception&){return false;}
}
struct Result {bool artwork=false,restarted=true;};
inline Result Add(const fs::path& root,const fs::path& steam_path,bool restart){
 RequireArtwork(ArtworkFolder(root));
 const auto user=UserDirectory(steam_path);
 if(user.empty())throw Error(L"No active Steam account was found. Sign in to Steam once, then try again.",L"Etkin Steam hesabı bulunamadı. Steam'de bir kez oturum açıp yeniden dene.");
 struct Restart{fs::path path;bool enabled;~Restart(){if(enabled)try{Launch(path,L"");}catch(...){}}} restart_on_error{steam_path,false};
 if(Running()){
  if(!restart)throw Error(L"Close Steam first, then try again.",L"Önce Steam'i kapatıp yeniden dene.");
  if(!Launch(steam_path,L"-shutdown"))throw Error(L"Could not close Steam.",L"Steam kapatılamadı.");
  restart_on_error.enabled=true;
  const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(20);
  while(Running()&&std::chrono::steady_clock::now()<deadline)std::this_thread::sleep_for(std::chrono::milliseconds(250));
  if(Running())throw Error(L"Steam did not close. Exit it from the tray icon and try again.",L"Steam kapanmadı. Bildirim alanındaki simgesinden çıkış yapıp yeniden dene.");
 }
 const auto launcher=root/L"MotoGP09-10Launcher.exe";
 if(!fs::is_regular_file(launcher))throw Error(L"Keep MotoGP09-10Launcher.exe beside the bin folder.",L"MotoGP09-10Launcher.exe dosyasını bin klasörünün yanında tut.");
 const auto config=user/L"config";fs::create_directories(config);const auto path=config/L"shortcuts.vdf";
 const auto before=ReadBinary(path);const auto exe="\""+Utf8(fs::absolute(launcher).wstring())+"\"";
 const auto after=AddShortcut(before,exe,"\""+Utf8(fs::absolute(root).wstring())+"\"",Utf8((ArtworkFolder(root)/L"icon.png").wstring()));
 CommitShortcuts(path,before,after);
 Result result;result.artwork=Artwork(config/L"grid",ArtworkFolder(root),ShortcutId(exe,kName));
 if(restart_on_error.enabled){result.restarted=Launch(steam_path,L"");restart_on_error.enabled=false;}
 return result;
}
}
