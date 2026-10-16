#pragma once
#include "core.h"
#include <commctrl.h>

namespace launcher {
inline constexpr UINT kNoticesCommand=0x120;
// License materials travel inside the EXE; this read-only viewer makes them
// available without dropping documentation or temporary files beside the game.
class NoticesDialog {
 public:
  HWND window{},text_control{};
  NoticesDialog(HWND owner,HFONT font,double scale,bool turkish,std::wstring text)
      :owner_(owner),font_(font),scale_(scale),turkish_(turkish){
    for(size_t i=0;i<text.size();++i){if(text[i]==L'\n'&&(i==0||text[i-1]!=L'\r'))text_+=L'\r';text_+=text[i];}
  }
  HWND Create(){
    WNDCLASSW cls{};cls.hInstance=GetModuleHandleW(nullptr);cls.lpfnWndProc=Proc;
    cls.lpszClassName=L"MotoGPThirdPartyNotices";cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    if(!GetClassInfoW(cls.hInstance,cls.lpszClassName,&cls)&&!RegisterClassW(&cls))throw std::runtime_error("Cannot register notices viewer");
    RECT rect{0,0,S(700),S(470)};const DWORD style=WS_POPUP|WS_CAPTION|WS_SYSMENU;
    AdjustWindowRectEx(&rect,style,FALSE,WS_EX_DLGMODALFRAME);RECT owner;GetWindowRect(owner_,&owner);
    window=CreateWindowExW(WS_EX_DLGMODALFRAME,cls.lpszClassName,turkish_?L"Üçüncü taraf bildirimleri":L"Third-party notices",style,
        owner.left+(owner.right-owner.left-(rect.right-rect.left))/2,owner.top+(owner.bottom-owner.top-(rect.bottom-rect.top))/2,
        rect.right-rect.left,rect.bottom-rect.top,owner_,nullptr,cls.hInstance,this);
    if(!window)throw std::runtime_error("Cannot open notices viewer");return window;
  }
  void Show(){
    Create();EnableWindow(owner_,FALSE);ShowWindow(window,SW_SHOW);SetFocus(text_control);
    MSG message{};
    while(IsWindow(window)){
      const auto result=GetMessageW(&message,nullptr,0,0);
      if(result<=0){if(result==0)PostQuitMessage(static_cast<int>(message.wParam));break;}
      if(!IsDialogMessageW(window,&message)){TranslateMessage(&message);DispatchMessageW(&message);}
    }
    if(IsWindow(window))DestroyWindow(window);
    if(IsWindow(owner_)){EnableWindow(owner_,TRUE);SetActiveWindow(owner_);}
  }
 private:
  HWND owner_;HFONT font_;double scale_;bool turkish_;std::wstring text_;
  int S(int value)const{return static_cast<int>(value*scale_+.5);}
  static LRESULT CALLBACK Proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    auto* self=reinterpret_cast<NoticesDialog*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
    if(msg==WM_NCCREATE){self=static_cast<NoticesDialog*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));self->window=hwnd;}
    if(!self)return DefWindowProcW(hwnd,msg,wp,lp);
    switch(msg){
      case WM_CREATE:{
        self->text_control=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL|WS_HSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|ES_AUTOHSCROLL,
            self->S(16),self->S(16),self->S(668),self->S(396),hwnd,reinterpret_cast<HMENU>(100),GetModuleHandleW(nullptr),nullptr);
        SendMessageW(self->text_control,EM_SETLIMITTEXT,self->text_.size()+1,0);
        SetWindowTextW(self->text_control,self->text_.c_str());SendMessageW(self->text_control,WM_SETFONT,reinterpret_cast<WPARAM>(self->font_),TRUE);
        auto close=CreateWindowExW(0,L"BUTTON",self->turkish_?L"Kapat":L"Close",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,
            self->S(570),self->S(428),self->S(114),self->S(28),hwnd,reinterpret_cast<HMENU>(IDOK),GetModuleHandleW(nullptr),nullptr);
        SendMessageW(close,WM_SETFONT,reinterpret_cast<WPARAM>(self->font_),TRUE);return 0;}
      case WM_CTLCOLORSTATIC:case WM_CTLCOLOREDIT:{auto dc=reinterpret_cast<HDC>(wp);SetTextColor(dc,RGB(235,238,242));SetBkColor(dc,RGB(24,27,34));SetDCBrushColor(dc,RGB(24,27,34));return reinterpret_cast<LRESULT>(GetStockObject(DC_BRUSH));}
      case WM_ERASEBKGND:{RECT r;GetClientRect(hwnd,&r);SetDCBrushColor(reinterpret_cast<HDC>(wp),RGB(24,27,34));FillRect(reinterpret_cast<HDC>(wp),&r,reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));return 1;}
      case WM_COMMAND:if(LOWORD(wp)==IDOK||LOWORD(wp)==IDCANCEL){DestroyWindow(hwnd);return 0;}break;
      case WM_CLOSE:DestroyWindow(hwnd);return 0;
      case WM_NCDESTROY:self->window=nullptr;SetWindowLongPtrW(hwnd,GWLP_USERDATA,0);break;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
  }
};
}
