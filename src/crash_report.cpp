#include "crash_report.h"
#include "portable_paths.h"
#ifdef _WIN32
#include <DbgHelp.h>
#include <TlHelp32.h>
#include <bcrypt.h>
#include <array>
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
namespace motogp::crash {
namespace {
namespace fs=std::filesystem;
std::atomic_flag reporting=ATOMIC_FLAG_INIT;
template<class T> bool Read(HANDLE process,uint64_t address,T& value){
  SIZE_T received=0;return address && ReadProcessMemory(process,reinterpret_cast<void*>(address),&value,sizeof(value),&received) && received==sizeof(value);
}
std::string ReadText(HANDLE process,uint64_t address,size_t maximum){
  std::string text;for(size_t i=0;i<maximum;++i){char c=0;if(!Read(process,address+i,c))return {};if(!c)return text;text+=c;}return {};
}
std::string CppDetail(HANDLE process,const EXCEPTION_RECORD& exception){
  // Inspect bounded MSVC x64 RTTI; never call methods in the failing process.
  if(exception.ExceptionCode!=0xE06D7363 || exception.NumberParameters<4 || exception.ExceptionInformation[0]!=0x19930520)return {};
  const auto base=exception.ExceptionInformation[3],object=exception.ExceptionInformation[1];
  std::array<uint32_t,4> info{};if(!Read(process,exception.ExceptionInformation[2],info))return {};
  int32_t count=0;const uint64_t array=base+info[3];if(!Read(process,array,count) || count<1 || count>32)return {};
  std::string result;
  for(int i=0;i<count;++i){uint32_t rva=0;std::array<int32_t,7> type{};
    if(!Read(process,array+4+i*4,rva) || !Read(process,base+rva,type))continue;
    const auto name=ReadText(process,base+static_cast<uint32_t>(type[1])+16,256);
    if(i==0 && !name.empty())result="MSVC C++ type: "+name;
    if(name==".?AVexception@std@@" && type[2]>=0 && type[2]<128 && type[3]==-1){
      uint64_t message=0;if(Read(process,object+type[2]+8,message)){const auto text=ReadText(process,message,2048);if(!text.empty())result+="\nC++ message: "+text;}
    }
  }
  return result;
}

fs::path Folder(){
  std::array<wchar_t,32768> path{};const auto length=GetModuleFileNameW(nullptr,path.data(),static_cast<DWORD>(path.size()));
  return length && length<path.size()?fs::path(std::wstring(path.data(),length)).parent_path():fs::current_path();
}
std::string Utf8(const std::wstring& value){
  const int n=WideCharToMultiByte(CP_UTF8,0,value.data(),static_cast<int>(value.size()),nullptr,0,nullptr,nullptr);
  std::string text(n,'\0');if(n)WideCharToMultiByte(CP_UTF8,0,value.data(),static_cast<int>(value.size()),text.data(),n,nullptr,nullptr);return text;
}
std::string Hash(const fs::path& file){
  BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};std::string result;
  if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return "unavailable";
  if(BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0){
    std::ifstream input(file,std::ios::binary);std::array<unsigned char,65536> bytes{};bool good=static_cast<bool>(input);
    while(good && input){input.read(reinterpret_cast<char*>(bytes.data()),bytes.size());if(input.gcount() && BCryptHashData(hash,bytes.data(),static_cast<ULONG>(input.gcount()),0)<0)good=false;}
    std::array<unsigned char,32> digest{};
    if(good && !input.bad() && BCryptFinishHash(hash,digest.data(),digest.size(),0)>=0){for(auto c:digest){result+="0123456789abcdef"[c>>4];result+="0123456789abcdef"[c&15];}}
    BCryptDestroyHash(hash);
  }
  BCryptCloseAlgorithmProvider(algorithm,0);return result.empty()?"unavailable":result;
}
fs::path ReportDirectory(DWORD pid,DWORD tid){
  SYSTEMTIME time{};GetSystemTime(&time);wchar_t name[160];
  swprintf_s(name,L"crash-%04u%02u%02u-%02u%02u%02u-%03u-%lu-%lu",time.wYear,time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond,time.wMilliseconds,pid,tid);
  fs::path root=portable::Root(Folder())/L"logs";std::error_code error;fs::create_directories(root,error);
  if(error){std::array<wchar_t,32768> local{};const auto n=GetEnvironmentVariableW(L"LOCALAPPDATA",local.data(),static_cast<DWORD>(local.size()));
    if(n && n<local.size())root=fs::path(std::wstring(local.data(),n))/L"MotoGP0910"/L"crashes";
    else {GetTempPathW(static_cast<DWORD>(local.size()),local.data());root=fs::path(local.data())/L"MotoGP0910"/L"crashes";}
    fs::create_directories(root);
  }
  auto folder=root/name;for(unsigned suffix=1;!fs::create_directory(folder);++suffix)folder=root/(std::wstring(name)+L"-"+std::to_wstring(suffix));return folder;
}
void CopyEvidence(const fs::path& folder,std::ostream& report){
  std::error_code error;const auto root=portable::Root(Folder());fs::path latest;fs::file_time_type time=fs::file_time_type::min();
  for(const auto& file:fs::directory_iterator(root/L"logs",error)){
    if(!file.is_regular_file(error) || file.path().extension()!=L".log")continue;
    const auto modified=file.last_write_time(error);if(!error && modified>=time){time=modified;latest=file.path();}
  }
  if(!latest.empty()){fs::copy_file(latest,folder/L"game.log",fs::copy_options::overwrite_existing,error);report<<"Game log: "<<(error?error.message():"game.log")<<'\n';}
  for(const auto* name:{L"config.json",L"runtime-manifest.json"}){
    const auto source=std::wstring_view(name)==L"runtime-manifest.json"?Folder()/name:root/name;
    if(!fs::is_regular_file(source,error))continue;fs::copy_file(source,folder/name,fs::copy_options::overwrite_existing,error);
    if(error)report<<"Evidence copy failed: "<<Utf8(name)<<" ("<<error.message()<<")\n";
  }
}
void Modules(DWORD pid,std::ostream& report){
  const auto snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);if(snapshot==INVALID_HANDLE_VALUE){report<<"Module snapshot unavailable: "<<GetLastError()<<'\n';return;}
  MODULEENTRY32W entry{sizeof(entry)};report<<"\nModules (base / size / path):\n";
  if(Module32FirstW(snapshot,&entry))do{report<<"0x"<<std::hex<<reinterpret_cast<uintptr_t>(entry.modBaseAddr)<<" 0x"<<entry.modBaseSize<<std::dec<<" "<<Utf8(entry.szExePath)<<'\n';}while(Module32NextW(snapshot,&entry));
  CloseHandle(snapshot);
}
void Stack(HANDLE process,DWORD tid,CONTEXT context,std::ostream& report){
  const auto thread=OpenThread(THREAD_GET_CONTEXT|THREAD_QUERY_INFORMATION,FALSE,tid);if(!thread){report<<"Stack thread unavailable: "<<GetLastError()<<'\n';return;}
  SymSetOptions(SYMOPT_DEFERRED_LOADS|SYMOPT_UNDNAME|SYMOPT_FAIL_CRITICAL_ERRORS|SYMOPT_NO_PROMPTS);
  const auto initialized=SymInitializeW(process,Folder().c_str(),TRUE);
  report<<"\nNative stack (module + offset; symbols when locally available):\n";
  STACKFRAME64 frame{};frame.AddrPC.Offset=context.Rip;frame.AddrStack.Offset=context.Rsp;frame.AddrFrame.Offset=context.Rbp;
  frame.AddrPC.Mode=frame.AddrStack.Mode=frame.AddrFrame.Mode=AddrModeFlat;
  DWORD64 previous=0;
  for(unsigned i=0;i<64;++i){
    const bool walked=initialized && StackWalk64(IMAGE_FILE_MACHINE_AMD64,process,thread,&frame,&context,nullptr,SymFunctionTableAccess64,SymGetModuleBase64,nullptr);
    if(!walked && i)break;
    if(!frame.AddrPC.Offset || frame.AddrPC.Offset==previous)break;
    previous=frame.AddrPC.Offset;report<<i<<": 0x"<<std::hex<<previous;
    IMAGEHLP_MODULE64 module{};module.SizeOfStruct=sizeof(module);
    if(initialized && SymGetModuleInfo64(process,previous,&module))report<<" "<<module.ModuleName<<"+0x"<<(previous-module.BaseOfImage);
    alignas(SYMBOL_INFO) std::array<char,sizeof(SYMBOL_INFO)+MAX_SYM_NAME> bytes{};auto* symbol=reinterpret_cast<SYMBOL_INFO*>(bytes.data());symbol->SizeOfStruct=sizeof(SYMBOL_INFO);symbol->MaxNameLen=MAX_SYM_NAME;DWORD64 displacement=0;
    if(initialized && SymFromAddr(process,previous,&displacement,symbol))report<<" "<<symbol->Name<<"+0x"<<displacement;
    report<<std::dec<<'\n';
    if(!walked)break;
  }
  if(initialized)SymCleanup(process);CloseHandle(thread);
}
LONG WINAPI Unhandled(EXCEPTION_POINTERS* exception){
  if(IsDebuggerPresent())return EXCEPTION_CONTINUE_SEARCH;
  Write(GetCurrentProcess(),GetCurrentProcessId(),GetCurrentThreadId(),exception->ExceptionRecord,exception->ContextRecord,"direct EXE / unhandled native exception");
  return EXCEPTION_EXECUTE_HANDLER;
}
[[noreturn]] void Terminate(){
  const char* detail="std::terminate: no active standard exception";char message[2048]{};
  try{if(auto exception=std::current_exception())std::rethrow_exception(exception);}catch(const std::exception& error){snprintf(message,sizeof(message),"Uncaught C++ exception: %s",error.what());detail=message;}catch(...){detail="Uncaught non-standard C++ exception";}
  if(IsDebuggerPresent())OutputDebugStringA(detail);
  else {CONTEXT context{};RtlCaptureContext(&context);EXCEPTION_RECORD exception{};exception.ExceptionCode=0xE06D7363;exception.ExceptionAddress=reinterpret_cast<void*>(context.Rip);
    Write(GetCurrentProcess(),GetCurrentProcessId(),GetCurrentThreadId(),&exception,&context,"direct EXE / C++ terminate",detail);}
  std::abort();
}
}
bool Write(HANDLE process,DWORD pid,DWORD tid,const EXCEPTION_RECORD* exception,CONTEXT* context,const char* origin,const char* detail) noexcept {
  if(reporting.test_and_set())return false;
  try{
    const auto folder=ReportDirectory(pid,tid);std::ofstream report(folder/L"report.txt",std::ios::binary);if(!report)return false;
    report<<"MotoGP 09/10 native crash report v1\nSDK: 0.10.0.24-dev.gbd833a2\nOrigin: "<<origin<<"\nPID: "<<pid<<"\nTID: "<<tid<<'\n';
    if(detail)report<<"Detail: "<<detail<<'\n';
    if(exception){const auto cpp=CppDetail(process,*exception);if(!cpp.empty())report<<cpp<<'\n';}
    if(exception){report<<"Exception: 0x"<<std::hex<<std::setw(8)<<std::setfill('0')<<exception->ExceptionCode<<"\nAddress: 0x"<<reinterpret_cast<uintptr_t>(exception->ExceptionAddress)<<"\nFlags: 0x"<<exception->ExceptionFlags<<'\n';
      for(unsigned i=0;i<std::min<DWORD>(exception->NumberParameters,EXCEPTION_MAXIMUM_PARAMETERS);++i)report<<"Parameter["<<std::dec<<i<<"]: 0x"<<std::hex<<exception->ExceptionInformation[i]<<'\n';report<<std::dec;
    }else report<<"No native exception event; exit status alone does not establish a cause.\n";
    std::array<wchar_t,32768> image{};DWORD length=static_cast<DWORD>(image.size());if(QueryFullProcessImageNameW(process,0,image.data(),&length)){
      const fs::path program(std::wstring(image.data(),length));report<<"Image: "<<Utf8(program.wstring())<<"\nImage SHA-256: "<<Hash(program)<<'\n';
    }
    if(context){report<<"\nRegisters:\nRIP=0x"<<std::hex<<context->Rip<<" RSP=0x"<<context->Rsp<<" RBP=0x"<<context->Rbp<<"\nRAX=0x"<<context->Rax<<" RBX=0x"<<context->Rbx<<" RCX=0x"<<context->Rcx<<" RDX=0x"<<context->Rdx<<"\nRSI=0x"<<context->Rsi<<" RDI=0x"<<context->Rdi<<" R8=0x"<<context->R8<<" R9=0x"<<context->R9<<"\nR10=0x"<<context->R10<<" R11=0x"<<context->R11<<" R12=0x"<<context->R12<<" R13=0x"<<context->R13<<"\nR14=0x"<<context->R14<<" R15=0x"<<context->R15<<std::dec<<'\n';}
    Modules(pid,report);
    const auto dump=CreateFileW((folder/L"crash.dmp").c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(dump!=INVALID_HANDLE_VALUE){
      EXCEPTION_RECORD record{};EXCEPTION_POINTERS pointers{};MINIDUMP_EXCEPTION_INFORMATION info{};
      if(exception && context){record=*exception;record.ExceptionRecord=nullptr;pointers={&record,context};info.ThreadId=tid;info.ExceptionPointers=&pointers;info.ClientPointers=FALSE;}
      const auto success=MiniDumpWriteDump(process,pid,dump,static_cast<MINIDUMP_TYPE>(MiniDumpNormal|MiniDumpWithThreadInfo|MiniDumpWithUnloadedModules),exception && context?&info:nullptr,nullptr,nullptr);
      const auto error=success?0:GetLastError();CloseHandle(dump);report<<"\nMinidump: "<<(success?"crash.dmp":"FAILED")<<" (Win32 error "<<error<<")\n";
      if(!success){std::error_code ignored;fs::remove(folder/L"crash.dmp",ignored);}
    }else report<<"Minidump file unavailable: "<<GetLastError()<<'\n';
    if(context)Stack(process,tid,*context,report);CopyEvidence(folder,report);
    report<<"\nThe dump contains process memory fragments. Keep the report and matching build for diagnosis.\n";report.flush();return static_cast<bool>(report);
  }catch(...){return false;}
}
void Install() noexcept {SetUnhandledExceptionFilter(Unhandled);std::set_terminate(Terminate);}
}
#endif
