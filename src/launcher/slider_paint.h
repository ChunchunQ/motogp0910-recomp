#pragma once
#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>
#include <algorithm>
namespace launcher {
inline RECT SliderTrack(HWND control){
  RECT channel{},thumb{};SendMessageW(control,TBM_GETCHANNELRECT,0,reinterpret_cast<LPARAM>(&channel));SendMessageW(control,TBM_GETTHUMBRECT,0,reinterpret_cast<LPARAM>(&thumb));
  // Native channel bounds include half a thumb at either end. Use the thumb
  // center's actual travel for both pointer mapping and the painted track.
  const auto half=(thumb.right-thumb.left)/2;
  channel.left+=half;channel.right=std::max(channel.left,channel.right-half-1);return channel;
}
// Native trackbars invalidate only the old/new thumb rectangles. Our channel
// fill also depends on the value, so a page click needs the full client clip.
// Expand damage before BeginPaint; keep native input, capture and notifications.
inline LRESULT CALLBACK SliderDamageProc(HWND control,UINT message,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR){
  if((message==WM_LBUTTONDOWN||message==WM_LBUTTONDBLCLK)&&IsWindowEnabled(control)){
    const auto low=SendMessageW(control,TBM_GETRANGEMIN,0,0),high=SendMessageW(control,TBM_GETRANGEMAX,0,0);
    if(high>low){
      const auto track=SliderTrack(control);const int start=track.left,end=track.right;
      const auto x=std::clamp<int>(GET_X_LPARAM(lp),start,std::max(start,end));
      const auto value=low+(end>start?MulDiv(x-start,static_cast<int>(high-low),end-start):0);
      SendMessageW(control,TBM_SETPOS,TRUE,value);
      SendMessageW(GetParent(control),WM_HSCROLL,MAKEWPARAM(TB_THUMBPOSITION,static_cast<WORD>(value)),reinterpret_cast<LPARAM>(control));
      // Coarse ranges can round to a position farther than half a thumb from
      // the pointer. Start the native drag at the new thumb center so it does
      // not interpret the original point as another page-step click.
      RECT thumb{};SendMessageW(control,TBM_GETTHUMBRECT,0,reinterpret_cast<LPARAM>(&thumb));
      lp=MAKELPARAM((thumb.left+thumb.right)/2,(thumb.top+thumb.bottom)/2);
    }
    // The native handler now hits the repositioned thumb, preserving capture,
    // keyboard navigation and normal dragging after a direct channel jump.
  }
  if(message==WM_PAINT&&GetUpdateRect(control,nullptr,FALSE))InvalidateRect(control,nullptr,FALSE);
  if(message==WM_NCDESTROY)RemoveWindowSubclass(control,SliderDamageProc,id);
  return DefSubclassProc(control,message,wp,lp);
}
inline void AttachSliderPaint(HWND control){SetWindowSubclass(control,SliderDamageProc,22,0);}
}
