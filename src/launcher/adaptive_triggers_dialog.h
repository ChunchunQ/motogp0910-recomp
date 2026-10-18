#pragma once
#include "core.h"
#include "modal_style.h"
#include "tooltips.h"
#include "slider_paint.h"
#include <commctrl.h>
#include <dwmapi.h>
namespace launcher {
// Owned modal editor: it changes pending launcher settings; Save / PLAY writes
// the same keys used by the live F1 controls. Cancel leaves the profile intact.
class AdaptiveDialog {
 public:
  HWND window{};
  AdaptiveDialog(HWND owner,HFONT font,bool tr,double scale,motogp::adaptive::Profile& profile)
      :owner_(owner),font_(font),tr_(tr),scale_(scale),profile_(profile),style_{font,scale}{}
  ~AdaptiveDialog(){if(window)DestroyWindow(window);}
  HWND Create(){
    WNDCLASSW cls{};cls.lpfnWndProc=Proc;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"MotoGPAdaptiveTriggers";cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    RegisterClassW(&cls);
    RECT rect{0,0,S(680),S(418)};AdjustWindowRectEx(&rect,WS_CAPTION|WS_SYSMENU,0,WS_EX_DLGMODALFRAME);
    RECT parent{};GetWindowRect(owner_,&parent);const auto w=rect.right-rect.left,h=rect.bottom-rect.top;
    window=CreateWindowExW(WS_EX_DLGMODALFRAME,cls.lpszClassName,T(L"DualSense adaptive triggers (Experimental)",L"DualSense adaptif tetikler (Deneysel)"),WS_CAPTION|WS_SYSMENU|WS_CLIPCHILDREN,
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
  HWND owner_{};HFONT font_{};bool tr_=false;double scale_=1;motogp::adaptive::Profile& profile_;ModalStyle style_;Tooltips help_;
  int S(int n)const{return static_cast<int>(n*scale_+.5);}
  const wchar_t* T(const wchar_t* en,const wchar_t* tr)const{return tr_?tr:en;}
  HWND Add(const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int w,int h,int id=0){
    auto child=CreateWindowExW(0,cls,text,WS_CHILD|WS_VISIBLE|style,S(x),S(y),S(w),S(h),window,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
    SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(font_),TRUE);
    if(std::wstring_view(cls)==L"COMBOBOX")style_.AttachCombo(child);
    if(std::wstring_view(cls)==L"BUTTON"&&(style&BS_TYPEMASK)==BS_AUTOCHECKBOX)style_.AttachCheckbox(child);
    if(std::wstring_view(cls)==TRACKBAR_CLASSW)AttachSliderPaint(child);
    return child;
  }
  void UpdateAvailability(){
    const bool enabled=SendDlgItemMessageW(window,100,BM_GETCHECK,0,0)==BST_CHECKED;
    for(int side=0;side<2;++side){const int base=110+side*10;const auto mode=SendDlgItemMessageW(window,base,CB_GETCURSEL,0,0);
      EnableWindow(GetDlgItem(window,base),enabled);
      for(int row=1;row<=3;++row){const bool active=enabled&&mode>0&&(row!=2||mode==2);EnableWindow(GetDlgItem(window,base+row),active);EnableWindow(GetDlgItem(window,base+row+3),active);}
    }
  }
  void Build(){
    help_.Create(window,scale_);
    auto enabled=Add(L"BUTTON",T(L"Enable DualSense adaptive triggers",L"DualSense adaptif tetikleri etkinleştir"),WS_TABSTOP|BS_AUTOCHECKBOX,24,20,632,30,100);
    SendMessageW(enabled,BM_SETCHECK,profile_.enabled?BST_CHECKED:BST_UNCHECKED,0);
    const auto requirement=Add(L"STATIC",T(L"Requires DualSense / Edge and SDL",L"DualSense / Edge ve SDL gerekir"),0,24,62,632,24);
    const auto hardware_tip=T(L"USB or Bluetooth. Fixed resistance for acceleration / braking; it does not respond to speed or RPM. Other controllers keep normal triggers.",L"USB veya Bluetooth. Gaz / fren için sabit dirençtir; hıza veya motor devrine göre değişmez. Diğer kontrolcülerin tetikleri normal çalışır.");
    help_.Set(enabled,hardware_tip);help_.Set(requirement,hardware_tip);
    for(int side=0;side<2;++side){const auto& p=side?profile_.left:profile_.right;int x=24+side*332,id=110+side*10;
      Add(L"STATIC",side?T(L"LEFT TRIGGER · L2",L"SOL TETİK · L2"):T(L"RIGHT TRIGGER · R2",L"SAĞ TETİK · R2"),0,x,110,300,26);
      auto mode=Add(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST|CBS_OWNERDRAWFIXED|CBS_HASSTRINGS|WS_VSCROLL,x,144,300,180,id);
      for(auto text:{T(L"Off",L"Kapalı"),T(L"Constant resistance",L"Sabit direnç"),T(L"Resistance zone",L"Direnç aralığı")})SendMessageW(mode,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));
      SendMessageW(mode,CB_SETITEMHEIGHT,static_cast<WPARAM>(-1),S(23));SendMessageW(mode,CB_SETITEMHEIGHT,0,S(23));
      help_.Set(mode,T(L"Constant resistance holds a single resistance level. Resistance zone limits it to the selected start and end positions.",L"Sabit direnç, tek bir direnç düzeyi uygular. Direnç aralığı, başlangıç ve bitiş konumları arasına sınırlar."));
      SendMessageW(mode,CB_SETCURSEL,p.mode=="weapon"?2:p.mode=="feedback"?1:0,0);
      const int values[]{p.start,p.end,p.strength};
      for(int row=0;row<3;++row){const int y=190+row*48;
        const auto label=Add(L"STATIC",row==0?T(L"Start position",L"Başlangıç konumu"):row==1?T(L"End (zone only)",L"Bitiş (direnç aralığı)"):T(L"Strength · 0 = off",L"Güç · 0 = kapalı"),0,x,y,250,21);
        auto bar=Add(TRACKBAR_CLASSW,L"",WS_TABSTOP|TBS_HORZ|TBS_NOTICKS,x-5,y+20,260,26,id+row+1);
        const auto tip=row==2?T(L"Zero strength releases resistance. Supported strength: 0–8.",L"Sıfır güç direnci kaldırır. Desteklenen güç: 0–8."):T(L"Positions 0–9 run from released to fully pressed. Resistance-zone end must be greater than start.",L"Konum 0–9, serbestten tam basılıya uzanır. Direnç aralığının bitişi başlangıçtan büyük olmalı.");
        help_.Set(bar,tip);help_.Set(label,tip);
        SendMessageW(bar,TBM_SETRANGE,TRUE,MAKELPARAM(0,row==2?8:9));SendMessageW(bar,TBM_SETPOS,TRUE,values[row]);
        Add(L"STATIC",std::to_wstring(values[row]).c_str(),SS_RIGHT,x+265,y+21,35,24,id+row+4);
      }
    }
    Add(L"BUTTON",T(L"Apply",L"Uygula"),WS_TABSTOP|BS_OWNERDRAW,410,374,114,30,IDOK);
    Add(L"BUTTON",T(L"Cancel",L"İptal"),WS_TABSTOP|BS_OWNERDRAW,538,374,114,30,IDCANCEL);UpdateAvailability();
  }
  static LRESULT CALLBACK Proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    auto* self=reinterpret_cast<AdaptiveDialog*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
    if(msg==WM_NCCREATE){self=static_cast<AdaptiveDialog*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);self->window=hwnd;SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));}
    if(!self)return DefWindowProcW(hwnd,msg,wp,lp);
    switch(msg){
      case WM_CREATE:self->Build();return 0;
      case WM_ERASEBKGND:{RECT r;GetClientRect(hwnd,&r);auto dc=reinterpret_cast<HDC>(wp);SetDCBrushColor(dc,RGB(23,26,33));FillRect(dc,&r,reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));return 1;}
      case WM_CTLCOLORSTATIC:case WM_CTLCOLORBTN:case WM_CTLCOLORLISTBOX:return self->style_.Color(reinterpret_cast<HDC>(wp),reinterpret_cast<HWND>(lp));
      case WM_MEASUREITEM:{auto* item=reinterpret_cast<MEASUREITEMSTRUCT*>(lp);if(item->CtlType==ODT_COMBOBOX){item->itemHeight=self->S(23);return TRUE;}break;}
      case WM_DRAWITEM:self->style_.DrawItem(*reinterpret_cast<DRAWITEMSTRUCT*>(lp));return TRUE;
      case WM_NOTIFY:{const auto* header=reinterpret_cast<NMHDR*>(lp);const int id=GetDlgCtrlID(header->hwndFrom);
        if(header->code==NM_CUSTOMDRAW&&((id>=111&&id<=113)||(id>=121&&id<=123)))return self->style_.DrawSlider(*reinterpret_cast<NMCUSTOMDRAW*>(lp));break;}
      case WM_HSCROLL:{auto c=reinterpret_cast<HWND>(lp);const int id=GetDlgCtrlID(c);if((id>=111&&id<=113)||(id>=121&&id<=123))SetDlgItemTextW(hwnd,id+3,std::to_wstring(SendMessageW(c,TBM_GETPOS,0,0)).c_str());return 0;}
      case WM_COMMAND:if(LOWORD(wp)==100||((LOWORD(wp)==110||LOWORD(wp)==120)&&HIWORD(wp)==CBN_SELCHANGE)){self->UpdateAvailability();return 0;}
        if(LOWORD(wp)==IDOK){if(self->Commit())DestroyWindow(hwnd);else MessageBoxW(hwnd,self->T(L"Resistance-zone end must be greater than start.",L"Direnç aralığının bitişi başlangıçtan büyük olmalı."),L"DualSense",MB_OK|MB_ICONINFORMATION);return 0;}
        if(LOWORD(wp)==IDCANCEL){DestroyWindow(hwnd);return 0;}break;
      case WM_CLOSE:DestroyWindow(hwnd);return 0;
      case WM_NCDESTROY:self->window=nullptr;SetWindowLongPtrW(hwnd,GWLP_USERDATA,0);break;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
  }
};
}
