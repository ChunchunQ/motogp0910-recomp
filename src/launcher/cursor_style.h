#pragma once
#include <windows.h>
#include <commctrl.h>
namespace launcher {
inline bool SetActionCursor(HWND control,LPARAM hit){
  if(LOWORD(hit)!=HTCLIENT||!IsWindowEnabled(control))return false;
  wchar_t cls[32]{};GetClassNameW(control,cls,32);
  if(lstrcmpiW(cls,L"BUTTON")&&lstrcmpiW(cls,L"COMBOBOX")&&lstrcmpiW(cls,L"ComboLBox")&&lstrcmpiW(cls,TRACKBAR_CLASSW))return false;
  SetCursor(LoadCursorW(nullptr,IDC_HAND));return true;
}
}
