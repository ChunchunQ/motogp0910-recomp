#pragma once
#include "buffered_paint.h"
#include <windowsx.h>
#include <algorithm>

namespace launcher {
// ComboLBox keeps native selection/keyboard behavior. A visible in-client rail
// scrolls the native list's top index, without committing a different option.
struct PopupScroll {
  struct Geometry {RECT client{},rail{},track{},thumb{};int count=0,visible=0,top=0,maximum=0,travel=0;};
  bool dragging=false,hiding_native=false;int grab=0;
  static Geometry Measure(HWND list){
    Geometry g;GetClientRect(list,&g.client);
    const auto count=SendMessageW(list,LB_GETCOUNT,0,0);if(count<=0||g.client.bottom<=0)return g;
    int row=static_cast<int>(SendMessageW(list,LB_GETITEMHEIGHT,0,0));
    if(row<=0){RECT r;if(SendMessageW(list,LB_GETITEMRECT,0,reinterpret_cast<LPARAM>(&r))==LB_ERR)return g;row=r.bottom-r.top;}
    if(row<=0)return g;
    g.count=static_cast<int>(count);g.visible=std::max(1,int(g.client.bottom)/row);g.maximum=std::max(0,g.count-g.visible);
    g.top=std::clamp(static_cast<int>(SendMessageW(list,LB_GETTOPINDEX,0,0)),0,g.maximum);
    if(!g.maximum)return g;
    const int width=std::max(12,MulDiv(14,GetDpiForWindow(list),96));
    g.rail={g.client.right-width,0,g.client.right,g.client.bottom};
    g.track={g.rail.left+width/2-2,5,g.rail.left+width/2+2,std::max(6,int(g.client.bottom)-5)};
    const int height=g.track.bottom-g.track.top;
    const int thumb=std::min(height,std::max(20,MulDiv(height,g.visible,g.count)));g.travel=height-thumb;
    const int y=g.track.top+(g.maximum?MulDiv(g.travel,g.top,g.maximum):0);
    g.thumb={g.rail.left+3,y,g.rail.right-3,y+thumb};return g;
  }
  static int ReservedWidth(HWND combo){
    COMBOBOXINFO info{sizeof(info)};if(!GetComboBoxInfo(combo,&info))return 0;
    const auto g=Measure(info.hwndList);return g.maximum?g.rail.right-g.rail.left:0;
  }
  void HideNative(HWND list){
    if(hiding_native||!(GetWindowLongPtrW(list,GWL_STYLE)&WS_VSCROLL))return;
    hiding_native=true;ShowScrollBar(list,SB_VERT,FALSE);hiding_native=false;
  }
  static void SetTop(HWND list,int value){
    SendMessageW(list,LB_SETTOPINDEX,value,0);InvalidateRect(list,nullptr,FALSE);
  }
  void Move(HWND list,int y){
    const auto g=Measure(list);if(!g.maximum||!g.travel)return;
    const int offset=std::clamp(y-int(g.track.top)-grab,0,g.travel);
    SetTop(list,MulDiv(offset,g.maximum,g.travel));
  }
  bool Input(HWND list,UINT message,LPARAM lp){
    if(message==WM_CAPTURECHANGED){dragging=false;return false;}
    if(message==WM_MOUSEMOVE){
      if(dragging){Move(list,GET_Y_LPARAM(lp));return true;}
      const auto g=Measure(list);POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};
      // The rail is outside the item hit area: merely moving onto its thumb
      // must not change the native list's hovered/current selection.
      if(g.maximum&&PtInRect(&g.rail,p))return true;
    }
    if(message==WM_LBUTTONUP&&dragging){Move(list,GET_Y_LPARAM(lp));dragging=false;return true;}
    if(message!=WM_LBUTTONDOWN&&message!=WM_LBUTTONDBLCLK)return false;
    const auto g=Measure(list);POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};
    if(!g.maximum||!PtInRect(&g.rail,p))return false;
    dragging=true;grab=PtInRect(&g.thumb,p)?p.y-g.thumb.top:(g.thumb.bottom-g.thumb.top)/2;
    // Retain ComboLBox's existing capture; replacing/releasing it would close
    // a native dropdown in the middle of a scrollbar drag.
    Move(list,p.y);return true;
  }
  static void Paint(HWND list,HDC dc){
    const auto g=Measure(list);if(!g.maximum)return;
    PaintBuffered(dc,g.rail,[&](HDC memory,RECT area){
      SetDCBrushColor(memory,RGB(32,36,45));FillRect(memory,&area,reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
      const auto old_pen=SelectObject(memory,GetStockObject(NULL_PEN)),old_brush=SelectObject(memory,GetStockObject(DC_BRUSH));
      auto track=g.track,thumb=g.thumb;OffsetRect(&track,-g.rail.left,-g.rail.top);OffsetRect(&thumb,-g.rail.left,-g.rail.top);
      SetDCBrushColor(memory,RGB(48,53,64));RoundRect(memory,track.left,track.top,track.right,track.bottom,4,4);
      SetDCBrushColor(memory,RGB(110,121,139));RoundRect(memory,thumb.left,thumb.top,thumb.right,thumb.bottom,6,6);
      SelectObject(memory,old_brush);SelectObject(memory,old_pen);
    });
  }
};
}
