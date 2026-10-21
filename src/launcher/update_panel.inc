  // Included inside App. Network work never runs on the window thread.
  HWND update_notice{},update_panel{},update_notes{},update_scroll{},update_action{},update_cancel{},update_close{};
  bool update_close_hot=false;
  ScrollDrag update_drag;
  int update_wheel=0;
  std::optional<updates::Release> update_release;
  std::future<updates::CheckResult> update_check;
  std::future<updates::StagedUpdate> update_download;
  std::future<std::unique_ptr<updates::Installation>> update_prepare;
  std::unique_ptr<updates::Installation> update_installation;
  std::optional<updates::StagedUpdate> update_staged;
  std::atomic_bool update_cancelled{false};
  std::atomic<uint64_t> update_received{0},update_total{0};
  bool update_finishing=false,update_busy=false,update_preparing=false,update_handoff=false,update_open=false;
  bool update_preview=false;
  std::vector<std::pair<HWND,bool>> update_disabled_controls;
  int update_panel_height=0,update_animation_from=0;
  ULONGLONG update_animation_start=0;
  static constexpr int kUpdateWidth=390,kUpdateHeight=360,kUpdateNotesY=58,kUpdateNotesHeight=240,kUpdateActionY=310;

  void StopUpdates(){
    update_cancelled=true;
    if(update_check.valid())update_check.wait();
    if(update_download.valid()){
      try{auto staged=update_download.get();updates::DiscardStaged(staged);}catch(...){}
    }
    if(update_prepare.valid()){
      try{update_installation=update_prepare.get();}catch(...){}
    }
    update_installation.reset();
    if(update_staged&&!update_handoff)updates::DiscardStaged(*update_staged);
  }
  void SetUpdatePreparation(bool preparing){
    update_preparing=preparing;
    if(preparing){
      for(auto c:{page_view,path,browse,save,defaults,language,verify_files,play}){update_disabled_controls.emplace_back(c,IsWindowEnabled(c)!=FALSE);EnableWindow(c,FALSE);}
    }else{
      for(auto [c,enabled]:update_disabled_controls)EnableWindow(c,enabled);
      update_disabled_controls.clear();
    }
  }
  bool UpdateShortcut(const MSG& message){
    if(message.message==WM_KEYDOWN&&message.wParam==VK_ESCAPE&&update_open){ToggleUpdatePanel();return true;}return false;
  }
  bool UpdateGameRunning()const{return process||updates::GameRunning(exe);}
  void LayoutUpdatePanel(){
    if(!update_panel)return;
    SetWindowPos(update_panel,HWND_TOP,S(kWidth-kUpdateWidth-18),S(kCaptionHeight+18),S(kUpdateWidth),S(update_panel_height),SWP_NOACTIVATE|SWP_NOCOPYBITS);
    if(update_action)SetWindowPos(update_action,nullptr,S(16),S(kUpdateActionY),S(kUpdateWidth-32-(update_busy?112:0)),S(34),SWP_NOZORDER|SWP_NOACTIVATE);
    if(update_cancel)SetWindowPos(update_cancel,nullptr,S(kUpdateWidth-116),S(kUpdateActionY),S(100),S(34),SWP_NOZORDER|SWP_NOACTIVATE);
    if(update_scroll)InvalidateRect(update_scroll,nullptr,FALSE);
    if(update_notice)SetWindowPos(update_notice,HWND_TOP,S(kWidth-138),0,S(46),S(kCaptionHeight),SWP_NOACTIVATE);
  }
  void ToggleUpdatePanel(){
    if(!update_release)return;
    CancelCapture();help.Pop();update_close_hot=false;InvalidateRect(update_close,nullptr,FALSE);update_open=!update_open;update_animation_from=update_panel_height;update_animation_start=GetTickCount64();
    ShowWindow(update_panel,SW_SHOW);SetWindowPos(update_panel,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
    SetTimer(window,5,16,nullptr);
    if(!update_open&&IsChild(update_panel,GetFocus()))SetFocus(update_notice);
  }
  void AnimateUpdatePanel(){
    const double t=std::min(1.0,(GetTickCount64()-update_animation_start)/180.0),ease=1.0-std::pow(1.0-t,3.0);
    const int target=update_open?kUpdateHeight:0;
    update_panel_height=static_cast<int>(update_animation_from+(target-update_animation_from)*ease);
    LayoutUpdatePanel();
    if(t>=1){KillTimer(window,5);if(!update_open)ShowWindow(update_panel,SW_HIDE);}
  }
  void DrawUpdateNotice(HDC dc,HWND control,bool pressed){
    DrawSurface(dc,control);RECT r;GetClientRect(control,&r);
    Gdiplus::Graphics g(dc);g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    if(pressed){Gdiplus::SolidBrush shade(Gdiplus::Color(45,255,255,255));g.FillRectangle(&shade,0,0,r.right,r.bottom);}
    Gdiplus::Pen pen(Gdiplus::Color(255,255,106,19),static_cast<float>(1.5*scale));
    const float cx=r.right*.5f,cy=r.bottom*.5f,unit=static_cast<float>(scale);
    // Download arrow and tray, drawn as vectors for every launcher DPI scale.
    g.DrawLine(&pen,cx,cy-7*unit,cx,cy+2*unit);
    g.DrawLine(&pen,cx-4*unit,cy-2*unit,cx,cy+2*unit);
    g.DrawLine(&pen,cx+4*unit,cy-2*unit,cx,cy+2*unit);
    g.DrawLine(&pen,cx-6*unit,cy+4*unit,cx-6*unit,cy+7*unit);
    g.DrawLine(&pen,cx-6*unit,cy+7*unit,cx+6*unit,cy+7*unit);
    g.DrawLine(&pen,cx+6*unit,cy+7*unit,cx+6*unit,cy+4*unit);
  }
  void DrawUpdateClose(HDC dc,HWND control,bool pressed,bool focused){
    DrawSurface(dc,control);RECT bounds;GetClientRect(control,&bounds);
    {
    Gdiplus::Graphics graphics(dc);graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    if(update_close_hot||pressed){
      Gdiplus::SolidBrush shade(Gdiplus::Color(pressed?65:35,255,255,255));
      graphics.FillRectangle(&shade,0,0,bounds.right,bounds.bottom);
    }
    const float cx=bounds.right*.5f,cy=bounds.bottom*.5f,half=static_cast<float>(6*scale);
    Gdiplus::Pen pen(Gdiplus::Color(245,242,244,247),static_cast<float>(1.6*scale));
    pen.SetStartCap(Gdiplus::LineCapRound);pen.SetEndCap(Gdiplus::LineCapRound);
    graphics.DrawLine(&pen,cx-half,cy-half,cx+half,cy+half);
    graphics.DrawLine(&pen,cx+half,cy-half,cx-half,cy+half);
    }
    if(focused){InflateRect(&bounds,-S(3),-S(3));DrawFocusRect(dc,&bounds);}
  }
  static LRESULT CALLBACK UpdateCloseProc(HWND control,UINT message,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR data){
    auto& app=*reinterpret_cast<App*>(data);
    if(message==WM_SETCURSOR&&LOWORD(lp)==HTCLIENT){SetCursor(LoadCursorW(nullptr,IDC_HAND));return TRUE;}
    if(message==WM_MOUSEMOVE){
      if(!app.update_close_hot){app.update_close_hot=true;InvalidateRect(control,nullptr,FALSE);}
      TRACKMOUSEEVENT track{sizeof(track),TME_LEAVE,control,0};TrackMouseEvent(&track);
    }
    if(message==WM_MOUSELEAVE||message==WM_CANCELMODE){app.update_close_hot=false;InvalidateRect(control,nullptr,FALSE);}
    if(message==WM_NCDESTROY)RemoveWindowSubclass(control,UpdateCloseProc,id);
    return DefSubclassProc(control,message,wp,lp);
  }
  void RefreshUpdatePanel(){
    RefreshSteam();
    RefreshDlc();
    if(!update_panel||!update_release)return;
    SetWindowTextW(update_notice,T(L"Launcher update",L"Launcher güncellemesi"));
    SetWindowTextW(update_close,T(L"Close update panel",L"Güncelleme panelini kapat"));
    Tip(update_close,L"Close update panel",L"Güncelleme panelini kapat");
    auto notes=update_release->NotesFor(turkish?"tr":"en");
    if(notes.empty())notes=turkish?"Bu sürüm için değişiklik notu yok.":"No release notes for this version.";
    auto wide=Wide(notes);std::wstring multiline;
    for(size_t i=0;i<wide.size();++i){if(wide[i]==L'\n'&&(i==0||wide[i-1]!=L'\r'))multiline+=L'\r';multiline+=wide[i];}
    if(Text(update_notes)!=multiline){SetWindowTextW(update_notes,multiline.c_str());SendMessageW(update_notes,EM_SETSEL,0,0);SendMessageW(update_notes,EM_LINESCROLL,0,-SendMessageW(update_notes,EM_GETFIRSTVISIBLELINE,0,0));}
    const bool game=UpdateGameRunning();
    std::wstring action=T(L"Update now",L"Şimdi güncelle");
    if(update_preparing)action=T(L"Preparing installation…",L"Kurulum hazırlanıyor…");
    else if(update_busy&&update_cancelled)action=T(L"Cancelling…",L"İptal ediliyor…");
    else if(update_busy){
      const auto total=update_total.load(),received=update_received.load();
      action=T(L"Downloading…",L"İndiriliyor…");
      if(total)action+=L" "+std::to_wstring(std::min<uint64_t>(100,received*100/total))+L"%";
    }else if(game)action=T(L"Close the game to update",L"Güncellemek için oyunu kapat");
    if(Text(update_action)!=action)SetWindowTextW(update_action,action.c_str());
    EnableWindow(update_action,!dlc_busy&&!steam_busy&&!update_finishing&&!update_busy&&!importing_iso&&!verifying&&!game&&(update_preview||update_release->CanInstall()));
    SetWindowTextW(update_cancel,T(L"Cancel",L"İptal"));
    ShowWindow(update_cancel,update_busy?SW_SHOW:SW_HIDE);EnableWindow(update_cancel,!update_preparing&&!update_cancelled);
    EnableWindow(play,game_ready&&!dlc_busy&&!steam_busy&&!update_finishing&&!update_busy&&!process&&!verifying&&!importing_iso);
    LayoutUpdatePanel();
    InvalidateRect(update_panel,nullptr,FALSE);
  }
  std::array<int,3> UpdateScrollState()const{
    RECT text{};SendMessageW(update_notes,EM_GETRECT,0,reinterpret_cast<LPARAM>(&text));
    const auto dc=GetDC(update_notes);const auto old=SelectObject(dc,reinterpret_cast<HFONT>(SendMessageW(update_notes,WM_GETFONT,0,0)));
    TEXTMETRICW metrics{};GetTextMetricsW(dc,&metrics);SelectObject(dc,old);ReleaseDC(update_notes,dc);
    const int visible=std::max(1,int(text.bottom-text.top)/std::max(1,int(metrics.tmHeight)));
    return {int(SendMessageW(update_notes,EM_GETLINECOUNT,0,0)),visible,int(SendMessageW(update_notes,EM_GETFIRSTVISIBLELINE,0,0))};
  }
  void ScrollUpdateNotes(int requested){
    const auto state=UpdateScrollState();const int target=std::clamp(requested,0,std::max(0,state[0]-state[1]));
    SendMessageW(update_notes,EM_LINESCROLL,0,target-state[2]);InvalidateRect(update_scroll,nullptr,FALSE);
  }
  void WheelUpdateNotes(WPARAM wp){
    update_wheel+=GET_WHEEL_DELTA_WPARAM(wp);const int ticks=update_wheel/WHEEL_DELTA;update_wheel%=WHEEL_DELTA;
    UINT lines=3;SystemParametersInfoW(SPI_GETWHEELSCROLLLINES,0,&lines,0);const auto state=UpdateScrollState();
    ScrollUpdateNotes(state[2]-ticks*(lines==WHEEL_PAGESCROLL?state[1]:int(std::min<UINT>(lines,100))));
  }
  static LRESULT CALLBACK UpdateNotesProc(HWND control,UINT message,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR data){
    auto& app=*reinterpret_cast<App*>(data);
    if(message==WM_MOUSEWHEEL){app.WheelUpdateNotes(wp);return 0;}
    if(message==WM_KEYDOWN&&wp==VK_ESCAPE){if(app.update_open)app.ToggleUpdatePanel();return 0;}
    const auto result=DefSubclassProc(control,message,wp,lp);
    if(message==WM_KEYDOWN||message==WM_VSCROLL||message==WM_SIZE||message==WM_SETTEXT||message==EM_LINESCROLL)
      InvalidateRect(app.update_scroll,nullptr,FALSE);
    if(message==WM_NCDESTROY)RemoveWindowSubclass(control,UpdateNotesProc,id);
    return result;
  }
  static LRESULT CALLBACK UpdateScrollProc(HWND control,UINT message,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR data){
    auto& app=*reinterpret_cast<App*>(data);
    if(message==WM_NCDESTROY){RemoveWindowSubclass(control,UpdateScrollProc,id);return DefSubclassProc(control,message,wp,lp);}
    if(message==WM_ERASEBKGND)return 1;
    if(message==WM_MOUSEWHEEL){app.WheelUpdateNotes(wp);return 0;}
    const auto state=app.UpdateScrollState();int next=state[2];
    if(app.RailInput(control,message,lp,state[0],state[1],next,app.update_drag)){
      app.ScrollUpdateNotes(next);return 0;
    }
    if(message==WM_PAINT||message==WM_PRINTCLIENT){
      PAINTSTRUCT paint{};const auto dc=message==WM_PAINT?BeginPaint(control,&paint):reinterpret_cast<HDC>(wp);
      RECT bounds;GetClientRect(control,&bounds);FillRect(dc,&bounds,app.card);
      const auto rail=app.Rail(control,state[0],state[1],state[2]);
      if(rail.maximum)app.Rounded(dc,rail.thumb,RGB(85,93,107),RGB(85,93,107),6);
      if(message==WM_PAINT)EndPaint(control,&paint);return 0;
    }
    return DefSubclassProc(control,message,wp,lp);
  }
  static LRESULT CALLBACK UpdatePanelProc(HWND control,UINT message,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR data){
    auto& app=*reinterpret_cast<App*>(data);
    if(message==WM_ERASEBKGND)return 1;
    if(message==WM_MOUSEWHEEL){app.WheelUpdateNotes(wp);return 0;}
    if(message==WM_PAINT){
      PAINTSTRUCT ps;auto dc=BeginPaint(control,&ps);RECT bounds;GetClientRect(control,&bounds);
      PaintBuffered(dc,bounds,[&](HDC target,RECT){
        app.DrawSurface(target,control);
        auto full=bounds;full.bottom=app.S(kUpdateHeight);app.Rounded(target,full,kCard,kBorder,10);
        SetBkMode(target,TRANSPARENT);SetTextColor(target,kText);SelectObject(target,app.bold);
        RECT title{app.S(16),app.S(12),app.S(kUpdateWidth-52),app.S(46)};
        auto text=std::wstring(app.T(L"New launcher version: v",L"Yeni Launcher sürümü: v"))+(app.update_release?Wide(app.update_release->version):L"");
        DrawTextW(target,text.c_str(),-1,&title,DT_WORDBREAK|DT_NOPREFIX|DT_END_ELLIPSIS);
      });EndPaint(control,&ps);return 0;
    }
    if(message==WM_KEYDOWN&&wp==VK_ESCAPE){if(app.update_open)app.ToggleUpdatePanel();return 0;}
    if(message==WM_CTLCOLORSTATIC||message==WM_CTLCOLOREDIT){SetTextColor(reinterpret_cast<HDC>(wp),kMuted);SetBkColor(reinterpret_cast<HDC>(wp),kCard);return reinterpret_cast<LRESULT>(app.card);}
    if(message==WM_COMMAND||message==WM_DRAWITEM)return SendMessageW(app.window,message,wp,lp);
    if(message==WM_NCDESTROY)RemoveWindowSubclass(control,UpdatePanelProc,id);
    return DefSubclassProc(control,message,wp,lp);
  }
  void BuildUpdatePanel(){
    update_notice=Control(L"BUTTON",T(L"Launcher update",L"Launcher güncellemesi"),WS_TABSTOP|BS_PUSHBUTTON,kWidth-138,-kCaptionHeight,46,kCaptionHeight,kUpdateNotice);
    ShowWindow(update_notice,SW_HIDE);
    WNDCLASSW cls{};cls.lpfnWndProc=DefWindowProcW;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"MotoGPUpdatePanel";cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    if(!GetClassInfoW(cls.hInstance,cls.lpszClassName,&cls)&&!RegisterClassW(&cls))throw std::runtime_error("Cannot create update panel class");
    update_panel=Control(cls.lpszClassName,L"",WS_CLIPCHILDREN,kWidth-kUpdateWidth-18,18,kUpdateWidth,kUpdateHeight);
    SetWindowSubclass(update_panel,UpdatePanelProc,1,reinterpret_cast<DWORD_PTR>(this));
    SetWindowLongPtrW(update_panel,GWL_EXSTYLE,WS_EX_CONTROLPARENT);
    update_close=Control(L"BUTTON",L"Close update panel",WS_TABSTOP|BS_PUSHBUTTON,kUpdateWidth-42,8,32,32,kUpdateClose,update_panel);
    SetWindowSubclass(update_close,UpdateCloseProc,31,reinterpret_cast<DWORD_PTR>(this));
    update_notes=Control(L"EDIT",L"",WS_TABSTOP|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL,16,kUpdateNotesY,kUpdateWidth-54,kUpdateNotesHeight,0,update_panel);
    update_scroll=Control(L"STATIC",L"",SS_NOTIFY,kUpdateWidth-36,kUpdateNotesY,20,kUpdateNotesHeight,0,update_panel);
    SetWindowSubclass(update_notes,UpdateNotesProc,29,reinterpret_cast<DWORD_PTR>(this));
    SetWindowSubclass(update_scroll,UpdateScrollProc,30,reinterpret_cast<DWORD_PTR>(this));
    update_action=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,16,kUpdateActionY,kUpdateWidth-32,34,kUpdateNow,update_panel);
    update_cancel=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,kUpdateWidth-116,kUpdateActionY,100,34,kUpdateCancel,update_panel);
    ShowWindow(update_panel,SW_HIDE);ShowWindow(update_cancel,SW_HIDE);LayoutUpdatePanel();
  }
  void StartUpdateCheck(){
    int count=0;auto arguments=CommandLineToArgvW(GetCommandLineW(),&count);
    if(arguments){for(int i=1;i<count;++i)if(std::wstring_view(arguments[i])==L"--preview-update")update_preview=true;LocalFree(arguments);}
    if(update_preview){
      update_release=updates::Release{"1.1.11",{}, {},{
          {"en","UPDATE PREVIEW\n\nSample release notes\n\n• Updated launcher interface.\n• Improved controller and focus handling.\n• Added ISO import and clearer title-update warnings.\n\nThis is a visual preview. No update is downloaded or installed."},
          {"tr","GÜNCELLEME ÖNİZLEMESİ\n\nÖrnek değişiklik notları\n\n• Launcher arayüzü güncellendi.\n• Kontrolcü ve odak geçişleri iyileştirildi.\n• ISO çıkarma ve belirgin oyun güncellemesi uyarıları eklendi.\n\nBu görsel bir önizlemedir. Güncelleme indirilmez veya kurulmaz."}}};
      ShowWindow(update_notice,SW_SHOWNOACTIVATE);RefreshUpdatePanel();return;
    }
    update_finishing=updates::InstallationActive(exe,false);
    if(update_finishing){EnableWindow(play,FALSE);SetTimer(window,4,150,nullptr);}
    if(!updates::Configured(updates::kConfiguration))return;
    try{update_check=std::async(std::launch::async,[this]{return updates::CheckForUpdate(updates::kConfiguration,&update_cancelled);});SetTimer(window,4,150,nullptr);}catch(...){}
  }
  void StartUpdateDownload(){
    if(steam_busy||dlc_busy)return;
    if(update_preview){
      if(UpdateGameRunning()||importing_iso||verifying)return;
      MessageBoxW(window,T(L"Visual preview only. No files were downloaded, installed or changed.",
          L"Yalnızca görsel önizleme. Hiçbir dosya indirilmedi, kurulmadı veya değiştirilmedi."),
          T(L"Update preview",L"Güncelleme önizlemesi"),MB_OK|MB_ICONINFORMATION);return;
    }
    if(!update_release||update_finishing||update_busy||importing_iso||verifying||UpdateGameRunning()||!update_release->CanInstall())return;
    update_cancelled=false;update_received=0;update_total=0;update_busy=true;RefreshUpdatePanel();
    const auto release=*update_release;
    try{update_download=std::async(std::launch::async,[this,release]{return updates::DownloadVerified(updates::kConfiguration,release,&update_cancelled,[this](uint64_t received,uint64_t total){update_received=received;update_total=total;});});}
    catch(...){update_busy=false;RefreshUpdatePanel();throw;}
  }
  void PollUpdates(){
    try{
      if(update_finishing&&!updates::InstallationActive(exe,false)){
        update_finishing=false;EnableWindow(play,game_ready&&!dlc_busy&&!steam_busy&&!update_busy&&!process&&!verifying&&!importing_iso);
        if(!update_check.valid()&&!update_release){KillTimer(window,4);return;}
      }
      if(update_check.valid()&&update_check.wait_for(std::chrono::seconds(0))==std::future_status::ready){
        auto result=update_check.get();
        if(result.status==updates::CheckStatus::Available){update_release=std::move(result.release);ShowWindow(update_notice,SW_SHOW);LayoutUpdatePanel();}
        // Startup errors are quiet. No timer retries or button-triggered checks.
        if(!update_release&&!update_finishing){KillTimer(window,4);return;}
      }
      if(update_download.valid()&&update_download.wait_for(std::chrono::seconds(0))==std::future_status::ready){
        update_staged=update_download.get();
        if(update_cancelled){updates::DiscardStaged(*update_staged);update_staged.reset();update_busy=false;}
        else{
          if(UpdateGameRunning())throw std::runtime_error("Close the game before installing this update");
          SaveSettings();SetUpdatePreparation(true);
          auto staged=*update_staged;const auto root=exe;
          std::vector<fs::path> protected_roots{GameRoot(),AchievementRoot(),motogp::dlc::ContentRoot(exe)};
          auto metadata=achievements::MetadataRoot(exe,Get(config.table,"metadata_root",std::string{}));if(!metadata.empty())protected_roots.push_back(metadata);
          for(auto key:{"cache_root","update_data_root","metadata_root"}){
            auto value=Get(config.table,key,std::string{});if(!value.empty()){auto p=fs::path(Wide(value));protected_roots.push_back(p.is_absolute()?p:exe/p);}
          }
          const bool tr=turkish;
          update_prepare=std::async(std::launch::async,[this,staged,root,protected_roots,tr]{return updates::PrepareInstallation(staged,root,protected_roots,tr,&update_cancelled);});
        }
      }
      if(update_prepare.valid()&&update_prepare.wait_for(std::chrono::seconds(0))==std::future_status::ready){
        update_installation=update_prepare.get();
        if(UpdateGameRunning())throw std::runtime_error("Close the game before installing this update");
        updates::CommitInstallation(*update_installation);update_handoff=true;
        PostMessageW(window,WM_CLOSE,0,0);return;
      }
    }catch(const std::exception& e){
      update_installation.reset();if(update_staged){updates::DiscardStaged(*update_staged);update_staged.reset();}
      update_busy=false;SetUpdatePreparation(false);
      if(!update_cancelled)MessageBoxW(window,(std::wstring(T(L"Could not update the launcher.\n",L"Launcher güncellenemedi.\n"))+ErrorDetails(e)).c_str(),T(L"Launcher update",L"Launcher güncellemesi"),MB_OK|MB_ICONERROR);
    }
    RefreshUpdatePanel();
  }
