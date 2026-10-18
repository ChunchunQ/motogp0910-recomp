#pragma once
#include <windows.h>
#include <commctrl.h>
#include <algorithm>
#include <string>
#include "buffered_paint.h"
#include "popup_scroll.h"

namespace launcher {
inline constexpr COLORREF kPopupBackground=RGB(32,36,45);
inline void DrawPopupItemContents(const DRAWITEMSTRUCT& item,HFONT font,double scale){
  const auto dc=item.hDC;const int saved=SaveDC(dc);
  auto size=[&](int value){return std::max(1,static_cast<int>(value*scale+.5));};
  const bool closed=bool(item.itemState&ODS_COMBOBOXEDIT);
  auto r=item.rcItem;SetDCBrushColor(dc,kPopupBackground);
  FillRect(dc,&r,reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
  const bool selected=!closed&&bool(item.itemState&ODS_SELECTED),disabled=bool(item.itemState&ODS_DISABLED);
  if(selected){
    RECT highlight=r;InflateRect(&highlight,-size(3),-size(2));
    const auto brush=CreateSolidBrush(RGB(43,48,59));const auto old_brush=SelectObject(dc,brush);
    const auto old_pen=SelectObject(dc,GetStockObject(NULL_PEN));
    RoundRect(dc,highlight.left,highlight.top,highlight.right,highlight.bottom,size(6),size(6));
    SelectObject(dc,old_pen);SelectObject(dc,old_brush);DeleteObject(brush);
  }
  SetBkMode(dc,TRANSPARENT);SelectObject(dc,font);
  SetTextColor(dc,disabled?RGB(117,125,138):selected?RGB(255,170,48):RGB(242,244,247));
  std::wstring text;
  if(item.itemID!=UINT(-1)){
    const auto length=SendMessageW(item.hwndItem,CB_GETLBTEXTLEN,item.itemID,0);
    if(length>=0){text.resize(length+1);SendMessageW(item.hwndItem,CB_GETLBTEXT,item.itemID,reinterpret_cast<LPARAM>(text.data()));text.resize(length);}
  }
  r.left+=size(closed?8:12);r.right-=size(12);
  DrawTextW(dc,text.c_str(),-1,&r,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
  RestoreDC(dc,saved);
}
inline void DrawPopupItem(const DRAWITEMSTRUCT& item,HFONT font,double scale){
  auto bounds=item.rcItem;
  // Native hover/selection can repaint a single row synchronously, without a
  // list WM_PAINT afterwards. Keep that row's buffer out of the scrollbar so
  // its thumb cannot disappear between native list paints.
  if(!(item.itemState&ODS_COMBOBOXEDIT))bounds.right-=PopupScroll::ReservedWidth(item.hwndItem);
  PaintBuffered(item.hDC,bounds,[&](HDC dc,RECT area){
    auto buffered=item;buffered.hDC=dc;buffered.rcItem=area;
    DrawPopupItemContents(buffered,font,scale);
  });
}
}
