#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace {
// No C/C++ runtime imports: the public entry point must also start on systems
// where the bundled VC runtime exists only in bin, before the UI is loaded.
wchar_t root[32768], program[32768];
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
  constexpr wchar_t suffix[]=L"bin\\motogp0910-ui.dll";
  if(slash+sizeof(suffix)/sizeof(wchar_t)>32768)Fail();
  lstrcpynW(program,root,32768);
  lstrcatW(program,suffix);
  if(!SetCurrentDirectoryW(root))Fail();
  // Resolve the UI and its runtime dependencies only from the installation's
  // bin directory and Windows' standard safe locations, never the caller's cwd.
  const auto module=LoadLibraryExW(program,nullptr,
      LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
  if(!module)Fail();
  using Entry=int (WINAPI*)(HINSTANCE,HINSTANCE,PWSTR,int);
  const auto entry=reinterpret_cast<Entry>(GetProcAddress(module,"MotoGPLauncherMain"));
  if(!entry)Fail();
  const auto result=entry(GetModuleHandleW(nullptr),nullptr,GetCommandLineW(),SW_SHOWDEFAULT);
  FreeLibrary(module);
  ExitProcess(static_cast<UINT>(result));
}
