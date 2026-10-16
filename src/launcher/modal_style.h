#pragma once
#include "core.h"
#include <commctrl.h>
#include <uxtheme.h>
namespace launcher {
// Shared native modal styling; no wheel changes on a closed selector.
struct ModalStyle {
  HFONT font;double scale;
  int S(int n)const{return static_cast<int>(n*scale+.5);}
  static LRESULT CALLBACK ComboProc(HWND control,UINT message,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR data){
    auto& style=*reinterpret_cast<ModalStyle*>(data);
    if(message==WM_MOUSEWHEEL&&!SendMessageW(control,CB_GETDROPPEDSTATE,0,0))return 0;
    if(message==WM_ERASEBKGND)return 1;
    if(message==WM_PAINT){
      PAINTSTRUCT ps;const auto dc=BeginPaint(control,&ps);RECT r;GetClientRect(control,&r);
      SetDCBrushColor(dc,RGB(32,36,45));FillRect(dc,&r,reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
      const auto pen=CreatePen(PS_SOLID,1,GetFocus()==control?RGB(255,170,48):RGB(62,68,80));const auto old_pen=SelectObject(dc,pen);const auto old_brush=SelectObject(dc,GetStockObject(NULL_BRUSH));Rectangle(dc,0,0,r.right,r.bottom);
      SelectObject(dc,old_brush);SelectObject(dc,old_pen);DeleteObject(pen);
      const auto previous=SelectObject(dc,style.font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,IsWindowEnabled(control)?RGB(242,244,247):RGB(117,125,138));
      const int length=GetWindowTextLengthW(control);std::wstring text(length+1,L'\0');text.resize(GetWindowTextW(control,text.data(),static_cast<int>(text.size())));
      RECT label{style.S(8),0,r.right-style.S(27),r.bottom};DrawTextW(dc,text.c_str(),-1,&label,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX);SelectObject(dc,previous);
      const auto arrow=CreatePen(PS_SOLID,std::max(1,style.S(1)),RGB(157,165,178));const auto old=SelectObject(dc,arrow);const int x=r.right-style.S(13),y=r.bottom/2;
      MoveToEx(dc,x-style.S(4),y-style.S(2),nullptr);LineTo(dc,x,y+style.S(2));LineTo(dc,x+style.S(4),y-style.S(2));SelectObject(dc,old);DeleteObject(arrow);
      EndPaint(control,&ps);return 0;
    }
    if(message==WM_NCDESTROY)RemoveWindowSubclass(control,ComboProc,id);
    return DefSubclassProc(control,message,wp,lp);
  }
  void AttachCombo(HWND control){SetWindowSubclass(control,ComboProc,19,reinterpret_cast<DWORD_PTR>(this));}
  LRESULT DrawSlider(NMCUSTOMDRAW& draw)const{
    if(draw.dwDrawStage!=CDDS_PREPAINT)return CDRF_DODEFAULT;
    const int saved=SaveDC(draw.hdc);RECT r,channel{},thumb{};GetClientRect(draw.hdr.hwndFrom,&r);
    SetDCBrushColor(draw.hdc,RGB(23,26,33));FillRect(draw.hdc,&r,reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    SendMessageW(draw.hdr.hwndFrom,TBM_GETCHANNELRECT,0,reinterpret_cast<LPARAM>(&channel));SendMessageW(draw.hdr.hwndFrom,TBM_GETTHUMBRECT,0,reinterpret_cast<LPARAM>(&thumb));
    const int cy=r.bottom/2,cx=(thumb.left+thumb.right)/2;const bool enabled=IsWindowEnabled(draw.hdr.hwndFrom);
    RECT track{channel.left,cy-S(2),channel.right,cy+S(2)};SetDCBrushColor(draw.hdc,RGB(48,53,64));FillRect(draw.hdc,&track,reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    const auto color=enabled?RGB(255,170,48):RGB(85,93,107);track.right=cx;SetDCBrushColor(draw.hdc,color);FillRect(draw.hdc,&track,reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    SelectObject(draw.hdc,GetStockObject(DC_BRUSH));SelectObject(draw.hdc,GetStockObject(NULL_PEN));Ellipse(draw.hdc,cx-S(5),cy-S(5),cx+S(5),cy+S(5));
    if(GetFocus()==draw.hdr.hwndFrom){InflateRect(&r,-S(2),-S(2));DrawFocusRect(draw.hdc,&r);}RestoreDC(draw.hdc,saved);return CDRF_SKIPDEFAULT;
  }
  LRESULT Color(HDC dc,HWND control)const{
    wchar_t cls[32]{};GetClassNameW(control,cls,32);const bool field=std::wstring_view(cls)==L"EDIT";
    SetTextColor(dc,IsWindowEnabled(control)?RGB(235,238,242):RGB(117,125,138));const auto color=field?RGB(32,36,45):RGB(23,26,33);SetBkColor(dc,color);SetDCBrushColor(dc,color);return reinterpret_cast<LRESULT>(GetStockObject(DC_BRUSH));
  }
  void DrawItem(const DRAWITEMSTRUCT& item)const{
    const auto dc=item.hDC;auto r=item.rcItem;const int saved=SaveDC(dc);const bool button=item.CtlType==ODT_BUTTON,primary=button&&item.CtlID==IDOK,disabled=(item.itemState&ODS_DISABLED)!=0;
    const auto fill=disabled?RGB(40,43,50):primary?RGB(255,170,48):(item.itemState&ODS_SELECTED)?RGB(57,47,35):RGB(32,36,45);
    SetDCBrushColor(dc,fill);FillRect(dc,&r,reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));SetBkMode(dc,TRANSPARENT);SetTextColor(dc,disabled?RGB(117,125,138):primary?RGB(13,15,20):RGB(242,244,247));SelectObject(dc,font);
    std::wstring text;
    if(item.CtlType==ODT_COMBOBOX&&item.itemID!=UINT(-1)){const auto length=SendMessageW(item.hwndItem,CB_GETLBTEXTLEN,item.itemID,0);if(length>=0){text.resize(length+1);SendMessageW(item.hwndItem,CB_GETLBTEXT,item.itemID,reinterpret_cast<LPARAM>(text.data()));text.resize(length);}}
    else if(button){text.resize(GetWindowTextLengthW(item.hwndItem)+1);text.resize(GetWindowTextW(item.hwndItem,text.data(),static_cast<int>(text.size())));}
    r.left+=S(8);r.right-=S(8);DrawTextW(dc,text.c_str(),-1,&r,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX|(button?DT_CENTER:DT_LEFT));
    if(item.itemState&ODS_FOCUS){auto focus=item.rcItem;InflateRect(&focus,-S(3),-S(3));DrawFocusRect(dc,&focus);}RestoreDC(dc,saved);
  }
};
}
