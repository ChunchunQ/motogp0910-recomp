  // Included inside App. Extraction and verification never run on the UI thread.
  std::future<fs::path> iso_future;
  std::atomic<bool> iso_cancel{false};
  iso::Progress iso_progress;
  bool importing_iso=false,disk_files_present=false,title_update_valid=false,title_update_present=false,game_ready=false;
  HWND title_update_notice{};
  size_t iso_reference_count=0;
  void StopIsoImport(){iso_cancel=true;if(iso_future.valid())iso_future.wait();}
  void RefreshGameFilesAction(){
    if(!verify_files||!path)return;
    if(!importing_iso&&!verifying&&!dlc_busy){
      disk_files_present=false;title_update_valid=false;title_update_present=false;
      try{const auto root=GameRoot();disk_files_present=fs::is_regular_file(root/L"default.xex")&&
          fs::is_directory(root/L"xenondata")&&!fs::is_empty(root/L"xenondata")&&Sha256(root/L"default.xex")==kXexHash;
        if(fs::is_regular_file(root/L"default.xexp")){title_update_present=true;title_update_valid=Sha256(root/L"default.xexp")==kPatchHash;}
        else if(fs::is_regular_file(root/kUpdate)){title_update_present=true;title_update_valid=Sha256(root/kUpdate)==kUpdateHash;}
      }catch(...){game_ready=false;}
    }
    if(!disk_files_present||!title_update_valid)game_ready=false;
    RefreshDlc();
    std::wstring action;
    if(importing_iso){
      if(iso_cancel)action=T(L"Cancelling…",L"İptal ediliyor…");
      else{action=T(L"Cancel extraction",L"Çıkarmayı iptal et");
        const auto total=iso_progress.total.load();
        if(total)action=std::wstring(T(L"Cancel (",L"İptal (%"))+std::to_wstring(std::min<uint64_t>(100,iso_progress.copied.load()*100/total))+(turkish?L")":L"%)");
        if(iso_progress.verifying)action=T(L"Cancel check",L"Kontrolü iptal et");
      }
    }else if(verifying){
      action=T(L"Cancel check",L"Kontrolü iptal et");
      if(verify_total)action=std::wstring(T(L"Cancel (",L"İptal (%"))+std::to_wstring(std::min<size_t>(100,verify_progress.load()*100/verify_total))+(turkish?L")":L"%)");
    }else if(dlc_busy||(disk_files_present&&title_update_valid&&!dlc_complete))action=DlcAction();
    else action=!disk_files_present?T(L"Select ISO",L"ISO seç"):!title_update_valid?T(L"Select title update",L"Oyun güncellemesi seç"):T(L"Verify game files",L"Dosyaları doğrula");
    SetWindowTextW(verify_files,action.c_str());
    EnableWindow(verify_files,!process&&!steam_busy&&!update_busy&&!update_finishing&&
        (!dlc_busy||(!dlc_cancel&&dlc_progress.phase!=2))&&
        (!importing_iso||!iso_cancel)&&(!verifying||!verify_cancel));
    if(importing_iso||!disk_files_present)Tip(verify_files,L"Extract your original Xbox 360 ISO directly into this launcher's game folder. Click again to cancel.",
        L"Özgün Xbox 360 ISO'nun içeriğini doğrudan Launcher'ın game klasörüne çıkarır. İptal etmek için yeniden tıkla.");
    else if(!title_update_valid)Tip(verify_files,L"Select your supported title-update file or default.xexp patch. It is installed into the selected game folder.",
        L"Desteklenen oyun güncellemesi dosyanı veya default.xexp yamasını seç. Seçili oyun klasörüne kurulur.");
    else if(dlc_busy||!dlc_complete)Tip(verify_files,L"Install all three Xbox 360 DLC packages from ZIP, RAR or 7z archives, or extracted folders. Required in every connection mode. Game files and saves are preserved.",
        L"Üç Xbox 360 DLC paketini ZIP, RAR veya 7z arşivlerinden ya da çıkarılmış klasörlerden kur. Tüm bağlantı modlarında gereklidir. Oyun dosyaları ve kayıtları korunur.");
    else Tip(verify_files,L"Check original game files against the supported hashes. No files are changed or downloaded. Click again to cancel.",
        L"Özgün oyun dosyalarının bütünlüğünü doğrular. Dosyaları değiştirmez veya indirmez. İptal etmek için yeniden tıkla.");
    if(title_update_notice){
      SetWindowTextW(title_update_notice,!disk_files_present?T(L"Game files are missing",L"Oyun dosyaları eksik"):
          title_update_present&&!title_update_valid?T(L"Title update is invalid",L"Oyun güncellemesi geçersiz"):!title_update_valid?T(L"Title update is missing",L"Oyun güncellemesi eksik"):
          (std::wstring(T(L"DLC packages missing: ",L"Eksik DLC paketi: "))+std::to_wstring(std::count(dlc_installed.begin(),dlc_installed.end(),false))+L" / 3").c_str());
      SendMessageW(title_update_notice,WM_SETFONT,reinterpret_cast<WPARAM>(bold),TRUE);
      ShowWindow(title_update_notice,(!disk_files_present||!title_update_valid||!dlc_complete)&&current_page==3?SW_SHOW:SW_HIDE);
      Tip(title_update_notice,L"Select ISO, then Select title update, then Install DLC. All three DLC packages are required before playing in any connection mode.",
          L"Sırayla ISO seç, Oyun güncellemesi seç ve DLC kur düğmelerini kullan. Oyunu herhangi bir bağlantı modunda açmak için üç DLC paketi de gereklidir.");
    }
    Status({},{});
  }
  fs::path PickGameSource(bool update){
    IFileOpenDialog* raw=nullptr;
    if(FAILED(CoCreateInstance(CLSID_FileOpenDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&raw))))
      throw Error(L"Could not open the file picker.",L"Dosya seçme penceresi açılamadı.");
    struct Release {IFileOpenDialog* dialog;~Release(){dialog->Release();}} release{raw};
    DWORD options=0;raw->GetOptions(&options);raw->SetOptions(options|FOS_FORCEFILESYSTEM|FOS_PATHMUSTEXIST|FOS_FILEMUSTEXIST);
    const COMDLG_FILTERSPEC iso_filter{L"Xbox 360 ISO (*.iso)",L"*.iso"};
    const COMDLG_FILTERSPEC update_filter{T(L"All files (*.*)",L"Tüm dosyalar (*.*)"),L"*.*"};
    if(update)raw->SetFileTypes(1,&update_filter);else{raw->SetFileTypes(1,&iso_filter);raw->SetDefaultExtension(L"iso");}
    raw->SetTitle(update?T(L"Select your MotoGP 09/10 title update or default.xexp",L"MotoGP 09/10 oyun güncellemesi veya default.xexp dosyanı seç"):
        T(L"Select your original MotoGP 09/10 Xbox 360 ISO",L"Özgün MotoGP 09/10 Xbox 360 ISO'nu seç"));
    const auto shown=raw->Show(window);if(shown==HRESULT_FROM_WIN32(ERROR_CANCELLED))return {};
    if(FAILED(shown))throw std::runtime_error("Could not show the file picker");
    IShellItem* item=nullptr;if(FAILED(raw->GetResult(&item)))throw std::runtime_error("Could not read the selected file");
    PWSTR name=nullptr;const auto result=item->GetDisplayName(SIGDN_FILESYSPATH,&name);item->Release();
    if(FAILED(result))throw std::runtime_error("Could not read the selected file path");
    fs::path selected;try{selected=name;}catch(...){CoTaskMemFree(name);throw;}CoTaskMemFree(name);return selected;
  }
  void StartTitleUpdateImport(){
    if(importing_iso||verifying||process||update_busy||update_finishing||update_preparing||dlc_busy||UpdateGameRunning())return;
    const auto source=PickGameSource(true);if(source.empty())return;
    iso::InstallTitleUpdate(source,GameRoot());ReadyStatus();
  }
  void StartIsoImport(){
    if(importing_iso){iso_cancel=true;RefreshGameFilesAction();EnableWindow(verify_files,FALSE);return;}
    if(process||verifying||update_busy||update_finishing||update_preparing||dlc_busy||UpdateGameRunning())return;
    const auto source=PickGameSource(false);if(source.empty())return;
    const auto target=exe/L"game";
    const auto references=GameFileReferences(EmbeddedBytes(103));const auto previous=GameRoot();iso_reference_count=references.size();
    iso_cancel=false;iso_progress.copied=0;iso_progress.total=0;iso_progress.checked=0;iso_progress.verifying=false;
    iso_future=std::async(std::launch::async,[this,source,target,previous,references]{return iso::Import(source,target,previous,references,iso_cancel,iso_progress);});
    importing_iso=true;game_ready=false;for(auto control:{path,browse,play,save,defaults})EnableWindow(control,FALSE);
    RefreshGameFilesAction();RefreshUpdatePanel();SetTimer(window,6,200,nullptr);
  }
  void PollIsoImport(){
    if(!importing_iso||!iso_future.valid())return;
    if(iso_future.wait_for(std::chrono::seconds(0))!=std::future_status::ready){RefreshGameFilesAction();return;}
    importing_iso=false;KillTimer(window,6);
    for(auto control:{path,browse,play,save,defaults,verify_files})EnableWindow(control,!process&&!update_busy&&!update_finishing);
    try{
      const auto root=iso_future.get();SetWindowTextW(path,root.c_str());
      Config latest(config.path);Put(latest.table,"game_data_root",Utf8(root.wstring()));latest.Save();config=std::move(latest);
    }catch(const iso::Cancelled&){}
    catch(const std::exception& error){ErrorBox(error);}
    ReadyStatus();RefreshUpdatePanel();
  }
