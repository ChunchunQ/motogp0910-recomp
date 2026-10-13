#include "core.h"
#include "hardware.h"
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
                 kKeyboard, kMute, kSave, kDefaults, kLogs, kReport, kPlay, kLanguage, kTabs, kScaleX, kScaleY, kEffect, kSharpness, kAnisotropic,
                 kLetterbox, kDither, kVolume, kBindingList, kBindingEdit, kCapture, kClearBinding, kResetBinding, kPostEffect, kShaderCache, kMsaa, kAdapter };
struct App {
  fs::path exe = ExecutableFolder();
  Config config{exe / "motogp0910.toml"};
  Config preferences{exe / "motogp0910-launcher.toml"};
  Settings settings = Settings::Load(config.table);
  bool turkish = Get(preferences.table, "language", std::string("en")) == "tr";
  HWND window{}, path{}, size{}, backend{}, language{}, status{}, title{}, subtitle{},
       folder_label{}, display_label{}, size_label{}, input_label{}, help{},
       fullscreen{}, vsync{}, keyboard{}, mute{}, browse{}, save{}, defaults{}, logs{}, report{}, play{};
  HWND adapter{}, msaa{}, post_effect{}, shader_cache{}, tabs{}, scale_x{}, scale_y{}, effect{}, sharpness{}, sharp_value{}, anisotropic{},
       letterbox{}, dither{}, volume{}, volume_value{}, bindings{}, binding_edit{},
       binding_label{}, capture{}, clear_binding{}, reset_binding{};
  struct Label { HWND control; const wchar_t* en; const wchar_t* tr; };
  std::vector<Label> labels;
  std::array<std::vector<HWND>,3> pages;
  std::array<std::string,kBindings.size()> edited_bindings=settings.bindings;
  std::vector<Adapter> adapters=HardwareAdapters();
  int current_page=0, binding_index=0;
  bool filling_binding=false, capturing=false;
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
  HWND PageControl(int page,const wchar_t* cls,const wchar_t* text,DWORD style,
                   int x,int y,int w,int h,int id=0) {
    auto control=Control(cls,text,style,x,y,w,h,id);pages[page].push_back(control);return control;
  }
  HWND LabelControl(int page,const wchar_t* en,const wchar_t* tr,int x,int y,int w,int h=24) {
    auto c=PageControl(page,L"STATIC",L"",0,x,y,w,h);labels.push_back({c,en,tr});return c;
  }
  void FillCombo(HWND combo,std::initializer_list<std::wstring> items) {
    auto index=SendMessageW(combo,CB_GETCURSEL,0,0);
    SendMessageW(combo,CB_RESETCONTENT,0,0);
    for(auto& item:items) SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(item.c_str()));
    SendMessageW(combo,CB_SETCURSEL,index<0?0:index,0);
  }
  void ShowPage(int page) {
    capturing=false; current_page=page;
    for(int i=0;i<3;++i) for(auto control:pages[i]) ShowWindow(control,i==page?SW_SHOW:SW_HIDE);
    SetWindowTextW(capture,T(L"Press a key…",L"Tuşa bas…"));
    InvalidateRect(window,nullptr,TRUE);
  }
  void UpdateSharpness() {
    auto mode=SendMessageW(effect,CB_GETCURSEL,0,0);
    EnableWindow(sharpness,!process && mode!=0);
    auto value=SendMessageW(sharpness,TBM_GETPOS,0,0);
    SetWindowTextW(sharp_value,(std::to_wstring(value)+L"%").c_str());
  }
  void Localize() {
    SetWindowTextW(window,L"MotoGP 09/10 Launcher");
    SetWindowTextW(subtitle,T(L"PC RECOMPILATION  /  ReXGlue v0.10.0",L"PC YENİDEN DERLEME  /  ReXGlue v0.10.0"));
    SetWindowTextW(folder_label,T(L"GAME LIBRARY",L"OYUN KLASÖRÜ"));
    SetWindowTextW(browse,T(L"Browse…",L"Gözat…"));
    for(auto& label:labels) SetWindowTextW(label.control,T(label.en,label.tr));
    SetWindowTextW(fullscreen,T(L"Fullscreen",L"Tam ekran"));
    SetWindowTextW(vsync,L"VSync");
    SetWindowTextW(letterbox,T(L"Keep aspect ratio",L"En-boy oranını koru"));
    SetWindowTextW(msaa,T(L"Native 2× MSAA",L"Yerel 2× MSAA"));
    SetWindowTextW(shader_cache,T(L"Shader cache",L"Shader önbelleği"));
    FillCombo(post_effect,{T(L"Off (default)",L"Kapalı (varsayılan)"),L"FXAA",L"FXAA Extreme"});
    SetWindowTextW(dither,T(L"Reduce color banding",L"Renk bantlaşmasını azalt"));
    SetWindowTextW(keyboard,T(L"Enable keyboard controls",L"Klavye kontrolünü aç"));
    SetWindowTextW(mute,T(L"Mute all audio",L"Tüm sesleri kapat"));
    SetWindowTextW(save,T(L"Save settings",L"Ayarları kaydet"));
    SetWindowTextW(defaults,T(L"Restore defaults",L"Varsayılan ayarlar"));
    SetWindowTextW(logs,T(L"Open logs",L"Log klasörü"));
    SetWindowTextW(report,T(L"Error report",L"Hata raporu"));
    SetWindowTextW(play,T(process?L"GAME RUNNING":L"PLAY",process?L"OYUN ÇALIŞIYOR":L"OYUNU BAŞLAT"));
    SetWindowTextW(capture,T(capturing?L"Listening… Esc to cancel":L"Press a key…",capturing?L"Dinleniyor… Esc: iptal":L"Tuşa bas…"));
    SetWindowTextW(clear_binding,T(L"Unbind",L"Tuşu kaldır"));
    SetWindowTextW(reset_binding,T(L"Reset action",L"Tuşu sıfırla"));
    for(int i=0;i<3;++i) {
      auto text=i==0?T(L"Graphics",L"Grafik"):i==1?T(L"Audio",L"Ses"):T(L"Keyboard & controller",L"Klavye ve kontrolcü");
      TCITEMW item{};item.mask=TCIF_TEXT;item.pszText=const_cast<wchar_t*>(text);
      TabCtrl_SetItem(tabs,i,&item);
    }
    auto gpu_selected=SendMessageW(adapter,CB_GETCURSEL,0,0);
    SendMessageW(adapter,CB_RESETCONTENT,0,0);
    SendMessageW(adapter,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(T(L"Automatic (recommended)",L"Otomatik (önerilen)")));
    for(const auto& gpu:adapters) SendMessageW(adapter,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(gpu.name.c_str()));
    SendMessageW(adapter,CB_SETCURSEL,gpu_selected<0?0:gpu_selected,0);
    auto selected=SendMessageW(size,CB_GETCURSEL,0,0);
    SendMessageW(size,CB_RESETCONTENT,0,0);
    for(auto [w,h]:dimensions) {
      std::wstring text=(w||h)?std::to_wstring(w)+L" × "+std::to_wstring(h):T(L"Automatic / desktop",L"Otomatik / masaüstü");
      SendMessageW(size,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));
    }
    SendMessageW(size,CB_SETCURSEL,selected<0?0:selected,0);
    FillCombo(backend,{L"SDL — Xbox / PlayStation / USB",T(L"XInput — Xbox-compatible",L"XInput — Xbox uyumlu")});
    FillCombo(effect,{T(L"Bilinear (default)",L"Bilinear (varsayılan)"),L"AMD FSR 1.0",L"AMD CAS"});
    FillCombo(anisotropic,{T(L"Use game setting",L"Oyunun ayarı"),T(L"Off",L"Kapalı"),L"1×",L"2×",L"4×",L"8×",L"16×"});
    LVCOLUMNW column{};column.mask=LVCF_TEXT;
    column.pszText=const_cast<wchar_t*>(T(L"Xbox 360 action",L"Xbox 360 eylemi"));ListView_SetColumn(bindings,0,&column);
    column.pszText=const_cast<wchar_t*>(T(L"Keyboard binding",L"Klavye tuşu"));ListView_SetColumn(bindings,1,&column);
    for(size_t i=0;i<kBindings.size();++i)
      ListView_SetItemText(bindings,i,0,const_cast<wchar_t*>(T(kBindings[i].english,kBindings[i].turkish)));
    SetWindowTextW(binding_label,T(kBindings[binding_index].english,kBindings[binding_index].turkish));
    SetWindowTextW(status,(turkish?status_tr:status_en).c_str());
    UpdateSharpness();
    RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);
  }
  void SelectBinding(int index) {
    if(index<0 || index>=static_cast<int>(kBindings.size())) return;
    capturing=false; binding_index=index; filling_binding=true;
    SetWindowTextW(binding_edit,Wide(edited_bindings[index]).c_str());
    SetWindowTextW(binding_label,T(kBindings[index].english,kBindings[index].turkish));
    filling_binding=false;
    SetWindowTextW(capture,T(L"Press a key…",L"Tuşa bas…"));
  }
  bool CaptureKey(const MSG& message) {
    if(!capturing || (message.message!=WM_KEYDOWN && message.message!=WM_SYSKEYDOWN)) return false;
    auto vk=static_cast<UINT>(message.wParam);
    if(vk==VK_ESCAPE) {capturing=false;SetWindowTextW(capture,T(L"Press a key…",L"Tuşa bas…"));return true;}
    if(vk==VK_SHIFT || vk==VK_CONTROL || vk==VK_MENU) return true;
    auto key=KeyName(vk);if(key.empty()) return true;
    std::string value;
    if(GetKeyState(VK_CONTROL)&0x8000) value+="Ctrl+";
    if(GetKeyState(VK_MENU)&0x8000) value+="Alt+";
    if(GetKeyState(VK_SHIFT)&0x8000) value+="Shift+";
    value+=key;capturing=false;
    SetWindowTextW(binding_edit,Wide(value).c_str());
    SetWindowTextW(capture,T(L"Press a key…",L"Tuşa bas…"));return true;
  }
  void SetControls(const Settings& s) {
    auto found=std::find(dimensions.begin(),dimensions.end(),std::pair{s.width,s.height});
    if(found==dimensions.end()) { dimensions.emplace_back(s.width,s.height); Localize(); found=dimensions.end()-1; }
    SendMessageW(size,CB_SETCURSEL,std::distance(dimensions.begin(),found),0);
    int gpu_index=0;
    if(s.adapter!=-1) {
      auto gpu=std::find_if(adapters.begin(),adapters.end(),[&](const Adapter& value){return value.index==s.adapter;});
      if(gpu==adapters.end()) {
        adapters.push_back({s.adapter,L"DXGI adapter #"+std::to_wstring(s.adapter)});
        SendMessageW(adapter,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(adapters.back().name.c_str()));gpu=adapters.end()-1;
      }
      gpu_index=static_cast<int>(std::distance(adapters.begin(),gpu))+1;
    }
    SendMessageW(adapter,CB_SETCURSEL,gpu_index,0);
    SendMessageW(backend,CB_SETCURSEL,s.backend=="xinput"?1:0,0);
    SendMessageW(scale_x,CB_SETCURSEL,std::clamp(s.scale_x,1,8)-1,0);
    SendMessageW(scale_y,CB_SETCURSEL,std::clamp(s.scale_y,1,8)-1,0);
    SendMessageW(effect,CB_SETCURSEL,s.effect=="fsr"?1:s.effect=="cas"?2:0,0);
    SendMessageW(post_effect,CB_SETCURSEL,s.post_effect=="fxaa"?1:s.post_effect=="fxaa_extreme"?2:0,0);
    SendMessageW(anisotropic,CB_SETCURSEL,std::clamp(s.anisotropic,-1,5)+1,0);
    SendMessageW(volume,TBM_SETPOS,TRUE,std::clamp(s.volume,0,100));
    SetWindowTextW(volume_value,(std::to_wstring(std::clamp(s.volume,0,100))+L"%").c_str());
    SendMessageW(sharpness,TBM_SETPOS,TRUE,s.effect=="cas"?static_cast<int>((std::isfinite(s.cas_sharpness)?std::clamp(s.cas_sharpness,0.0,1.0):0.0)*100):static_cast<int>(100-(std::isfinite(s.fsr_reduction)?std::clamp(s.fsr_reduction,0.0,2.0):0.2)*50));
    for(auto [control,checked]:{std::pair{fullscreen,s.fullscreen}, {vsync,s.vsync},{keyboard,s.keyboard},{mute,s.mute},
        {letterbox,s.letterbox},{dither,s.dither},{shader_cache,s.shader_cache},{msaa,s.msaa}})
      SendMessageW(control,BM_SETCHECK,checked?BST_CHECKED:BST_UNCHECKED,0);
    edited_bindings=s.bindings;
    for(size_t i=0;i<kBindings.size();++i) {
      auto text=Wide(edited_bindings[i]);ListView_SetItemText(bindings,i,1,text.data());
    }
    SelectBinding(binding_index);UpdateSharpness();
  }
  Settings GetControls() const {
    Settings s=settings;
    auto index=SendMessageW(size,CB_GETCURSEL,0,0);
    if(index<0 || static_cast<size_t>(index)>=dimensions.size()) throw std::runtime_error("Invalid window size selection");
    std::tie(s.width,s.height)=dimensions[index];
    auto gpu_index=SendMessageW(adapter,CB_GETCURSEL,0,0);
    if(gpu_index<0 || gpu_index>static_cast<LRESULT>(adapters.size())) throw std::runtime_error("Invalid GPU selection");
    s.adapter=gpu_index==0?-1:adapters[gpu_index-1].index;
    s.backend=SendMessageW(backend,CB_GETCURSEL,0,0)==1?"xinput":"sdl";
    s.scale_x=SendMessageW(scale_x,CB_GETCURSEL,0,0)+1;
    s.scale_y=SendMessageW(scale_y,CB_GETCURSEL,0,0)+1;
    auto mode=SendMessageW(effect,CB_GETCURSEL,0,0);s.effect=mode==1?"fsr":mode==2?"cas":"bilinear";
    s.anisotropic=SendMessageW(anisotropic,CB_GETCURSEL,0,0)-1;
    s.volume=SendMessageW(volume,TBM_GETPOS,0,0);
    if(mode==1) s.fsr_reduction=2.0-SendMessageW(sharpness,TBM_GETPOS,0,0)/50.0;
    if(mode==2) s.cas_sharpness=SendMessageW(sharpness,TBM_GETPOS,0,0)/100.0;
    s.fullscreen=SendMessageW(fullscreen,BM_GETCHECK,0,0)==BST_CHECKED;
    s.vsync=SendMessageW(vsync,BM_GETCHECK,0,0)==BST_CHECKED;
    s.keyboard=SendMessageW(keyboard,BM_GETCHECK,0,0)==BST_CHECKED;
    s.mute=SendMessageW(mute,BM_GETCHECK,0,0)==BST_CHECKED;
    s.letterbox=SendMessageW(letterbox,BM_GETCHECK,0,0)==BST_CHECKED;
    s.dither=SendMessageW(dither,BM_GETCHECK,0,0)==BST_CHECKED;
    auto aa=SendMessageW(post_effect,CB_GETCURSEL,0,0);s.post_effect=aa==1?"fxaa":aa==2?"fxaa_extreme":"none";
    s.msaa=SendMessageW(msaa,BM_GETCHECK,0,0)==BST_CHECKED;
    s.shader_cache=SendMessageW(shader_cache,BM_GETCHECK,0,0)==BST_CHECKED;
    s.bindings=edited_bindings;s.Validate();return s;
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
    heading=CreateFontW(-S(32),0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    title=Control(L"STATIC",L"MotoGP 09/10",0,28,18,600,44);
    SendMessageW(title,WM_SETFONT,reinterpret_cast<WPARAM>(heading),TRUE);
    subtitle=Control(L"STATIC",L"",0,30,67,580,24);
    Control(L"STATIC",L"Language / Dil",0,670,23,200,24);
    language=Control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,670,52,200,140,kLanguage);
    SendMessageW(language,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"English"));
    SendMessageW(language,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Türkçe"));
    SendMessageW(language,CB_SETCURSEL,turkish?1:0,0);
    folder_label=Control(L"STATIC",L"",0,28,128,842,24);
    path=Control(L"EDIT",ResolveGameRoot(exe,config.table).c_str(),WS_TABSTOP|ES_AUTOHSCROLL,28,158,704,32,kPath);
    SendMessageW(path,EM_SETLIMITTEXT,32760,0);
    browse=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,744,158,126,32,kBrowse);
    tabs=Control(WC_TABCONTROLW,L"",WS_TABSTOP,28,214,842,30,kTabs);
    for(int i=0;i<3;++i) {TCITEMW item{};item.mask=TCIF_TEXT;item.pszText=const_cast<wchar_t*>(L" ");TabCtrl_InsertItem(tabs,i,&item);}
    auto combo=[&](int page,int x,int y,int w,int id) {return PageControl(page,L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,x,y,w,220,id);};
    LabelControl(0,L"GPU / GRAPHICS ADAPTER",L"GPU / EKRAN KARTI",40,267,370);
    adapter=combo(0,40,293,370,kAdapter);
    LabelControl(0,L"RENDER QUALITY",L"İŞLEME KALİTESİ",458,267,380);
    LabelControl(0,L"Output window size",L"Çıkış pencere boyutu",40,338,350);
    size=combo(0,40,364,370,kSize);
    LabelControl(0,L"Internal render scale (X / Y)",L"İç çözünürlük ölçeği (X / Y)",458,308,380);
    scale_x=combo(0,458,334,183,kScaleX);scale_y=combo(0,655,334,183,kScaleY);
    for(int i=1;i<=8;++i) {auto text=std::to_wstring(i)+L"×";SendMessageW(scale_x,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));SendMessageW(scale_y,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));}
    LabelControl(0,L"Upscaling / sharpening",L"Büyütme / keskinleştirme",40,402,370);
    effect=combo(0,40,428,370,kEffect);
    LabelControl(0,L"Anisotropic texture filtering",L"Anizotropik doku filtreleme",458,383,380);
    anisotropic=combo(0,458,410,380,kAnisotropic);
    LabelControl(0,L"Sharpness",L"Keskinlik",40,470,310);
    sharp_value=PageControl(0,L"STATIC",L"90%",SS_RIGHT,344,470,66,24);
    sharpness=PageControl(0,TRACKBAR_CLASSW,L"",WS_TABSTOP|TBS_HORZ|TBS_NOTICKS,36,494,378,32,kSharpness);
    SendMessageW(sharpness,TBM_SETRANGE,TRUE,MAKELPARAM(0,100));
    fullscreen=PageControl(0,L"BUTTON",L"",WS_TABSTOP|BS_AUTOCHECKBOX,458,457,183,26,kFullscreen);
    vsync=PageControl(0,L"BUTTON",L"",WS_TABSTOP|BS_AUTOCHECKBOX,655,457,183,26,kVsync);
    letterbox=PageControl(0,L"BUTTON",L"",WS_TABSTOP|BS_AUTOCHECKBOX,458,491,183,26,kLetterbox);
    dither=PageControl(0,L"BUTTON",L"",WS_TABSTOP|BS_AUTOCHECKBOX,655,491,210,26,kDither);
    msaa=PageControl(0,L"BUTTON",L"",WS_TABSTOP|BS_AUTOCHECKBOX,40,535,370,26,kMsaa);
    LabelControl(0,L"FSR enlarges the final image.\nHigher render scale uses more GPU memory.",
        L"FSR son görüntüyü büyütür.\nYüksek iç ölçek GPU belleğini artırır.",40,578,370,34);
    LabelControl(0,L"Anti-aliasing",L"Kenar yumuşatma",458,533,380);
    post_effect=combo(0,458,560,183,kPostEffect);
    shader_cache=PageControl(0,L"BUTTON",L"",WS_TABSTOP|BS_AUTOCHECKBOX,655,559,190,28,kShaderCache);
    LabelControl(1,L"MASTER VOLUME",L"ANA SES DÜZEYİ",40,267,798);
    LabelControl(1,L"Controls the final output, including music, engines and menu sounds.",
        L"Müzik, motor ve menü sesleri dahil toplam çıkış sesini ayarlar.",40,309,798);
    volume=PageControl(1,TRACKBAR_CLASSW,L"",WS_TABSTOP|TBS_HORZ|TBS_NOTICKS,36,360,710,42,kVolume);
    SendMessageW(volume,TBM_SETRANGE,TRUE,MAKELPARAM(0,100));
    volume_value=PageControl(1,L"STATIC",L"100%",SS_RIGHT,770,366,68,30);
    mute=PageControl(1,L"BUTTON",L"",WS_TABSTOP|BS_AUTOCHECKBOX,40,432,798,28,kMute);
    LabelControl(1,L"The volume is saved with your game settings.\nUse the game's own options for separate music and effects levels.",
        L"Ses düzeyi oyun ayarlarınla birlikte kaydedilir.\nMüzik ve efektleri ayrı ayarlamak için oyunun kendi seçeneklerini kullan.",40,510,798,56);
    LabelControl(2,L"KEYBOARD MAPPINGS",L"KLAVYE EŞLEMELERİ",40,267,460);
    bindings=PageControl(2,WC_LISTVIEWW,L"",WS_TABSTOP|LVS_REPORT|LVS_SINGLESEL|LVS_SHOWSELALWAYS,40,299,478,267,kBindingList);
    ListView_SetExtendedListViewStyle(bindings,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);
    LVCOLUMNW col{};col.mask=LVCF_WIDTH|LVCF_TEXT;col.cx=S(238);col.pszText=const_cast<wchar_t*>(L"Action");ListView_InsertColumn(bindings,0,&col);
    col.cx=S(210);col.pszText=const_cast<wchar_t*>(L"Binding");ListView_InsertColumn(bindings,1,&col);
    for(size_t i=0;i<kBindings.size();++i) {LVITEMW item{};item.mask=LVIF_TEXT;item.iItem=i;item.pszText=const_cast<wchar_t*>(kBindings[i].english);ListView_InsertItem(bindings,&item);}
    binding_label=PageControl(2,L"STATIC",L"",0,546,299,300,24);
    binding_edit=PageControl(2,L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,546,331,292,32,kBindingEdit);
    SendMessageW(binding_edit,EM_SETLIMITTEXT,512,0);
    capture=PageControl(2,L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,546,375,292,34,kCapture);
    clear_binding=PageControl(2,L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,546,420,136,30,kClearBinding);
    reset_binding=PageControl(2,L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,694,420,144,30,kResetBinding);
    LabelControl(2,L"Controller backend",L"Kontrolcü bağlantısı",546,470,292);
    backend=combo(2,546,499,292,kBackend);
    keyboard=PageControl(2,L"BUTTON",L"",WS_TABSTOP|BS_AUTOCHECKBOX,546,540,310,26,kKeyboard);
    LabelControl(2,L"Examples: Space  /  Shift+Up  /  Q,I (alternative keys). Empty = unbound.",
        L"Örnek: Space  /  Shift+Up  /  Q,I (alternatif tuşlar). Boş = tuş atanmamış.",40,577,798,30);
    status=Control(L"STATIC",L"",0,28,619,842,36);
    save=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,28,668,156,38,kSave);
    defaults=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,194,668,174,38,kDefaults);
    logs=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,378,668,126,38,kLogs);
    report=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,514,668,140,38,kReport);
    play=Control(L"BUTTON",L"",WS_TABSTOP|BS_DEFPUSHBUTTON,670,660,200,54,kPlay);
    Localize(); SetControls(settings);ListView_SetItemState(bindings,0,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
    ShowPage(0); ReadyStatus();
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
    capturing=false;
    for(auto control:{path,browse,save,defaults,play}) EnableWindow(control,!value);
    for(auto& page:pages) for(auto control:page) EnableWindow(control,!value);
    Localize();
  }
  void Command(int id,int event) {
    try {
      if(id==kLanguage && event==CBN_SELCHANGE) {
        turkish=SendMessageW(language,CB_GETCURSEL,0,0)==1;
        Localize(); Put(preferences.table,"language",std::string(turkish?"tr":"en")); preferences.Save();
      } else if(id==kBindingEdit && event==EN_CHANGE && !filling_binding) {
        edited_bindings[binding_index]=Utf8(Text(binding_edit));
        auto text=Text(binding_edit);ListView_SetItemText(bindings,binding_index,1,text.data());
      } else if(id==kEffect && event==CBN_SELCHANGE) {
        auto mode=SendMessageW(effect,CB_GETCURSEL,0,0);
        SendMessageW(sharpness,TBM_SETPOS,TRUE,mode==2?static_cast<int>(settings.cas_sharpness*100):static_cast<int>(100-settings.fsr_reduction*50));
        UpdateSharpness();
      } else if(id==kPath && event==EN_CHANGE) {
        Status(L"Press PLAY to check and launch the selected game folder.",L"Seçilen klasörü kontrol edip açmak için OYUNU BAŞLAT'a bas.");
      } else if(event==BN_CLICKED) switch(id) {
        case kCapture: capturing=true;SetFocus(capture);SetWindowTextW(capture,T(L"Listening… Esc to cancel",L"Dinleniyor… Esc: iptal"));break;
        case kClearBinding: SetWindowTextW(binding_edit,L"");break;
        case kResetBinding: SetWindowTextW(binding_edit,Wide(kBindings[binding_index].defaults).c_str());break;
        case kBrowse: Browse(); break;
        case kSave: SaveSettings(); break;
        case kDefaults: settings=Settings{};SetControls(settings); Status(L"Defaults selected. Save or press PLAY to apply.",L"Varsayılanlar seçildi. Uygulamak için kaydet veya oyunu başlat."); break;
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
    case WM_NOTIFY: {
      auto* notify=reinterpret_cast<NMHDR*>(lparam);
      if(notify->idFrom==kTabs && notify->code==TCN_SELCHANGE) app->ShowPage(TabCtrl_GetCurSel(app->tabs));
      if(notify->idFrom==kBindingList && notify->code==LVN_ITEMCHANGED) {
        auto* change=reinterpret_cast<NMLISTVIEW*>(lparam);
        if((change->uChanged&LVIF_STATE) && (change->uNewState&LVIS_SELECTED) && !(change->uOldState&LVIS_SELECTED)) app->SelectBinding(change->iItem);
      }
      return 0;
    }
    case WM_HSCROLL: {
      if(reinterpret_cast<HWND>(lparam)==app->volume) SetWindowTextW(app->volume_value,(std::to_wstring(SendMessageW(app->volume,TBM_GETPOS,0,0))+L"%").c_str());
      if(reinterpret_cast<HWND>(lparam)==app->sharpness) {
        auto value=SendMessageW(app->sharpness,TBM_GETPOS,0,0);
        auto mode=SendMessageW(app->effect,CB_GETCURSEL,0,0);
        if(mode==1) app->settings.fsr_reduction=2.0-value/50.0;
        if(mode==2) app->settings.cas_sharpness=value/100.0;
        app->UpdateSharpness();
      }
      return 0;
    }
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
  INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_STANDARD_CLASSES|ICC_TAB_CLASSES|ICC_LISTVIEW_CLASSES|ICC_BAR_CLASSES};InitCommonControlsEx(&controls);
  try {
    App app;
    auto mutex_name=L"Local\\MotoGP0910Launcher-"+std::to_wstring(std::hash<std::wstring>{}(app.exe.wstring()));
    HANDLE mutex=CreateMutexW(nullptr,FALSE,mutex_name.c_str());
    if(!mutex) throw std::runtime_error("Could not initialize launcher lock");
    if(GetLastError()==ERROR_ALREADY_EXISTS) {
      CloseHandle(mutex);MessageBoxW(nullptr,app.T(L"The launcher is already open.",L"Launcher zaten açık."),L"MotoGP 09/10",MB_OK);CoUninitialize();return 0;
    }
    RECT work{};SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);
    app.scale=std::max(0.7,std::min({GetDpiForSystem()/96.0,(work.right-work.left-40)/900.0,(work.bottom-work.top-50)/740.0}));
    WNDCLASSW cls{};cls.lpfnWndProc=WindowProc;cls.hInstance=instance;cls.lpszClassName=L"MotoGP0910Launcher";
    cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.hIcon=LoadIconW(nullptr,IDI_APPLICATION);
    RegisterClassW(&cls);
    RECT size{0,0,app.S(900),app.S(740)};
    constexpr DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX;
    AdjustWindowRectEx(&size,style,FALSE,0);
    auto window=CreateWindowExW(0,cls.lpszClassName,L"MotoGP 09/10 Launcher",style,
        (work.right-size.right+size.left)/2,(work.bottom-size.bottom+size.top)/2,
        size.right-size.left,size.bottom-size.top,nullptr,nullptr,instance,&app);
    if(!window) { CloseHandle(mutex);throw std::runtime_error("Cannot create launcher window"); }
    ShowWindow(window,show);UpdateWindow(window);
    MSG message;
    while(GetMessageW(&message,nullptr,0,0)>0) {
      if(app.CaptureKey(message)) continue;
      if(!IsDialogMessageW(window,&message)) { TranslateMessage(&message);DispatchMessageW(&message); }
    }
    CloseHandle(mutex);
  } catch(const std::exception& e) {
    MessageBoxW(nullptr,(L"Cannot open launcher / Launcher açılamadı:\n"+ErrorDetails(e)).c_str(),L"MotoGP 09/10",MB_OK|MB_ICONERROR);
    CoUninitialize();return 1;
  }
  CoUninitialize();return 0;
}
