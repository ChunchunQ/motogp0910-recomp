#pragma once
#include "core.h"
#include "popup_animation.h"
#include "popup_style.h"
#include "checkbox_paint.h"
#include "slider_paint.h"
#include <commctrl.h>
#include <uxtheme.h>
#include <map>
namespace launcher {
// Shared native modal styling; no wheel changes on a closed selector.
struct ModalStyle {
  HFONT font;double scale;
  std::map<HWND,bool> checks;
  int S(int n)const{return static_cast<int>(n*scale+.5);}
  void PaintCombo(HDC target,HWND control)const{
    RECT bounds;GetClientRect(control,&bounds);
    PaintBuffered(target,bounds,[&](HDC dc,RECT){
    RECT r;GetClientRect(control,&r);
      SetDCBrushColor(dc,RGB(32,36,45));FillRect(dc,&r,reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
      const bool open=SendMessageW(control,CB_GETDROPPEDSTATE,0,0)!=0;
      const auto pen=CreatePen(PS_SOLID,1,open?kLauncherAccent:RGB(62,68,80));const auto old_pen=SelectObject(dc,pen);const auto old_brush=SelectObject(dc,GetStockObject(NULL_BRUSH));RoundRect(dc,0,0,r.right,r.bottom,S(6),S(6));
      SelectObject(dc,old_brush);SelectObject(dc,old_pen);DeleteObject(pen);
      const auto previous=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,IsWindowEnabled(control)?RGB(242,244,247):RGB(117,125,138));
      const int length=GetWindowTextLengthW(control);std::wstring text(length+1,L'\0');text.resize(GetWindowTextW(control,text.data(),static_cast<int>(text.size())));
      RECT label{S(8),0,r.right-S(27),r.bottom};DrawTextW(dc,text.c_str(),-1,&label,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX);SelectObject(dc,previous);
      const auto arrow=CreatePen(PS_SOLID,std::max(1,S(1)),RGB(157,165,178));const auto old=SelectObject(dc,arrow);const int x=r.right-S(13),y=r.bottom/2;
      MoveToEx(dc,x-S(4),y+(open?S(2):-S(2)),nullptr);LineTo(dc,x,y+(open?-S(2):S(2)));LineTo(dc,x+S(4),y+(open?S(2):-S(2)));SelectObject(dc,old);DeleteObject(arrow);
    });
  }
  static LRESULT CALLBACK ComboProc(HWND control,UINT message,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR data){
    auto& style=*reinterpret_cast<ModalStyle*>(data);
    if(PopupAnimation::RoutePointer(control,message,wp,lp))return 0;
    if(message==WM_MOUSEWHEEL&&!SendMessageW(control,CB_GETDROPPEDSTATE,0,0))return 0;
    if(message==WM_ERASEBKGND)return 1;
    if(message==WM_PAINT){
      PAINTSTRUCT ps;const auto dc=BeginPaint(control,&ps);style.PaintCombo(dc,control);
      EndPaint(control,&ps);return 0;
    }
    if(message==WM_PRINTCLIENT){style.PaintCombo(reinterpret_cast<HDC>(wp),control);return 0;}
    if(message==WM_NCDESTROY)RemoveWindowSubclass(control,ComboProc,id);
    return DefSubclassProc(control,message,wp,lp);
  }
  void AttachCombo(HWND control){SendMessageW(control,CB_SETITEMHEIGHT,0,S(32));PopupAnimation::Attach(control);SetWindowSubclass(control,ComboProc,19,reinterpret_cast<DWORD_PTR>(this));}
  static LRESULT CALLBACK CheckboxProc(HWND control,UINT message,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR data){
    auto& style=*reinterpret_cast<ModalStyle*>(data);
    if(message==BM_GETCHECK)return style.checks.at(control)?BST_CHECKED:BST_UNCHECKED;
    if(message==BM_SETCHECK){style.checks.at(control)=wp==BST_CHECKED;InvalidateRect(control,nullptr,FALSE);return 0;}
    if(message==WM_LBUTTONDBLCLK)return DefSubclassProc(control,WM_LBUTTONDOWN,wp,lp);
    if(message==WM_ERASEBKGND)return 1;
    if(message==WM_PAINT||message==WM_PRINTCLIENT){
      PAINTSTRUCT paint{};const auto target=message==WM_PAINT?BeginPaint(control,&paint):reinterpret_cast<HDC>(wp);
      style.PaintCheckboxControl(target,control);if(message==WM_PAINT)EndPaint(control,&paint);return 0;
    }
    if(message==WM_NCDESTROY){style.checks.erase(control);RemoveWindowSubclass(control,CheckboxProc,id);}
    return DefSubclassProc(control,message,wp,lp);
  }
  void PaintCheckboxControl(HDC target,HWND control)const{
      RECT bounds;GetClientRect(control,&bounds);
      PaintBuffered(target,bounds,[&](HDC dc,RECT){const int saved=SaveDC(dc);
      RECT r;GetClientRect(control,&r);SetDCBrushColor(dc,RGB(23,26,33));FillRect(dc,&r,reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
      RECT box{S(1),(r.bottom-S(18))/2,S(19),(r.bottom+S(18))/2};
      PaintCheckbox(dc,box,SendMessageW(control,BM_GETCHECK,0,0)==BST_CHECKED,IsWindowEnabled(control),bool(SendMessageW(control,BM_GETSTATE,0,0)&BST_PUSHED));
      std::wstring text(GetWindowTextLengthW(control)+1,L'\0');text.resize(GetWindowTextW(control,text.data(),static_cast<int>(text.size())));
      SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,IsWindowEnabled(control)?RGB(242,244,247):RGB(117,125,138));r.left+=S(29);
      DrawTextW(dc,text.c_str(),-1,&r,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
      RestoreDC(dc,saved);});
  }
  void AttachCheckbox(HWND control){checks.emplace(control,false);SetWindowSubclass(control,CheckboxProc,26,reinterpret_cast<DWORD_PTR>(this));}
  LRESULT DrawSlider(NMCUSTOMDRAW& draw)const{
    if(draw.dwDrawStage!=CDDS_PREPAINT)return CDRF_DODEFAULT;
    const int saved=SaveDC(draw.hdc);RECT r,thumb{};const auto channel=SliderTrack(draw.hdr.hwndFrom);GetClientRect(draw.hdr.hwndFrom,&r);
    SetDCBrushColor(draw.hdc,RGB(23,26,33));FillRect(draw.hdc,&r,reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    SendMessageW(draw.hdr.hwndFrom,TBM_GETTHUMBRECT,0,reinterpret_cast<LPARAM>(&thumb));
    const int cy=r.bottom/2,cx=(thumb.left+thumb.right)/2;const bool enabled=IsWindowEnabled(draw.hdr.hwndFrom);
    RECT track{channel.left,cy-S(2),channel.right,cy+S(2)};SetDCBrushColor(draw.hdc,RGB(48,53,64));FillRect(draw.hdc,&track,reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    const auto color=enabled?kLauncherAccent:RGB(85,93,107);track.right=cx;SetDCBrushColor(draw.hdc,color);FillRect(draw.hdc,&track,reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    SelectObject(draw.hdc,GetStockObject(DC_BRUSH));SelectObject(draw.hdc,GetStockObject(NULL_PEN));Ellipse(draw.hdc,cx-S(5),cy-S(5),cx+S(5),cy+S(5));
    RestoreDC(draw.hdc,saved);return CDRF_SKIPDEFAULT;
  }
  LRESULT Color(HDC dc,HWND control)const{
    wchar_t cls[32]{};GetClassNameW(control,cls,32);const bool field=lstrcmpiW(cls,L"EDIT")==0;
    SetTextColor(dc,IsWindowEnabled(control)?RGB(235,238,242):RGB(117,125,138));const auto color=field?RGB(32,36,45):lstrcmpiW(cls,L"ComboLBox")==0?kPopupBackground:RGB(23,26,33);SetBkColor(dc,color);SetDCBrushColor(dc,color);return reinterpret_cast<LRESULT>(GetStockObject(DC_BRUSH));
  }
  void DrawItem(const DRAWITEMSTRUCT& item)const{
    if(checks.contains(item.hwndItem)){PaintCheckboxControl(item.hDC,item.hwndItem);return;}
    if(item.CtlType==ODT_COMBOBOX){DrawPopupItem(item,font,scale);PopupAnimation::RequestRefresh(item.hwndItem);return;}
    PaintBuffered(item.hDC,item.rcItem,[&](HDC dc,RECT area){auto buffered=item;buffered.hDC=dc;buffered.rcItem=area;DrawItemContents(buffered);});
  }
  void DrawItemContents(const DRAWITEMSTRUCT& item)const{
    const auto dc=item.hDC;auto r=item.rcItem;const int saved=SaveDC(dc);const bool button=item.CtlType==ODT_BUTTON,primary=button&&item.CtlID==IDOK,disabled=(item.itemState&ODS_DISABLED)!=0;
    const auto fill=disabled?RGB(40,43,50):primary?kLauncherAccent:(item.itemState&ODS_SELECTED)?RGB(57,35,28):RGB(32,36,45);
    SetDCBrushColor(dc,fill);FillRect(dc,&r,reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));SetBkMode(dc,TRANSPARENT);SetTextColor(dc,disabled?RGB(117,125,138):primary?RGB(13,15,20):RGB(242,244,247));SelectObject(dc,font);
    std::wstring text;
    if(item.CtlType==ODT_COMBOBOX&&item.itemID!=UINT(-1)){const auto length=SendMessageW(item.hwndItem,CB_GETLBTEXTLEN,item.itemID,0);if(length>=0){text.resize(length+1);SendMessageW(item.hwndItem,CB_GETLBTEXT,item.itemID,reinterpret_cast<LPARAM>(text.data()));text.resize(length);}}
    else if(button){text.resize(GetWindowTextLengthW(item.hwndItem)+1);text.resize(GetWindowTextW(item.hwndItem,text.data(),static_cast<int>(text.size())));}
    r.left+=S(8);r.right-=S(8);DrawTextW(dc,text.c_str(),-1,&r,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX|(button?DT_CENTER:DT_LEFT));
    RestoreDC(dc,saved);
  }
};
}
