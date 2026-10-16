#pragma once
#include "core.h"
#include "ui_ids.h"
#include "modal_style.h"
#include "tooltips.h"
#include <dwmapi.h>
namespace launcher {
// Edits only the pending network subset. Apply never overwrites unsaved
// graphics/input options; Save / PLAY uses the existing config and arguments.
class NetworkDialog {
 public:
  HWND window{};
  NetworkDialog(HWND owner,HFONT font,bool tr,double scale,Settings& pending)
      :owner_(owner),style_{font,scale},tr_(tr),pending_(pending){}
  ~NetworkDialog(){if(window)DestroyWindow(window);}
  static void CopyNetwork(Settings& to,const Settings& from){
    to.network_mode=from.network_mode;to.lan_enabled=from.network_mode=="lan";to.online_server=from.online_server;
    to.lan_player_name=from.lan_player_name;to.lan_room_name=from.lan_room_name;to.lan_bind_ip=from.lan_bind_ip;to.lan_host_ip=from.lan_host_ip;
  }
  HWND Create(){
    WNDCLASSW cls{};cls.lpfnWndProc=Proc;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"MotoGPNetworkSettings";cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);RegisterClassW(&cls);
    RECT r{0,0,S(640),S(pending_.network_mode=="offline"?158:316)};constexpr DWORD flags=WS_CAPTION|WS_SYSMENU|WS_CLIPCHILDREN;
    AdjustWindowRectEx(&r,flags,FALSE,WS_EX_DLGMODALFRAME);RECT owner;GetWindowRect(owner_,&owner);
    const int width=r.right-r.left,height=r.bottom-r.top;
    window=CreateWindowExW(WS_EX_DLGMODALFRAME,cls.lpszClassName,T(L"Multiplayer Settings",L"Çok Oyunculu Ayarları"),flags,owner.left+(owner.right-owner.left-width)/2,owner.top+(owner.bottom-owner.top-height)/2,width,height,owner_,nullptr,cls.hInstance,this);
    if(!window)throw std::runtime_error("Cannot open multiplayer settings");BOOL dark=TRUE;DwmSetWindowAttribute(window,20,&dark,sizeof(dark));return window;
  }
  void Show(){
    Create();EnableWindow(owner_,FALSE);ShowWindow(window,SW_SHOW);SetFocus(GetDlgItem(window,kLanEnabled));MSG message{};
    while(window){const auto result=GetMessageW(&message,nullptr,0,0);if(result<=0){if(!result)PostQuitMessage(static_cast<int>(message.wParam));break;}if(!IsDialogMessageW(window,&message)){TranslateMessage(&message);DispatchMessageW(&message);}}
    EnableWindow(owner_,TRUE);SetActiveWindow(owner_);
  }
  bool Commit(std::wstring* error=nullptr){
    try{
      const auto selected=SendDlgItemMessageW(window,kLanEnabled,CB_GETCURSEL,0,0);if(selected<0||selected>2)throw std::runtime_error("Invalid connection mode");
      Settings next;next.network_mode=std::array{"offline","lan","online"}[selected];next.lan_enabled=selected==1;
      next.lan_player_name=Utf8(Text(kLanPlayer));next.lan_room_name=Utf8(Text(kLanRoom));next.lan_bind_ip=Utf8(Text(kLanAdapter));next.lan_host_ip=Utf8(Text(kLanHost));next.online_server=Utf8(Text(kOnlineServer));
      next.Validate();CopyNetwork(pending_,next);return true;
    }catch(const Error& e){if(error)*error=tr_?e.turkish:e.english;}
    catch(const std::exception& e){if(error)*error=Wide(e.what());}return false;
  }
  void UpdateMode(){
    help_.Pop();
    const auto mode=SendDlgItemMessageW(window,kLanEnabled,CB_GETCURSEL,0,0);
    const auto focus=GetFocus();
    for(int id:{kLanPlayer,kLanRoom,kLanAdapter,kLanHost,kOnlineServer}){
      const bool visible=id==kOnlineServer?mode==2:(id==kLanAdapter||id==kLanHost)?mode==1:mode>0;
      const auto control=GetDlgItem(window,id);
      if(!visible&&focus==control)SetFocus(GetDlgItem(window,kLanEnabled));
      EnableWindow(control,visible);ShowWindow(control,visible?SW_SHOW:SW_HIDE);
      ShowWindow(field_labels_.at(id),visible?SW_SHOW:SW_HIDE);
    }
    const int actions=mode>0?268:110;
    for(auto [id,x]:{std::pair{IDOK,374},std::pair{IDCANCEL,500}})SetWindowPos(GetDlgItem(window,id),nullptr,S(x),S(actions),S(116),S(32),SWP_NOZORDER|SWP_NOACTIVATE);
    RECT bounds{0,0,S(640),S(mode>0?316:158)},position;
    AdjustWindowRectEx(&bounds,static_cast<DWORD>(GetWindowLongPtrW(window,GWL_STYLE)),FALSE,WS_EX_DLGMODALFRAME);
    GetWindowRect(window,&position);MONITORINFO monitor{sizeof(monitor)};GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&monitor);
    const int width=bounds.right-bounds.left,height=bounds.bottom-bounds.top;
    SetWindowPos(window,nullptr,std::clamp<int>(position.left,monitor.rcWork.left,std::max<int>(monitor.rcWork.left,monitor.rcWork.right-width)),
        std::clamp<int>(position.top,monitor.rcWork.top,std::max<int>(monitor.rcWork.top,monitor.rcWork.bottom-height)),width,height,SWP_NOZORDER|SWP_NOACTIVATE);
    RedrawWindow(window,nullptr,nullptr,RDW_ERASE|RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW);
  }

 private:
  HWND owner_{};ModalStyle style_;bool tr_;Settings& pending_;
  Tooltips help_;std::map<int,HWND> field_labels_;
  int S(int n)const{return style_.S(n);}
  const wchar_t* T(const wchar_t* en,const wchar_t* tr)const{return tr_?tr:en;}
  std::wstring Text(int id)const{const auto c=GetDlgItem(window,id);std::wstring text(GetWindowTextLengthW(c)+1,L'\0');text.resize(GetWindowTextW(c,text.data(),static_cast<int>(text.size())));return text;}
  HWND Add(const wchar_t* cls,const wchar_t* text,DWORD flags,int x,int y,int w,int h,int id=0){
    if(std::wstring_view(cls)==L"STATIC")flags|=SS_NOPREFIX;
    const auto child=CreateWindowExW(0,cls,text,WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|flags,S(x),S(y),S(w),S(h),window,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
    if(!child)throw std::runtime_error("Cannot create multiplayer field");SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(style_.font),TRUE);
    if(std::wstring_view(cls)==L"EDIT")SetWindowTheme(child,L"",L"");return child;
  }
  void Build(){
    help_.Create(window,style_.scale);
    const auto mode_label=Add(L"STATIC",T(L"Connection mode",L"Bağlantı modu"),0,24,20,592,22);
    const auto mode=Add(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST|CBS_OWNERDRAWFIXED|CBS_HASSTRINGS,24,48,592,180,kLanEnabled);style_.AttachCombo(mode);
    for(auto label:{T(L"Offline",L"Çevrimdışı"),L"LAN",T(L"Online (Experimental)",L"Çevrimiçi (Deneysel)")})SendMessageW(mode,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label));
    SendMessageW(mode,CB_SETITEMHEIGHT,WPARAM(-1),S(24));SendMessageW(mode,CB_SETITEMHEIGHT,0,S(24));SendMessageW(mode,CB_SETCURSEL,pending_.network_mode=="online"?2:pending_.network_mode=="lan"?1:0,0);
    auto edit=[&](const wchar_t* label,int x,int y,int width,int id,const std::string& value,int limit,const wchar_t* tip){
      field_labels_[id]=Add(L"STATIC",label,0,x,y,width,24);
      const auto field=Add(L"EDIT",Wide(value).c_str(),WS_TABSTOP|ES_AUTOHSCROLL|WS_BORDER,x,y+28,width,30,id);SendMessageW(field,EM_SETLIMITTEXT,limit,0);
      help_.Set(field,tip);help_.Set(field_labels_[id],tip);
    };
    edit(T(L"Player name",L"Oyuncu adı"),24,94,284,kLanPlayer,pending_.lan_player_name,15,T(L"Name shown to other players. Up to 15 UTF-8 bytes.",L"Diğer oyuncuların gördüğü ad. En fazla 15 UTF-8 bayt."));
    edit(T(L"Room name (host)",L"Oda adı (kurucu)"),332,94,284,kLanRoom,pending_.lan_room_name,31,T(L"Used when you host a room. Up to 31 UTF-8 bytes. Both players need the same build and Player Match.",L"Odayı kurduğunda kullanılır. En fazla 31 UTF-8 bayt. Oyuncular aynı buildi ve Player Match seçeneğini kullanmalı."));
    edit(T(L"Local IPv4 (optional)",L"Yerel IPv4 (isteğe bağlı)"),24,180,284,kLanAdapter,pending_.lan_bind_ip,15,T(L"Leave empty for automatic selection, including virtual LAN adapters. Both players must join the same local or virtual LAN.",L"Sanal LAN bağdaştırıcıları dahil otomatik seçim için boş bırak. Oyuncular aynı yerel veya sanal LAN ağına bağlanmalı."));
    edit(T(L"Host IPv4 (optional)",L"Kurucu IPv4 (isteğe bağlı)"),332,180,284,kLanHost,pending_.lan_host_ip,15,T(L"Rooms are discovered automatically. Enter the host's LAN IPv4 only if discovery fails.",L"Odalar otomatik bulunur. Yalnızca oda bulunamazsa kurucunun LAN IPv4 adresini gir."));
    edit(T(L"Online server (HTTPS)",L"Çevrimiçi sunucu (HTTPS)"),24,180,592,kOnlineServer,pending_.online_server,512,T(L"Use the same running community HTTPS server as the other players. This launcher does not host a server. Use the same build and Player Match.",L"Diğer oyuncularla aynı, çalışır durumdaki topluluk HTTPS sunucusunu kullan. Launcher sunucu açmaz. Aynı buildi ve Player Match seçeneğini kullanın."));
    const auto mode_tip=T(L"Offline uses no multiplayer service. LAN works on local or virtual networks. Online uses a community server. Switching modes keeps your entered details.",L"Çevrimdışı, çok oyunculu hizmete bağlanmaz. LAN, yerel veya sanal ağlarda çalışır. Çevrimiçi, topluluk sunucusunu kullanır. Mod değiştirirken girilen bilgiler korunur.");
    help_.Set(mode,mode_tip);help_.Set(mode_label,mode_tip);
    Add(L"BUTTON",T(L"Apply",L"Uygula"),WS_TABSTOP|BS_OWNERDRAW,374,268,116,32,IDOK);
    Add(L"BUTTON",T(L"Cancel",L"İptal"),WS_TABSTOP|BS_OWNERDRAW,500,268,116,32,IDCANCEL);UpdateMode();
  }
  static LRESULT CALLBACK Proc(HWND hwnd,UINT message,WPARAM wp,LPARAM lp){
    auto* self=reinterpret_cast<NetworkDialog*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
    if(message==WM_NCCREATE){self=static_cast<NetworkDialog*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);self->window=hwnd;SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));}
    if(!self)return DefWindowProcW(hwnd,message,wp,lp);
    switch(message){
      case WM_CREATE:try{self->Build();return 0;}catch(...){return -1;}
      case WM_ERASEBKGND:{RECT r;GetClientRect(hwnd,&r);SetDCBrushColor(reinterpret_cast<HDC>(wp),RGB(23,26,33));FillRect(reinterpret_cast<HDC>(wp),&r,reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));return 1;}
      case WM_CTLCOLORSTATIC:case WM_CTLCOLORBTN:case WM_CTLCOLOREDIT:case WM_CTLCOLORLISTBOX:return self->style_.Color(reinterpret_cast<HDC>(wp),reinterpret_cast<HWND>(lp));
      case WM_MEASUREITEM:reinterpret_cast<MEASUREITEMSTRUCT*>(lp)->itemHeight=self->S(24);return TRUE;
      case WM_DRAWITEM:self->style_.DrawItem(*reinterpret_cast<DRAWITEMSTRUCT*>(lp));return TRUE;
      case WM_COMMAND:
        if(LOWORD(wp)==kLanEnabled&&HIWORD(wp)==CBN_SELCHANGE){self->UpdateMode();return 0;}
        if(LOWORD(wp)==IDOK){std::wstring error;if(self->Commit(&error))DestroyWindow(hwnd);else MessageBoxW(hwnd,error.c_str(),self->T(L"Check multiplayer settings",L"Çok oyunculu ayarlarını kontrol et"),MB_OK|MB_ICONWARNING);return 0;}
        if(LOWORD(wp)==IDCANCEL){DestroyWindow(hwnd);return 0;}break;
      case WM_CLOSE:DestroyWindow(hwnd);return 0;
      case WM_DESTROY:self->window=nullptr;return 0;
    }
    return DefWindowProcW(hwnd,message,wp,lp);
  }
};
}
