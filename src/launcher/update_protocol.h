#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cwctype>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace launcher::updates::detail {
namespace fs = std::filesystem;
inline constexpr wchar_t kLauncher[] = L"MotoGP09-10Launcher.exe";
inline constexpr wchar_t kHelper[] = L"motogp0910-update-helper.exe";
struct Handle {
  HANDLE value{};
  explicit Handle(HANDLE handle = nullptr) : value(handle) {}
  ~Handle(){if(value&&value!=INVALID_HANDLE_VALUE)CloseHandle(value);}
  Handle(const Handle&) = delete;
  Handle& operator=(const Handle&) = delete;
  Handle(Handle&& other) noexcept : value(std::exchange(other.value,nullptr)) {}
};
inline std::wstring WideText(std::string_view value){
  if(value.empty())return {};
  int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0);
  if(!count)throw std::runtime_error("Invalid UTF-8 update text");
  std::wstring result(count,L'\0');MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),result.data(),count);return result;
}
inline std::string Utf8Text(std::wstring_view value){
  if(value.empty())return {};
  int count=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0,nullptr,nullptr);
  if(!count)throw std::runtime_error("Invalid UTF-16 update text");
  std::string result(count,'\0');WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),result.data(),count,nullptr,nullptr);return result;
}
inline std::wstring Fold(std::wstring text){for(auto& c:text)c=static_cast<wchar_t>(std::towlower(c));return text;}
inline std::wstring PathKey(const fs::path& path){return Fold(fs::weakly_canonical(fs::absolute(path)).wstring());}
inline bool Token(std::wstring_view token){return token.size()==32&&std::all_of(token.begin(),token.end(),[](wchar_t c){return (c>=L'0'&&c<=L'9')||(c>=L'a'&&c<=L'f');});}
inline std::wstring EventName(std::wstring_view token,std::wstring_view suffix){return L"Local\\MotoGP0910Update-"+std::wstring(token)+L"-"+std::wstring(suffix);}
inline std::wstring LockName(const fs::path& root){
  uint64_t hash=14695981039346656037ull;
  for(auto c:PathKey(root)){hash^=static_cast<uint16_t>(c);hash*=1099511628211ull;}
  return L"Local\\MotoGP0910UpdateLock-"+std::to_wstring(hash);
}
inline std::wstring AckToken(){
  int count{};auto arguments=CommandLineToArgvW(GetCommandLineW(),&count);std::wstring token;
  if(arguments){for(int i=1;i+1<count;++i)if(std::wstring_view(arguments[i])==L"--update-ack"&&Token(arguments[i+1])){token=arguments[i+1];break;}LocalFree(arguments);}
  return token;
}
inline std::wstring Quote(std::wstring_view value){
  // CommandLineToArgvW / CRT argument quoting, including trailing backslashes.
  std::wstring result=L"\"";size_t slashes=0;
  for(auto c:value){if(c==L'\\'){++slashes;continue;}if(c==L'\"'){result.append(slashes*2+1,L'\\');result+=c;}else{result.append(slashes,L'\\');result+=c;}slashes=0;}
  result.append(slashes*2,L'\\');return result+L"\"";
}
inline Handle Spawn(const fs::path& program,std::wstring arguments,const fs::path& cwd,bool hidden=true){
  auto command=Quote(program.wstring())+L" "+arguments;STARTUPINFOW startup{sizeof(startup)};PROCESS_INFORMATION process{};
  if(!CreateProcessW(program.c_str(),command.data(),nullptr,nullptr,FALSE,hidden?CREATE_NO_WINDOW:0,nullptr,cwd.c_str(),&startup,&process))throw std::runtime_error("Could not start update process");
  CloseHandle(process.hThread);return Handle(process.hProcess);
}
inline bool RunningGame(const fs::path& root){
  Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0));
  if(snapshot.value==INVALID_HANDLE_VALUE)return true;
  PROCESSENTRY32W entry{};entry.dwSize=sizeof(entry);
  if(!Process32FirstW(snapshot.value,&entry))return true;
  const auto expected=PathKey(root/L"bin"/L"motogp0910.exe"),legacy=PathKey(root/L"motogp0910.exe");
  do{
    if(_wcsicmp(entry.szExeFile,L"motogp0910.exe"))continue;
    Handle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,entry.th32ProcessID));
    std::array<wchar_t,32768> buffer{};DWORD size=static_cast<DWORD>(buffer.size());
    if(!process.value||!QueryFullProcessImageNameW(process.value,0,buffer.data(),&size))return true;
    auto key=PathKey(std::wstring(buffer.data(),size));if(key==expected||key==legacy)return true;
  }while(Process32NextW(snapshot.value,&entry));return false;
}
inline std::string FileHash(const fs::path& path){
  BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};std::array<unsigned char,32> digest{};
  if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("Cannot initialize update checksum");
  if(BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)<0){BCryptCloseAlgorithmProvider(algorithm,0);throw std::runtime_error("Cannot create update checksum");}
  std::ifstream input(path,std::ios::binary);std::array<char,65536> buffer{};bool ok=static_cast<bool>(input);
  while(ok&&input){input.read(buffer.data(),buffer.size());if(input.gcount()&&BCryptHashData(hash,reinterpret_cast<PUCHAR>(buffer.data()),static_cast<ULONG>(input.gcount()),0)<0)ok=false;}
  ok=ok&&!input.bad()&&BCryptFinishHash(hash,digest.data(),static_cast<ULONG>(digest.size()),0)>=0;
  BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(algorithm,0);if(!ok)throw std::runtime_error("Cannot verify update file");
  std::string result;for(auto byte:digest){result+="0123456789abcdef"[byte>>4];result+="0123456789abcdef"[byte&15];}return result;
}
}  // namespace launcher::updates::detail
