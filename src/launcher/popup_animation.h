#pragma once
#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <algorithm>
#include <new>
#include "popup_scroll.h"
#include "cursor_style.h"

namespace launcher {
// Keep native combo input, accessibility and geometry. Only paint/opacity are
// animated. A passive snapshot finishes the close after native capture ends.
struct PopupAnimation {
  static constexpr UINT_PTR kSubclass=25,kTimer=0x4d47;
  static constexpr ULONGLONG kDuration=150;
  static inline HWND closing_popup{};
  ULONGLONG started=0;
  bool active=false,owns_layer=false,shown=false,rounding=false;
  HWND combo{};PopupScroll scroll;
  static bool Enabled(){
    BOOL enabled=TRUE;SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION,0,&enabled,0);return enabled!=FALSE;
  }
  static void DismissClosing(){if(closing_popup)DestroyWindow(closing_popup);}
  struct Closing {
    HBITMAP bitmap{};ULONGLONG started{};BYTE alpha=255;
    ~Closing(){if(bitmap)DeleteObject(bitmap);}
    static LRESULT CALLBACK Proc(HWND window,UINT message,WPARAM wp,LPARAM lp){
      auto* self=reinterpret_cast<Closing*>(GetWindowLongPtrW(window,GWLP_USERDATA));
      if(!self)return DefWindowProcW(window,message,wp,lp);
      switch(message){
        case WM_ERASEBKGND:return 1;
        case WM_NCHITTEST:return HTTRANSPARENT;
        case WM_MOUSEACTIVATE:return MA_NOACTIVATE;
        case WM_PAINT:{PAINTSTRUCT ps;const auto dc=BeginPaint(window,&ps);const auto copy=CreateCompatibleDC(dc);if(copy){const auto previous=SelectObject(copy,self->bitmap);RECT r;GetClientRect(window,&r);BitBlt(dc,0,0,r.right,r.bottom,copy,0,0,SRCCOPY);SelectObject(copy,previous);DeleteDC(copy);}EndPaint(window,&ps);return 0;}
        case WM_TIMER:if(wp==kTimer){const auto elapsed=GetTickCount64()-self->started;if(elapsed>=kDuration){DestroyWindow(window);return 0;}SetLayeredWindowAttributes(window,0,static_cast<BYTE>(self->alpha*(kDuration-elapsed)/kDuration),LWA_ALPHA);return 0;}break;
        case WM_NCDESTROY:KillTimer(window,kTimer);if(closing_popup==window)closing_popup=nullptr;SetWindowLongPtrW(window,GWLP_USERDATA,0);delete self;break;
      }
      return DefWindowProcW(window,message,wp,lp);
    }
    static void Show(HWND source,HWND owner){
      DismissClosing();RECT r;if(!GetWindowRect(source,&r)||r.right<=r.left||r.bottom<=r.top)return;
      auto* self=new(std::nothrow) Closing;if(!self)return;
      const auto dc=GetWindowDC(source),copy=dc?CreateCompatibleDC(dc):nullptr;
      if(copy)self->bitmap=CreateCompatibleBitmap(dc,r.right-r.left,r.bottom-r.top);
      bool copied=false;if(self->bitmap){const auto old=SelectObject(copy,self->bitmap);copied=BitBlt(copy,0,0,r.right-r.left,r.bottom-r.top,dc,0,0,SRCCOPY)!=FALSE;SelectObject(copy,old);}
      if(copy)DeleteDC(copy);if(dc)ReleaseDC(source,dc);if(!copied){delete self;return;}
      DWORD flags=0;GetLayeredWindowAttributes(source,nullptr,&self->alpha,&flags);
      if(!self->alpha){delete self;return;}
      WNDCLASSW cls{};cls.lpfnWndProc=Proc;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"MotoGPPopupClose";RegisterClassW(&cls);
      const auto window=CreateWindowExW(WS_EX_LAYERED|WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW|WS_EX_TRANSPARENT,cls.lpszClassName,L"",WS_POPUP,r.left,r.top,r.right-r.left,r.bottom-r.top,owner,nullptr,cls.hInstance,nullptr);
      if(!window){delete self;return;}
      SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));
      closing_popup=window;self->started=GetTickCount64();
      const auto region=CreateRectRgn(0,0,0,0);if(GetWindowRgn(source,region)!=ERROR){if(!SetWindowRgn(window,region,FALSE))DeleteObject(region);}else DeleteObject(region);
      if(!SetLayeredWindowAttributes(window,0,self->alpha,LWA_ALPHA)||!SetTimer(window,kTimer,15,nullptr)){DestroyWindow(window);return;}
      SetWindowPos(window,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_SHOWWINDOW);UpdateWindow(window);
    }
  };
  static void Round(HWND window){
    RECT r;if(!GetWindowRect(window,&r))return;
    const int radius=std::max(4,MulDiv(8,GetDpiForWindow(window),96));
    const auto region=CreateRoundRectRgn(0,0,r.right-r.left+1,r.bottom-r.top+1,radius,radius);
    if(region&&!SetWindowRgn(window,region,TRUE))DeleteObject(region);
  }
  void Position(HWND window,WINDOWPOS& pos){
    RECT field,r;if(!GetWindowRect(combo,&field)||!GetWindowRect(window,&r))return;
    const int height=(pos.flags&SWP_NOSIZE)?r.bottom-r.top:pos.cy;
    const int old_y=(pos.flags&SWP_NOMOVE)?r.top:pos.y;
    const int gap=std::max(2,MulDiv(4,GetDpiForWindow(combo),96));
    MONITORINFO monitor{sizeof(monitor)};
    if(!GetMonitorInfoW(MonitorFromWindow(combo,MONITOR_DEFAULTTONEAREST),&monitor))return;
    const auto& work=monitor.rcWork;
    int y=old_y<field.top?field.top-height-gap:field.bottom+gap;
    if(y+height>work.bottom&&field.top-height-gap>=work.top)y=field.top-height-gap;
    else if(y<work.top&&field.bottom+gap+height<=work.bottom)y=field.bottom+gap;
    if(pos.flags&SWP_NOMOVE)pos.x=r.left;
    pos.y=std::clamp(y,int(work.top),std::max(int(work.top),int(work.bottom)-height));pos.flags&=~SWP_NOMOVE;
  }
  void Stop(HWND window){
    KillTimer(window,kTimer);active=false;
    if(owns_layer){SetLayeredWindowAttributes(window,0,255,LWA_ALPHA);SetWindowLongPtrW(window,GWL_EXSTYLE,GetWindowLongPtrW(window,GWL_EXSTYLE)&~WS_EX_LAYERED);owns_layer=false;}
  }
  void Start(HWND window){
    DismissClosing();Stop(window);shown=true;
    InvalidateRect(combo,nullptr,FALSE);
    if(!Enabled())return;
    const auto style=GetWindowLongPtrW(window,GWL_EXSTYLE);if(style&WS_EX_LAYERED)return;
    SetWindowLongPtrW(window,GWL_EXSTYLE,style|WS_EX_LAYERED);owns_layer=(GetWindowLongPtrW(window,GWL_EXSTYLE)&WS_EX_LAYERED)!=0;
    if(!owns_layer)return;
    if(!SetLayeredWindowAttributes(window,0,0,LWA_ALPHA)){Stop(window);return;}
    // Native Windows may finish its own popup show transaction before timers
    // are dispatched. Start elapsed time on the first rendered timer frame,
    // so that delay cannot consume our entire fade and reveal an opaque list.
    started=0;active=SetTimer(window,kTimer,15,nullptr)!=0;if(!active)Stop(window);
  }
  void Hide(HWND window){
    if(shown&&Enabled()&&IsWindowVisible(window))Closing::Show(window,GetAncestor(combo,GA_ROOT));
    scroll.dragging=false;shown=false;KillTimer(window,kTimer);active=false;
    // WM_SHOWWINDOW / WINDOWPOSCHANGING arrive before the native list is
    // actually hidden. Restoring alpha=255 or removing WS_EX_LAYERED here
    // briefly reveals the old list underneath its closing snapshot on DWM.
    // Hand over to the snapshot while retaining the invisible source layer;
    // restore native styles only after the hide transaction has completed.
    if(owns_layer)SetLayeredWindowAttributes(window,0,0,LWA_ALPHA);
    InvalidateRect(combo,nullptr,FALSE);
  }
  static LRESULT CALLBACK Proc(HWND window,UINT message,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR data){
    auto& state=*reinterpret_cast<PopupAnimation*>(data);
    if(message==WM_SETCURSOR&&SetActionCursor(window,lp))return TRUE;
    if(message==WM_SHOWWINDOW){if(wp&&!state.shown)state.Start(window);else if(!wp)state.Hide(window);}
    if(message==WM_WINDOWPOSCHANGING){auto& pos=*reinterpret_cast<WINDOWPOS*>(lp);if(pos.flags&SWP_SHOWWINDOW){state.Position(window,pos);if(!state.shown)state.Start(window);}else if(pos.flags&SWP_HIDEWINDOW)state.Hide(window);}
    if(message==WM_WINDOWPOSCHANGED){const auto result=DefSubclassProc(window,message,wp,lp);if(!IsWindowVisible(window))state.Stop(window);if(state.shown)state.scroll.HideNative(window);if(state.shown&&!state.rounding){state.rounding=true;Round(window);state.rounding=false;}return result;}
    if(state.scroll.Input(window,message,lp))return 0;
    if(message==WM_PAINT||message==WM_PRINTCLIENT){
      state.scroll.HideNative(window);const auto result=DefSubclassProc(window,message,wp,lp);
      const auto dc=message==WM_PRINTCLIENT?reinterpret_cast<HDC>(wp):GetDC(window);
      if(dc){PopupScroll::Paint(window,dc);if(message!=WM_PRINTCLIENT)ReleaseDC(window,dc);}return result;
    }
    if(message==WM_NCPAINT){const auto result=DefSubclassProc(window,message,wp,lp);const auto dc=GetWindowDC(window);if(dc){RECT r;GetWindowRect(window,&r);const auto pen=CreatePen(PS_SOLID,1,RGB(48,53,64));const auto old_pen=SelectObject(dc,pen),old_brush=SelectObject(dc,GetStockObject(NULL_BRUSH));RoundRect(dc,0,0,r.right-r.left,r.bottom-r.top,6,6);SelectObject(dc,old_brush);SelectObject(dc,old_pen);DeleteObject(pen);ReleaseDC(window,dc);}return result;}
    if(message==WM_TIMER&&wp==kTimer){
      if(state.active){if(!state.started)state.started=GetTickCount64();const auto elapsed=GetTickCount64()-state.started;const auto alpha=static_cast<BYTE>(255*std::min(elapsed,kDuration)/kDuration);SetLayeredWindowAttributes(window,0,alpha,LWA_ALPHA);if(elapsed>=kDuration){KillTimer(window,kTimer);state.active=false;}}return 0;
    }
    if(message==WM_NCDESTROY){state.shown=false;state.Stop(window);RemoveWindowSubclass(window,Proc,id);delete &state;}
    return DefSubclassProc(window,message,wp,lp);
  }
  static void Attach(HWND combo){
    COMBOBOXINFO info{sizeof(info)};DWORD_PTR existing=0;
    if(!GetComboBoxInfo(combo,&info)||!info.hwndList||GetWindowSubclass(info.hwndList,Proc,kSubclass,&existing))return;
    auto* state=new(std::nothrow) PopupAnimation;
    if(state){state->combo=combo;if(!SetWindowSubclass(info.hwndList,Proc,kSubclass,reinterpret_cast<DWORD_PTR>(state)))delete state;else SetWindowTheme(info.hwndList,L"DarkMode_Explorer",nullptr);}
  }
};
}
