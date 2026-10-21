#pragma once
#include "core.h"
#include "iso_import.h"
#include "dlc_packages.h"
#include "update_protocol.h"
#include <map>
#include <functional>
#include <cwctype>

namespace launcher::dlc {
using Cancel=const std::atomic<bool>&;
inline void Check(Cancel cancel){iso::CheckCancel(cancel);}
inline void SafeArchivePath(std::string_view path){
  if(path.empty()||path.size()>2048||path.front()=='/'||path.front()=='\\')throw std::runtime_error("Invalid DLC archive path");
  for(unsigned char c:path)if(c<32||c==127||c==':')throw std::runtime_error("Invalid DLC archive path");
  std::string component;
  for(size_t i=0;i<=path.size();++i){
    if(i==path.size()||path[i]=='/'||path[i]=='\\'){
      if(component==".."||component==".")throw std::runtime_error("Invalid DLC archive path");component.clear();
    }else component+=path[i];
  }
}
inline fs::path ArchiveTool(const fs::path& app){
  const auto folder=app/L"resources"/L"archive";
  for(auto [name,hash]:{std::pair{L"7z.exe","95fff19b73eff77c87c0d7bd32822e43fb90dd2932459f962eb532d8dfb6ba53"},
      std::pair{L"7z.dll","899cd73d7fc60e1c1c35c7de0f5bed3dc69bfe2cecbd48014d868a347970a0b1"}}){
    if(!fs::is_regular_file(folder/name)||Sha256(folder/name)!=hash)
      throw Error(L"The archive reader is missing or invalid. Extract the complete launcher package.",L"Arşiv okuyucusu eksik veya geçersiz. Launcher paketini eksiksiz çıkar.");
  }
  return folder/L"7z.exe";
}
inline std::string ToolOutput(const fs::path& file){
  if(!fs::is_regular_file(file)||fs::file_size(file)>8*1024*1024)throw std::runtime_error("Invalid archive reader output");
  std::ifstream input(file,std::ios::binary);std::string text((std::istreambuf_iterator<char>(input)),{});
  if(input.bad())throw std::runtime_error("Could not read archive listing");return text;
}
inline void RunArchiveTool(const fs::path& tool,const std::vector<std::wstring>& args,const fs::path& stage,
    const fs::path& log,Cancel cancel){
  using updates::detail::Handle;using updates::detail::Quote;
  SECURITY_ATTRIBUTES security{sizeof(security),nullptr,TRUE};
  Handle output(CreateFileW(log.c_str(),GENERIC_WRITE,FILE_SHARE_READ,&security,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr));
  Handle input(CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,OPEN_EXISTING,0,nullptr));
  if(!output.value||output.value==INVALID_HANDLE_VALUE||!input.value||input.value==INVALID_HANDLE_VALUE)
    throw std::runtime_error("Could not prepare archive reader output");
  auto command=Quote(tool.wstring());for(const auto& arg:args)command+=L" "+Quote(arg);
  STARTUPINFOW start{};start.cb=sizeof(start);start.dwFlags=STARTF_USESTDHANDLES;
  start.hStdOutput=output.value;start.hStdError=output.value;start.hStdInput=input.value;
  PROCESS_INFORMATION process{};
  if(!CreateProcessW(tool.c_str(),command.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW,nullptr,stage.c_str(),&start,&process))
    throw Error(L"Could not start the archive reader.",L"Arşiv okuyucusu başlatılamadı.");
  CloseHandle(process.hThread);
  struct Child {HANDLE handle;~Child(){if(WaitForSingleObject(handle,0)==WAIT_TIMEOUT){TerminateProcess(handle,1);WaitForSingleObject(handle,5000);}CloseHandle(handle);}} child{process.hProcess};
  for(;;){
    Check(cancel);const auto result=WaitForSingleObject(child.handle,100);
    if(result==WAIT_OBJECT_0)break;
    if(result!=WAIT_TIMEOUT)throw std::runtime_error("Archive reader wait failed");
    LARGE_INTEGER bytes{};if(!GetFileSizeEx(output.value,&bytes)||bytes.QuadPart>8*1024*1024)throw std::runtime_error("Archive listing is too large");
  }
  DWORD code=1;if(!GetExitCodeProcess(child.handle,&code)||code!=0)
    throw Error(L"The archive could not be read. Use a complete, unencrypted ZIP, RAR or 7z archive.",L"Arşiv okunamadı. Eksiksiz ve şifresiz bir ZIP, RAR veya 7z arşivi kullan.");
}
struct ArchiveItem {const Package* package;std::string path;};
inline std::vector<ArchiveItem> ParseArchiveListing(std::string_view listing){
  const auto marker=listing.find("\n----------\n"),windows_marker=listing.find("\n----------\r\n");
  const auto start=marker!=std::string_view::npos?marker+12:windows_marker!=std::string_view::npos?windows_marker+13:std::string_view::npos;
  if(start==std::string_view::npos)throw std::runtime_error("Unsupported archive listing");
  std::istringstream input{std::string(listing.substr(start))};std::string line;
  std::map<std::string,std::string> row;std::set<std::string> ids;std::vector<ArchiveItem> result;size_t entries=0;
  const auto flush=[&]{
    if(row.empty())return;if(++entries>1024)throw std::runtime_error("DLC archive has too many entries");
    if(!row.contains("Path"))throw std::runtime_error("Invalid archive entry");const auto& path=row.at("Path");SafeArchivePath(path);
    for(const auto* key:{"Symbolic Link","Hard Link","Link"})if(row.contains(key)&&!row.at(key).empty())throw std::runtime_error("DLC archives cannot contain links");
    if(row["Encrypted"]=="+")throw std::runtime_error("Encrypted DLC archives are unsupported");
    const auto& attributes=row["Attributes"];
    if(row["Folder"]=="+"||(!attributes.empty()&&attributes[0]=='D')){row.clear();return;}
    const auto slash=path.find_last_of("/\\");const auto name=path.substr(slash==std::string::npos?0:slash+1);
    const auto* package=ByName(name);
    if(!package||row["Size"]!=std::to_string(package->bytes)||!ids.insert(package->id).second)
      throw Error(L"The archive must contain only the supported MotoGP 09/10 DLC packages, without duplicates.",L"Arşiv yalnızca desteklenen MotoGP 09/10 DLC paketlerini içermeli; aynı paketten birden fazla olmamalı.");
    result.push_back({package,path});row.clear();
  };
  while(std::getline(input,line)){
    if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty()){flush();continue;}
    const auto equal=line.find(" = ");if(equal==std::string::npos)continue;
    if(!row.emplace(line.substr(0,equal),line.substr(equal+3)).second)throw std::runtime_error("Ambiguous archive listing");
  }
  flush();if(result.empty()||result.size()>3)throw Error(L"No supported MotoGP 09/10 DLC package was found.",L"Desteklenen bir MotoGP 09/10 DLC paketi bulunamadı.");return result;
}
inline std::vector<ArchiveItem> UnpackArchive(const fs::path& source,const fs::path& app,const fs::path& stage,Cancel cancel){
  const auto tool=ArchiveTool(app),listing=stage/L"listing.txt",output=stage/L"packages";fs::create_directory(output);
  auto extension=source.extension().wstring();for(auto& c:extension)c=static_cast<wchar_t>(std::towlower(c));
  const auto type=extension==L".zip"?L"-tzip":extension==L".rar"?L"-trar":extension==L".7z"?L"-t7z":nullptr;
  if(!type)throw Error(L"Select a ZIP, RAR or 7z archive, or choose the extracted folder.",L"Bir ZIP, RAR veya 7z arşivi seç ya da çıkarılmış klasörü kullan.");
  iso::Handle lock(CreateFileW(source.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));
  if(lock.value==INVALID_HANDLE_VALUE)throw std::runtime_error("Could not lock the selected archive for reading");
  RunArchiveTool(tool,{L"l",type,L"-slt",L"-sccUTF-8",L"-p-",L"--",source.wstring()},stage,listing,cancel);
  auto entries=ParseArchiveListing(ToolOutput(listing));uint64_t required=96*1024*1024;
  for(const auto& item:entries)required+=item.package->bytes+item.package->payload_bytes;
  if(fs::space(stage).available<required)throw Error(L"There is not enough free space to import the DLC archive.",L"DLC arşivini kurmak için yeterli boş alan yok.");
  const auto list=stage/L"files.txt";{std::ofstream f(list,std::ios::binary);for(const auto& item:entries)f<<item.path<<'\n';f.close();if(!f)throw std::runtime_error("Could not prepare archive file list");}
  RunArchiveTool(tool,{L"e",type,L"-y",L"-bd",L"-bso0",L"-bsp0",L"-sccUTF-8",L"-scsUTF-8",L"-spd",L"-p-",L"-i@"+list.wstring(),L"-o"+output.wstring(),L"--",source.wstring()},stage,stage/L"extract.txt",cancel);
  for(const auto& entry:entries)if(!fs::is_regular_file(output/entry.package->id)||fs::file_size(output/entry.package->id)!=entry.package->bytes)throw std::runtime_error("DLC archive extraction was incomplete");
  return entries;
}
}
