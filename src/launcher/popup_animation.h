#pragma once
#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <algorithm>
#include <cstdint>
#include <new>
#include "popup_scroll.h"
#include "cursor_style.h"

namespace launcher {
// WPF reference: one transparent popup surface, 150 ms Fade in both directions,
// with destruction delayed until fade-out ends. Native ComboLBox retains input,
// selection and accessibility; only its visual presentation lives here.
struct PopupAnimation {
  static constexpr UINT_PTR kSubclass=25,kTimer=0x4d47;
  static constexpr UINT kRefresh=WM_APP+0x347;
  static constexpr ULONGLONG kDuration=150;
  static inline HWND visual_popup{},closing_popup{};
  bool active=false,owns_layer=false,shown=false,rounding=false,wheel_scroll=false;
  bool rendering=false,refresh_pending=false;
  int wheel_remainder=0;
  HWND combo{},list{};PopupScroll scroll;
  struct Surface;
  Surface* surface{};
  bool Wheel(HWND list,WPARAM wp){
    if(!wheel_scroll||!SendMessageW(combo,CB_GETDROPPEDSTATE,0,0))return false;
    wheel_remainder+=GET_WHEEL_DELTA_WPARAM(wp);
    const int notches=wheel_remainder/WHEEL_DELTA;wheel_remainder%=WHEEL_DELTA;
    UINT lines=3;SystemParametersInfoW(SPI_GETWHEELSCROLLLINES,0,&lines,0);
    const auto g=PopupScroll::Measure(list);
    const auto step=lines==WHEEL_PAGESCROLL?g.visible:std::min<UINT>(lines,g.maximum);
    if(notches&&step)PopupScroll::SetTop(list,static_cast<int>(std::clamp<int64_t>(int64_t(g.top)-int64_t(notches)*step,0,g.maximum)));
    return true;
  }
  static bool ScrollWheel(HWND combo,WPARAM wp){
    COMBOBOXINFO info{sizeof(info)};DWORD_PTR data=0;
    return GetComboBoxInfo(combo,&info)&&GetWindowSubclass(info.hwndList,Proc,kSubclass,&data)&&data&&reinterpret_cast<PopupAnimation*>(data)->Wheel(info.hwndList,wp);
  }
  static bool Enabled(){
    BOOL enabled=TRUE;SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION,0,&enabled,0);return enabled!=FALSE;
  }
  static void DismissClosing(){if(closing_popup)DestroyWindow(closing_popup);}
  struct Surface {
    PopupAnimation* owner{};HWND window{};HBITMAP bitmap{};SIZE size{};POINT position{};
    BYTE alpha=0,from=0,target=255;ULONGLONG started=0;bool region_valid=false;
    ~Surface(){if(bitmap)DeleteObject(bitmap);}
    bool Upload(){
      const auto dc=CreateCompatibleDC(nullptr);if(!dc||!bitmap){if(dc)DeleteDC(dc);return false;}
      const auto old=SelectObject(dc,bitmap);POINT origin{};BLENDFUNCTION blend{AC_SRC_OVER,0,alpha,0};
      const bool result=UpdateLayeredWindow(window,nullptr,&position,&size,dc,&origin,0,&blend,ULW_ALPHA)!=FALSE;
      SelectObject(dc,old);DeleteDC(dc);return result;
    }
    bool Capture(){
      if(owner->rendering||!owner->shown||!IsWindowVisible(owner->list))return false;
      RECT r;if(!GetWindowRect(owner->list,&r)||r.right<=r.left||r.bottom<=r.top)return false;
      owner->rendering=true;
      const auto screen=GetDC(nullptr),dc=screen?CreateCompatibleDC(screen):nullptr;
      const SIZE next{r.right-r.left,r.bottom-r.top};
      if(dc&&(!bitmap||next.cx!=size.cx||next.cy!=size.cy)){
        const auto replacement=CreateCompatibleBitmap(screen,next.cx,next.cy);
        if(replacement){if(bitmap)DeleteObject(bitmap);bitmap=replacement;size=next;region_valid=false;}
      }
      bool painted=false;
      if(dc&&bitmap&&size.cx==next.cx&&size.cy==next.cy){
        const auto old=SelectObject(dc,bitmap);
        // Print the control at full color. Its desktop alpha is always zero,
        // so capturing desktop pixels here would produce an empty/dim image.
        painted=PrintWindow(owner->list,dc,0)!=FALSE;SelectObject(dc,old);
      }
      if(dc)DeleteDC(dc);if(screen)ReleaseDC(nullptr,screen);owner->rendering=false;
      if(!painted)return false;
      position={r.left,r.top};
      if(!region_valid){const auto region=CreateRectRgn(0,0,0,0);
        if(GetWindowRgn(owner->list,region)!=ERROR){region_valid=SetWindowRgn(window,region,FALSE)!=FALSE;if(!region_valid)DeleteObject(region);}else DeleteObject(region);}
      return Upload();
    }
    void Fade(BYTE next){
      KillTimer(window,kTimer);from=alpha;target=next;started=0;
      owner->active=Enabled()&&from!=target;
      if(!owner->active){alpha=target;Upload();if(!target)DestroyWindow(window);return;}
      if(!SetTimer(window,kTimer,10,nullptr)){alpha=target;owner->active=false;Upload();if(!target)DestroyWindow(window);}
    }
    static LRESULT CALLBACK Proc(HWND window,UINT message,WPARAM wp,LPARAM lp){
      auto* self=reinterpret_cast<Surface*>(GetWindowLongPtrW(window,GWLP_USERDATA));
      if(!self)return DefWindowProcW(window,message,wp,lp);
      switch(message){
        case WM_ERASEBKGND:return 1;
        case WM_NCHITTEST:return HTTRANSPARENT;
        case WM_MOUSEACTIVATE:return MA_NOACTIVATE;
        case WM_PAINT:ValidateRect(window,nullptr);return 0;
        case WM_TIMER:if(wp==kTimer){
          if(!self->started)self->started=GetTickCount64();
          const auto elapsed=std::min(GetTickCount64()-self->started,kDuration);
          self->alpha=static_cast<BYTE>(int(self->from)+(int(self->target)-int(self->from))*static_cast<int>(elapsed)/static_cast<int>(kDuration));
          if(!self->Upload()){DestroyWindow(window);return 0;}
          if(elapsed==kDuration){KillTimer(window,kTimer);self->owner->active=false;if(!self->target)DestroyWindow(window);}
          return 0;
        }break;
        case WM_NCDESTROY:
          KillTimer(window,kTimer);if(visual_popup==window)visual_popup=nullptr;if(closing_popup==window)closing_popup=nullptr;
          self->owner->surface=nullptr;self->owner->active=false;SetWindowLongPtrW(window,GWLP_USERDATA,0);delete self;break;
      }
      return DefWindowProcW(window,message,wp,lp);
    }
  };
  static void RequestRefresh(HWND combo){
    COMBOBOXINFO info{sizeof(info)};DWORD_PTR data=0;
    if(!GetComboBoxInfo(combo,&info)||!GetWindowSubclass(info.hwndList,Proc,kSubclass,&data)||!data)return;
    auto& state=*reinterpret_cast<PopupAnimation*>(data);
    if(state.rendering||state.refresh_pending||!state.shown||!state.surface)return;
    state.refresh_pending=PostMessageW(info.hwndList,kRefresh,0,0)!=FALSE;
  }
  bool EnsureSurface(){
    if(surface)return true;
    if(visual_popup)DestroyWindow(visual_popup);
    auto* next=new(std::nothrow) Surface;if(!next)return false;next->owner=this;
    WNDCLASSW cls{};cls.lpfnWndProc=Surface::Proc;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"MotoGPPopupSurface";RegisterClassW(&cls);
    const auto window=CreateWindowExW(WS_EX_LAYERED|WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW|WS_EX_TRANSPARENT,cls.lpszClassName,L"",WS_POPUP,0,0,0,0,GetAncestor(combo,GA_ROOT),nullptr,cls.hInstance,nullptr);
    if(!window){delete next;return false;}
    next->window=window;surface=next;visual_popup=window;SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(next));return true;
  }
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
    if(owns_layer){SetLayeredWindowAttributes(window,0,255,LWA_ALPHA);SetWindowLongPtrW(window,GWL_EXSTYLE,GetWindowLongPtrW(window,GWL_EXSTYLE)&~WS_EX_LAYERED);owns_layer=false;}
  }
  void Start(HWND window){
    shown=true;wheel_remainder=0;InvalidateRect(combo,nullptr,FALSE);
    const auto style=GetWindowLongPtrW(window,GWL_EXSTYLE);
    if((style&WS_EX_LAYERED)&&!owns_layer)return;
    if(!owns_layer){SetWindowLongPtrW(window,GWL_EXSTYLE,style|WS_EX_LAYERED);owns_layer=(GetWindowLongPtrW(window,GWL_EXSTYLE)&WS_EX_LAYERED)!=0;}
    if(!owns_layer||!SetLayeredWindowAttributes(window,0,0,LWA_ALPHA)){Stop(window);return;}
    if(!EnsureSurface()){Stop(window);return;}
    if(closing_popup==surface->window)closing_popup=nullptr;
    // Capture/show is completed after the native show transaction, when its
    // final row geometry and rounded region are available.
  }
  void ShowSurface(){
    if(rendering||!shown||!surface)return;
    if(!surface->Capture()){DestroyWindow(surface->window);Stop(list);return;}
    const auto window=surface->window;
    if(!IsWindowVisible(window)||surface->target==0){surface->Fade(255);SetWindowPos(window,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_SHOWWINDOW);}
  }
  void Hide(HWND window){
    scroll.dragging=false;refresh_pending=false;
    if(shown&&surface){
      surface->Capture();shown=false;
      // Native input/capture closes immediately. The SAME already visible
      // surface fades out; early close/reopen never swaps compositor windows.
      closing_popup=surface->window;surface->Fade(0);
    }else shown=false;
    if(owns_layer)SetLayeredWindowAttributes(window,0,0,LWA_ALPHA);
    InvalidateRect(combo,nullptr,FALSE);
  }
  static LRESULT CALLBACK Proc(HWND window,UINT message,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR data){
    auto& state=*reinterpret_cast<PopupAnimation*>(data);
    if(message==kRefresh){state.refresh_pending=false;if(state.shown&&state.surface)state.surface->Capture();return 0;}
    if(message==WM_MOUSEWHEEL&&state.Wheel(window,wp)){RequestRefresh(state.combo);return 0;}
    if(message==WM_SETCURSOR&&SetActionCursor(window,lp))return TRUE;
    if(message==WM_SHOWWINDOW){if(wp&&!state.shown)state.Start(window);else if(!wp)state.Hide(window);}
    if(message==WM_WINDOWPOSCHANGING){auto& pos=*reinterpret_cast<WINDOWPOS*>(lp);if(pos.flags&SWP_SHOWWINDOW){state.Position(window,pos);if(!state.shown)state.Start(window);}else if(pos.flags&SWP_HIDEWINDOW)state.Hide(window);}
    if(message==WM_WINDOWPOSCHANGED){
      const auto result=DefSubclassProc(window,message,wp,lp);
      if(!IsWindowVisible(window))state.Stop(window);
      if(state.shown){state.scroll.HideNative(window);if(!state.rounding){state.rounding=true;Round(window);state.rounding=false;}state.ShowSurface();}
      return result;
    }
    if(state.scroll.Input(window,message,lp)){RequestRefresh(state.combo);return 0;}
    if(message==WM_PAINT||message==WM_PRINTCLIENT){
      state.scroll.HideNative(window);const auto result=DefSubclassProc(window,message,wp,lp);
      const auto dc=message==WM_PRINTCLIENT?reinterpret_cast<HDC>(wp):GetDC(window);
      if(dc){PopupScroll::Paint(window,dc);if(message!=WM_PRINTCLIENT)ReleaseDC(window,dc);}
      RequestRefresh(state.combo);return result;
    }
    if(message==WM_NCPAINT){
      const auto result=DefSubclassProc(window,message,wp,lp);const auto dc=GetWindowDC(window);
      if(dc){RECT r;GetWindowRect(window,&r);const auto pen=CreatePen(PS_SOLID,1,RGB(48,53,64));const auto old_pen=SelectObject(dc,pen),old_brush=SelectObject(dc,GetStockObject(NULL_BRUSH));RoundRect(dc,0,0,r.right-r.left,r.bottom-r.top,6,6);SelectObject(dc,old_brush);SelectObject(dc,old_pen);DeleteObject(pen);ReleaseDC(window,dc);}
      RequestRefresh(state.combo);return result;
    }
    if(message==WM_NCDESTROY){state.shown=false;if(state.surface)DestroyWindow(state.surface->window);state.Stop(window);RemoveWindowSubclass(window,Proc,id);delete &state;}
    return DefSubclassProc(window,message,wp,lp);
  }
  static void Attach(HWND combo,bool wheel_scroll=false){
    COMBOBOXINFO info{sizeof(info)};DWORD_PTR existing=0;
    if(!GetComboBoxInfo(combo,&info)||!info.hwndList||GetWindowSubclass(info.hwndList,Proc,kSubclass,&existing))return;
    auto* state=new(std::nothrow) PopupAnimation;
    if(state){state->combo=combo;state->list=info.hwndList;state->wheel_scroll=wheel_scroll;if(!SetWindowSubclass(info.hwndList,Proc,kSubclass,reinterpret_cast<DWORD_PTR>(state)))delete state;else SetWindowTheme(info.hwndList,L"DarkMode_Explorer",nullptr);}
  }
};
}
