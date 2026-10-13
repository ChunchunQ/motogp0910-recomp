// Compiled into the pinned v0.10.0 runtime, not the game executable.
#include "shared_sdl.h"

#ifdef _WIN32
extern "C" __declspec(dllexport) const MotogpSharedSDLApiV1* Motogp_GetSharedSDL(uint32_t version) {
  static const MotogpSharedSDLApiV1 api{
    1,sizeof(MotogpSharedSDLApiV1),
#define MOTOGP_SDL_ADDRESS(name,symbol) &symbol,
    MOTOGP_SHARED_SDL_FUNCTIONS(MOTOGP_SDL_ADDRESS)
#undef MOTOGP_SDL_ADDRESS
  };
  return version==1?&api:nullptr;
}
#endif
