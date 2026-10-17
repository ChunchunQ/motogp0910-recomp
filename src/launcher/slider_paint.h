#pragma once
#include <windows.h>
#include <commctrl.h>
namespace launcher {
// Native trackbars invalidate only the old/new thumb rectangles. Our channel
// fill also depends on the value, so a page click needs the full client clip.
// Expand damage before BeginPaint; keep native input, capture and notifications.
inline LRESULT CALLBACK SliderDamageProc(HWND control,UINT message,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR){
  if(message==WM_PAINT&&GetUpdateRect(control,nullptr,FALSE))InvalidateRect(control,nullptr,FALSE);
  if(message==WM_NCDESTROY)RemoveWindowSubclass(control,SliderDamageProc,id);
  return DefSubclassProc(control,message,wp,lp);
}
inline void AttachSliderPaint(HWND control){SetWindowSubclass(control,SliderDamageProc,22,0);}
}
