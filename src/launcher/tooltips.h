#pragma once
#include <windows.h>
#include <commctrl.h>
#include <cstddef>
#include <map>
#include <string>
#include <string_view>
namespace launcher {
// Use the system tooltip rather than another painted popup. Owning the strings
// keeps TOOLINFO text valid through localization and native hover callbacks.
class Tooltips {
 public:
  // sizeof(TOOLINFO) includes lpReserved (v6). Our launcher intentionally uses
  // the default common-controls activation context, which can be v5 on Windows.
  // These older sizes contain every field we use and work with both versions.
  static constexpr UINT kInfoSize=offsetof(TOOLINFOW,lParam)+sizeof(LPARAM);
  static constexpr UINT kLegacyInfoSize=offsetof(TOOLINFOW,lpszText)+sizeof(LPWSTR);
  HWND window{};
  Tooltips()=default;
  Tooltips(const Tooltips&)=delete;
  Tooltips& operator=(const Tooltips&)=delete;
  ~Tooltips(){if(IsWindow(window))DestroyWindow(window);}
  void Create(HWND owner,double scale){
    window=CreateWindowExW(WS_EX_TOPMOST,TOOLTIPS_CLASSW,nullptr,WS_POPUP|TTS_ALWAYSTIP|TTS_NOPREFIX,
        CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,owner,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!window){OutputDebugStringW(L"MotoGP launcher: hover help unavailable; continuing without tooltips.\n");return;}
    SetWindowSubclass(window,VisibilityProc,27,reinterpret_cast<DWORD_PTR>(this));
    SendMessageW(window,TTM_SETDELAYTIME,TTDT_INITIAL,550);
    SendMessageW(window,TTM_SETDELAYTIME,TTDT_RESHOW,100);
    SendMessageW(window,TTM_SETDELAYTIME,TTDT_AUTOPOP,15000);
    SetScale(scale);
  }
  void SetScale(double scale){if(window)SendMessageW(window,TTM_SETMAXTIPWIDTH,0,static_cast<LPARAM>(360*scale+.5));}
  void Pop()const{if(window)SendMessageW(window,TTM_POP,0,0);}
  void Activate(bool active){active_=active;if(window){Pop();SendMessageW(window,TTM_ACTIVATE,active,0);}}
  bool Set(HWND control,const wchar_t* text){
    if(!IsWindow(window)||!IsWindow(control))return false;
    wchar_t cls[32]{};GetClassNameW(control,cls,32);
    if(std::wstring_view(cls)==L"STATIC")SetWindowLongPtrW(control,GWL_STYLE,GetWindowLongPtrW(control,GWL_STYLE)|SS_NOTIFY);
    auto [entry,inserted]=texts_.try_emplace(control,Entry{text});
    if(!inserted)entry->second.text=text;
    TOOLINFOW tool{entry->second.info_size};tool.uFlags=TTF_IDISHWND|TTF_SUBCLASS;
    tool.hwnd=GetParent(control);tool.uId=reinterpret_cast<UINT_PTR>(control);tool.lpszText=entry->second.text.data();
    if(!inserted){SendMessageW(window,TTM_UPDATETIPTEXTW,0,reinterpret_cast<LPARAM>(&tool));return true;}
    if(SendMessageW(window,TTM_ADDTOOLW,0,reinterpret_cast<LPARAM>(&tool)))return true;
    tool.cbSize=kLegacyInfoSize;
    if(SendMessageW(window,TTM_ADDTOOLW,0,reinterpret_cast<LPARAM>(&tool))){entry->second.info_size=tool.cbSize;return true;}
    // Hover help must never prevent startup, saving, language changes or opening
    // a settings dialog. Remove failed records so a later update can retry.
    texts_.erase(entry);
    OutputDebugStringW(L"MotoGP launcher: one hover hint could not be registered; continuing.\n");return false;
  }
 private:
  bool active_=true;
  static LRESULT CALLBACK VisibilityProc(HWND window,UINT message,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR data){
    const auto& self=*reinterpret_cast<Tooltips*>(data);
    // A queued hover timer can outlive TTM_ACTIVATE(false). Keep its pending
    // show transaction hidden while a dropdown owns the interaction.
    if(message==WM_WINDOWPOSCHANGING&&!self.active_){auto& pos=*reinterpret_cast<WINDOWPOS*>(lp);if(pos.flags&SWP_SHOWWINDOW)pos.flags=(pos.flags&~SWP_SHOWWINDOW)|SWP_HIDEWINDOW;}
    if(message==WM_NCDESTROY)RemoveWindowSubclass(window,VisibilityProc,id);
    return DefSubclassProc(window,message,wp,lp);
  }
  struct Entry {std::wstring text;UINT info_size=kInfoSize;};
  std::map<HWND,Entry> texts_;
};
}
