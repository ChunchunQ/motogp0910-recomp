#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace {
// No C/C++ runtime imports: the public entry point must also start on systems
// where the bundled VC runtime exists only in bin, before the UI is loaded.
wchar_t root[32768], program[32768], command[32768];
STARTUPINFOW startup;
PROCESS_INFORMATION process;
[[noreturn]] void Fail() {
  MessageBoxW(nullptr,L"Could not start the launcher. Extract the complete ZIP and keep the bin folder beside MotoGP09-10Launcher.exe.",L"MotoGP 09/10",MB_OK|MB_ICONERROR);
  ExitProcess(1);
}
}
extern "C" void WINAPI LauncherEntry() {
  const DWORD length=GetModuleFileNameW(nullptr,root,32768);
  if(!length||length>=32768)Fail();
  DWORD slash=length;
  while(slash&&root[slash-1]!=L'\\'&&root[slash-1]!=L'/')--slash;
  if(!slash)Fail();
  root[slash]=0;
  constexpr wchar_t suffix[]=L"bin\\motogp0910-launcher.exe";
  if(slash+sizeof(suffix)/sizeof(wchar_t)>32768)Fail();
  lstrcpynW(program,root,32768);
  lstrcatW(program,suffix);
  const auto original=GetCommandLineW();
  if(lstrlenW(original)>=32768)Fail();
  lstrcpynW(command,original,32768);
  startup.cb=sizeof(startup);
  if(!CreateProcessW(program,command,nullptr,nullptr,FALSE,0,nullptr,root,&startup,&process))Fail();
  CloseHandle(process.hThread);
  WaitForSingleObject(process.hProcess,INFINITE);
  DWORD result=1;GetExitCodeProcess(process.hProcess,&result);
  CloseHandle(process.hProcess);ExitProcess(result);
}
