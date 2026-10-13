#include "livery_readback.h"
#include <rex/hook.h>
#include <rex/logging.h>
#include <stdexcept>
#include <cstdint>
#if REX_PLATFORM_WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

REX_EXTERN(__imp__sub_82370A60);
namespace motogp::livery {
namespace {
using ScopeFunction=uint32_t(*)(int32_t);
ScopeFunction scope_function=nullptr;
}
void Initialize() {
#if REX_PLATFORM_WIN32
  const auto module=GetModuleHandleW(L"rexgpu-xenos.dll");
  scope_function=module?reinterpret_cast<ScopeFunction>(GetProcAddress(module,"Motogp_LiveryReadbackScope")):nullptr;
#else
  const auto module=dlopen("librexgpu-xenos.so",RTLD_NOW|RTLD_NOLOAD);
  scope_function=module?reinterpret_cast<ScopeFunction>(dlsym(module,"Motogp_LiveryReadbackScope")):nullptr;
  if(module)dlclose(module);
#endif
  if(!scope_function)throw std::runtime_error("The GPU runtime is missing the career livery fix. Replace both runtime DLLs from this build.");
}
}
REX_HOOK_RAW(sub_82370A60) {
  // The livery builder renders custom colours/sponsors per mip, resolves,
  // waits for its GPU fence, then CPU-locks and copies the resolved surface.
  // The SDK's ordinary GPU-only resolves leave guest RAM stale. Require an
  // immediate readback until the last mip's fence and CPU copies complete.
  using namespace motogp::livery;
  if(!ctx.r3.u32 || !scope_function) {__imp__sub_82370A60(ctx,base);return;}
  struct Scope {
    Scope(){scope_function(1);}
    ~Scope(){scope_function(-1);}
  } scope;
  REXLOG_INFO("LIVERY_READBACK: generating custom texture with synchronous resolve readback");
  __imp__sub_82370A60(ctx,base);
}
