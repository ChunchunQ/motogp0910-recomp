  // Fixed outer window, scrollable child content. Settings and native keyboard
  // traversal stay accessible without shrinking text to fit every page at once.
  HWND page_view{},page_content{},verify_files{};
  std::array<int,6> page_scroll{},page_extent{};
  static constexpr int kPageX=28,kPageY=136,kPageWidth=520,kPageHeight=328;
  std::unique_ptr<Gdiplus::Bitmap> cover,paint_background;
  std::future<GameVerification> verify_future;
  std::atomic<bool> verify_cancel{false};
  std::atomic<size_t> verify_progress{0};
  size_t verify_total=0;
  fs::path verified_root;
  bool verifying=false;
  HWND FindControl(int id)const {
    for(const auto& p:placements)if(GetDlgCtrlID(p.control)==id)return p.control;
    return nullptr;
  }
  int PageMaximum()const{return std::max(0,page_extent[current_page]-kPageHeight);}
  void ScrollPage(int requested,bool force=false){
    const int next=std::clamp(requested,0,PageMaximum());
    if(!force&&page_scroll[current_page]==next)return;
    page_scroll[current_page]=next;
    // Move one clipped surface. Repositioning every native combo / trackbar
    // on each wheel tick lets their synchronous Windows paints expose partial
    // frames, even with SWP_NOREDRAW. Child coordinates now stay unchanged.
    SetWindowPos(page_content,nullptr,0,-S(next),S(kPageWidth-22),S(std::max(kPageHeight,page_extent[current_page])),SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOCOPYBITS);
    if(!switching_page)RedrawWindow(page_view,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW);
  }
  void Reveal(HWND control){
    for(const auto& p:placements)if(p.control==control){
      wchar_t cls[32];GetClassNameW(control,cls,32);
      const int h=std::wstring_view(cls)==L"COMBOBOX"?32:p.h;
      const int offset=page_scroll[current_page];
      if(p.y<offset)ScrollPage(p.y);
      else if(p.y+h>offset+kPageHeight)ScrollPage(p.y+h-kPageHeight+4);
      break;
    }
  }
  static LRESULT CALLBACK PageChildProc(HWND control,UINT message,WPARAM wparam,LPARAM lparam,UINT_PTR id,DWORD_PTR data){
    auto& app=*reinterpret_cast<App*>(data);
    if(message==WM_LBUTTONDOWN||message==WM_LBUTTONDBLCLK){
      const auto previous=app.pointer_focus;app.pointer_focus=control;
      const auto result=DefSubclassProc(control,message,wparam,lparam);app.pointer_focus=previous;return result;
    }
    // Revealing keyboard focus is helpful, but moving a partly visible field
    // during its mouse-down invalidates the native hit test and loses clicks.
    if(message==WM_SETFOCUS&&!app.switching_page&&!app.pointer_focus)app.Reveal(control);
    if(message==WM_NCDESTROY)RemoveWindowSubclass(control,PageChildProc,id);
    return DefSubclassProc(control,message,wparam,lparam);
  }
  static LRESULT CALLBACK PageProc(HWND control,UINT message,WPARAM wparam,LPARAM lparam,UINT_PTR id,DWORD_PTR data){
    auto& app=*reinterpret_cast<App*>(data);
    int requested=app.page_scroll[app.current_page];
    if(app.RailInput(control,message,lparam,app.page_extent[app.current_page],kPageHeight,requested,app.page_drag)){
      app.ScrollPage(requested);return 0;
    }
    switch(message){
      case WM_PAINT:{PAINTSTRUCT ps;const auto dc=BeginPaint(control,&ps);RECT r;GetClientRect(control,&r);FillRect(dc,&r,app.background);
        app.PaintRail(dc,control,app.page_extent[app.current_page],kPageHeight,app.page_scroll[app.current_page]);EndPaint(control,&ps);return 0;}
      case WM_ERASEBKGND:return 1;
      case WM_VSCROLL:{int next=app.page_scroll[app.current_page];
        switch(LOWORD(wparam)){case SB_LINEUP:next-=28;break;case SB_LINEDOWN:next+=28;break;
          case SB_PAGEUP:next-=kPageHeight;break;case SB_PAGEDOWN:next+=kPageHeight;break;
          case SB_TOP:next=0;break;case SB_BOTTOM:next=app.PageMaximum();break;
          case SB_THUMBTRACK:case SB_THUMBPOSITION:{SCROLLINFO si{sizeof(si),SIF_TRACKPOS};GetScrollInfo(control,SB_VERT,&si);next=si.nTrackPos;break;}}
        app.ScrollPage(next);return 0;}
      case WM_MOUSEWHEEL:
        if(app.current_page==5)return SendMessageW(app.achievement_list,message,wparam,lparam);
        app.ScrollPage(app.page_scroll[app.current_page]-GET_WHEEL_DELTA_WPARAM(wparam)*42/WHEEL_DELTA);return 0;
      case WM_COMMAND:case WM_NOTIFY:case WM_DRAWITEM:case WM_MEASUREITEM:case WM_CONTEXTMENU:
      case WM_HSCROLL:case WM_CTLCOLORSTATIC:case WM_CTLCOLORBTN:case WM_CTLCOLOREDIT:case WM_CTLCOLORLISTBOX:
        return SendMessageW(app.window,message,wparam,lparam);
      case WM_NCDESTROY:RemoveWindowSubclass(control,PageProc,id);break;
    }
    return DefSubclassProc(control,message,wparam,lparam);
  }
  static LRESULT CALLBACK ContentProc(HWND control,UINT message,WPARAM wparam,LPARAM lparam,UINT_PTR id,DWORD_PTR data){
    auto& app=*reinterpret_cast<App*>(data);
    switch(message){
      case WM_PAINT:{PAINTSTRUCT ps;const auto dc=BeginPaint(control,&ps);RECT r;GetClientRect(control,&r);FillRect(dc,&r,app.background);if(app.current_page==5)app.PaintAchievementSummary(dc);EndPaint(control,&ps);return 0;}
      case WM_ERASEBKGND:return 1;
      case WM_MOUSEWHEEL:return SendMessageW(app.page_view,message,wparam,lparam);
      case WM_COMMAND:case WM_NOTIFY:case WM_DRAWITEM:case WM_MEASUREITEM:case WM_CONTEXTMENU:
      case WM_HSCROLL:case WM_CTLCOLORSTATIC:case WM_CTLCOLORBTN:case WM_CTLCOLOREDIT:case WM_CTLCOLORLISTBOX:
        return SendMessageW(app.window,message,wparam,lparam);
      case WM_NCDESTROY:RemoveWindowSubclass(control,ContentProc,id);break;
    }
    return DefSubclassProc(control,message,wparam,lparam);
  }
  void LoadCover(){
    const auto bytes=EmbeddedBytes(102);
    auto stream=SHCreateMemStream(reinterpret_cast<const BYTE*>(bytes.data()),static_cast<UINT>(bytes.size()));
    if(!stream)throw std::runtime_error("Cannot load launcher artwork");
    auto source=std::unique_ptr<Gdiplus::Bitmap>(Gdiplus::Bitmap::FromStream(stream));
    if(source && source->GetLastStatus()==Gdiplus::Ok)
      cover.reset(source->Clone(0,0,source->GetWidth(),source->GetHeight(),PixelFormat32bppARGB));
    source.reset();stream->Release();
    if(!cover || cover->GetLastStatus()!=Gdiplus::Ok)throw std::runtime_error("Cannot decode launcher artwork");
  }
  void DrawBackground(HDC dc){
    RECT r;GetClientRect(window,&r);FillRect(dc,&r,background);
    if(!cover)return;
    if(!paint_background || paint_background->GetWidth()!=static_cast<UINT>(r.right) || paint_background->GetHeight()!=static_cast<UINT>(r.bottom)){
      paint_background=std::make_unique<Gdiplus::Bitmap>(std::max<LONG>(1,r.right),std::max<LONG>(1,r.bottom),PixelFormat32bppARGB);
      Gdiplus::Graphics g(paint_background.get());g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
      g.DrawImage(cover.get(),Gdiplus::Rect(0,0,r.right,r.bottom));
      // An opaque form surface blends into the art; caption buttons keep the
      // real picture behind them. Cache by size rather than resampling on hover.
      const int fade=S(760);
      Gdiplus::LinearGradientBrush shade(Gdiplus::Point(0,0),Gdiplus::Point(fade,0),Gdiplus::Color(255,13,15,20),Gdiplus::Color(0,13,15,20));
      // A broad eased blend starts beyond the opaque settings viewport. No
      // child-window edge or sudden linear fade is exposed beside the form.
      Gdiplus::Color colors[]{Gdiplus::Color(255,13,15,20),Gdiplus::Color(255,13,15,20),Gdiplus::Color(244,13,15,20),Gdiplus::Color(211,13,15,20),Gdiplus::Color(143,13,15,20),Gdiplus::Color(51,13,15,20),Gdiplus::Color(0,13,15,20)};
      Gdiplus::REAL positions[]{0.f,550.f/760,580.f/760,610.f/760,650.f/760,700.f/760,1.f};shade.SetInterpolationColors(colors,positions,7);
      g.FillRectangle(&shade,0,0,fade,r.bottom);
    }
    Gdiplus::Graphics target(dc);target.DrawImage(paint_background.get(),0,0,r.right,r.bottom);
  }
  void LayoutNavigation(){
    const auto dc=GetDC(window);const auto previous=SelectObject(dc,bold);
    constexpr std::array<int,6> order{3,0,4,1,2,5};
    std::array<int,6> widths{};int total=0;
    for(int i:order){SIZE extent{};const auto text=Text(navigation[i]);GetTextExtentPoint32W(dc,text.c_str(),static_cast<int>(text.size()),&extent);widths[i]=extent.cx+S(8);total+=widths[i];}
    SelectObject(dc,previous);ReleaseDC(window,dc);
    const int available=S(kPageWidth),gap=std::clamp((available-total)/5,0,S(12));
    int x=S(kPageX)+(available-total-5*gap)/2;
    // Position in device pixels so fractional DPI rounding cannot accumulate
    // into different gaps. Keep logical placements only for generic rescaling.
    for(int i:order){SetWindowPos(navigation[i],nullptr,x,S(94+kCaptionHeight),widths[i],S(32),SWP_NOZORDER|SWP_NOACTIVATE);x+=widths[i]+gap;}
  }
  void BuildControls(){
    CreateFonts();LoadCover();
    // Retain localized page titles for accessibility; the compact header uses
    // the active tab instead of duplicating a large page heading.
    title=Control(L"STATIC",L"",0,28,68,1,1);subtitle=Control(L"STATIC",L"",0,30,70,1,1);
    ShowWindow(title,SW_HIDE);ShowWindow(subtitle,SW_HIDE);
    for(int i=0;i<6;++i)navigation[i]=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,28,94,1,32,kNavGraphics+i);
    page_view=Control(L"STATIC",L"",WS_CLIPCHILDREN,kPageX,kPageY,kPageWidth,kPageHeight);
    SetWindowLongPtrW(page_view,GWL_EXSTYLE,GetWindowLongPtrW(page_view,GWL_EXSTYLE)|WS_EX_CONTROLPARENT|WS_EX_COMPOSITED);
    SetWindowSubclass(page_view,PageProc,1,reinterpret_cast<DWORD_PTR>(this));
    page_content=Control(L"STATIC",L"",WS_CLIPCHILDREN,0,0,kPageWidth-22,kPageHeight,0,page_view);
    SetWindowLongPtrW(page_content,GWL_EXSTYLE,GetWindowLongPtrW(page_content,GWL_EXSTYLE)|WS_EX_CONTROLPARENT);
    SetWindowSubclass(page_content,ContentProc,1,reinterpret_cast<DWORD_PTR>(this));
    auto combo=[&](int page,int x,int y,int w,int id){return PageControl(page,L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,x,y,w,220,id);};
    auto edit=[&](int page,int x,int y,int w,int id,int limit){auto c=PageControl(page,L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL|WS_BORDER,x,y,w,30,id);SendMessageW(c,EM_SETLIMITTEXT,limit,0);return c;};
    auto check=[&](int page,int x,int y,int w,int id){return PageControl(page,L"BUTTON",L"",WS_TABSTOP|BS_AUTOCHECKBOX,x,y,w,28,id);};
    auto field=[&](int page,const wchar_t* en,const wchar_t* tr,int x,int y,int id){LabelControl(page,en,tr,x,y,236,24);return combo(page,x,y+26,236,id);};
    folder_label=PageControl(3,L"STATIC",L"",0,0,0,494,20);
    path=edit(3,0,24,388,kPath,32760);SetWindowTextW(path,ResolveGameRoot(exe,config.table).c_str());
    browse=PageControl(3,L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,400,24,94,30,kBrowse);
    verify_files=PageControl(3,L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,0,64,164,30,kVerifyFiles);
    LabelControl(3,L"Checks original files. No changes are made.",L"Özgün dosyaları kontrol eder. Değişiklik yapmaz.",176,66,318,32);
    language=field(3,L"Launcher language",L"Launcher dili",0,112,kLanguage);
    SendMessageW(language,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"English"));SendMessageW(language,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Türkçe"));
    game_language=field(3,L"Game language",L"Oyun dili",258,112,kGameLanguage);FillCombo(game_language,{L"English",L"Français",L"Deutsch",L"Español",L"Italiano",L"Türkçe"});
    prompt_style=field(3,L"Button prompts",L"Tuş simgeleri",0,188,kPromptStyle);
    lan_enabled=field(3,L"Connection mode",L"Bağlantı modu",258,188,kLanEnabled);
    LabelControl(3,L"Player name",L"Oyuncu adı",0,264,236);lan_player=edit(3,0,290,236,kLanPlayer,15);
    LabelControl(3,L"Room name (host)",L"Oda adı (kurucu)",258,264,236);lan_room=edit(3,258,290,236,kLanRoom,31);
    LabelControl(3,L"LAN local IPv4 (empty = Auto / Radmin)",L"LAN yerel IPv4 (boş = Otomatik / Radmin)",0,340,236,36);lan_adapter=edit(3,0,378,236,kLanAdapter,15);
    LabelControl(3,L"LAN host IPv4 (empty = discovery)",L"LAN kurucu IPv4 (boş = oda arama)",258,340,236,36);lan_host=edit(3,258,378,236,kLanHost,15);
    LabelControl(3,L"Online server (HTTPS address)",L"Çevrimiçi sunucu (HTTPS adresi)",0,428,494);online_server=edit(3,0,454,494,kOnlineServer,512);
    crash_diagnostics=check(3,0,502,494,kCrashDiagnostics);SetCheck(crash_diagnostics,Get(preferences.table,"crash_monitor",true));
    LabelControl(3,L"LAN uses your local network / Radmin. Online uses a community server.\nUse the same build and Player Match.",L"LAN, yerel ağı / Radmin’i kullanır. Çevrimiçi için aynı sunucuyu seç.\nAynı buildi ve Player Match seçeneğini kullan.",0,550,494,64);
    adapter=field(0,L"Graphics adapter",L"Ekran kartı",0,0,kAdapter);
    render_resolution=field(0,L"Internal resolution",L"İç çözünürlük",258,0,kRenderResolution);
    size=field(0,L"Output window size",L"Çıkış pencere boyutu",0,68,kSize);
    anisotropic=field(0,L"Texture filtering",L"Doku filtreleme",258,68,kAnisotropic);
    fullscreen=field(0,L"Display mode",L"Ekran modu",0,136,kFullscreen);
    post_effect=field(0,L"Anti-aliasing",L"Kenar yumuşatma",258,136,kPostEffect);
    refresh_rate=field(0,L"Refresh rate",L"Yenileme hızı",0,204,kRefreshRate);
    fps_limit=field(0,L"FPS limit",L"FPS sınırı",258,204,kFpsLimit);
    effect=field(0,L"Upscaling / sharpening",L"Ölçekleme / keskinleştirme",0,272,kEffect);
    fsr_quality=field(0,L"FSR quality mode",L"FSR kalite modu",258,272,kFsrQuality);
    vsync=check(0,0,354,236,kVsync);msaa=check(0,258,354,236,kMsaa);
    letterbox=check(0,0,396,236,kLetterbox);aspect=combo(0,258,396,236,kAspect);
    dither=check(0,0,438,236,kDither);shader_cache=check(0,258,438,236,kShaderCache);
    LabelControl(0,L"Sharpness",L"Keskinlik",0,480,400);sharp_value=PageControl(0,L"STATIC",L"90%",SS_RIGHT,424,480,70,22);
    sharpness=PageControl(0,TRACKBAR_CLASSW,L"",WS_TABSTOP|TBS_HORZ|TBS_NOTICKS,0,508,494,30,kSharpness);SendMessageW(sharpness,TBM_SETRANGE,TRUE,MAKELPARAM(0,100));
    fsr_notice=PageControl(0,L"STATIC",L"",0,0,560,494,44);
    LabelControl(0,L"Borderless and windowed use desktop Hz. FPS cap does not unlock the game’s own limit.",L"Kenarlıksız ve pencereli mod masaüstü Hz hızını kullanır. FPS sınırı, oyunun kendi sınırını kaldırmaz.",0,620,494,48);
    for(size_t i=0;i<engine_choices.size();++i)engine_choices[i]=field(4,kEngineEffects[i].english,kEngineEffects[i].turkish,(i%2)*258,(i/2)*82,kEngineBase+i);
    shadow_distance=field(4,L"Shadow render distance",L"Gölge çizim mesafesi",258,82,kShadowDistance);
    LabelControl(4,L"Game default preserves the original effects and track settings.\nOff disables the selected effect. Higher distance can increase GPU work.",L"Oyunun ayarı, özgün efektleri ve pist ayarlarını korur.\nKapalı seçimi ilgili efekti kapatır. Yüksek mesafe GPU yükünü artırabilir.",0,190,494,70);
    LabelControl(1,L"Master volume",L"Ana ses düzeyi",0,0,390);volume_value=PageControl(1,L"STATIC",L"100%",SS_RIGHT,414,0,80,22);
    volume=PageControl(1,TRACKBAR_CLASSW,L"",WS_TABSTOP|TBS_HORZ|TBS_NOTICKS,0,36,494,40,kVolume);SendMessageW(volume,TBM_SETRANGE,TRUE,MAKELPARAM(0,100));
    mute=check(1,0,100,494,kMute);
    LabelControl(1,L"One level for music, engines and menu sounds. Use the in-game options for separate music and effects levels.\nLauncher changes apply on the next game launch.",L"Müzik, motor ve menü sesleri için toplam ses düzeyi. Ayrı ses düzeyleri için oyun içi seçenekleri kullan.\nLauncher değişiklikleri sonraki açılışta uygulanır.",0,160,494,86);
    backend=field(2,L"Controller",L"Kontrolcü",0,0,kBackend);keyboard=check(2,258,26,236,kKeyboard);
    adaptive_triggers=PageControl(2,L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,0,76,494,32,kAdaptiveTriggers);
    LabelControl(2,L"Type a key or use … to capture. Right-click to unbind or reset. Esc cancels capture.",L"Tuşu yaz veya … ile yakala. Sağ tık: tuşu kaldır / sıfırla. Esc: iptal.",0,120,494,38);
    for(size_t i=0;i<kBindings.size();++i){const int y=170+static_cast<int>(i)*34;
      LabelControl(2,kBindings[i].english,kBindings[i].turkish,0,y+3,232,26);
      binding_fields[i]=edit(2,244,y,210,kBindingBase+i,512);SetWindowSubclass(binding_fields[i],BindingProc,1,reinterpret_cast<DWORD_PTR>(this));
      binding_buttons[i]=PageControl(2,L"BUTTON",L"…",WS_TABSTOP|BS_PUSHBUTTON,466,y,28,28,kCaptureBase+i);
    }
    BuildAchievements();
    status=Control(L"STATIC",L"",0,44,470,504,30);SendMessageW(status,WM_SETFONT,reinterpret_cast<WPARAM>(small_font),TRUE);
    save=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,28,510,104,32,kSave);
    defaults=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,140,510,104,32,kDefaults);
    logs=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,252,510,68,32,kLogs);
    report=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,328,510,78,32,kReport);
    play=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,414,504,134,40,kPlay);
    Localize();SetControls(settings);for(auto& page:pages)for(auto c:page)ShowWindow(c,SW_HIDE);ShowPage(3);ReadyStatus();
  }
  void StartVerification(){
    if(process)return;
    if(verifying){verify_cancel=true;SetWindowTextW(verify_files,T(L"Cancelling…",L"İptal ediliyor…"));EnableWindow(verify_files,FALSE);return;}
    const auto references=GameFileReferences(EmbeddedBytes(103));
    verified_root=GameRoot();verify_total=references.size();verify_progress=0;verify_cancel=false;
    const auto application=exe,root=verified_root;
    verify_future=std::async(std::launch::async,[this,application,root,references]{
      std::wstring core_error;
      try{Validate(application,root);}catch(const std::exception& e){core_error=ErrorDetails(e);}
      auto result=VerifyGameData(root,references,verify_cancel,verify_progress);
      if(!core_error.empty()){++result.unreadable;result.problems.insert(result.problems.begin(),L"XEX / UPDATE / PACKAGE | "+core_error);}
      return result;
    });
    verifying=true;
    for(auto control:{path,browse,play})EnableWindow(control,FALSE);
    SetWindowTextW(verify_files,T(L"Cancel check",L"Kontrolü iptal et"));
    Status(L"Checking original game files…",L"Özgün oyun dosyaları kontrol ediliyor…");SetTimer(window,3,200,nullptr);
  }
  void PollVerification(){
    if(!verifying || !verify_future.valid())return;
    if(verify_future.wait_for(std::chrono::seconds(0))!=std::future_status::ready){
      const auto progress=std::to_wstring(verify_progress.load())+L" / "+std::to_wstring(verify_total);
      Status(L"Checking files: "+progress+L". You can cancel.",L"Dosyalar kontrol ediliyor: "+progress+L". İptal edebilirsin.");return;
    }
    verifying=false;KillTimer(window,3);
    for(auto control:{path,browse,play,verify_files})EnableWindow(control,!process);
    SetWindowTextW(verify_files,T(L"Verify game files",L"Dosyaları doğrula"));
    try{
      const auto result=verify_future.get();
      if(result.cancelled){Status(L"File verification cancelled. No files were changed.",L"Dosya kontrolü iptal edildi. Hiçbir dosya değiştirilmedi.");return;}
      const auto count=std::to_wstring(result.matched),issues=std::to_wstring(result.missing+result.changed+result.unreadable);
      Status(result.Passed()?L"Verification passed: "+count+L" original data files + XEX/update checked.":L"Verification found "+issues+L" issue(s). Open logs for the report.",
          result.Passed()?L"Doğrulama başarılı: "+count+L" özgün veri dosyası ve XEX/güncelleme kontrol edildi.":L"Doğrulamada "+issues+L" sorun bulundu. Rapor için logları aç.",result.Passed());
      fs::create_directories(exe/L"logs");
      SYSTEMTIME now{};GetSystemTime(&now);wchar_t filename[100];
      swprintf_s(filename,L"verification-%04u%02u%02u-%02u%02u%02u-%lu.txt",now.wYear,now.wMonth,now.wDay,now.wHour,now.wMinute,now.wSecond,GetCurrentProcessId());
      const auto report_path=exe/L"logs"/filename;std::ofstream report_stream(report_path,std::ios::binary);
      report_stream<<"MotoGP 09/10 original game file verification\nGame folder: "<<Utf8(verified_root.wstring())
          <<"\nReference: supported Xbox 360 disk data / game 0.0.1.1\nMatched: "<<result.matched<<"\nMissing: "<<result.missing<<"\nChanged: "<<result.changed<<"\nUnreadable / core errors: "<<result.unreadable
          <<"\nAdditional files are permitted. No game files were changed.\n\n";
      for(const auto& problem:result.problems)report_stream<<Utf8(problem)<<'\n';
      report_stream.close();if(!report_stream)throw std::runtime_error("Could not write verification report");
      auto message=turkish?status_tr:status_en;
      message+=T(L"\n\nNo files were changed. Report:\n",L"\n\nHiçbir dosya değiştirilmedi. Rapor:\n")+report_path.wstring();
      MessageBoxW(window,message.c_str(),T(L"Game file verification",L"Oyun dosyası doğrulama"),MB_OK|(result.Passed()?MB_ICONINFORMATION:MB_ICONWARNING));
    }catch(const std::exception& e){ErrorBox(e);}
  }
  static LRESULT CALLBACK ComboFrameProc(HWND control,UINT message,WPARAM wparam,LPARAM lparam,UINT_PTR id,DWORD_PTR data){
    auto& app=*reinterpret_cast<App*>(data);
    if(message==WM_MOUSEWHEEL&&!SendMessageW(control,CB_GETDROPPEDSTATE,0,0)){
      // A closed selector never changes a saved setting just because the
      // pointer happens to be over it. Scroll the surrounding page instead.
      return SendMessageW(app.page_view,WM_MOUSEWHEEL,wparam,lparam);
    }
    if(message==WM_ERASEBKGND)return 1;
    if(message==WM_PAINT){
      PAINTSTRUCT ps;const auto dc=BeginPaint(control,&ps);RECT r;GetClientRect(control,&r);FillRect(dc,&r,app.background);
      app.Rounded(dc,r,kField,GetFocus()==control?kAccent:kBorder,6);
      DRAWITEMSTRUCT item{};item.CtlType=ODT_COMBOBOX;item.hwndItem=control;item.hDC=dc;item.rcItem=r;item.rcItem.left+=app.S(1);item.rcItem.top+=app.S(1);item.rcItem.right-=app.S(27);item.rcItem.bottom-=app.S(1);
      item.itemID=static_cast<UINT>(SendMessageW(control,CB_GETCURSEL,0,0));if(!IsWindowEnabled(control))item.itemState=ODS_DISABLED;
      app.DrawCombo(item);
      const auto pen=CreatePen(PS_SOLID,std::max(1,app.S(1)),IsWindowEnabled(control)?kMuted:kBorder);const auto old=SelectObject(dc,pen);
      const int x=r.right-app.S(13),y=(r.top+r.bottom)/2;MoveToEx(dc,x-app.S(4),y-app.S(2),nullptr);LineTo(dc,x,y+app.S(2));LineTo(dc,x+app.S(4),y-app.S(2));SelectObject(dc,old);DeleteObject(pen);
      EndPaint(control,&ps);return 0;
    }
    if(message==WM_NCDESTROY)RemoveWindowSubclass(control,ComboFrameProc,id);
    return DefSubclassProc(control,message,wparam,lparam);
  }
  ScrollDrag page_drag,achievement_drag;
  int achievement_total=0;
  ScrollRail Rail(HWND control,int total,int view,int offset)const{
    RECT client;GetClientRect(control,&client);ScrollRail r;
    r.track={client.right-S(18),S(3),client.right-S(2),client.bottom-S(3)};
    r.maximum=std::max(0,total-view);
    const int height=std::max<int>(1,r.track.bottom-r.track.top);
    const int thumb=std::min(height,std::max(S(24),total>0?static_cast<int>(int64_t(height)*view/total):height));
    r.travel=height-thumb;
    const int y=r.track.top+(r.maximum?static_cast<int>(int64_t(r.travel)*std::clamp(offset,0,r.maximum)/r.maximum):0);
    r.thumb={r.track.left+S(4),y,r.track.right-S(3),y+thumb};return r;
  }
  void PaintRail(HDC dc,HWND control,int total,int view,int offset){
    RECT client;GetClientRect(control,&client);RECT clear{client.right-S(20),0,client.right,client.bottom};FillRect(dc,&clear,background);
    const auto r=Rail(control,total,view,offset);
    if(r.maximum)Rounded(dc,r.thumb,RGB(85,93,107),RGB(85,93,107),6);
  }
  bool RailInput(HWND control,UINT message,LPARAM lparam,int total,int view,int& next,ScrollDrag& drag){
    const auto rail=Rail(control,total,view,next);
    if(message==WM_LBUTTONDOWN){POINT p{GET_X_LPARAM(lparam),GET_Y_LPARAM(lparam)};
      if(!rail.maximum||!PtInRect(&rail.track,p))return false;
      if(PtInRect(&rail.thumb,p)){drag={true,p.y,next};SetCapture(control);}
      else next=std::clamp(next+(p.y<rail.thumb.top?-view:view),0,rail.maximum);
      return true;
    }
    if(message==WM_MOUSEMOVE&&drag.active&&GetCapture()==control){
      next=std::clamp(drag.origin_offset+static_cast<int>(int64_t(GET_Y_LPARAM(lparam)-drag.origin_y)*rail.maximum/std::max(1,rail.travel)),0,rail.maximum);return true;
    }
    if(message==WM_LBUTTONUP&&drag.active){drag.active=false;if(GetCapture()==control)ReleaseCapture();return true;}
    if(message==WM_CAPTURECHANGED)drag.active=false;
    return false;
  }
