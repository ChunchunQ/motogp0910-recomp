#pragma once
#include <windows.h>
#include <commctrl.h>
#include <map>
#include <string>
#include <string_view>
#include <stdexcept>
namespace launcher {
// Use the system tooltip rather than another painted popup. Owning the strings
// keeps TOOLINFO text valid through localization and native hover callbacks.
class Tooltips {
 public:
  HWND window{};
  Tooltips()=default;
  Tooltips(const Tooltips&)=delete;
  Tooltips& operator=(const Tooltips&)=delete;
  ~Tooltips(){if(IsWindow(window))DestroyWindow(window);}
  void Create(HWND owner,double scale){
    window=CreateWindowExW(WS_EX_TOPMOST,TOOLTIPS_CLASSW,nullptr,WS_POPUP|TTS_ALWAYSTIP|TTS_NOPREFIX,
        CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,owner,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!window)throw std::runtime_error("Cannot create launcher tooltips");
    SendMessageW(window,TTM_SETDELAYTIME,TTDT_INITIAL,550);
    SendMessageW(window,TTM_SETDELAYTIME,TTDT_RESHOW,100);
    SendMessageW(window,TTM_SETDELAYTIME,TTDT_AUTOPOP,15000);
    SetScale(scale);
  }
  void SetScale(double scale){if(window)SendMessageW(window,TTM_SETMAXTIPWIDTH,0,static_cast<LPARAM>(360*scale+.5));}
  void Pop()const{if(window)SendMessageW(window,TTM_POP,0,0);}
  void Set(HWND control,const wchar_t* text){
    if(!window||!control)return;
    wchar_t cls[32]{};GetClassNameW(control,cls,32);
    if(std::wstring_view(cls)==L"STATIC")SetWindowLongPtrW(control,GWL_STYLE,GetWindowLongPtrW(control,GWL_STYLE)|SS_NOTIFY);
    auto [entry,inserted]=texts_.try_emplace(control,text);
    if(!inserted)entry->second=text;
    TOOLINFOW tool{sizeof(tool)};tool.uFlags=TTF_IDISHWND|TTF_SUBCLASS;
    tool.hwnd=GetParent(control);tool.uId=reinterpret_cast<UINT_PTR>(control);tool.lpszText=entry->second.data();
    if(inserted){if(!SendMessageW(window,TTM_ADDTOOLW,0,reinterpret_cast<LPARAM>(&tool)))throw std::runtime_error("Cannot attach launcher tooltip");}
    else SendMessageW(window,TTM_UPDATETIPTEXTW,0,reinterpret_cast<LPARAM>(&tool));
  }
 private:
  std::map<HWND,std::wstring> texts_;
};
}
