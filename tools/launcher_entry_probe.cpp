// Loaded through the production public entry point to verify the UI DLL ABI,
// Unicode arguments, working directory and return status without a child EXE.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <fstream>
#include <filesystem>
#include <string>

namespace {
std::string Utf8(const wchar_t* value) {
  const int size=WideCharToMultiByte(CP_UTF8,0,value,-1,nullptr,0,nullptr,nullptr);
  if(size<=0)return {};
  std::string bytes(size,'\0');
  WideCharToMultiByte(CP_UTF8,0,value,-1,bytes.data(),size,nullptr,nullptr);
  bytes.pop_back();return bytes;
}
}
extern "C" __declspec(dllexport) int WINAPI MotoGPLauncherMain(HINSTANCE instance,HINSTANCE,PWSTR,int) {
  if(instance!=GetModuleHandleW(nullptr))return 19;
  int count=0;auto** args=CommandLineToArgvW(GetCommandLineW(),&count);
  if(!args)return 20;
  std::ofstream arguments("launch-arguments.txt",std::ios::binary);
  for(int i=1;i<count;++i)arguments<<Utf8(args[i])<<'\n';
  LocalFree(args);
  std::ofstream cwd("launch-cwd.txt",std::ios::binary);
  cwd<<Utf8(std::filesystem::current_path().c_str());
  return arguments&&cwd?17:21;
}
