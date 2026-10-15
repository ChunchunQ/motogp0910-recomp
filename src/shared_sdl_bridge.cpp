// Compiled into the pinned SDK runtime, not the game executable.
#include "shared_sdl.h"
#include <cmath>

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

#ifdef _WIN32
// Startup-only display control, on the SDL/UI thread. Keep the existing input
// function-table ABI unchanged so older controller probes remain compatible.
extern "C" __declspec(dllexport) int32_t Motogp_ConfigureHostDisplay(
    uint32_t version,void* hwnd,int32_t kind,int32_t width,int32_t height,int32_t hz) {
  if(version!=1 || kind<0 || kind>2 || width<0 || height<0 || hz<0 || hz>1000)return -1;
  if(!hwnd)return -2;
  int count=0;SDL_Window** windows=SDL_GetWindows(&count);SDL_Window* window=nullptr;
  if(windows) {
    for(int i=0;i<count;++i) {
      if(SDL_GetPointerProperty(SDL_GetWindowProperties(windows[i]),
          SDL_PROP_WINDOW_WIN32_HWND_POINTER,nullptr)==hwnd) {
        window=windows[i];
        break;
      }
    }
    SDL_free(windows);
  }
  if(!window)return -2;
  SDL_DisplayMode chosen{};const SDL_DisplayMode* fullscreen=nullptr;
  if(kind==2){
    const auto display=SDL_GetDisplayForWindow(window);
    const auto* desktop=SDL_GetDesktopDisplayMode(display);
    if(!desktop)return -3;
    const int wanted_width=width>0?width:desktop->w,wanted_height=height>0?height:desktop->h;
    const float wanted_hz=hz>0?static_cast<float>(hz):desktop->refresh_rate;
    if(!SDL_GetClosestFullscreenDisplayMode(display,wanted_width,wanted_height,wanted_hz,false,&chosen) ||
       chosen.w!=wanted_width || chosen.h!=wanted_height ||
       // DEVMODE reports integer Hz (59 for a 59.94 Hz timing on some drivers).
       (hz>0 && std::abs(chosen.refresh_rate-wanted_hz)>=1.0f))return -3;
    fullscreen=&chosen;
  }
  if(!SDL_SetWindowFullscreenMode(window,fullscreen) || !SDL_SetWindowFullscreen(window,kind!=0) ||
     !SDL_SyncWindow(window))return -4;
  return 0;
}
#endif

#ifdef _WIN32
// Route effects through the same SDL instance that owns the physical gamepad.
extern "C" __declspec(dllexport) bool Motogp_SendGamepadEffect(
    uint32_t version,SDL_Gamepad* pad,const void* data,int size) {
  if(version!=1||!pad||!data||size!=47)return false;
  return SDL_SendGamepadEffect(pad,data,size);
}
#endif
