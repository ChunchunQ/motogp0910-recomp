#pragma once
#include "core.h"
#include <commctrl.h>
#include <dwmapi.h>
namespace launcher {
// Owned modal editor: it changes pending launcher settings; Save / PLAY writes
// the same keys used by the live F1 controls. Cancel leaves the profile intact.
class AdaptiveDialog {
 public:
  HWND window{};
  AdaptiveDialog(HWND owner,HFONT font,bool tr,double scale,motogp::adaptive::Profile& profile)
      :owner_(owner),font_(font),tr_(tr),scale_(scale),profile_(profile){}
  ~AdaptiveDialog(){if(window)DestroyWindow(window);}
  HWND Create(){
    WNDCLASSW cls{};cls.lpfnWndProc=Proc;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"MotoGPAdaptiveTriggers";cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    RegisterClassW(&cls);
    RECT rect{0,0,S(680),S(470)};AdjustWindowRectEx(&rect,WS_CAPTION|WS_SYSMENU,0,WS_EX_DLGMODALFRAME);
    RECT parent{};GetWindowRect(owner_,&parent);const auto w=rect.right-rect.left,h=rect.bottom-rect.top;
    window=CreateWindowExW(WS_EX_DLGMODALFRAME,cls.lpszClassName,T(L"DualSense adaptive triggers (Experimental)",L"DualSense adaptif tetikler (Deneysel)"),WS_CAPTION|WS_SYSMENU,
      parent.left+(parent.right-parent.left-w)/2,parent.top+(parent.bottom-parent.top-h)/2,w,h,owner_,nullptr,cls.hInstance,this);
    if(!window)throw std::runtime_error("Cannot open DualSense settings");
    BOOL dark=TRUE;DwmSetWindowAttribute(window,20,&dark,sizeof(dark));
    return window;
  }
  void Show(){
    Create();EnableWindow(owner_,FALSE);ShowWindow(window,SW_SHOW);SetFocus(GetDlgItem(window,100));
    MSG message{};while(window){const auto result=GetMessageW(&message,nullptr,0,0);if(result<=0){if(!result)PostQuitMessage(static_cast<int>(message.wParam));break;}
      if(!IsDialogMessageW(window,&message)){TranslateMessage(&message);DispatchMessageW(&message);}}
    EnableWindow(owner_,TRUE);SetActiveWindow(owner_);
  }
  bool Commit(){
    auto next=profile_;next.enabled=SendDlgItemMessageW(window,100,BM_GETCHECK,0,0)==BST_CHECKED;
    for(int side=0;side<2;++side){auto& p=side?next.left:next.right;const int base=110+side*10;
      const auto mode=SendDlgItemMessageW(window,base,CB_GETCURSEL,0,0);if(mode<0||mode>2)return false;
      p.mode=std::array{"off","feedback","weapon"}[mode];
      p.start=SendDlgItemMessageW(window,base+1,TBM_GETPOS,0,0);p.end=SendDlgItemMessageW(window,base+2,TBM_GETPOS,0,0);p.strength=SendDlgItemMessageW(window,base+3,TBM_GETPOS,0,0);
    }
    if(!next.Valid())return false;profile_=std::move(next);return true;
  }
 private:
  HWND owner_{};HFONT font_{};bool tr_=false;double scale_=1;motogp::adaptive::Profile& profile_;
  int S(int n)const{return static_cast<int>(n*scale_+.5);}
  const wchar_t* T(const wchar_t* en,const wchar_t* tr)const{return tr_?tr:en;}
  HWND Add(const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int w,int h,int id=0){
    auto child=CreateWindowExW(0,cls,text,WS_CHILD|WS_VISIBLE|style,S(x),S(y),S(w),S(h),window,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
    SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(font_),TRUE);return child;
  }
  void Build(){
    auto enabled=Add(L"BUTTON",T(L"Enable DualSense adaptive triggers",L"DualSense adaptif tetikleri etkinleştir"),WS_TABSTOP|BS_AUTOCHECKBOX,24,20,632,30,100);
    SendMessageW(enabled,BM_SETCHECK,profile_.enabled?BST_CHECKED:BST_UNCHECKED,0);
    Add(L"STATIC",T(L"Native DualSense / Edge + SDL required. USB or Bluetooth.\nStatic resistance profiles; physical DualSense testing is pending.",L"Yerel DualSense / Edge + SDL gerekir. USB veya Bluetooth.\nSabit direnç profilleri; fiziksel DualSense testi henüz yapılmadı."),0,24,62,632,55);
    for(int side=0;side<2;++side){const auto& p=side?profile_.left:profile_.right;int x=24+side*332,id=110+side*10;
      Add(L"STATIC",side?T(L"LEFT TRIGGER · L2",L"SOL TETİK · L2"):T(L"RIGHT TRIGGER · R2",L"SAĞ TETİK · R2"),0,x,134,300,26);
      auto mode=Add(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST|CBS_OWNERDRAWFIXED|CBS_HASSTRINGS|WS_VSCROLL,x,168,300,180,id);
      for(auto text:{T(L"Off",L"Kapalı"),T(L"Feedback (constant resistance)",L"Feedback (sabit direnç)"),T(L"Weapon (resistance section)",L"Weapon (direnç bölgesi)")})SendMessageW(mode,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));
      SendMessageW(mode,CB_SETITEMHEIGHT,static_cast<WPARAM>(-1),S(23));SendMessageW(mode,CB_SETITEMHEIGHT,0,S(23));
      SendMessageW(mode,CB_SETCURSEL,p.mode=="weapon"?2:p.mode=="feedback"?1:0,0);
      const int values[]{p.start,p.end,p.strength};
      for(int row=0;row<3;++row){const int y=214+row*48;
        Add(L"STATIC",row==0?T(L"Start position",L"Başlangıç konumu"):row==1?T(L"End (Weapon only)",L"Bitiş (yalnızca Weapon)"):T(L"Strength · 0 = off",L"Güç · 0 = kapalı"),0,x,y,250,21);
        auto bar=Add(TRACKBAR_CLASSW,L"",WS_TABSTOP|TBS_HORZ|TBS_NOTICKS,x-5,y+20,260,26,id+row+1);
        SendMessageW(bar,TBM_SETRANGE,TRUE,MAKELPARAM(0,row==2?8:9));SendMessageW(bar,TBM_SETPOS,TRUE,values[row]);
        Add(L"STATIC",std::to_wstring(values[row]).c_str(),SS_RIGHT,x+265,y+21,35,24,id+row+4);
      }
    }
    Add(L"STATIC",T(L"Weapon: end must exceed start. Other controllers keep normal triggers.",L"Weapon: bitiş başlangıçtan büyük olmalı. Diğer kollar normal çalışır."),0,24,374,632,40);
    Add(L"BUTTON",T(L"Apply",L"Uygula"),WS_TABSTOP|BS_OWNERDRAW,410,426,114,30,IDOK);
    Add(L"BUTTON",T(L"Cancel",L"İptal"),WS_TABSTOP|BS_OWNERDRAW,538,426,114,30,IDCANCEL);
  }
  static LRESULT CALLBACK Proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    auto* self=reinterpret_cast<AdaptiveDialog*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
    if(msg==WM_NCCREATE){self=static_cast<AdaptiveDialog*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);self->window=hwnd;SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));}
    if(!self)return DefWindowProcW(hwnd,msg,wp,lp);
    switch(msg){
      case WM_CREATE:self->Build();return 0;
      case WM_ERASEBKGND:{RECT r;GetClientRect(hwnd,&r);auto dc=reinterpret_cast<HDC>(wp);SetDCBrushColor(dc,RGB(24,27,34));FillRect(dc,&r,reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));return 1;}
      case WM_CTLCOLORSTATIC:case WM_CTLCOLORBTN:case WM_CTLCOLORLISTBOX:{auto dc=reinterpret_cast<HDC>(wp);SetTextColor(dc,RGB(235,238,242));SetBkColor(dc,RGB(24,27,34));SetDCBrushColor(dc,RGB(24,27,34));return reinterpret_cast<LRESULT>(GetStockObject(DC_BRUSH));}
      case WM_MEASUREITEM:{auto* item=reinterpret_cast<MEASUREITEMSTRUCT*>(lp);if(item->CtlType==ODT_COMBOBOX){item->itemHeight=self->S(23);return TRUE;}break;}
      case WM_DRAWITEM:{auto* item=reinterpret_cast<DRAWITEMSTRUCT*>(lp);auto dc=item->hDC;auto rect=item->rcItem;const bool primary=item->CtlType==ODT_BUTTON&&item->CtlID==IDOK;
        SetDCBrushColor(dc,primary?RGB(255,170,48):RGB(32,36,45));FillRect(dc,&rect,reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        SetBkMode(dc,TRANSPARENT);SetTextColor(dc,primary?RGB(13,15,20):RGB(242,244,247));auto old=SelectObject(dc,self->font_);std::wstring text;
        if(item->CtlType==ODT_COMBOBOX&&item->itemID!=static_cast<UINT>(-1)){const auto length=SendMessageW(item->hwndItem,CB_GETLBTEXTLEN,item->itemID,0);if(length>=0){text.resize(length+1);SendMessageW(item->hwndItem,CB_GETLBTEXT,item->itemID,reinterpret_cast<LPARAM>(text.data()));text.resize(length);}}
        else if(item->CtlType==ODT_BUTTON){wchar_t title[64]{};GetWindowTextW(item->hwndItem,title,64);text=title;}
        rect.left+=self->S(6);rect.right-=self->S(6);DrawTextW(dc,text.c_str(),-1,&rect,DT_SINGLELINE|DT_VCENTER|(item->CtlType==ODT_BUTTON?DT_CENTER:DT_LEFT)|DT_END_ELLIPSIS);
        if(item->itemState&ODS_FOCUS){auto focus=item->rcItem;InflateRect(&focus,-3,-3);DrawFocusRect(dc,&focus);}SelectObject(dc,old);return TRUE;}
      case WM_HSCROLL:{auto c=reinterpret_cast<HWND>(lp);const int id=GetDlgCtrlID(c);if((id>=111&&id<=113)||(id>=121&&id<=123))SetDlgItemTextW(hwnd,id+3,std::to_wstring(SendMessageW(c,TBM_GETPOS,0,0)).c_str());return 0;}
      case WM_COMMAND:if(LOWORD(wp)==IDOK){if(self->Commit())DestroyWindow(hwnd);else MessageBoxW(hwnd,self->T(L"Weapon end must be greater than start.",L"Weapon bitişi başlangıçtan büyük olmalı."),L"DualSense",MB_OK|MB_ICONINFORMATION);return 0;}
        if(LOWORD(wp)==IDCANCEL){DestroyWindow(hwnd);return 0;}break;
      case WM_CLOSE:DestroyWindow(hwnd);return 0;
      case WM_NCDESTROY:self->window=nullptr;SetWindowLongPtrW(hwnd,GWLP_USERDATA,0);break;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
  }
};
}
