#pragma once
#include <windows.h>
#include <commctrl.h>
#include <algorithm>
#include <new>

namespace launcher {
struct PopupAnimation {
  static constexpr UINT_PTR kSubclass=25,kTimer=0x4d47;
  static constexpr ULONGLONG kDuration=160;
  ULONGLONG started=0;
  bool active=false,owns_layer=false;
  static bool Enabled(){
    BOOL enabled=TRUE;SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION,0,&enabled,0);return enabled!=FALSE;
  }
  void Stop(HWND window){
    KillTimer(window,kTimer);active=false;
    if(owns_layer){
      SetLayeredWindowAttributes(window,0,255,LWA_ALPHA);
      SetWindowLongPtrW(window,GWL_EXSTYLE,GetWindowLongPtrW(window,GWL_EXSTYLE)&~WS_EX_LAYERED);
      owns_layer=false;
    }
  }
  void Start(HWND window){
    Stop(window);
    if(!Enabled())return;
    // The ComboLBox is a native top-level popup. Fade only its opacity; the
    // native control retains selection, capture, hit testing and keyboard input.
    const auto style=GetWindowLongPtrW(window,GWL_EXSTYLE);
    if(style&WS_EX_LAYERED)return;
    SetWindowLongPtrW(window,GWL_EXSTYLE,style|WS_EX_LAYERED);
    owns_layer=(GetWindowLongPtrW(window,GWL_EXSTYLE)&WS_EX_LAYERED)!=0;
    if(!owns_layer)return;
    if(!SetLayeredWindowAttributes(window,0,32,LWA_ALPHA)){Stop(window);return;}
    started=GetTickCount64();active=SetTimer(window,kTimer,15,nullptr)!=0;
    if(!active)Stop(window);
  }
  static LRESULT CALLBACK Proc(HWND window,UINT message,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR data){
    auto& state=*reinterpret_cast<PopupAnimation*>(data);
    if(message==WM_SHOWWINDOW){if(wp&&!state.owns_layer)state.Start(window);else if(!wp)state.Stop(window);}
    // ComboLBox also uses SetWindowPos directly; that path can omit
    // WM_SHOWWINDOW. Prepare opacity before the native popup becomes visible.
    if(message==WM_WINDOWPOSCHANGING){const auto flags=reinterpret_cast<WINDOWPOS*>(lp)->flags;
      if((flags&SWP_SHOWWINDOW)&&!state.owns_layer)state.Start(window);
      else if(flags&SWP_HIDEWINDOW)state.Stop(window);
    }
    if(message==WM_TIMER&&wp==kTimer){
      if(state.active){
        const double t=std::min(1.,double(GetTickCount64()-state.started)/kDuration);
        const auto opacity=static_cast<BYTE>(32+223*(1-(1-t)*(1-t)));
        SetLayeredWindowAttributes(window,0,opacity,LWA_ALPHA);
        if(t>=1){KillTimer(window,kTimer);state.active=false;}
      }
      return 0;
    }
    if(message==WM_NCDESTROY){state.Stop(window);RemoveWindowSubclass(window,Proc,id);delete &state;}
    return DefSubclassProc(window,message,wp,lp);
  }
  static void Attach(HWND combo){
    COMBOBOXINFO info{sizeof(info)};DWORD_PTR existing=0;
    if(!GetComboBoxInfo(combo,&info)||!info.hwndList||GetWindowSubclass(info.hwndList,Proc,kSubclass,&existing))return;
    auto* state=new(std::nothrow) PopupAnimation;
    if(state&&!SetWindowSubclass(info.hwndList,Proc,kSubclass,reinterpret_cast<DWORD_PTR>(state)))delete state;
  }
};
}
