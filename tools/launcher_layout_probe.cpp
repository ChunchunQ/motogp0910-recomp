// Real launcher controls, configuration and Win32 painting. This optional
// target includes production UI code; it does not replace any runtime behavior.
#include "../src/launcher/main.cpp"
#include <iostream>

namespace {
int ui_checks=0;
void CheckUI(bool ok,const char* message){if(!ok)throw std::runtime_error(message);++ui_checks;}
void Screenshot(HWND window,const fs::path& file){
  RECT r;GetClientRect(window,&r);auto dc=GetDC(window),copy=CreateCompatibleDC(dc);
  BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=r.right;
  info.bmiHeader.biHeight=-r.bottom;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
  void* data=nullptr;auto bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&data,nullptr,0);auto old=SelectObject(copy,bitmap);
  CheckUI(bitmap && BitBlt(copy,0,0,r.right,r.bottom,dc,0,0,SRCCOPY),"Capture actual launcher window");
  const DWORD bytes=r.right*r.bottom*4;BITMAPFILEHEADER header{};header.bfType=0x4D42;
  header.bfOffBits=sizeof(header)+sizeof(info.bmiHeader);header.bfSize=header.bfOffBits+bytes;
  std::ofstream output(file,std::ios::binary);output.write(reinterpret_cast<const char*>(&header),sizeof(header));
  output.write(reinterpret_cast<const char*>(&info.bmiHeader),sizeof(info.bmiHeader));output.write(static_cast<const char*>(data),bytes);
  CheckUI(bool(output),"Screenshot saved");SelectObject(copy,old);DeleteObject(bitmap);DeleteDC(copy);ReleaseDC(window,dc);
}
void LayoutCheck(App& app){
  RECT client;GetClientRect(app.window,&client);std::vector<std::pair<RECT,HWND>> visible;
  for(auto& item:app.placements){if(!IsWindowVisible(item.control))continue;RECT r;GetWindowRect(item.control,&r);
    MapWindowPoints(nullptr,app.window,reinterpret_cast<POINT*>(&r),2);
    CheckUI(r.left>=0 && r.top>=0 && r.right<=client.right && r.bottom<=client.bottom,"Visible control fits client area");
    for(auto& [previous,control]:visible){RECT overlap;const bool intersects=IntersectRect(&overlap,&previous,&r);
      if(intersects)std::cerr<<"Overlap: scale="<<app.scale<<" page="<<app.current_page<<" tr="<<app.turkish
        <<" first="<<GetDlgCtrlID(control)<<" second="<<GetDlgCtrlID(item.control)
        <<" region="<<overlap.left<<","<<overlap.top<<","<<overlap.right<<","<<overlap.bottom<<'\n';
      CheckUI(!intersects,"Visible controls do not overlap");}
    visible.emplace_back(r,item.control);wchar_t cls[32];GetClassNameW(item.control,cls,32);
    if(std::wstring_view(cls)==L"STATIC"){
      auto dc=GetDC(item.control);auto previous=SelectObject(dc,reinterpret_cast<HFONT>(SendMessageW(item.control,WM_GETFONT,0,0)));
      RECT measured{0,0,r.right-r.left,0};auto text=app.Text(item.control);
      DrawTextW(dc,text.c_str(),-1,&measured,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);
      CheckUI(measured.bottom<=r.bottom-r.top,"Localized static text fits its height");
      SelectObject(dc,previous);ReleaseDC(item.control,dc);
    }
  }
}
}
int wmain(int argc,wchar_t** argv){
  if(argc==2&&std::wstring_view(argv[1])==L"--fixture-child")return 0;
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
  Gdiplus::GdiplusStartupInput gdip;ULONG_PTR token{};Gdiplus::GdiplusStartup(&token,&gdip,nullptr);
  INITCOMMONCONTROLSEX common{sizeof(common),ICC_STANDARD_CLASSES|ICC_BAR_CLASSES};InitCommonControlsEx(&common);
  int result=0;
  try{
    if(argc!=3)throw std::runtime_error("Usage: launcher_ui_probe GAME_ROOT SCREENSHOT_DIRECTORY (run in an isolated executable folder)");
    const auto pictures=fs::path(argv[2]);fs::create_directories(pictures);
    App app;app.scale=1;const auto game=fs::path(argv[1]);
    WNDCLASSW cls{};cls.hInstance=GetModuleHandleW(nullptr);cls.lpfnWndProc=WindowProc;cls.lpszClassName=L"MotoGPLauncherUIProbe";cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);RegisterClassW(&cls);
    RECT bounds{0,0,kWidth,kHeight};constexpr DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU;
    AdjustWindowRectEx(&bounds,style,FALSE,0);
    auto window=CreateWindowExW(0,cls.lpszClassName,L"MotoGP 09/10 Launcher",style,20,20,bounds.right-bounds.left,bounds.bottom-bounds.top,nullptr,nullptr,cls.hInstance,&app);
    CheckUI(window!=nullptr,"Production UI window created");ShowWindow(window,SW_SHOW);SetForegroundWindow(window);UpdateWindow(window);
    SetWindowTextW(app.path,game.c_str());app.ReadyStatus();
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
    SendMessageW(app.achievement_list,WM_KEYDOWN,VK_END,0);UpdateWindow(app.achievement_list);CheckUI(app.achievement_scroll>0,"Keyboard scroll reaches the final achievements");SendMessageW(app.achievement_list,WM_KEYDOWN,VK_HOME,0);UpdateWindow(app.achievement_list);CheckUI(app.achievement_scroll==0,"Keyboard scroll returns to the first achievement");
    for(double scale:{.75,1.,1.25,1.5}){
      app.Rescale(scale);RECT size{0,0,app.S(kWidth),app.S(kHeight)};AdjustWindowRectEx(&size,style,FALSE,0);
      SetWindowPos(window,nullptr,20,20,size.right-size.left,size.bottom-size.top,SWP_NOZORDER);
      for(bool tr:{false,true}){
        app.turkish=tr;
        for(int page=0;page<6;++page){app.ShowPage(page);CheckUI(bool(IsWindowVisible(app.path))==(page==3) && bool(IsWindowVisible(app.browse))==(page==3),"Game folder appears only on General");CheckUI(bool(IsWindowVisible(app.language))==(page==3),"Launcher language appears only on General");RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);LayoutCheck(app);
          if(scale==1. || (scale==.75 && tr && page==2)){
            const auto tab=page==0?L"graphics":page==1?L"audio":page==2?L"controls":page==3?L"general":page==4?L"advanced":L"achievements";
            auto file=std::wstring(L"launcher-")+tab+(tr?L"-turkish":L"-english")+(scale==.75?L"-compact":L"")+L".bmp";Screenshot(window,pictures/file);
          }
        }
      }
    }
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
    app.SetControls(Settings{});CheckUI(!IsWindowEnabled(app.fsr_quality),"Bilinear disables FSR quality selector");
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
    for(auto [index,id]:{std::pair{0,1},std::pair{1,4},std::pair{2,3},std::pair{3,5},std::pair{4,6}}){
      SendMessageW(app.game_language,CB_SETCURSEL,index,0);
      const auto chosen=app.GetControls();toml::table table;chosen.Store(table);
      CheckUI(Settings::Load(table).game_language==id,"Game language survives save/load");
      auto args=Arguments(game,chosen,table);
      CheckUI(std::count(args.begin(),args.end(),L"--user_language="+std::to_wstring(id))==1,"Game language forwarded once");
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
    Config preferences(app.preferences.path);CheckUI(Get(preferences.table,"language",std::string{})=="tr","UI language selection saved");
    {
      // Simulate a real child process saving live values while launcher fields
      // are disabled. Its exit must refresh both controls and config baseline.
      std::wstring command=Quote((ExecutableFolder()/L"launcher_layout_probe.exe").wstring())+L" --fixture-child";
      STARTUPINFOW startup{sizeof(startup)};PROCESS_INFORMATION child{};
      CheckUI(CreateProcessW(nullptr,command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&child),"Start actual launcher child fixture");
      CloseHandle(child.hThread);app.process=child.hProcess;app.Running(true);
      Config external(app.config.path);Put(external.table,"audio_master_volume",58);Put(external.table,"fps_limit",165);Put(external.table,"keybind_a",std::string("Ctrl+J"));external.Save();
      CheckUI(WaitForSingleObject(app.process,10000)==WAIT_OBJECT_0,"Child fixture exited");app.Poll();
      CheckUI(!app.process&&app.GetControls().volume==58&&app.GetControls().fps_limit==165&&app.GetControls().bindings[0]=="Ctrl+J","Child exit reloads live values into actual launcher controls");
      app.SaveSettings();Config after(app.config.path);auto current=Settings::Load(after.table);
      CheckUI(current.volume==58&&current.fps_limit==165&&current.bindings[0]=="Ctrl+J"&&Get(after.table,"custom_note",std::string{})=="keep me","Next launcher save preserves live edits and unrelated config");
    }
    DestroyWindow(window);std::cout<<ui_checks<<" actual launcher UI checks passed\n";
  }catch(const std::exception& e){std::cerr<<"FAIL after "<<ui_checks<<" checks: "<<e.what()<<'\n';result=1;}
  Gdiplus::GdiplusShutdown(token);CoUninitialize();return result;
}
