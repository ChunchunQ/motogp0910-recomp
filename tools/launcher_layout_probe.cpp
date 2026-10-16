// Real launcher controls, configuration and Win32 painting. This optional
// target includes production UI code; it does not replace any runtime behavior.
#include "../src/launcher/main.cpp"
#include <iostream>
#include <thread>
#include <atomic>

namespace {
int ui_checks=0;
void CheckUI(bool ok,const char* message){if(!ok)throw std::runtime_error(message);++ui_checks;}
void Screenshot(HWND window,const fs::path& file,bool whole=false,bool include_popups=false){
  RECT r;POINT origin{};if(whole){GetWindowRect(window,&r);origin={r.left,r.top};r.right-=r.left;r.bottom-=r.top;r.left=r.top=0;}else{GetClientRect(window,&r);if(include_popups)ClientToScreen(window,&origin);}
  auto dc=include_popups?GetDC(nullptr):whole?GetWindowDC(window):GetDC(window),copy=CreateCompatibleDC(dc);
  BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=r.right;
  info.bmiHeader.biHeight=-r.bottom;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
  void* data=nullptr;auto bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&data,nullptr,0);auto old=SelectObject(copy,bitmap);
  CheckUI(bitmap && BitBlt(copy,0,0,r.right,r.bottom,dc,include_popups?origin.x:0,include_popups?origin.y:0,SRCCOPY|CAPTUREBLT),"Capture actual launcher window");
  const DWORD bytes=r.right*r.bottom*4;BITMAPFILEHEADER header{};header.bfType=0x4D42;
  header.bfOffBits=sizeof(header)+sizeof(info.bmiHeader);header.bfSize=header.bfOffBits+bytes;
  std::ofstream output(file,std::ios::binary);output.write(reinterpret_cast<const char*>(&header),sizeof(header));
  output.write(reinterpret_cast<const char*>(&info.bmiHeader),sizeof(info.bmiHeader));output.write(static_cast<const char*>(data),bytes);
  CheckUI(bool(output),"Screenshot saved");SelectObject(copy,old);DeleteObject(bitmap);DeleteDC(copy);ReleaseDC(include_popups?nullptr:window,dc);
}
void PumpUI(){MSG message;while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}}
void ClickControl(HWND control,int y){
  RECT r;GetClientRect(control,&r);POINT p{r.right/2,y};ClientToScreen(control,&p);
  std::atomic<bool> done=false,ok=true;
  std::thread mouse([&]{SetCursorPos(p.x,p.y);Sleep(25);for(DWORD flags:{MOUSEEVENTF_LEFTDOWN,MOUSEEVENTF_LEFTUP}){INPUT input{};input.type=INPUT_MOUSE;input.mi.dwFlags=flags;if(SendInput(1,&input,sizeof(input))!=1)ok=false;Sleep(25);}done=true;});
  while(!done){PumpUI();Sleep(2);}mouse.join();PumpUI();CheckUI(ok,"Actual mouse input delivered to the native control");
}
void CheckSurface(App& app,HWND control,int x,int y){
  RECT r;GetClientRect(control,&r);const auto target=GetDC(app.window),paint=CreateCompatibleDC(target),expected=CreateCompatibleDC(target);
  const auto bitmap=CreateCompatibleBitmap(target,r.right,r.bottom);const auto previous=SelectObject(paint,bitmap);
  SetDCBrushColor(paint,RGB(255,255,255));FillRect(paint,&r,reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
  SendMessageW(control,WM_PRINTCLIENT,reinterpret_cast<WPARAM>(paint),PRF_CLIENT);
  const auto previous_expected=SelectObject(expected,app.paint_background_bitmap);
  POINT root{x,y};MapWindowPoints(control,app.window,&root,1);
  CheckUI(GetPixel(paint,x,y)==GetPixel(expected,root.x,root.y),"Child and themed-parent backgrounds align exactly with the full-window artwork");
  SelectObject(paint,previous);SelectObject(expected,previous_expected);DeleteObject(bitmap);DeleteDC(paint);DeleteDC(expected);ReleaseDC(app.window,target);
}
std::wstring TooltipText(HWND tooltip,HWND control,UINT size=Tooltips::kInfoSize){
  wchar_t text[2048]{};TOOLINFOW tool{size};tool.hwnd=GetParent(control);tool.uId=reinterpret_cast<UINT_PTR>(control);tool.lpszText=text;
  SendMessageW(tooltip,TTM_GETTEXTW,std::size(text),reinterpret_cast<LPARAM>(&tool));return text;
}
struct TooltipCompatibility {
  UINT accepted_size=Tooltips::kInfoSize,last_update_size=0;int rejected=0;bool reject_adds=false;
};
LRESULT CALLBACK TooltipCompatibilityProc(HWND window,UINT message,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR data){
  auto& policy=*reinterpret_cast<TooltipCompatibility*>(data);
  if(message==TTM_ADDTOOLW||message==TTM_UPDATETIPTEXTW){
    const auto& tool=*reinterpret_cast<TOOLINFOW*>(lp);
    if(tool.cbSize>policy.accepted_size||(message==TTM_ADDTOOLW&&policy.reject_adds)){++policy.rejected;return FALSE;}
    if(message==TTM_UPDATETIPTEXTW)policy.last_update_size=tool.cbSize;
  }
  if(message==WM_NCDESTROY)RemoveWindowSubclass(window,TooltipCompatibilityProc,id);
  return DefSubclassProc(window,message,wp,lp);
}
void CheckTooltipCompatibility(HWND owner){
  // Wine accepts the v6 struct without an activation manifest; native Windows
  // v5 rejects it. Keep that stricter boundary in front of the real tooltip.
  CheckUI(Tooltips::kInfoSize==TTTOOLINFOW_V2_SIZE&&Tooltips::kLegacyInfoSize==TTTOOLINFOW_V1_SIZE,"Tooltip compatibility sizes match the Windows SDK layouts");
  const auto parent=CreateWindowExW(0,L"STATIC",L"Tooltip compatibility fixture",WS_CHILD,0,0,240,100,owner,nullptr,GetModuleHandleW(nullptr),nullptr);
  const auto child=CreateWindowExW(0,L"BUTTON",L"Nested hint target",WS_CHILD,0,0,200,30,parent,nullptr,GetModuleHandleW(nullptr),nullptr);
  CheckUI(parent&&child,"Nested tooltip compatibility fixture created");
  {
    TooltipCompatibility policy;Tooltips help;help.Create(owner,1);SetWindowSubclass(help.window,TooltipCompatibilityProc,901,reinterpret_cast<DWORD_PTR>(&policy));
    TOOLINFOW unsupported{sizeof(unsupported)};unsupported.uFlags=TTF_IDISHWND|TTF_SUBCLASS;unsupported.hwnd=parent;unsupported.uId=reinterpret_cast<UINT_PTR>(child);wchar_t old_text[]=L"Old oversized registration";unsupported.lpszText=old_text;
    CheckUI(!SendMessageW(help.window,TTM_ADDTOOLW,0,reinterpret_cast<LPARAM>(&unsupported)),"Strict native-v5 boundary reproduces rejection of the former v6-sized TOOLINFO");
    CheckUI(help.Set(child,L"Compatible registration"),"Compatible tooltip registration succeeds at a native-v5 size boundary");
    CheckUI(TooltipText(help.window,child)==L"Compatible registration","Compatible registration retains the real nested tooltip text");
    CheckUI(help.Set(child,L"Türkçe yardım"),"Existing tool localization succeeds with the compatible layout");
    CheckUI(policy.last_update_size==Tooltips::kInfoSize&&SendMessageW(help.window,TTM_GETTOOLCOUNT,0,0)==1&&TooltipText(help.window,child)==L"Türkçe yardım","Localization updates one tool without an oversized structure or duplicate");
  }
  {
    TooltipCompatibility policy;policy.accepted_size=Tooltips::kLegacyInfoSize;Tooltips help;help.Create(owner,1);SetWindowSubclass(help.window,TooltipCompatibilityProc,902,reinterpret_cast<DWORD_PTR>(&policy));
    CheckUI(help.Set(child,L"Legacy fallback")&&policy.rejected==1,"A smaller legacy registration is retried when v2 is rejected");
    CheckUI(help.Set(child,L"Updated legacy fallback")&&policy.last_update_size==Tooltips::kLegacyInfoSize&&TooltipText(help.window,child,Tooltips::kLegacyInfoSize)==L"Updated legacy fallback","Fallback tools retain their accepted structure size during updates");
  }
  {
    TooltipCompatibility policy;policy.reject_adds=true;Tooltips help;help.Create(owner,1);SetWindowSubclass(help.window,TooltipCompatibilityProc,903,reinterpret_cast<DWORD_PTR>(&policy));
    CheckUI(!help.Set(child,L"Unavailable help")&&policy.rejected==2&&SendMessageW(help.window,TTM_GETTOOLCOUNT,0,0)==0,"A completely rejected hint is optional and cannot throw a launcher startup error");
    policy.reject_adds=false;CheckUI(help.Set(child,L"Retry after rejection")&&SendMessageW(help.window,TTM_GETTOOLCOUNT,0,0)==1&&TooltipText(help.window,child)==L"Retry after rejection","Rejected registrations do not leave stale records and can be retried");
  }
  {Tooltips missing;CheckUI(!missing.Set(child,L"Unavailable tooltip window"),"Unavailable tooltip service leaves controls usable");missing.Pop();missing.SetScale(1.5);}
  DestroyWindow(parent);
}
void CheckHelp(App& app,const fs::path& pictures){
  app.ShowPage(3);app.ScrollPage(0);app.turkish=false;app.Localize();
  wchar_t cls[80]{};GetClassNameW(app.help.window,cls,80);CheckUI(std::wstring_view(cls)==TOOLTIPS_CLASSW,"Help uses the real Windows tooltip control");
  const auto count=SendMessageW(app.help.window,TTM_GETTOOLCOUNT,0,0);
  CheckUI(TooltipText(app.help.window,app.verify_files).find(L"No files are changed")!=std::wstring::npos,"File verification help is available on hover instead of a static paragraph");
  app.turkish=true;app.Localize();CheckUI(TooltipText(app.help.window,app.verify_files).find(L"Dosyalar değiştirilmez")!=std::wstring::npos,"Tooltip text changes with launcher language");
  app.turkish=false;app.Localize();CheckUI(SendMessageW(app.help.window,TTM_GETTOOLCOUNT,0,0)==count,"Localization updates existing tooltip tools without duplicating them");
  const auto selection=SendMessageW(app.effect,CB_GETCURSEL,0,0);SendMessageW(app.effect,CB_SETCURSEL,3,0);app.UpdateSharpness();
  CheckUI(TooltipText(app.help.window,app.effect).find(L"No frame generation")!=std::wstring::npos,"Experimental upscaling caveats remain available in the relevant tooltip");
  SendMessageW(app.effect,CB_SETCURSEL,selection,0);app.UpdateSharpness();
  SetForegroundWindow(app.window);SetCursorPos(1700,1000);PumpUI();
  RECT control;GetClientRect(app.verify_files,&control);POINT p{control.right/2,control.bottom/2};ClientToScreen(app.verify_files,&p);SetCursorPos(p.x,p.y);
  const auto deadline=GetTickCount64()+4000;
  while(!IsWindowVisible(app.help.window)&&GetTickCount64()<deadline){PumpUI();Sleep(10);}
  CheckUI(IsWindowVisible(app.help.window),"A native tooltip appears after actual mouse hover");
  TOOLINFOW active{Tooltips::kInfoSize};SendMessageW(app.help.window,TTM_GETCURRENTTOOLW,0,reinterpret_cast<LPARAM>(&active));
  CheckUI(active.uId==reinterpret_cast<UINT_PTR>(app.verify_files),"Mouse hover displays help for the intended nested control");
  UpdateWindow(app.help.window);PumpUI();Screenshot(app.window,pictures/L"launcher-tooltip-english.bmp",false,true);
  app.ShowPage(0);CheckUI(!IsWindowVisible(app.help.window),"Changing pages dismisses the outgoing tooltip");SetCursorPos(1700,1000);PumpUI();
}
void LayoutCheck(App& app){
  RECT client;GetClientRect(app.window,&client);std::vector<std::pair<RECT,HWND>> visible;
  for(auto& item:app.placements){if(!IsWindowVisible(item.control) || item.control==app.page_view || item.control==app.page_content)continue;RECT r;GetWindowRect(item.control,&r);
    MapWindowPoints(nullptr,app.window,reinterpret_cast<POINT*>(&r),2);
    const auto uncut=r;
    if(GetParent(item.control)==app.page_content){RECT panel;GetClientRect(app.page_view,&panel);MapWindowPoints(app.page_view,app.window,reinterpret_cast<POINT*>(&panel),2);
      RECT clipped;if(!IntersectRect(&clipped,&r,&panel))continue;r=clipped;
      CheckUI(uncut.left>=panel.left && uncut.right<=panel.right,"Page controls fit the actual scroll viewport width");}
    CheckUI(r.left>=0 && r.top>=0 && r.right<=client.right && r.bottom<=client.bottom,"Visible control fits client area");
    for(auto& [previous,control]:visible){RECT overlap;const bool intersects=IntersectRect(&overlap,&previous,&r);
      if(intersects)std::cerr<<"Overlap: scale="<<app.scale<<" page="<<app.current_page<<" tr="<<app.turkish
        <<" first="<<GetDlgCtrlID(control)<<" second="<<GetDlgCtrlID(item.control)
        <<" region="<<overlap.left<<","<<overlap.top<<","<<overlap.right<<","<<overlap.bottom<<'\n';
      CheckUI(!intersects,"Visible controls do not overlap");}
    visible.emplace_back(r,item.control);wchar_t cls[32];GetClassNameW(item.control,cls,32);
    if(std::wstring_view(cls)==L"STATIC"){
      auto dc=GetDC(item.control);auto previous=SelectObject(dc,reinterpret_cast<HFONT>(SendMessageW(item.control,WM_GETFONT,0,0)));
      RECT measured{0,0,uncut.right-uncut.left,0};auto text=app.Text(item.control);
      DrawTextW(dc,text.c_str(),-1,&measured,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);
      CheckUI(measured.bottom<=uncut.bottom-uncut.top,"Localized static text fits its height");
      SelectObject(dc,previous);ReleaseDC(item.control,dc);
    }
  }
}
void CheckPolish(App& app){
  const auto dc=GetDC(app.window);auto old=SelectObject(dc,app.font);
  wchar_t face[80]{};GetTextFaceW(dc,80,face);
  CheckUI(std::wstring_view(face)==L"Open Sans","Embedded body font is selected instead of a system fallback");
  constexpr wchar_t turkish[]=L"ĞğİıŞşÇçÖöÜü";WORD glyphs[std::size(turkish)-1]{};
  CheckUI(GetGlyphIndicesW(dc,turkish,std::size(turkish)-1,glyphs,GGI_MARK_NONEXISTING_GLYPHS)!=GDI_ERROR,"Embedded font can be queried for Turkish coverage");
  for(auto glyph:glyphs)CheckUI(glyph!=0xFFFF,"All Turkish letters have real glyphs in the launcher font");
  SelectObject(dc,app.bold);int previous_end=-1,first_gap=-1;RECT first{},last{};
  for(int i:{3,0,4,1,2,5}){
    RECT r;GetWindowRect(app.navigation[i],&r);MapWindowPoints(nullptr,app.window,reinterpret_cast<POINT*>(&r),2);
    SIZE extent{};const auto text=app.Text(app.navigation[i]);GetTextExtentPoint32W(dc,text.c_str(),static_cast<int>(text.size()),&extent);
    CheckUI(extent.cx+app.S(8)<=r.right-r.left,"Every tab fits its real font without truncation");
    const int text_start=r.left+(r.right-r.left-extent.cx)/2;
    if(previous_end>=0){const int gap=text_start-previous_end;if(first_gap<0)first_gap=gap;CheckUI(std::abs(gap-first_gap)<=1,"Visible tab text has equal spacing, including Controls / Achievements");}
    else first=r;
    previous_end=text_start+extent.cx;last=r;
  }
  CheckUI(first.left>=app.S(App::kPageX)&&last.right<=app.S(App::kPageX+App::kPageWidth),"The six tabs stay within their content column");
  CheckUI(std::abs(first.left+last.right-app.S(App::kPageX)*2-app.S(App::kPageWidth))<=1,"Navigation is centered as one group");
  const std::array<int,6> order{3,0,4,1,2,5};
  for(size_t i=0;i+1<order.size();++i)CheckUI(GetNextDlgTabItem(app.window,app.navigation[order[i]],FALSE)==app.navigation[order[i+1]],"Keyboard tab order follows the visible navigation order");
  SelectObject(dc,old);

  RECT play,save,status;GetWindowRect(app.play,&play);GetWindowRect(app.save,&save);GetWindowRect(app.status,&status);
  MapWindowPoints(nullptr,app.window,reinterpret_cast<POINT*>(&play),2);
  CheckUI(std::abs(play.right-app.S(kWidth-28))<=1&&play.right-play.left>=app.S(220)&&play.bottom-play.top>=app.S(64),"PLAY is large and anchored to the right edge");
  CheckUI(save.bottom<=status.top,"Auxiliary actions appear above the status line");
  const auto search_dc=GetDC(app.achievement_search_label);const auto search_font=SelectObject(search_dc,app.font);SIZE text_size{};const auto search=app.Text(app.achievement_search_label);GetTextExtentPoint32W(search_dc,search.c_str(),static_cast<int>(search.size()),&text_size);
  RECT search_bounds;GetClientRect(app.achievement_search_label,&search_bounds);CheckUI(text_size.cx<=search_bounds.right&&text_size.cy<=search_bounds.bottom,"Search label fits its complete localized text without clipping");SelectObject(search_dc,search_font);ReleaseDC(app.achievement_search_label,search_dc);

  const auto a=app.CaptionBounds();const auto baseline=CreateCompatibleDC(dc),paint=CreateCompatibleDC(dc);
  const auto back_bitmap=CreateCompatibleBitmap(dc,a.width,a.height),paint_bitmap=CreateCompatibleBitmap(dc,a.width,a.height);
  const auto old_back=SelectObject(baseline,back_bitmap),old_paint=SelectObject(paint,paint_bitmap);
  app.DrawBackground(baseline);const auto* cached=app.paint_background.get();const int hover=app.caption_hover,pressed=app.caption_pressed;
  app.caption_hover=app.caption_pressed=0;app.DrawCaption(paint);
  int minimize_width=0;
  for(int hit:{HTMINBUTTON,HTCLOSE}){
    const int right=a.width-(hit==HTMINBUTTON?app.S(46):0),cx=right-app.S(46)/2,cy=a.height/2;
    int left=a.width,top=a.height,bottom=0,end=0;
    for(int y=cy-app.S(8);y<=cy+app.S(8);++y)for(int x=cx-app.S(8);x<=cx+app.S(8);++x)if(GetPixel(paint,x,y)!=GetPixel(baseline,x,y)){
      left=std::min(left,x);end=std::max(end,x);top=std::min(top,y);bottom=std::max(bottom,y);
    }
    const int width=end-left+1,height=bottom-top+1;
    CheckUI(width>=app.S(9)&&width<=app.S(13),"Caption symbols have a normal consistent width");
    if(hit==HTMINBUTTON)minimize_width=width;
    // A diagonal antialiased stroke can cover one extra pixel on each side.
    else CheckUI(std::abs(width-minimize_width)<=2&&height>=app.S(9),"Close X has the same nominal size as minimize and is not a tiny text glyph");
  }
  for(int hit:{HTMINBUTTON,HTCLOSE}){
    const int right=a.width-(hit==HTMINBUTTON?app.S(46):0),x=right-app.S(6),other_x=a.width-(hit==HTCLOSE?app.S(52):app.S(6)),y=app.S(5);
    app.caption_hover=hit;app.caption_pressed=0;app.DrawCaption(paint);
    const auto background=GetPixel(baseline,x,y),hovered=GetPixel(paint,x,y);
    CheckUI(hovered!=background,"Hover draws a visible translucent background on either caption button");
    CheckUI(hovered!=(hit==HTCLOSE?RGB(232,45,39):RGB(255,255,255)),"Hover preserves artwork through its tint");
    CheckUI(GetPixel(paint,other_x,y)==GetPixel(baseline,other_x,y),"Hover does not tint the other caption button");
    app.caption_pressed=hit;app.DrawCaption(paint);CheckUI(GetPixel(paint,x,y)!=hovered,"Pressed button gives distinct feedback");
    app.caption_hover=app.caption_pressed=0;app.DrawCaption(paint);CheckUI(GetPixel(paint,x,y)==background,"Leaving restores the artwork exactly with no backing tile");
  }
  CheckUI(app.paint_background.get()==cached,"Caption interaction reuses the cached background");
  app.caption_hover=hover;app.caption_pressed=pressed;
  SelectObject(baseline,old_back);SelectObject(paint,old_paint);DeleteObject(back_bitmap);DeleteObject(paint_bitmap);DeleteDC(baseline);DeleteDC(paint);ReleaseDC(app.window,dc);
}
}
void CheckEmbeddedNotices(){
  const auto source=Wide(EmbeddedBytes(104));
  CheckUI(source.size()>50000&&source.find(L"ReXGlue-LICENSE")!=std::wstring::npos&&source.find(L"FidelityFX-LICENSE")!=std::wstring::npos,"Required third-party notices are embedded in the Windows resource");
  const auto owner=CreateWindowExW(0,L"STATIC",L"Notices test owner",WS_OVERLAPPEDWINDOW,20,20,1000,800,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
  const auto font=CreateFontW(-13,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
  for(bool turkish:{false,true})for(double scale:{1.,1.5}){
    NoticesDialog dialog(owner,font,scale,turkish,source);const auto modal=dialog.Create();ShowWindow(modal,SW_SHOWNOACTIVATE);UpdateWindow(modal);
    CheckUI(modal&&dialog.text_control&&GetDlgItem(modal,IDOK),"Notices window creates its full text and close action");
    std::wstring text(GetWindowTextLengthW(dialog.text_control)+1,L'\0');text.resize(GetWindowTextW(dialog.text_control,text.data(),static_cast<int>(text.size())));
    std::wstring expected;for(size_t i=0;i<source.size();++i){if(source[i]==L'\n'&&(i==0||source[i-1]!=L'\r'))expected+=L'\r';expected+=source[i];}
    CheckUI(text==expected,"Every notice, including the end of the bundle, is available in the viewer");
    CheckUI(GetWindowLongPtrW(dialog.text_control,GWL_STYLE)&ES_READONLY,"The notice viewer is read-only");
    SendMessageW(dialog.text_control,WM_CHAR,L'x',0);CheckUI(GetWindowTextLengthW(dialog.text_control)==static_cast<int>(text.size()),"Typing cannot alter the notice text");
    SendMessageW(dialog.text_control,EM_LINESCROLL,0,100000);CheckUI(SendMessageW(dialog.text_control,EM_GETFIRSTVISIBLELINE,0,0)>0,"The complete legal bundle can be scrolled");
    RECT client;GetClientRect(modal,&client);for(auto control:{dialog.text_control,GetDlgItem(modal,IDOK)}){RECT r;GetWindowRect(control,&r);MapWindowPoints(nullptr,modal,reinterpret_cast<POINT*>(&r),2);CheckUI(r.left>=0&&r.top>=0&&r.right<=client.right&&r.bottom<=client.bottom,"Notice controls fit their window at different scales");}
    SendMessageW(modal,WM_COMMAND,IDCANCEL,0);CheckUI(!dialog.window&&IsWindow(owner),"Closing the viewer leaves its launcher owner intact");
  }
  DeleteObject(font);DestroyWindow(owner);std::cout<<ui_checks<<" embedded-notices checks passed\n";
}
int wmain(int argc,wchar_t** argv){
  if(argc==2&&std::wstring_view(argv[1])==L"--fixture-child")return 0;
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
  Gdiplus::GdiplusStartupInput gdip;ULONG_PTR token{};Gdiplus::GdiplusStartup(&token,&gdip,nullptr);
  INITCOMMONCONTROLSEX common{sizeof(common),ICC_STANDARD_CLASSES|ICC_BAR_CLASSES};InitCommonControlsEx(&common);
  int result=0;
  if(argc==2&&std::wstring_view(argv[1])==L"--notices-only"){
    try{CheckEmbeddedNotices();}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';result=1;}
    Gdiplus::GdiplusShutdown(token);CoUninitialize();return result;
  }
  try{
    if(argc!=3)throw std::runtime_error("Usage: launcher_ui_probe GAME_ROOT SCREENSHOT_DIRECTORY (run in an isolated executable folder)");
    const auto pictures=fs::path(argv[2]);fs::create_directories(pictures);
    App app;app.scale=1;const auto game=fs::path(argv[1]);
    WNDCLASSW cls{};cls.hInstance=GetModuleHandleW(nullptr);cls.lpfnWndProc=WindowProc;cls.lpszClassName=L"MotoGPLauncherUIProbe";cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);RegisterClassW(&cls);
    RECT bounds{0,0,kWidth,kHeight+kCaptionHeight};constexpr DWORD style=WS_POPUP|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN;
    AdjustWindowRectEx(&bounds,style,FALSE,0);
    auto window=CreateWindowExW(0,cls.lpszClassName,L"MotoGP 09/10 Launcher",style,20,20,bounds.right-bounds.left,bounds.bottom-bounds.top,nullptr,nullptr,cls.hInstance,&app);
    CheckUI(window!=nullptr,"Production UI window created");ShowWindow(window,SW_SHOW);SetForegroundWindow(window);UpdateWindow(window);
    SetWindowTextW(app.path,game.c_str());app.ReadyStatus();
    CheckTooltipCompatibility(window);
    CheckHelp(app,pictures);app.ShowPage(3);
    {const auto dc=GetDC(window);CheckUI(GetPixel(dc,app.S(4),app.S(4))==kBackground,"Caption blends into the artwork instead of a sidebar strip");ReleaseDC(window,dc);}
    {const auto dc=GetDC(window);const auto memory=CreateCompatibleDC(dc);const auto bitmap=CreateCompatibleBitmap(dc,app.S(kWidth),app.S(kHeight+kCaptionHeight));const auto old=SelectObject(memory,bitmap);
      app.DrawBackground(memory);
      for(int offset:{6,37,57,86})CheckUI(GetPixel(dc,app.S(kWidth-offset),app.S(5))==GetPixel(memory,app.S(kWidth-offset),app.S(5)),"Minimize and close have the real background behind them, without a box");
      SelectObject(memory,old);DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(window,dc);
    }
    {RECT frame;GetWindowRect(window,&frame);const auto caption=app.CaptionBounds();const auto y=frame.top+(caption.height+caption.border)/2;
      for(int x=caption.border;x<caption.width-caption.border;++x)CheckUI(SendMessageW(window,WM_NCHITTEST,0,MAKELPARAM(frame.left+x,y))!=HTMAXBUTTON,"No visible caption position exposes a maximize button");
      CheckUI(SendMessageW(window,WM_NCHITTEST,0,MAKELPARAM(frame.right-caption.border-app.S(69),y))==HTMINBUTTON&&SendMessageW(window,WM_NCHITTEST,0,MAKELPARAM(frame.right-caption.border-app.S(23),y))==HTCLOSE,"Caption retains minimize and close actions");
      SendMessageW(window,WM_SYSCOMMAND,SC_MAXIMIZE,0);CheckUI(!IsZoomed(window),"Maximize command cannot enlarge the fixed launcher");
    }

    { // Read-only verification uses a shipped reference, not the user's files as a new baseline.
      const auto references=GameFileReferences(EmbeddedBytes(103));CheckUI(references.size()==4386,"Full original disk-data reference is embedded");
      std::atomic<bool> cancelled=false;std::atomic<size_t> progress=0;
      const auto verified=VerifyGameData(game,references,cancelled,progress);
      CheckUI(verified.Passed()&&verified.matched==references.size()&&progress==references.size(),"All original disk data match the shipped SHA-256 reference");
      const auto scratch=app.exe/L"verification-fixture";fs::create_directories(scratch);
      const auto sample=scratch/references.front().path;fs::create_directories(sample.parent_path());fs::copy_file(game/references.front().path,sample);
      const std::vector<GameFileReference> one{references.front()};const auto original=Sha256(sample);
      CheckUI(VerifyGameData(scratch,one,cancelled,progress).Passed()&&Sha256(sample)==original,"A successful check does not change the original bytes");
      {std::fstream f(sample,std::ios::in|std::ios::out|std::ios::binary);char c=0;f.read(&c,1);c^=1;f.seekp(0);f.write(&c,1);}
      const auto altered=Sha256(sample);CheckUI(VerifyGameData(scratch,one,cancelled,progress).changed==1&&Sha256(sample)==altered,"Same-size corruption is detected by SHA-256 and left untouched");
      fs::remove(sample);CheckUI(VerifyGameData(scratch,one,cancelled,progress).missing==1&&!fs::exists(sample),"Missing files are reported without recreating them");
      cancelled=true;CheckUI(VerifyGameData(game,references,cancelled,progress).cancelled,"Verification supports cancellation");
      bool rejected=false;try{GameFileReferences(std::string(64,'0')+"\t1\t../outside");}catch(const std::exception&){rejected=true;}CheckUI(rejected,"Reference manifest cannot escape the game root");
      app.StartVerification();CheckUI(app.verifying&&!IsWindowEnabled(app.play)&&!IsWindowEnabled(app.path)&&IsWindowEnabled(app.verify_files),"Actual verification runs asynchronously and keeps cancellation accessible");
      app.StartVerification();const auto limit=GetTickCount64()+15000;
      while(app.verifying&&GetTickCount64()<limit){app.PollVerification();Sleep(10);}
      CheckUI(!app.verifying&&IsWindowEnabled(app.play)&&IsWindowEnabled(app.path),"Cancelling the actual worker restores the launcher controls");
      // Complete the real asynchronous path too. Only dismiss this launcher's
      // own result dialog, as a user pressing OK would do.
      app.StartVerification();std::atomic<bool> dismiss_done=false;
      std::thread dismiss([&]{while(!dismiss_done){auto dialog=FindWindowW(L"#32770",nullptr);
        if(dialog&&GetWindow(dialog,GW_OWNER)==window){PostMessageW(dialog,WM_COMMAND,IDOK,0);break;}Sleep(10);}});
      while(app.verifying){app.PollVerification();Sleep(10);}dismiss_done=true;dismiss.join();
      CheckUI(app.status_positive&&app.status_en.starts_with(L"Verification passed")&&IsWindowEnabled(app.play),"Successful background verification updates status, writes a report and unlocks PLAY");
      bool report_found=false;for(const auto& entry:fs::directory_iterator(app.exe/L"logs"))if(entry.path().filename().wstring().starts_with(L"verification-"))report_found=true;
      CheckUI(report_found,"Verification result report is available through Open logs");
      app.ReadyStatus();
    }

    // Read the actual original XEX in the background; never execute the title
    // or touch its unlock save. Use a separate fixture profile for UI actions.
    const auto profile=app.exe/L"achievement-profile";
    Put(app.config.table,"user_data_root",Utf8(profile.wstring()));
    app.ShowPage(5);
    const auto deadline=GetTickCount64()+20000;
    while(app.catalog.entries.empty()&&app.catalog_error.empty()&&GetTickCount64()<deadline){
      app.PollAchievements();MSG msg;while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}Sleep(10);
    }
    if(!app.catalog_error.empty())std::cerr<<Utf8(app.catalog_error)<<'\n';
    CheckUI(!app.catalog.entries.empty()&&!app.catalog_future.valid(),"Actual XEX achievement catalog loads without starting the game");
    CheckUI(!fs::exists(profile),"Read-only catalog and progress loading never creates a profile or unlock save");
    uint64_t available=0;size_t icons=0;for(size_t i=0;i<app.catalog.entries.size();++i){const auto& entry=app.catalog.entries[i];available+=entry.info.gamerscore;CheckUI(entry.info.id&&!entry.info.label.empty()&&!entry.info.description.empty(),"Every actual game achievement has its original identity and text");if(app.achievement_icons[i])++icons;}
    CheckUI(icons==app.catalog.entries.size()&&available>0,"Original game artwork decodes for every achievement");
    std::cout<<"Catalog: "<<app.catalog.entries.size()<<" achievements, "<<available<<" G, "<<icons<<" original icons\n";
    const auto unlock_file=profile/L"achievements/434307F0.toml";fs::create_directories(unlock_file.parent_path());
    const auto first=app.catalog.entries[0].info.id,second=app.catalog.entries[1].info.id;
    const auto legacy_unlocks="unlocked = ["+std::to_string(first)+", "+std::to_string(second)+", 999999]\n";
    {std::ofstream f(unlock_file,std::ios::binary);f<<legacy_unlocks;}
    auto bytes=[&](const fs::path& file){std::ifstream f(file,std::ios::binary);return std::string(std::istreambuf_iterator<char>(f),{});};
    app.PollAchievements();CheckUI(app.unlocks.size()==3&&app.AchievementEarned(first)&&bytes(unlock_file)==legacy_unlocks,"Legacy unlock save is read without migration or writes");
    SendMessageW(app.achievement_filter,CB_SETCURSEL,1,0);app.Command(kAchievementFilter,CBN_SELCHANGE);
    CheckUI(app.achievement_rows.size()==2,"Unlocked list counts only known catalog IDs");
    SendMessageW(app.achievement_filter,CB_SETCURSEL,2,0);app.Command(kAchievementFilter,CBN_SELCHANGE);
    CheckUI(app.achievement_rows.size()==app.catalog.entries.size()-2,"Locked filter uses real save state");
    SendMessageW(app.achievement_filter,CB_SETCURSEL,0,0);SetWindowTextW(app.achievement_search,Wide(app.catalog.entries[0].info.label).c_str());app.FilterAchievements();
    CheckUI(!app.achievement_rows.empty()&&app.achievement_rows[0]==0,"Search finds original achievement labels");
    SetWindowTextW(app.achievement_search,L"no-such-achievement-fixture-98765");app.FilterAchievements();CheckUI(app.achievement_rows.empty(),"Search with no match has an empty state");SetWindowTextW(app.achievement_search,L"");app.FilterAchievements();
    const auto modern="[unlocked."+std::to_string(first)+"]\nfiletime = 123456789\n";
    {std::ofstream f(unlock_file,std::ios::binary);f<<modern;}app.PollAchievements();
    CheckUI(app.unlocks.size()==1&&app.unlocks.at(first)==123456789&&bytes(unlock_file)==modern,"Timestamped unlocks refresh without changing save bytes");
    {std::ofstream f(unlock_file,std::ios::binary);f<<"[broken";}app.PollAchievements();CheckUI(app.unlocks_stale&&app.unlocks.size()==1,"Corrupt or in-progress saves preserve last known progress");
    fs::remove(unlock_file);bool missing_rejected=false;try{achievements::ReadUnlocks(unlock_file);}catch(const std::exception&){missing_rejected=true;}CheckUI(missing_rejected,"A save disappearing during a read is never reported as an empty valid save");app.PollAchievements();CheckUI(app.unlocks_stale&&app.unlocks.size()==1,"SDK remove/rename save window cannot reset progress");
    {std::ofstream f(unlock_file,std::ios::binary);f<<legacy_unlocks;}app.PollAchievements();
    CheckUI(!app.unlocks_stale&&app.unlocks.size()==3,"Save retry recovers from transient failures");
    const auto prefs_file=profile/L"achievement-ui.toml";{std::ofstream f(prefs_file);f<<"sound_volume=17\nfuture_setting='keep me'\n";}
    app.PollAchievements();app.Command(kAchievementPopup,BN_CLICKED);app.Command(kAchievementSound,BN_CLICKED);
    auto options=motogp::achievements::ReadOptions(prefs_file);
    CheckUI(!options.popup&&!options.sound&&options.volume==17,"Actual launcher checkboxes save shared notification preferences without changing volume");
    CheckUI(toml::parse_file(prefs_file.string())["future_setting"].value_or(std::string{})=="keep me","Shared preference writes preserve unknown keys");
    app.Running(true);CheckUI(IsWindowEnabled(app.achievement_list)&&IsWindowEnabled(app.achievement_popup)&&!IsWindowEnabled(app.path),"Achievements remain usable while game settings are locked");app.Running(false);
    SendMessageW(app.achievement_filter,CB_SETCURSEL,1,0);app.FilterAchievements();
    const auto profile2=app.exe/L"second-achievement-profile";Put(app.config.table,"user_data_root",Utf8(profile2.wstring()));app.PollAchievements();
    CheckUI(app.unlocks_known&&!app.unlocks_stale&&app.unlocks.empty()&&app.achievement_rows.empty(),"Changing profile never leaks another profile's progress or filtered list");Put(app.config.table,"user_data_root",Utf8(profile.wstring()));app.PollAchievements();
    SendMessageW(app.achievement_filter,CB_SETCURSEL,0,0);app.FilterAchievements();
    app.ShowPage(5);RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);
    { // Actual OS mouse input, not a fabricated WM_VSCROLL / direct scroll call.
      RECT list;GetWindowRect(app.achievement_list,&list);RECT client;GetClientRect(app.achievement_list,&client);
      const auto rail=app.Rail(app.achievement_list,app.achievement_total,client.bottom,app.achievement_scroll);
      CheckUI(rail.maximum>0&&!(GetWindowLongPtrW(app.achievement_list,GWL_STYLE)&WS_VSCROLL),"Scroll rail has no native white scrollbar underneath");
      const int x=list.left+(rail.thumb.left+rail.thumb.right)/2;
      const int start=list.top+(rail.thumb.top+rail.thumb.bottom)/2;
      const int end=list.top+rail.track.bottom-6;std::atomic<bool> done=false,mouse_ok=true;
      std::thread mouse([&]{auto button=[&](DWORD flags){INPUT event{};event.type=INPUT_MOUSE;event.mi.dwFlags=flags;if(SendInput(1,&event,sizeof(event))!=1)mouse_ok=false;};
        SetCursorPos(x,start);button(MOUSEEVENTF_LEFTDOWN);Sleep(120);
        for(int i=1;i<=4;++i){SetCursorPos(x,start+(end-start)*i/4);Sleep(60);}button(MOUSEEVENTF_LEFTUP);done=true;
      });
      while(!done){MSG message;while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}Sleep(5);}mouse.join();
      MSG message;while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}UpdateWindow(app.achievement_list);
      CheckUI(mouse_ok&&app.achievement_scroll>0,"Dragging the actual painted scrollbar thumb scrolls achievements");
      SendMessageW(app.achievement_list,WM_KEYDOWN,VK_HOME,0);UpdateWindow(app.achievement_list);
    }

    SendMessageW(app.achievement_list,WM_KEYDOWN,VK_END,0);UpdateWindow(app.achievement_list);CheckUI(app.achievement_scroll>0,"Keyboard scroll reaches the final achievements");SendMessageW(app.achievement_list,WM_KEYDOWN,VK_HOME,0);UpdateWindow(app.achievement_list);CheckUI(app.achievement_scroll==0,"Keyboard scroll returns to the first achievement");
    for(double scale:{.75,1.,1.25,1.5}){
      app.Rescale(scale);RECT size{0,0,app.S(kWidth),app.S(kHeight+kCaptionHeight)};AdjustWindowRectEx(&size,style,FALSE,0);
      SetWindowPos(window,nullptr,20,20,size.right-size.left,size.bottom-size.top,SWP_NOZORDER);
      for(bool tr:{false,true}){
        app.turkish=tr;app.Localize();CheckPolish(app);
        for(int page=0;page<6;++page){app.ShowPage(page);CheckUI(bool(IsWindowVisible(app.path))==(page==3) && bool(IsWindowVisible(app.browse))==(page==3),"Game folder appears only on General");CheckUI(bool(IsWindowVisible(app.multiplayer_settings))==(page==3),"Multiplayer Settings button appears only on General");CheckUI(bool(IsWindowVisible(app.language))==(page==3),"Launcher language appears only on General");RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);LayoutCheck(app);
          for(int offset=160;offset<app.PageMaximum()+160;offset+=160){app.ScrollPage(offset);RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);LayoutCheck(app);}
          app.ScrollPage(0);RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);
          CheckSurface(app,app.page_view,app.S(App::kPageWidth)-1,app.S(App::kPageHeight)-1);
          CheckSurface(app,app.page_content,app.S(App::kFormWidth)-2,app.S(280));
          if(page==0)CheckSurface(app,app.sharp_value,app.S(35),app.S(22)-1);
          if(page==5){RECT list;GetClientRect(app.achievement_list,&list);CheckUI(list.bottom>=4*app.S(76)+3*app.S(6),"Achievement viewport fits at least four complete compact cards");}
          if(scale==1. || (scale==.75 && tr && page==2)){
            const auto tab=page==0?L"graphics":page==1?L"audio":page==2?L"controls":page==3?L"general":page==4?L"advanced":L"achievements";
            auto file=std::wstring(L"launcher-")+tab+(tr?L"-turkish":L"-english")+(scale==.75?L"-compact":L"")+L".bmp";Screenshot(window,pictures/file);if(scale==1.&&page==5&&!tr)Screenshot(window,pictures/L"launcher-achievements-full-window.bmp",true);
            if(scale==1.&&page==3&&!tr){app.caption_hover=HTCLOSE;app.PaintCaption();Screenshot(window,pictures/L"launcher-caption-hover.bmp");app.caption_hover=0;app.PaintCaption();}
          }
        }
      }
    }
    { // Regression for the reported stutter: tabs must never repopulate all
      // selectors / enumerate monitor modes or modify pending game settings.
      const auto generation=app.localization_generation;toml::table before;app.GetControls().Store(before);std::ostringstream original;original<<toml::toml_formatter{before};
      const auto begin=GetTickCount64();uint64_t slowest=0;
      for(int i=0;i<120;++i){const auto start=GetTickCount64();app.ShowPage(i%6);RedrawWindow(app.page_view,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);slowest=std::max<uint64_t>(slowest,GetTickCount64()-start);CheckUI(app.current_page==i%6,"Rapid switching leaves the requested tab selected");}
      CheckUI(app.localization_generation==generation,"Rapid tabs never rebuild selectors or query refresh modes");
      toml::table after;app.GetControls().Store(after);std::ostringstream current;current<<toml::toml_formatter{after};CheckUI(original.str()==current.str(),"Rapid switching preserves all unsaved settings");
      std::cout<<"120 painted tab switches: "<<GetTickCount64()-begin<<" ms total; slowest "<<slowest<<" ms\n";
      app.ShowPage(3);app.ScrollPage(0);
      const auto selected=SendMessageW(app.game_language,CB_GETCURSEL,0,0);
      SendMessageW(app.game_language,WM_MOUSEWHEEL,MAKEWPARAM(0,static_cast<WORD>(-WHEEL_DELTA)),0);
      CheckUI(SendMessageW(app.game_language,CB_GETCURSEL,0,0)==selected&&app.page_scroll[3]==0,"Wheel over closed Game language cannot change its value on a page that fits");
      SendMessageW(app.game_language,WM_MOUSEWHEEL,MAKEWPARAM(0,WHEEL_DELTA),0);
      CheckUI(SendMessageW(app.game_language,CB_GETCURSEL,0,0)==selected&&app.page_scroll[3]==0,"Reverse wheel preserves the selected language too");
      for(auto control:{app.effect,app.fullscreen,app.prompt_style,app.fps_limit}){
        const auto saved=SendMessageW(control,CB_GETCURSEL,0,0);SendMessageW(control,WM_MOUSEWHEEL,MAKEWPARAM(0,static_cast<WORD>(-WHEEL_DELTA)),0);CheckUI(saved==SendMessageW(control,CB_GETCURSEL,0,0),"A closed selector cannot be changed accidentally by the wheel");
      }
      app.ShowPage(0);app.ScrollPage(0);RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);
      CheckUI(!(GetWindowLongPtrW(app.page_view,GWL_STYLE)&WS_VSCROLL),"The page has no native scrollbar that could repaint white");
      const auto rail=app.Rail(app.page_view,app.page_extent[0],App::kPageHeight,0);
      for(int i=0;i<12;++i){SendMessageW(app.page_view,WM_NCPAINT,1,0);InvalidateRect(window,nullptr,FALSE);UpdateWindow(window);UpdateWindow(app.page_view);const auto dc=GetDC(app.page_view);const auto expected=CreateCompatibleDC(dc);const auto old=SelectObject(expected,app.paint_background_bitmap);POINT p{rail.track.left,rail.track.bottom-1};MapWindowPoints(app.page_view,window,&p,1);CheckUI(GetPixel(dc,rail.track.left,rail.track.bottom-1)==GetPixel(expected,p.x,p.y),"Parent/nonclient repaint retains the exact artwork behind the scroll rail");SelectObject(expected,old);DeleteDC(expected);ReleaseDC(app.page_view,dc);}
    }
    { // Regressions from the user's native Windows video.
      app.Rescale(1);SetWindowPos(window,nullptr,20,20,kWidth,kHeight+kCaptionHeight,SWP_NOZORDER);app.turkish=false;app.Localize();app.ShowPage(0);app.ScrollPage(0);
      CheckUI(GetWindowLongPtrW(app.page_view,GWL_EXSTYLE)&WS_EX_COMPOSITED,"The viewport composes its native children into a complete frame");
      std::map<HWND,RECT> fixed;
      for(auto c:app.pages[0]){RECT r;GetWindowRect(c,&r);MapWindowPoints(nullptr,app.page_content,reinterpret_cast<POINT*>(&r),2);fixed.emplace(c,r);}
      for(int i=0;i<48;++i){
        app.ScrollPage(i%2?app.PageMaximum():0);PumpUI();
        for(const auto& [c,before]:fixed){RECT now;GetWindowRect(c,&now);MapWindowPoints(nullptr,app.page_content,reinterpret_cast<POINT*>(&now),2);CheckUI(EqualRect(&before,&now),"Scrolling never repositions or resizes a native field within its content surface");}
        RECT panel;GetWindowRect(app.page_content,&panel);MapWindowPoints(nullptr,app.page_view,reinterpret_cast<POINT*>(&panel),2);CheckUI(panel.top==-app.S(app.page_scroll[0]),"Only the clipped content surface moves during scrolling");
        for(auto c:{app.effect,app.aspect,app.fullscreen,app.sharpness}){RECT r,view;GetWindowRect(c,&r);GetWindowRect(app.page_view,&view);POINT p{(r.left+r.right)/2,r.top+app.S(3)};if(!PtInRect(&view,p))continue;
          const auto dc=GetDC(window);ScreenToClient(window,&p);const auto pixel=GetPixel(dc,p.x,p.y);ReleaseDC(window,dc);
          CheckUI(pixel!=CLR_INVALID&&!(GetRValue(pixel)>225&&GetGValue(pixel)>225&&GetBValue(pixel)>225),"Visible fields stay dark after repeated scroll frames");
        }
      }
      app.ScrollPage(24);SetFocus(app.navigation[0]);RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);
      ClickControl(app.aspect,app.S(10));
      CheckUI(app.page_scroll[0]==24,"Clicking a low visible selector does not jump the page");
      CheckUI(SendMessageW(app.aspect,CB_GETDROPPEDSTATE,0,0),"The partially visible bottom selector opens on the first real mouse click");
      SendMessageW(app.aspect,CB_SHOWDROPDOWN,FALSE,0);SetFocus(app.navigation[0]);SetFocus(app.aspect);
      CheckUI(app.page_scroll[0]>24,"Keyboard focus still reveals the complete selector");
      app.ShowPage(3);app.ScrollPage(0);const auto generation=app.localization_generation;toml::table before;app.GetControls().Store(before);
      SendMessageW(app.language,CB_SHOWDROPDOWN,TRUE,0);SendMessageW(app.language,CB_SETCURSEL,1,0);app.Command(kLanguage,CBN_SELCHANGE);PumpUI();
      CheckUI(!app.turkish&&app.localization_generation==generation,"An open language popup is not rebuilt inside its native selection transaction");
      SendMessageW(app.language,CB_SHOWDROPDOWN,FALSE,0);PumpUI();
      CheckUI(app.turkish&&app.localization_generation==generation+1&&app.page_scroll[3]==0,"Closing the popup applies the language once without shifting the page");
      toml::table after;app.GetControls().Store(after);std::ostringstream a,b;a<<toml::toml_formatter{before};b<<toml::toml_formatter{after};CheckUI(a.str()==b.str(),"Deferred launcher localization preserves every pending game setting");
    }
    app.ShowPage(2);app.ScrollPage(0);app.Command(kCaptureBase+static_cast<int>(kBindings.size())-1,BN_CLICKED);
    CheckUI(app.capturing&&app.page_scroll[2]>0&&GetFocus()==app.binding_buttons.back(),"Focusing the final keyboard binding automatically reveals it in the compact viewport");
    {RECT field,panel;GetWindowRect(app.binding_buttons.back(),&field);GetWindowRect(app.page_view,&panel);
      CheckUI(field.top>=panel.top&&field.bottom<=panel.bottom,"The focused final keyboard capture button remains fully visible");}
    app.CancelCapture();
    for(auto maximum:{std::pair{1920,1080},std::pair{1366,768},std::pair{2560,1080},std::pair{3840,2160}}){
      auto sizes=WindowSizesFor(maximum);
      CheckUI(std::find(sizes.begin(),sizes.end(),maximum)!=sizes.end(),"Native monitor dimensions are offered");
      for(auto size:sizes)CheckUI(WindowSizeFits(size,maximum),"Output size never exceeds physical monitor");
      if(maximum==std::pair{1920,1080})CheckUI(std::find(sizes.begin(),sizes.end(),std::pair{2560,1440})==sizes.end() && std::find(sizes.begin(),sizes.end(),std::pair{3840,2160})==sizes.end(),"1080p monitor hides 1440p and 4K output");
    }
    for(const auto* mode:{"fsr2","fsr3"})for(const auto* quality:{"auto","nativeaa","quality","balanced","performance","ultra_performance"})for(const auto* aspect:{"auto","16:9","21:9","32:9"}){
      Settings requested;requested.effect=mode;requested.fsr_quality=quality;requested.aspect=aspect;
      app.SetControls(requested);const auto actual=app.GetControls();
      CheckUI(actual.effect==mode && actual.fsr_quality==quality && actual.aspect==aspect,"Experimental FSR quality and wide aspect controls round trip");
      CheckUI(IsWindowEnabled(app.fsr_quality),"Quality selector enabled only for temporal providers");
      toml::table config;actual.Store(config);const auto restored=Settings::Load(config);
      CheckUI(restored.effect==mode && restored.fsr_quality==quality && restored.aspect==aspect,"Experimental settings persist with their native types");
      const auto args=Arguments(game,actual,config);
      CheckUI(std::find(args.begin(),args.end(),L"--present_effect="+Wide(mode))!=args.end() && std::find(args.begin(),args.end(),L"--present_fsr_quality_mode="+Wide(quality))!=args.end() && std::find(args.begin(),args.end(),L"--aspect_ratio="+Wide(aspect))!=args.end(),"Child receives exact experimental options");
    }
    {auto initial=app.settings.adaptive;
      for(bool tr:{false,true})for(double scale:{.75,1.,1.25,1.5}){
        auto dialog_font=CreateFontW(-static_cast<int>(14*scale+.5),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Open Sans");
        struct FontCleanup{HFONT font;~FontCleanup(){DeleteObject(font);}} font_cleanup{dialog_font};
        AdaptiveDialog dialog(app.window,dialog_font,tr,scale,app.settings.adaptive);auto modal=dialog.Create();
        CheckUI(modal&&GetDlgItem(modal,100)&&GetDlgItem(modal,110)&&GetDlgItem(modal,120),"DualSense modal creates master and independent right/left controls");
        {NMCUSTOMDRAW draw{};draw.hdr.hwndFrom=GetDlgItem(modal,100);draw.hdr.code=NM_CUSTOMDRAW;draw.dwDrawStage=CDDS_PREPAINT;
          CheckUI(SendMessageW(modal,WM_NOTIFY,100,reinterpret_cast<LPARAM>(&draw))!=CDRF_SKIPDEFAULT,"The master checkbox keeps its button painter on themed Windows");}
        if(scale==1.){ShowWindow(modal,SW_SHOWNOACTIVATE);UpdateWindow(modal);Screenshot(modal,pictures/(tr?"dualsense-tr.bmp":"dualsense-en.bmp"));}
        RECT client;GetClientRect(modal,&client);
        for(auto id:{100,110,111,112,113,114,115,116,120,121,122,123,124,125,126,IDOK,IDCANCEL}){
          RECT r;GetWindowRect(GetDlgItem(modal,id),&r);MapWindowPoints(nullptr,modal,reinterpret_cast<POINT*>(&r),2);
          CheckUI(r.left>=0&&r.top>=0&&r.right<=client.right&&r.bottom<=client.bottom,"DualSense controls fit scaled modal");
        }
        {auto selector=GetDlgItem(modal,110);const auto mode=SendMessageW(selector,CB_GETCURSEL,0,0);SendMessageW(selector,WM_MOUSEWHEEL,MAKEWPARAM(0,static_cast<WORD>(-WHEEL_DELTA)),0);CheckUI(SendMessageW(selector,CB_GETCURSEL,0,0)==mode,"Closed DualSense selectors also ignore the mouse wheel");}
        CheckUI(!IsWindowEnabled(GetDlgItem(modal,110))&&!IsWindowEnabled(GetDlgItem(modal,111)),"Disabled master disables resistance editing");
        SendDlgItemMessageW(modal,100,BM_SETCHECK,BST_CHECKED,0);SendMessageW(modal,WM_COMMAND,MAKEWPARAM(100,BN_CLICKED),0);
        CheckUI(IsWindowEnabled(GetDlgItem(modal,112))&&!IsWindowEnabled(GetDlgItem(modal,122)),"Only a resistance-zone mode enables its end-position slider");
        SendDlgItemMessageW(modal,110,CB_SETCURSEL,1,0);SendMessageW(modal,WM_COMMAND,MAKEWPARAM(110,CBN_SELCHANGE),0);CheckUI(!IsWindowEnabled(GetDlgItem(modal,112))&&IsWindowEnabled(GetDlgItem(modal,111)),"Constant resistance ignores end position but keeps start available");
        SendDlgItemMessageW(modal,110,CB_SETCURSEL,2,0);SendMessageW(modal,WM_COMMAND,MAKEWPARAM(110,CBN_SELCHANGE),0);
        for(int id:{110,120}){wchar_t label[128]{};SendDlgItemMessageW(modal,id,CB_GETLBTEXT,2,reinterpret_cast<LPARAM>(label));CheckUI(std::wstring_view(label).find(L"Weapon")==std::wstring_view::npos,"Racing UI exposes resistance-zone terminology instead of Weapon");}
        SendDlgItemMessageW(modal,113,TBM_SETPOS,TRUE,8);
        CheckUI(dialog.Commit()&&app.settings.adaptive.enabled&&app.settings.adaptive.right.strength==8,"Modal commits all trigger controls");
        SendDlgItemMessageW(modal,111,TBM_SETPOS,TRUE,9);
        CheckUI(!dialog.Commit()&&app.settings.adaptive.right.start==3,"Invalid interval cannot partially replace pending launcher profile");
        DestroyWindow(modal);app.settings.adaptive=initial;
      }
      AdaptiveDialog dialog(app.window,app.font,false,1,app.settings.adaptive);auto modal=dialog.Create();SendDlgItemMessageW(modal,100,BM_SETCHECK,BST_CHECKED,0);SendMessageW(modal,WM_COMMAND,IDCANCEL,0);
      CheckUI(!dialog.window&&app.settings.adaptive==initial,"Cancel preserves all pending trigger values");
    }
    app.SetControls(Settings{});CheckUI(!IsWindowEnabled(app.fsr_quality),"Bilinear disables FSR quality selector");
    { // Multiplayer is an owned editor opened from General.
      const auto initial=app.settings;const int page=app.current_page;
      for(bool tr:{false,true})for(double scale:{.75,1.,1.25,1.5}){
        const auto dialog_font=CreateFontW(-static_cast<int>(14*scale+.5),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Open Sans");
        struct FontCleanup{HFONT font;~FontCleanup(){DeleteObject(font);}} font_cleanup{dialog_font};
        NetworkDialog dialog(window,dialog_font,tr,scale,app.settings);const auto modal=dialog.Create();ShowWindow(modal,SW_SHOWNOACTIVATE);UpdateWindow(modal);
        CheckUI(modal&&GetWindow(modal,GW_OWNER)==window,"Multiplayer creates an owned separate window");
        RECT client;GetClientRect(modal,&client);
        for(int id:std::array<int,8>{kLanEnabled,kLanPlayer,kLanRoom,kLanAdapter,kLanHost,kOnlineServer,IDOK,IDCANCEL}){if(!IsWindowVisible(GetDlgItem(modal,id)))continue;RECT r;GetWindowRect(GetDlgItem(modal,id),&r);MapWindowPoints(nullptr,modal,reinterpret_cast<POINT*>(&r),2);CheckUI(r.left>=0&&r.top>=0&&r.right<=client.right&&r.bottom<=client.bottom,"Multiplayer fields fit the modal at each DPI");}
        for(int mode=0;mode<3;++mode){
          SendDlgItemMessageW(modal,kLanEnabled,CB_SETCURSEL,mode,0);dialog.UpdateMode();SetDlgItemTextW(modal,kOnlineServer,L"https://play.example.com");
          const auto selector=GetDlgItem(modal,kLanEnabled);SendMessageW(selector,WM_MOUSEWHEEL,MAKEWPARAM(0,static_cast<WORD>(-WHEEL_DELTA)),0);CheckUI(SendMessageW(selector,CB_GETCURSEL,0,0)==mode,"Closed multiplayer selector ignores the mouse wheel");
          CheckUI(bool(IsWindowEnabled(GetDlgItem(modal,kOnlineServer)))==(mode==2)&&bool(IsWindowEnabled(GetDlgItem(modal,kLanAdapter)))==(mode==1)&&bool(IsWindowEnabled(GetDlgItem(modal,kLanHost)))==(mode==1),"Each connection mode enables only its relevant fields");
          for(int id:{kLanPlayer,kLanRoom,kLanAdapter,kLanHost,kOnlineServer})CheckUI(bool(IsWindowVisible(GetDlgItem(modal,id)))==(id==kOnlineServer?mode==2:(id==kLanAdapter||id==kLanHost)?mode==1:mode>0),"Only fields relevant to the selected connection mode are visible");
          GetClientRect(modal,&client);CheckUI(client.bottom==static_cast<int>((mode?316:158)*scale+.5),"Offline has a compact dialog without empty parameter rows");
          for(auto child=GetWindow(modal,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){if(!IsWindowVisible(child))continue;RECT r;GetWindowRect(child,&r);MapWindowPoints(nullptr,modal,reinterpret_cast<POINT*>(&r),2);CheckUI(r.left>=0&&r.top>=0&&r.right<=client.right&&r.bottom<=client.bottom,"Visible modal labels and controls fit each mode at each DPI");}
          CheckUI(dialog.Commit()&&app.GetControls().network_mode==std::array{"offline","lan","online"}[mode],"Multiplayer Apply preserves all three real connection modes");
          const auto current=app.GetControls();toml::table saved;current.Store(saved);const auto loaded=Settings::Load(saved);
          CheckUI(loaded.network_mode==current.network_mode&&loaded.online_server==current.online_server,"Applied multiplayer values survive normal launcher serialization");
          const auto arguments=Arguments(game,current,saved);CheckUI(std::count(arguments.begin(),arguments.end(),L"--network_mode="+Wide(current.network_mode))==1,"Applied multiplayer mode is forwarded to the game once");
          if(scale==1.){const auto prefix=mode==0?L"multiplayer-offline":mode==1?L"multiplayer":L"multiplayer-online";Screenshot(modal,pictures/(std::wstring(prefix)+(tr?L"-turkish.bmp":L"-english.bmp")));}
        }
        SetDlgItemTextW(modal,kOnlineServer,L"invalid server");const auto before=app.GetControls().online_server;CheckUI(!dialog.Commit()&&app.GetControls().online_server==before,"Invalid server cannot partially replace accepted network settings");
        SendDlgItemMessageW(modal,kLanEnabled,CB_SETCURSEL,1,0);SetDlgItemTextW(modal,kLanHost,L"999.1.1.1");CheckUI(!dialog.Commit(),"Invalid LAN IPv4 is rejected by existing validation");
        SendMessageW(modal,WM_COMMAND,IDCANCEL,0);CheckUI(!dialog.window&&app.current_page==page,"Cancelling the network dialog preserves the current launcher page");app.settings=initial;
      }
      NetworkDialog dialog(window,app.font,false,1,app.settings);auto modal=dialog.Create();SetDlgItemTextW(modal,kLanPlayer,L"Not applied");SendMessageW(modal,WM_COMMAND,IDCANCEL,0);CheckUI(app.settings.lan_player_name==initial.lan_player_name,"Cancel leaves pending network edits untouched");
      CheckUI(!GetDlgItem(window,kLanEnabled)&&!GetDlgItem(app.page_content,kLanEnabled),"Multiplayer controls no longer occupy General");
      std::atomic<bool> found=false;
      std::thread dismiss([&]{for(int i=0;i<500;++i){if(auto dialog=FindWindowW(L"MotoGPNetworkSettings",nullptr)){found=true;PostMessageW(dialog,WM_COMMAND,IDCANCEL,0);return;}Sleep(10);}});
      app.ShowPage(3);SendMessageW(app.multiplayer_settings,BM_CLICK,0,0);dismiss.join();
      CheckUI(found&&IsWindowEnabled(window)&&app.current_page==3&&!FindWindowW(L"MotoGPNetworkSettings",nullptr),"The General Multiplayer Settings button opens and closes its owned modal without changing the current page");
    }
    const auto saved_maximum=app.display_maximum;
    app.display_maximum={1920,1080};app.dimensions=WindowSizesFor(app.display_maximum);app.Localize();
    Settings oversize;oversize.width=3840;oversize.height=2160;app.SetControls(oversize);
    CheckUI(app.GetControls().width==0 && app.GetControls().height==0,"Oversized saved window uses desktop automatically");
    app.display_maximum=saved_maximum;app.dimensions=WindowSizesFor(saved_maximum);app.Localize();app.SetControls(Settings{});
    app.Rescale(1);app.turkish=false;app.ShowPage(0);
    for(auto [index,mode]:{std::pair{0,"fullscreen"},std::pair{1,"borderless"},std::pair{2,"windowed"}}){
      SendMessageW(app.fullscreen,CB_SETCURSEL,index,0);app.Command(kFullscreen,CBN_SELCHANGE);
      CheckUI(bool(IsWindowEnabled(app.refresh_rate))==(index==0),"Hz choice is active only for real fullscreen");
      SendMessageW(app.fps_limit,CB_SETCURSEL,5,0);
      auto chosen=app.GetControls();toml::table table;chosen.Store(table);
      const auto restored=Settings::Load(table);
      CheckUI(restored.display_mode==mode && restored.fullscreen==(index!=2),"Display mode and legacy fullscreen survive round trip");
      CheckUI(restored.fps_limit==144 && (index==0 || restored.refresh_rate==0),"FPS cap is independent of desktop Hz");
      const auto args=Arguments(game,chosen,table);
      CheckUI(std::count(args.begin(),args.end(),L"--display_mode="+Wide(mode))==1 && std::count(args.begin(),args.end(),L"--fps_limit=144")==1,"Mode and cap forwarded exactly once");
      app.turkish=!app.turkish;app.Localize();
      CheckUI(app.GetControls().display_mode==mode && app.GetControls().fps_limit==144,"Language change preserves mode and cap");
      for(size_t hz=1;hz<app.refresh_rates.size() && index==0;++hz){
        SendMessageW(app.refresh_rate,CB_SETCURSEL,hz,0);auto timed=app.GetControls();
        CheckUI(timed.refresh_rate==app.refresh_rates[hz] && timed.fps_limit==144,"Hardware Hz can vary independently from FPS cap");
      }
    }
    Settings legacy;legacy.fullscreen=false;app.SetControls(legacy);
    CheckUI(app.GetControls().display_mode=="windowed","Legacy fullscreen=false migrates to windowed");
    legacy.fullscreen=true;app.SetControls(legacy);
    CheckUI(app.GetControls().display_mode=="borderless","Legacy fullscreen=true preserves desktop fullscreen");
    Settings custom_rate;custom_rate.fps_limit=72;app.SetControls(custom_rate);app.Localize();
    CheckUI(app.GetControls().fps_limit==72,"Custom saved cap survives localization");
    app.Running(false);CheckUI(!IsWindowEnabled(app.refresh_rate),"Game exit keeps borderless Hz disabled");
    app.SetControls(Settings{});
    for(size_t field=0;field<kEngineEffects.size();++field){
      SendMessageW(app.engine_choices[field],CB_SETCURSEL,1,0);auto chosen=app.GetControls();toml::table table;chosen.Store(table);
      CheckUI(Settings::Load(table).engine_effects[field]=="off","Engine effect saved and restored");
      auto args=Arguments(game,chosen,table);CheckUI(std::count(args.begin(),args.end(),L"--"+Wide(kEngineEffects[field].key)+L"=off")==1,"Engine effect forwarded exactly once");
      app.turkish=!app.turkish;app.Localize();CheckUI(app.GetControls().engine_effects[field]=="off","Language change preserves engine override");
    }
    for(size_t distance=0;distance<kShadowDistances.size();++distance){
      SendMessageW(app.shadow_distance,CB_SETCURSEL,distance,0);auto chosen=app.GetControls();toml::table table;chosen.Store(table);
      CheckUI(Settings::Load(table).shadow_distance==kShadowDistances[distance],"Shadow distance round trip");
    }
    app.SetControls(Settings{});
    for(unsigned preset=0;preset<kRenderPresets.size();++preset){
      SendMessageW(app.render_resolution,CB_SETCURSEL,preset,0);auto s=app.GetControls();
      CheckUI(RenderDimensions(s)==std::pair{kRenderPresets[preset].width,kRenderPresets[preset].height},"UI preset maps to exact render dimensions");
      toml::table table;s.Store(table);auto restored=Settings::Load(table);
      CheckUI(MatchingRenderPreset(restored)==static_cast<int>(preset),"Render preset round trip");
      const auto args=Arguments(game,s,table);
      CheckUI(std::count(args.begin(),args.end(),L"--video_mode_width=1280")==1 && std::count(args.begin(),args.end(),L"--video_mode_height=720")==1,"Native source forwarded once");
    }
    for(auto [index,style]:{std::pair{0,"auto"},std::pair{1,"playstation"},std::pair{2,"xbox"},std::pair{3,"keyboard"}}){
      SendMessageW(app.prompt_style,CB_SETCURSEL,index,0);
      const auto chosen=app.GetControls();toml::table table;chosen.Store(table);
      CheckUI(Settings::Load(table).prompt_style==style,"Prompt style survives save/load");
      auto args=Arguments(game,chosen,table);
      CheckUI(std::count(args.begin(),args.end(),L"--button_prompt_style="+Wide(style))==1,"Selected style forwarded exactly once");
      app.turkish=!app.turkish;app.Localize();
      CheckUI(app.GetControls().prompt_style==style,"Language change preserves prompt style");
    }
    for(auto [index,id]:{std::pair{0,1},std::pair{1,4},std::pair{2,3},std::pair{3,5},std::pair{4,6},std::pair{5,1000}}){
      SendMessageW(app.game_language,CB_SETCURSEL,index,0);
      const auto chosen=app.GetControls();toml::table table;chosen.Store(table);
      CheckUI(Settings::Load(table).game_language==id,"Game language survives save/load");
      CheckUI(Get(table,"game_text_language",std::string{})==(id==1000?"turkish":"original"),"Turkish text selection is separate from native voice enum");
      auto args=Arguments(game,chosen,table);
      CheckUI(std::count(args.begin(),args.end(),L"--user_language="+std::to_wstring(id==1000?1:id))==1,"Game language forwarded once");
      app.turkish=!app.turkish;app.Localize();
      CheckUI(app.GetControls().game_language==id,"Launcher language does not alter game language");
    }
    auto custom=Settings{};custom.scale_x=2;custom.scale_y=3;custom.guest_width=1600;custom.guest_height=900;
    app.settings=custom;app.SetControls(custom);
    CheckUI(SendMessageW(app.render_resolution,CB_GETCURSEL,0,0)==3,"Custom asymmetric rendering retained");
    auto kept=app.GetControls();CheckUI(kept.scale_x==2 && kept.scale_y==3 && kept.guest_width==1600 && kept.guest_height==900,"Custom mode not silently changed");
    app.turkish=true;app.Localize();CheckUI(app.GetControls().guest_width==1600,"Language change preserves custom rendering");
    app.Command(kDefaults,BN_CLICKED);app.ShowPage(2);
    for(size_t i=0;i<kBindings.size();++i){SetWindowTextW(app.binding_fields[i],L"Ctrl+K");CheckUI(app.GetControls().bindings[i]=="Ctrl+K","Every direct binding field updates its actual setting");}
    app.Command(kCaptureBase,BN_CLICKED);MSG key{};key.message=WM_KEYDOWN;key.wParam=VK_RETURN;
    CheckUI(app.CaptureKey(key) && app.GetControls().bindings[0]=="Return","Inline key capture updates field");
    app.Command(kCaptureBase,BN_CLICKED);key.wParam=VK_ESCAPE;
    CheckUI(app.CaptureKey(key) && app.GetControls().bindings[0]=="Return" && !app.capturing,"Escape cancels capture without changing field");
    app.Command(kResetBinding,BN_CLICKED);CheckUI(app.GetControls().bindings[0]==kBindings[0].defaults,"Action reset restores only selected binding");
    app.Command(kClearBinding,BN_CLICKED);CheckUI(app.GetControls().bindings[0].empty(),"Individual action can be unbound");
    app.Command(kMsaa,BN_CLICKED);CheckUI(!app.GetControls().msaa,"Owner-drawn checkbox toggles setting");app.Command(kMsaa,BN_CLICKED);
    app.ShowPage(0);SendMessageW(app.render_resolution,CB_SETCURSEL,2,0);SendMessageW(app.volume,TBM_SETPOS,TRUE,31);
    app.Command(kVsync,BN_CLICKED);Put(app.config.table,"custom_note",std::string("keep me"));app.SaveSettings();
    Config saved(app.config.path);auto s=Settings::Load(saved.table);
    CheckUI(s.scale_x==3 && s.scale_y==3 && s.guest_width==1280 && s.guest_height==720,"Saved 4K preset retains native source");
    CheckUI(s.volume==31 && !s.vsync && s.msaa && s.bindings[0].empty(),"Actual UI values saved correctly");
    CheckUI(Get(saved.table,"custom_note",std::string{})=="keep me","Unrelated settings preserved");
    SendMessageW(app.language,CB_SETCURSEL,1,0);app.Command(kLanguage,CBN_SELCHANGE);
    PumpUI();
    Config preferences(app.preferences.path);CheckUI(Get(preferences.table,"language",std::string{})=="tr","UI language selection saved");
    {
      // Simulate a real child process saving live values while launcher fields
      // are disabled. Its exit must refresh both controls and config baseline.
      std::wstring command=Quote((ExecutableFolder()/L"launcher_layout_probe.exe").wstring())+L" --fixture-child";
      STARTUPINFOW startup{sizeof(startup)};PROCESS_INFORMATION child{};
      CheckUI(CreateProcessW(nullptr,command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&child),"Start actual launcher child fixture");
      CloseHandle(child.hThread);app.process=child.hProcess;app.Running(true);CheckUI(!IsWindowEnabled(app.multiplayer_settings),"Multiplayer launch-time options are disabled while a game runs");
      Config external(app.config.path);Put(external.table,"audio_master_volume",58);Put(external.table,"fps_limit",165);Put(external.table,"keybind_a",std::string("Ctrl+J"));external.Save();
      CheckUI(WaitForSingleObject(app.process,10000)==WAIT_OBJECT_0,"Child fixture exited");app.Poll();
      CheckUI(!app.process&&app.GetControls().volume==58&&app.GetControls().fps_limit==165&&app.GetControls().bindings[0]=="Ctrl+J","Child exit reloads live values into actual launcher controls");
      app.SaveSettings();Config after(app.config.path);auto current=Settings::Load(after.table);
      CheckUI(current.volume==58&&current.fps_limit==165&&current.bindings[0]=="Ctrl+J"&&Get(after.table,"custom_note",std::string{})=="keep me","Next launcher save preserves live edits and unrelated config");
    }
    { // Caption commands still work after replacing the native frame.
      auto caption_click=[&](int hit,int offset){RECT r;GetWindowRect(window,&r);const auto c=app.CaptionBounds();POINT p{r.right-app.S(offset),r.top+c.height/2};SetCursorPos(p.x,p.y);SendMessageW(window,WM_NCLBUTTONDOWN,hit,MAKELPARAM(p.x,p.y));SendMessageW(window,WM_LBUTTONUP,0,0);};
      caption_click(HTMINBUTTON,69);CheckUI(IsIconic(window),"Custom caption minimizes the actual launcher window");ShowWindow(window,SW_RESTORE);UpdateWindow(window);
      caption_click(HTCLOSE,23);CheckUI(!IsWindow(window),"Custom caption closes the actual launcher window");
    }
    std::cout<<ui_checks<<" actual launcher UI checks passed\n";
  }catch(const std::exception& e){std::cerr<<"FAIL after "<<ui_checks<<" checks: "<<e.what()<<'\n';result=1;}
  Gdiplus::GdiplusShutdown(token);CoUninitialize();return result;
}
