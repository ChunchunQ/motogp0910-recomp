#include "core.h"
#include <commctrl.h>
#include <shobjidl.h>
#include <shellapi.h>
#include <algorithm>
#include <memory>

using namespace launcher;
namespace {
std::wstring ErrorDetails(const std::exception& error) {
  try { return Wide(error.what()); }
  catch(const std::exception&) {
    // System-library errors may use the Windows code page on a Turkish OS.
    std::string text=error.what();
    int length=MultiByteToWideChar(CP_ACP,0,text.data(),static_cast<int>(text.size()),nullptr,0);
    std::wstring result(length,L'\0');
    if(length) MultiByteToWideChar(CP_ACP,0,text.data(),static_cast<int>(text.size()),result.data(),length);
    return result.empty()?L"System error":result;
  }
}
enum ControlId { kPath=101, kBrowse, kSize, kBackend, kFullscreen, kVsync,
                 kKeyboard, kMute, kSave, kDefaults, kLogs, kReport, kPlay, kLanguage };
struct App {
  fs::path exe = ExecutableFolder();
  Config config{exe / "motogp0910.toml"};
  Config preferences{exe / "motogp0910-launcher.toml"};
  Settings settings = Settings::Load(config.table);
  bool turkish = Get(preferences.table, "language", std::string("en")) == "tr";
  HWND window{}, path{}, size{}, backend{}, language{}, status{}, title{}, subtitle{},
       folder_label{}, display_label{}, size_label{}, input_label{}, help{},
       fullscreen{}, vsync{}, keyboard{}, mute{}, browse{}, save{}, defaults{}, logs{}, report{}, play{};
  HFONT font{}, heading{};
  HBRUSH background = CreateSolidBrush(RGB(245,246,249));
  HBRUSH header = CreateSolidBrush(RGB(24,29,39));
  HANDLE process{};
  double scale = 1.0;
  std::wstring status_en, status_tr;
  std::vector<std::pair<int,int>> dimensions{{0,0},{1280,720},{1600,900},{1920,1080},{2560,1440},{3840,2160}};
  ~App() { if(process) CloseHandle(process); DeleteObject(font); DeleteObject(heading); DeleteObject(background); DeleteObject(header); }
  const wchar_t* T(const wchar_t* en, const wchar_t* tr) const { return turkish ? tr : en; }
  int S(int value) const { return static_cast<int>(value*scale+0.5); }
  HWND Control(const wchar_t* cls, const wchar_t* text, DWORD style,
               int x,int y,int w,int h,int id=0) {
    auto child=CreateWindowExW(cls==std::wstring(L"EDIT") ? WS_EX_CLIENTEDGE : 0,
        cls,text,WS_CHILD|WS_VISIBLE|style,S(x),S(y),S(w),S(h),window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
    if(!child) throw std::runtime_error("Cannot create launcher control");
    SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);
    return child;
  }
  std::wstring Text(HWND control) const {
    std::wstring value(GetWindowTextLengthW(control)+1,L'\0');
    value.resize(GetWindowTextW(control,value.data(),static_cast<int>(value.size())));
    return value;
  }
  fs::path GameRoot() const {
    auto value=Text(path);
    if(value.empty()) return exe/L"game";
    auto root=fs::path(value);
    return fs::absolute(root.is_absolute()?root:exe/root).lexically_normal();
  }
  void Status(std::wstring en,std::wstring tr) {
    status_en=std::move(en); status_tr=std::move(tr);
    SetWindowTextW(status,(turkish?status_tr:status_en).c_str());
  }
  void ErrorBox(const std::exception& error) {
    if(auto* e=dynamic_cast<const Error*>(&error)) {
      Status(e->english,e->turkish);
      MessageBoxW(window,(turkish?e->turkish:e->english).c_str(),T(L"MotoGP 09/10 — Error",L"MotoGP 09/10 — Hata"),MB_OK|MB_ICONERROR);
    } else {
      auto detail=ErrorDetails(error);
      auto en=L"Could not complete the action. Settings and game data were not deleted.\n"+detail;
      auto tr=L"İşlem tamamlanamadı. Ayarlar ve oyun verileri silinmedi.\n"+detail;
      Status(en,tr); MessageBoxW(window,(turkish?tr:en).c_str(),T(L"Error",L"Hata"),MB_OK|MB_ICONERROR);
    }
  }
  void Localize() {
    SetWindowTextW(window,T(L"MotoGP 09/10 Launcher",L"MotoGP 09/10 Launcher"));
    SetWindowTextW(subtitle,T(L"PC RECOMPILATION  •  ReXGlue v0.10.0",L"PC YENİDEN DERLEME  •  ReXGlue v0.10.0"));
    SetWindowTextW(folder_label,T(L"GAME FILES",L"OYUN DOSYALARI"));
    SetWindowTextW(browse,T(L"Browse…",L"Gözat…"));
    SetWindowTextW(display_label,T(L"DISPLAY & CONTROLS",L"GÖRÜNTÜ VE KONTROLLER"));
    SetWindowTextW(size_label,T(L"Window size",L"Pencere boyutu"));
    SetWindowTextW(input_label,T(L"Controller backend",L"Kol bağlantısı"));
    SetWindowTextW(fullscreen,T(L"Fullscreen",L"Tam ekran"));
    SetWindowTextW(vsync,T(L"VSync",L"VSync"));
    SetWindowTextW(keyboard,T(L"Enable keyboard controls",L"Klavye kontrolünü aç"));
    SetWindowTextW(mute,T(L"Mute audio",L"Sesi kapat"));
    SetWindowTextW(save,T(L"Save settings",L"Ayarları kaydet"));
    SetWindowTextW(defaults,T(L"Restore defaults",L"Varsayılan ayarlar"));
    SetWindowTextW(logs,T(L"Open logs",L"Log klasörü"));
    SetWindowTextW(report,T(L"Create error report",L"Hata raporu oluştur"));
    SetWindowTextW(play,T(process?L"GAME RUNNING":L"PLAY",process?L"OYUN ÇALIŞIYOR":L"OYUNU BAŞLAT"));
    SetWindowTextW(help,T(L"Keyboard: Enter = Start  •  Space = A  •  Backspace = B\nGame files stay in game/; saves stay in your existing location.",
        L"Klavye: Enter = Start  •  Space = A  •  Backspace = B\nOyun dosyaları game/ içinde; kayıtların mevcut konumunda kalır."));
    auto selected=SendMessageW(size,CB_GETCURSEL,0,0);
    SendMessageW(size,CB_RESETCONTENT,0,0);
    for(auto [w,h]:dimensions) {
      std::wstring text=(w||h)?std::to_wstring(w)+L" × "+std::to_wstring(h):T(L"Automatic",L"Otomatik");
      SendMessageW(size,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));
    }
    SendMessageW(size,CB_SETCURSEL,selected<0?0:selected,0);
    auto input_selected=SendMessageW(backend,CB_GETCURSEL,0,0);
    SendMessageW(backend,CB_RESETCONTENT,0,0);
    SendMessageW(backend,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"SDL — Xbox / PlayStation / USB"));
    SendMessageW(backend,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(T(L"XInput — Xbox-compatible",L"XInput — Xbox uyumlu")));
    SendMessageW(backend,CB_SETCURSEL,input_selected<0?0:input_selected,0);
    SetWindowTextW(status,(turkish?status_tr:status_en).c_str());
  }
  void SetControls(const Settings& s) {
    auto found=std::find(dimensions.begin(),dimensions.end(),std::pair{s.width,s.height});
    if(found==dimensions.end()) { dimensions.emplace_back(s.width,s.height); Localize(); found=dimensions.end()-1; }
    SendMessageW(size,CB_SETCURSEL,std::distance(dimensions.begin(),found),0);
    SendMessageW(backend,CB_SETCURSEL,s.backend=="xinput"?1:0,0);
    for(auto [control,checked]:{std::pair{fullscreen,s.fullscreen}, {vsync,s.vsync},{keyboard,s.keyboard},{mute,s.mute}})
      SendMessageW(control,BM_SETCHECK,checked?BST_CHECKED:BST_UNCHECKED,0);
  }
  Settings GetControls() const {
    Settings s;
    auto index=SendMessageW(size,CB_GETCURSEL,0,0);
    if(index<0 || static_cast<size_t>(index)>=dimensions.size()) throw std::runtime_error("Invalid window size selection");
    std::tie(s.width,s.height)=dimensions[index];
    if(s.width<0 || s.height<0 || s.width>8192 || s.height>8192)
      throw Error(L"Invalid window size in the settings file. Select a supported size or restore defaults.",
                  L"Ayar dosyasındaki pencere boyutu geçersiz. Desteklenen bir boyut seç veya varsayılanlara dön.");
    s.backend=SendMessageW(backend,CB_GETCURSEL,0,0)==1?"xinput":"sdl";
    s.fullscreen=SendMessageW(fullscreen,BM_GETCHECK,0,0)==BST_CHECKED;
    s.vsync=SendMessageW(vsync,BM_GETCHECK,0,0)==BST_CHECKED;
    s.keyboard=SendMessageW(keyboard,BM_GETCHECK,0,0)==BST_CHECKED;
    s.mute=SendMessageW(mute,BM_GETCHECK,0,0)==BST_CHECKED;
    return s;
  }
  void ReadyStatus() {
    try {
      Validate(exe,GameRoot());
      Status(L"Basic game file checks passed. Title update 0.0.1.1 is available.",
             L"Temel oyun dosyası kontrolü geçti. Title update 0.0.1.1 mevcut.");
    } catch(const Error& e) { Status(e.english,e.turkish); }
      catch(const std::exception& e) { Status(L"Check the game folder: "+ErrorDetails(e),L"Oyun klasörünü kontrol et: "+ErrorDetails(e)); }
  }
  void BuildControls() {
    font=CreateFontW(-S(16),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    heading=CreateFontW(-S(29),0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    title=Control(L"STATIC",L"MotoGP 09/10",0,24,22,530,39);
    SendMessageW(title,WM_SETFONT,reinterpret_cast<WPARAM>(heading),TRUE);
    subtitle=Control(L"STATIC",L"",0,26,67,540,24);
    Control(L"STATIC",L"Language / Dil",0,590,23,164,24);
    language=Control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,590,52,164,140,kLanguage);
    SendMessageW(language,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"English"));
    SendMessageW(language,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Türkçe"));
    SendMessageW(language,CB_SETCURSEL,turkish?1:0,0);
    folder_label=Control(L"STATIC",L"",0,24,126,732,24);
    path=Control(L"EDIT",ResolveGameRoot(exe,config.table).c_str(),WS_TABSTOP|ES_AUTOHSCROLL,24,157,600,30,kPath);
    SendMessageW(path,EM_SETLIMITTEXT,32760,0);
    browse=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,636,157,120,30,kBrowse);
    status=Control(L"STATIC",L"",0,24,201,732,48);
    display_label=Control(L"STATIC",L"",0,24,265,732,24);
    size_label=Control(L"STATIC",L"",0,24,302,244,22);
    input_label=Control(L"STATIC",L"",0,310,302,310,22);
    size=Control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,24,327,244,210,kSize);
    backend=Control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,310,327,330,140,kBackend);
    SendMessageW(backend,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"SDL — Xbox / PlayStation / USB"));
    SendMessageW(backend,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"XInput — Xbox-compatible"));
    fullscreen=Control(L"BUTTON",L"",WS_TABSTOP|BS_AUTOCHECKBOX,24,371,244,26,kFullscreen);
    vsync=Control(L"BUTTON",L"",WS_TABSTOP|BS_AUTOCHECKBOX,310,371,250,26,kVsync);
    keyboard=Control(L"BUTTON",L"",WS_TABSTOP|BS_AUTOCHECKBOX,24,406,270,26,kKeyboard);
    mute=Control(L"BUTTON",L"",WS_TABSTOP|BS_AUTOCHECKBOX,310,406,250,26,kMute);
    help=Control(L"STATIC",L"",0,24,448,732,43);
    save=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,24,504,163,31,kSave);
    defaults=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,197,504,170,31,kDefaults);
    logs=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,377,504,155,31,kLogs);
    report=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,542,504,214,31,kReport);
    play=Control(L"BUTTON",L"",WS_TABSTOP|BS_DEFPUSHBUTTON,24,552,732,42,kPlay);
    Localize(); SetControls(settings); ReadyStatus();
  }
  void SaveSettings() {
    settings=GetControls(); settings.Store(config.table);
    Put(config.table,"game_data_root",Utf8(GameRoot().wstring()));
    config.Save();
    Status(L"Settings saved. They will be used on the next launch.",L"Ayarlar kaydedildi. Sonraki açılışta kullanılacak.");
  }
  void Browse() {
    IFileOpenDialog* dialog=nullptr;
    if(FAILED(CoCreateInstance(CLSID_FileOpenDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&dialog))))
      throw Error(L"Could not open the folder picker. You can type the game path instead.",L"Klasör seçici açılamadı. Oyun yolunu elle yazabilirsin.");
    DWORD flags=0; dialog->GetOptions(&flags);
    dialog->SetOptions(flags|FOS_PICKFOLDERS|FOS_FORCEFILESYSTEM);
    dialog->SetTitle(T(L"Select the folder containing default.xex",L"default.xex içeren klasörü seç"));
    IShellItem* initial=nullptr;
    if(SUCCEEDED(SHCreateItemFromParsingName(GameRoot().c_str(),nullptr,IID_PPV_ARGS(&initial)))) {
      dialog->SetFolder(initial); initial->Release();
    }
    if(SUCCEEDED(dialog->Show(window))) {
      IShellItem* item=nullptr;
      if(SUCCEEDED(dialog->GetResult(&item))) {
        PWSTR name=nullptr;
        if(SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH,&name))) {
          SetWindowTextW(path,name); CoTaskMemFree(name); ReadyStatus();
        }
        item->Release();
      }
    }
    dialog->Release();
  }
  void OpenFolder(const fs::path& folder) {
    fs::create_directories(folder);
    if(reinterpret_cast<INT_PTR>(ShellExecuteW(window,L"open",folder.c_str(),nullptr,nullptr,SW_SHOWNORMAL))<=32)
      throw Error(L"Could not open the folder: "+folder.wstring(),L"Klasör açılamadı: "+folder.wstring());
  }
  void CreateReport() {
    SYSTEMTIME now{}; GetSystemTime(&now);
    wchar_t name[100]; swprintf_s(name,L"report-%04u%02u%02u-%02u%02u%02u-%lu",
        now.wYear,now.wMonth,now.wDay,now.wHour,now.wMinute,now.wSecond,GetCurrentProcessId());
    auto folder=exe/L"logs"/name;
    for(unsigned suffix=2;fs::exists(folder);++suffix)
      folder=exe/L"logs"/(std::wstring(name)+L"-"+std::to_wstring(suffix));
    fs::create_directories(folder);
    auto s=GetControls(); toml::table summary; s.Store(summary);
    Put(summary,"game_data_root",Utf8(GameRoot().wstring()));
    std::ofstream output(folder/L"report.txt",std::ios::binary);
    output<<"MotoGP 09/10 launcher report — ReXGlue v0.10.0\nUTC timestamp: "<<Utf8(name)<<"\n\n";
    output<<toml::toml_formatter{summary}<<"\n\n";
    try { Validate(exe,GameRoot()); output<<"Basic file checks: PASS\n"; }
    catch(const std::exception& e) { output<<"Basic file checks: "<<e.what()<<'\n'; }
    std::vector<fs::directory_entry> files;
    for(const auto& file:fs::directory_iterator(exe/L"logs"))
      if(file.is_regular_file() && file.path().extension()==L".log") files.push_back(file);
    std::sort(files.begin(),files.end(),[](const auto& a,const auto& b){return a.last_write_time()>b.last_write_time();});
    for(size_t i=0;i<std::min<size_t>(3,files.size());++i) {
      std::ifstream log(files[i].path(),std::ios::binary);
      const auto length=files[i].file_size();
      if(length>2*1024*1024) log.seekg(length-2*1024*1024);
      std::ofstream tail(folder/(L"log-tail-"+std::to_wstring(i+1)+L".txt"),std::ios::binary);
      tail<<log.rdbuf();
      output<<"Log tail "<<i+1<<": "<<Utf8(files[i].path().filename().wstring())<<'\n';
      if(!tail) throw std::runtime_error("Could not write log report");
    }
    output.close(); if(!output) throw std::runtime_error("Could not write report");
    Status(L"Report created: "+folder.wstring(),L"Rapor oluşturuldu: "+folder.wstring());
    OpenFolder(folder);
  }
  void Running(bool value) {
    for(auto control:{path,browse,size,backend,fullscreen,vsync,keyboard,mute,save,defaults,play}) EnableWindow(control,!value);
    Localize();
  }
  void Command(int id,int event) {
    try {
      if(id==kLanguage && event==CBN_SELCHANGE) {
        turkish=SendMessageW(language,CB_GETCURSEL,0,0)==1;
        Localize(); Put(preferences.table,"language",std::string(turkish?"tr":"en")); preferences.Save();
      } else if(id==kPath && event==EN_CHANGE) {
        Status(L"Press PLAY to check and launch the selected game folder.",L"Seçilen klasörü kontrol edip açmak için OYUNU BAŞLAT'a bas.");
      } else if(event==BN_CLICKED) switch(id) {
        case kBrowse: Browse(); break;
        case kSave: SaveSettings(); break;
        case kDefaults: SetControls(Settings{}); Status(L"Defaults selected. Save or press PLAY to apply.",L"Varsayılanlar seçildi. Uygulamak için kaydet veya oyunu başlat."); break;
        case kLogs: OpenFolder(exe/L"logs"); break;
        case kReport: CreateReport(); break;
        case kPlay:
          if(process) break;
          Validate(exe,GameRoot()); SaveSettings();
          process=Start(exe,Arguments(GameRoot(),settings,config.table));
          Running(true); SetTimer(window,1,700,nullptr);
          Status(L"Game running. You may close the launcher without closing the game.",L"Oyun çalışıyor. Launcher'ı kapatman oyunu kapatmaz."); break;
      }
    } catch(const std::exception& e) { ErrorBox(e); }
  }
  void Poll() {
    if(!process || WaitForSingleObject(process,0)!=WAIT_OBJECT_0) return;
    DWORD code=0; GetExitCodeProcess(process,&code); CloseHandle(process); process=nullptr;
    KillTimer(window,1); Running(false);
    if(code) Status(L"Game exited (code "+std::to_wstring(code)+L"). Open logs or create an error report.",
        L"Oyun kapandı (kod "+std::to_wstring(code)+L"). Log klasörünü aç veya hata raporu oluştur.");
    else Status(L"Game closed. You can launch again.",L"Oyun kapandı. Tekrar başlatabilirsin.");
  }
};
LRESULT CALLBACK WindowProc(HWND window,UINT message,WPARAM wparam,LPARAM lparam) {
  auto* app=reinterpret_cast<App*>(GetWindowLongPtrW(window,GWLP_USERDATA));
  if(message==WM_NCCREATE) {
    app=static_cast<App*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
    app->window=window; SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(app));
  }
  if(!app) return DefWindowProcW(window,message,wparam,lparam);
  switch(message) {
    case WM_CREATE: try { app->BuildControls(); } catch(const std::exception& e){app->ErrorBox(e);return -1;} return 0;
    case WM_COMMAND: app->Command(LOWORD(wparam),HIWORD(wparam)); return 0;
    case WM_TIMER: app->Poll(); return 0;
    case WM_CTLCOLORSTATIC: case WM_CTLCOLORBTN: {
      auto control=reinterpret_cast<HWND>(lparam);
      RECT r; GetWindowRect(control,&r); MapWindowPoints(nullptr,window,reinterpret_cast<POINT*>(&r),2);
      bool dark=r.top<app->S(108);
      SetTextColor(reinterpret_cast<HDC>(wparam),dark?RGB(235,237,241):RGB(28,34,43));
      SetBkColor(reinterpret_cast<HDC>(wparam),dark?RGB(24,29,39):RGB(245,246,249));
      return reinterpret_cast<LRESULT>(dark?app->header:app->background);
    }
    case WM_ERASEBKGND: {
      RECT r; GetClientRect(window,&r); FillRect(reinterpret_cast<HDC>(wparam),&r,app->background);
      r.bottom=app->S(108); FillRect(reinterpret_cast<HDC>(wparam),&r,app->header);
      r.top=r.bottom;r.bottom+=app->S(4);auto accent=CreateSolidBrush(RGB(209,35,42));
      FillRect(reinterpret_cast<HDC>(wparam),&r,accent);DeleteObject(accent);return 1;
    }
    case WM_DESTROY: PostQuitMessage(0); return 0;
  }
  return DefWindowProcW(window,message,wparam,lparam);
}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int show) {
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
  INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_STANDARD_CLASSES};InitCommonControlsEx(&controls);
  try {
    App app;
    auto mutex_name=L"Local\\MotoGP0910Launcher-"+std::to_wstring(std::hash<std::wstring>{}(app.exe.wstring()));
    HANDLE mutex=CreateMutexW(nullptr,FALSE,mutex_name.c_str());
    if(!mutex) throw std::runtime_error("Could not initialize launcher lock");
    if(GetLastError()==ERROR_ALREADY_EXISTS) {
      CloseHandle(mutex);MessageBoxW(nullptr,app.T(L"The launcher is already open.",L"Launcher zaten açık."),L"MotoGP 09/10",MB_OK);CoUninitialize();return 0;
    }
    RECT work{};SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);
    app.scale=std::max(0.7,std::min({GetDpiForSystem()/96.0,(work.right-work.left-40)/780.0,(work.bottom-work.top-50)/620.0}));
    WNDCLASSW cls{};cls.lpfnWndProc=WindowProc;cls.hInstance=instance;cls.lpszClassName=L"MotoGP0910Launcher";
    cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.hIcon=LoadIconW(nullptr,IDI_APPLICATION);
    RegisterClassW(&cls);
    RECT size{0,0,app.S(780),app.S(620)};
    constexpr DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX;
    AdjustWindowRectEx(&size,style,FALSE,0);
    auto window=CreateWindowExW(0,cls.lpszClassName,L"MotoGP 09/10 Launcher",style,
        (work.right-size.right+size.left)/2,(work.bottom-size.bottom+size.top)/2,
        size.right-size.left,size.bottom-size.top,nullptr,nullptr,instance,&app);
    if(!window) { CloseHandle(mutex);throw std::runtime_error("Cannot create launcher window"); }
    ShowWindow(window,show);UpdateWindow(window);
    MSG message;
    while(GetMessageW(&message,nullptr,0,0)>0) {
      if(!IsDialogMessageW(window,&message)) { TranslateMessage(&message);DispatchMessageW(&message); }
    }
    CloseHandle(mutex);
  } catch(const std::exception& e) {
    MessageBoxW(nullptr,(L"Cannot open launcher / Launcher açılamadı:\n"+ErrorDetails(e)).c_str(),L"MotoGP 09/10",MB_OK|MB_ICONERROR);
    CoUninitialize();return 1;
  }
  CoUninitialize();return 0;
}
