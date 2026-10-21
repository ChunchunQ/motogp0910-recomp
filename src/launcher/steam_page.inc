  HWND steam_button{},steam_status{};
  std::future<steam::Result> steam_future;
  bool steam_busy=false,steam_added=false;
  std::wstring steam_message_en,steam_message_tr;
  void RefreshSteam(){
    if(!steam_button)return;
    SetWindowTextW(steam_button,steam_busy?T(L"Adding…",L"Ekleniyor…"):T(L"Add to Steam",L"Steam'e ekle"));
    EnableWindow(steam_button,!steam_busy&&!process&&!update_busy&&!update_finishing&&!dlc_busy);
    if(steam_busy||dlc_busy)EnableWindow(play,FALSE);
    const auto& message=turkish?steam_message_tr:steam_message_en;
    SetWindowTextW(steam_status,message.empty()?T(L"Adds the launcher to your Steam library with artwork.",L"Launcher'ı görselleriyle birlikte Steam kütüphanene ekler."):message.c_str());
  }
  void BuildSteam(){
    LabelControl(3,L"Steam",L"Steam",0,338,kFormWidth,22);
    steam_button=PageControl(3,L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,0,364,184,32,kAddToSteam);
    steam_status=PageControl(3,L"STATIC",L"",0,0,406,kFormWidth,64);
    SendMessageW(steam_status,WM_SETFONT,reinterpret_cast<WPARAM>(small_font),TRUE);
    RefreshSteam();
  }
  void AddToSteam(){
    if(steam_busy||process||update_busy||update_finishing||dlc_busy)return;
    const auto steam_path=steam::FindSteam();
    if(steam_path.empty())throw Error(L"Steam was not found. Install and sign in to Steam once, then try again.",L"Steam bulunamadı. Steam'i kurup bir kez oturum açtıktan sonra yeniden dene.");
    const bool running=steam::Running();
    if(running&&MessageBoxW(window,T(L"Steam needs to close to add the game. Close it now and reopen it afterwards?",L"Oyunu eklemek için Steam'in kapanması gerekiyor. Şimdi kapatılıp işlem sonunda yeniden açılsın mı?"),T(L"Add to Steam",L"Steam'e ekle"),MB_OKCANCEL|MB_ICONQUESTION)!=IDOK)return;
    steam_future=std::async(std::launch::async,[root=exe,steam_path,running]{return steam::Add(root,steam_path,running);});
    steam_busy=true;steam_added=false;steam_message_en=running?L"Closing Steam and adding the game…":L"Adding the game…";
    steam_message_tr=running?L"Steam kapatılıyor ve oyun ekleniyor…":L"Oyun ekleniyor…";
    RefreshUpdatePanel();RefreshSteam();EnableWindow(play,FALSE);SetTimer(window,8,200,nullptr);
  }
  void PollSteam(){
    if(!steam_busy||!steam_future.valid()||steam_future.wait_for(std::chrono::seconds(0))!=std::future_status::ready)return;
    steam_busy=false;KillTimer(window,8);
    try{
      const auto result=steam_future.get();
      steam_added=true;
      steam_message_en=L"Added to Steam. Launch MotoGP 09/10 from your library.";
      steam_message_tr=L"Steam'e eklendi. MotoGP 09/10'u kütüphanenden açabilirsin.";
      if(!result.artwork){steam_message_en=L"Added to Steam; some artwork is missing or could not be copied.";steam_message_tr=L"Steam'e eklendi; bazı kütüphane görselleri eksik veya kopyalanamadı.";}
      if(!result.restarted){steam_message_en+=L" Reopen Steam manually.";steam_message_tr+=L" Steam'i elle yeniden aç.";}
    }catch(const std::exception& e){steam_message_en=L"Could not add to Steam.";steam_message_tr=L"Steam'e eklenemedi.";ErrorBox(e);}
    RefreshUpdatePanel();RefreshSteam();EnableWindow(play,game_ready&&!process&&!verifying&&!importing_iso&&!update_busy&&!update_finishing&&!dlc_busy);
  }
