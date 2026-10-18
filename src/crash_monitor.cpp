#include "crash_report.h"
#include "launcher/core.h"
#include <shellapi.h>
#include <cwchar>
#include <cstring>
#include <string>
#include <vector>
namespace {
std::string DebugText(HANDLE process,const OUTPUT_DEBUG_STRING_INFO& info){
  const auto characters=std::min<size_t>(info.nDebugStringLength,2048);const auto bytes=characters*(info.fUnicode?2:1);
  if(!bytes)return {};std::vector<char> buffer(bytes+2,0);SIZE_T received=0;
  if(!ReadProcessMemory(process,info.lpDebugStringData,buffer.data(),bytes,&received))return {};
  if(info.fUnicode){std::wstring text(received/2,L'\0');std::memcpy(text.data(),buffer.data(),text.size()*2);while(!text.empty() && !text.back())text.pop_back();return launcher::Utf8(text);}
  std::string text(buffer.data(),received);while(!text.empty() && !text.back())text.pop_back();return text;
}
}
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int){
  HANDLE process_handle=nullptr;DWORD pid=0;
  try{
    const auto binaries=launcher::ExecutableFolder(),root=motogp::portable::Root(binaries),game=binaries/L"motogp0910.exe";
    int count=0;auto** arguments=CommandLineToArgvW(GetCommandLineW(),&count);if(!arguments)throw std::runtime_error("Cannot parse crash-monitor command");
    std::wstring command=launcher::Quote(game.wstring());for(int i=1;i<count;++i)command+=L" "+launcher::Quote(arguments[i]);LocalFree(arguments);
    STARTUPINFOW startup{sizeof(startup)};PROCESS_INFORMATION process{};
    if(!CreateProcessW(game.c_str(),command.data(),nullptr,nullptr,FALSE,DEBUG_ONLY_THIS_PROCESS,nullptr,root.c_str(),&startup,&process)){
      const auto detail="CreateProcess failed: Win32 error "+std::to_string(GetLastError());motogp::crash::Write(GetCurrentProcess(),GetCurrentProcessId(),GetCurrentThreadId(),nullptr,nullptr,"launcher crash monitor",detail.c_str());return 1;
    }
    process_handle=process.hProcess;pid=process.dwProcessId;CloseHandle(process.hThread);
    if(!DebugSetProcessKillOnExit(FALSE))throw std::runtime_error("Cannot configure crash-monitor detach behavior");
    bool first_breakpoint=true,reported=false;std::string debug_text;
    for(;;){
      DEBUG_EVENT event{};if(!WaitForDebugEvent(&event,1000)){if(GetLastError()==ERROR_SEM_TIMEOUT)continue;throw std::runtime_error("Crash-monitor debug event wait failed");}
      DWORD continuation=DBG_CONTINUE;bool exited=false;DWORD exit_code=0;
      switch(event.dwDebugEventCode){
        case CREATE_PROCESS_DEBUG_EVENT:if(event.u.CreateProcessInfo.hFile)CloseHandle(event.u.CreateProcessInfo.hFile);break;
        case LOAD_DLL_DEBUG_EVENT:if(event.u.LoadDll.hFile)CloseHandle(event.u.LoadDll.hFile);break;
        case OUTPUT_DEBUG_STRING_EVENT:
          try{auto text=DebugText(process_handle,event.u.DebugString);if(!text.empty()){if(debug_text.size()>16384)debug_text.clear();debug_text+=text+'\n';}}catch(...){}break;
        case EXCEPTION_DEBUG_EVENT:{
          const auto& exception=event.u.Exception.ExceptionRecord;
          if(event.u.Exception.dwFirstChance && exception.ExceptionCode==EXCEPTION_BREAKPOINT && first_breakpoint){first_breakpoint=false;break;}
          continuation=DBG_EXCEPTION_NOT_HANDLED;
          if(!event.u.Exception.dwFirstChance && !reported){
            CONTEXT context{};context.ContextFlags=CONTEXT_FULL;const auto thread=OpenThread(THREAD_GET_CONTEXT|THREAD_QUERY_INFORMATION,FALSE,event.dwThreadId);
            const bool captured=thread && GetThreadContext(thread,&context);if(thread)CloseHandle(thread);
            reported=motogp::crash::Write(process_handle,pid,event.dwThreadId,&exception,captured?&context:nullptr,"launcher / second-chance exception",debug_text.empty()?nullptr:debug_text.c_str());
          }break;
        }
        case EXIT_PROCESS_DEBUG_EVENT:
          exited=true;exit_code=event.u.ExitProcess.dwExitCode;
          if(exit_code && !reported){const auto detail="Process exit: "+std::to_string(exit_code)+"; no unhandled exception was observed";
            motogp::crash::Write(process_handle,pid,event.dwThreadId,nullptr,nullptr,"launcher / abnormal exit",detail.c_str());}break;
      }
      if(!ContinueDebugEvent(event.dwProcessId,event.dwThreadId,continuation))throw std::runtime_error("Cannot continue monitored process");
      if(exited){CloseHandle(process_handle);return static_cast<int>(exit_code);}
    }
  }catch(const std::exception& error){
    if(pid)DebugActiveProcessStop(pid);
    motogp::crash::Write(GetCurrentProcess(),GetCurrentProcessId(),GetCurrentThreadId(),nullptr,nullptr,"crash-monitor error",error.what());
    if(process_handle)CloseHandle(process_handle);return 1;
  }
}
