// Intentionally crash a tiny fixture, never the user's game or files.
#include "../src/crash_report.h"
#include "../src/launcher/core.h"
#include <intrin.h>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <stdexcept>
namespace {
__declspec(noinline) void AccessViolation(){*reinterpret_cast<volatile DWORD*>(uintptr_t(0x1234))=1;}
__declspec(noinline) void Handled(){__try{RaiseException(0xE1234567,0,0,nullptr);}__except(EXCEPTION_EXECUTE_HANDLER){}}
}
int wmain(int argc,wchar_t** argv){
  motogp::crash::Install();std::filesystem::create_directories("logs");
  {std::ofstream output("logs/motogp0910_001.log");output<<"Diagnostic fixture evidence\n";}
  if(argc!=2)return 1;
  if(!wcscmp(argv[1],L"parent")){
    const auto process=launcher::Start(launcher::ExecutableFolder(),{L"delay"},true);CloseHandle(process);return 0;
  }
  if(!wcscmp(argv[1],L"delay")){Sleep(1000);std::ofstream("logs/delay-complete.txt")<<"game survived parent exit\n";return 0;}
  if(!wcscmp(argv[1],L"clean"))return 0;
  if(!wcscmp(argv[1],L"handled")){Handled();return 0;}
  if(!wcscmp(argv[1],L"av")){AccessViolation();return 0;}
  if(!wcscmp(argv[1],L"fastfail")){__fastfail(7);}
  if(!wcscmp(argv[1],L"cpp"))throw std::runtime_error("Crash fixture: uncaught C++ failure");
  if(!wcscmp(argv[1],L"nonzero"))return 17;
  return 2;
}
