#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
namespace motogp::crash {
void Install() noexcept;
// External reports run while the debuggee is stopped, before its memory is lost.
bool Write(HANDLE process,DWORD pid,DWORD tid,const EXCEPTION_RECORD* exception,
           CONTEXT* context,const char* origin,const char* detail=nullptr) noexcept;
}
#else
namespace motogp::crash { inline void Install() noexcept {} }
#endif
