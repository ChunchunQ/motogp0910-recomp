#include "in_game_settings.h"
#include <rex/hook.h>
#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif
namespace {
PPCFunc* Original(const char* name){
#ifdef _WIN32
  auto* original=reinterpret_cast<PPCFunc*>(GetProcAddress(GetModuleHandleW(L"rexruntime.dll"),name));
#else
  auto* original=reinterpret_cast<PPCFunc*>(dlsym(RTLD_NEXT,name));
#endif
  if(!original)throw std::runtime_error("Original input export missing");return original;
}
}
// MnK's direct mapping-string readers must not race live rebinding on the UI
// thread. Preserve the original runtime implementation, arguments and results.
REX_HOOK_RAW(__imp__XamInputGetState){
  static auto* original=Original("__imp__XamInputGetState");
  const std::lock_guard lock(motogp::settings::InputValueMutex());original(ctx,base);
}
// The built-in SDK blocker neutralizes GetState but not the keystroke queue.
// Resolve the real DLL export when closed; never recursively call this hook.
REX_HOOK_RAW(__imp__XamInputGetKeystrokeEx){
  const std::lock_guard lock(motogp::settings::InputValueMutex());
  if(motogp::settings::GuestInputBlocked()){ctx.r3.u64=ctx.r5.u32?0x10D2:0xA0;return;} // X_ERROR_EMPTY / BAD_ARGUMENTS
  static auto* original=Original("__imp__XamInputGetKeystrokeEx");original(ctx,base);
}
