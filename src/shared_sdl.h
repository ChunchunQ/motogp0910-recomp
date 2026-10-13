#pragma once

#include <SDL3/SDL.h>
#include <cstdint>
#include <utility>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <stdexcept>
#endif

// The Windows SDK statically links SDL into rexruntime.dll and its consumers.
// Resolve this small versioned table from the runtime so gamepad observers and
// virtual-controller checks use the same SDL instance as the running game.
#define MOTOGP_SHARED_SDL_FUNCTIONS(X) \
  X(Init,SDL_Init) X(Quit,SDL_Quit) X(GetError,SDL_GetError) \
  X(GetGamepads,SDL_GetGamepads) X(GetGamepadTypeForID,SDL_GetGamepadTypeForID) \
  X(GetGamepadVendorForID,SDL_GetGamepadVendorForID) X(Free,SDL_free) \
  X(AddEventWatch,SDL_AddEventWatch) X(RemoveEventWatch,SDL_RemoveEventWatch) \
  X(AttachVirtualJoystick,SDL_AttachVirtualJoystick) X(DetachVirtualJoystick,SDL_DetachVirtualJoystick) \
  X(OpenGamepad,SDL_OpenGamepad) X(CloseGamepad,SDL_CloseGamepad) \
  X(GetGamepadJoystick,SDL_GetGamepadJoystick) X(SetJoystickVirtualAxis,SDL_SetJoystickVirtualAxis) \
  X(SetJoystickVirtualButton,SDL_SetJoystickVirtualButton) X(UpdateJoysticks,SDL_UpdateJoysticks) \
  X(PollEvent,SDL_PollEvent)

struct MotogpSharedSDLApiV1 {
  uint32_t version;
  uint32_t size;
#define MOTOGP_SDL_FIELD(name,symbol) decltype(&symbol) name;
  MOTOGP_SHARED_SDL_FUNCTIONS(MOTOGP_SDL_FIELD)
#undef MOTOGP_SDL_FIELD
};

namespace motogp::shared_sdl {
#ifdef _WIN32
inline const MotogpSharedSDLApiV1& Api() {
  static const auto* table=[] {
    using Getter=const MotogpSharedSDLApiV1* (*)(uint32_t);
    const auto module=GetModuleHandleW(L"rexruntime.dll");
    const auto getter=module?reinterpret_cast<Getter>(GetProcAddress(module,"Motogp_GetSharedSDL")):nullptr;
    const auto* result=getter?getter(1):nullptr;
    if(!result || result->version!=1 || result->size!=sizeof(MotogpSharedSDLApiV1))
      throw std::runtime_error("Replace rexruntime.dll with the runtime supplied in this MotoGP build.");
    return result;
  }();
  return *table;
}
#define MOTOGP_SDL_WRAPPER(name,symbol) \
  template<class... Args> inline decltype(auto) name(Args&&... args) {return Api().name(std::forward<Args>(args)...);}
#else
#define MOTOGP_SDL_WRAPPER(name,symbol) \
  template<class... Args> inline decltype(auto) name(Args&&... args) {return ::symbol(std::forward<Args>(args)...);}
#endif
MOTOGP_SHARED_SDL_FUNCTIONS(MOTOGP_SDL_WRAPPER)
#undef MOTOGP_SDL_WRAPPER
}
