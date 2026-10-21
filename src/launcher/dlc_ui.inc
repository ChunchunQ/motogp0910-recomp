  HWND dlc_button{};
  std::future<dlc::Result> dlc_future;
  std::atomic<bool> dlc_cancel{false};
  dlc::Progress dlc_progress;
  bool dlc_busy=false;
  bool dlc_checked=false,dlc_complete=false;
  fs::path dlc_checked_root;
  void StopDlc(){dlc_cancel=true;if(dlc_future.valid())dlc_future.wait();}
  motogp::dlc::Installation dlc_installed{};
  HWND dlc_missing{};
  void RefreshDlc(){
    const auto root=motogp::dlc::ContentRoot(exe);
    if(!dlc_busy&&(!dlc_checked||root!=dlc_checked_root)){
      dlc_installed=motogp::dlc::InspectInstallation(root);dlc_complete=motogp::dlc::Complete(dlc_installed);
      dlc_checked_root=root;dlc_checked=true;
    }
    if(!dlc_complete)game_ready=false;
    if(play)EnableWindow(play,game_ready&&!process&&!verifying&&!importing_iso&&!steam_busy&&!update_busy&&!update_finishing&&!dlc_busy);
    if(dlc_missing){
      std::wstring missing=T(L"Required DLC:\r\n",L"Gerekli DLC paketleri:\r\n");
      for(size_t i=0;i<dlc_installed.size();++i)if(!dlc_installed[i]){missing+=L"• ";missing+=dlc::kPackages[i].name;missing+=L"\r\n\r\n";}
      SetWindowTextW(dlc_missing,missing.c_str());
      ShowWindow(dlc_missing,current_page==3&&disk_files_present&&title_update_valid&&!dlc_complete?SW_SHOW:SW_HIDE);
    }
  }
  std::wstring DlcAction()const{
    if(!dlc_busy)return T(L"Install DLC",L"DLC kur");
    if(dlc_cancel)return T(L"Cancelling…",L"İptal ediliyor…");
    if(dlc_progress.phase==2)return T(L"Installing DLC…",L"DLC kuruluyor…");
    if(dlc_progress.phase==0)return T(L"Cancel DLC import",L"DLC kurulumunu iptal et");
    const auto total=dlc_progress.total.load();
    return std::wstring(T(L"Cancel (",L"İptal (%"))+std::to_wstring(total?std::min<uint64_t>(100,dlc_progress.done.load()*100/total):0)+(turkish?L")":L"%)");
  }
  void BuildDlc(){
    dlc_button=verify_files;
    dlc_missing=PageControl(3,L"STATIC",L"",0,kColumnX,128,kColumnWidth,198);
    SendMessageW(dlc_missing,WM_SETFONT,reinterpret_cast<WPARAM>(small_font),TRUE);
    RefreshDlc();
  }
  std::vector<fs::path> PickDlcSources(bool folder){
    IFileOpenDialog* raw=nullptr;
    if(FAILED(CoCreateInstance(CLSID_FileOpenDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&raw))))throw Error(L"Could not open the file picker.",L"Dosya seçme penceresi açılamadı.");
    struct Release{IFileOpenDialog* dialog;~Release(){dialog->Release();}} release{raw};
    DWORD options=0;raw->GetOptions(&options);
    if(FAILED(raw->SetOptions(options|FOS_FORCEFILESYSTEM|FOS_PATHMUSTEXIST|FOS_ALLOWMULTISELECT|(folder?FOS_PICKFOLDERS:FOS_FILEMUSTEXIST))))throw std::runtime_error("Could not enable multiple DLC selections");
    const COMDLG_FILTERSPEC filters[]{{T(L"DLC archives (*.zip;*.rar;*.7z)",L"DLC arşivleri (*.zip;*.rar;*.7z)"),L"*.zip;*.rar;*.7z"},{T(L"All files (*.*)",L"Tüm dosyalar (*.*)"),L"*.*"}};
    if(!folder)raw->SetFileTypes(2,filters);
    raw->SetTitle(folder?T(L"Select extracted DLC folders (Ctrl or Shift for multiple)",L"Çıkarılmış DLC klasörlerini seç (çoklu seçim: Ctrl veya Shift)"):T(L"Select MotoGP 09/10 DLC archives",L"MotoGP 09/10 DLC arşivlerini seç"));
    const auto result=raw->Show(window);if(result==HRESULT_FROM_WIN32(ERROR_CANCELLED))return {};
    if(FAILED(result))throw std::runtime_error("Could not show the DLC picker");
    IShellItemArray* items=nullptr;if(FAILED(raw->GetResults(&items)))throw std::runtime_error("Could not read the selected DLC sources");
    struct ItemsRelease{IShellItemArray* items;~ItemsRelease(){items->Release();}} items_release{items};
    DWORD count=0;if(FAILED(items->GetCount(&count))||!count||count>64)throw Error(L"Select between one and 64 DLC sources.",L"1 ile 64 arasında DLC kaynağı seç.");
    std::vector<fs::path> selected;selected.reserve(count);
    for(DWORD i=0;i<count;++i){
      IShellItem* item=nullptr;if(FAILED(items->GetItemAt(i,&item)))throw std::runtime_error("Could not read a selected DLC source");
      PWSTR name=nullptr;const auto status=item->GetDisplayName(SIGDN_FILESYSPATH,&name);item->Release();
      if(FAILED(status))throw std::runtime_error("Could not read the selected DLC path");
      try{selected.push_back(fs::weakly_canonical(name));}catch(...){CoTaskMemFree(name);throw;}CoTaskMemFree(name);
    }
    return selected;
  }
  void StartDlcImport(){
    help.Pop();
    if(dlc_busy){if(dlc_progress.phase!=2)dlc_cancel=true;RefreshGameFilesAction();return;}
    if(process||verifying||importing_iso||steam_busy||update_busy||update_finishing||UpdateGameRunning())return;
    dlc_checked=false;ReadyStatus();if(!disk_files_present||!title_update_valid||dlc_complete)return;
    help.Activate(false);
    struct HelpRestore{Tooltips& help;~HelpRestore(){help.Activate(true);}} help_restore{help};
    HMENU menu=CreatePopupMenu();if(!menu)throw std::runtime_error("Could not create DLC source menu");
    const auto brush=CreateSolidBrush(kPopupBackground);
    struct MenuRelease{HMENU menu;HBRUSH brush;~MenuRelease(){DestroyMenu(menu);DeleteObject(brush);}} menu_release{menu,brush};
    MENUINFO info{sizeof(info)};info.fMask=MIM_BACKGROUND;info.hbrBack=brush;SetMenuInfo(menu,&info);
    AppendMenuW(menu,MF_OWNERDRAW,1,T(L"Select archives…",L"Arşiv seç…"));AppendMenuW(menu,MF_OWNERDRAW,2,T(L"Select folders…",L"Klasör seç…"));
    RECT bounds{};GetWindowRect(dlc_button,&bounds);
    const auto choice=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_NONOTIFY|TPM_LEFTALIGN,bounds.left,bounds.bottom+S(4),0,window,nullptr);if(choice!=1&&choice!=2)return;
    const auto sources=PickDlcSources(choice==2);if(sources.empty())return;
    const auto application=exe;dlc_cancel=false;dlc_progress.done=0;dlc_progress.total=0;dlc_progress.phase=0;
    dlc_future=std::async(std::launch::async,[this,sources,application]{return dlc::Import(sources,application,dlc_cancel,dlc_progress);});
    dlc_busy=true;
    for(auto control:{path,browse,play,save,defaults})EnableWindow(control,FALSE);
    RefreshGameFilesAction();RefreshSteam();RefreshUpdatePanel();SetTimer(window,9,200,nullptr);
  }
  void PollDlc(){
    if(!dlc_busy||!dlc_future.valid())return;RefreshGameFilesAction();
    if(dlc_future.wait_for(std::chrono::seconds(0))!=std::future_status::ready)return;
    dlc_busy=false;KillTimer(window,9);
    for(auto control:{path,browse,play,save,defaults,verify_files})EnableWindow(control,!process&&!update_busy&&!update_finishing&&!steam_busy);
    try{dlc_future.get();}
    catch(const iso::Cancelled&){}
    catch(const std::exception& error){ErrorBox(error);}
    dlc_checked=false;ReadyStatus();RefreshDlc();RefreshSteam();RefreshUpdatePanel();
  }
