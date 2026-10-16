#include "core.h"
#include "file_verification.h"
#include "adaptive_triggers_dialog.h"
#include "hardware.h"
#include "achievements.h"
#include "../achievement_preferences.h"
#include <shlwapi.h>
#include <future>
#include "ui_ids.h"
#include "../window_icons.h"
#include <commctrl.h>
#include <shobjidl.h>
#include <shellapi.h>
#include <gdiplus.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <algorithm>
#include <memory>
#include <map>

using namespace launcher;
namespace {
constexpr int kWidth=940,kHeight=550,kCaptionHeight=28;
constexpr COLORREF kBackground=RGB(13,15,20),kSidebar=RGB(19,21,27),kCard=RGB(23,26,33),
    kField=RGB(32,36,45),kText=RGB(242,244,247),kMuted=RGB(157,165,178),
    kAccent=RGB(255,170,48),kRed=RGB(239,66,47),kBorder=RGB(48,53,64);
std::wstring ErrorDetails(const std::exception& error) {
  try {return Wide(error.what());} catch(const std::exception&) {
    std::string text=error.what();
    int n=MultiByteToWideChar(CP_ACP,0,text.data(),static_cast<int>(text.size()),nullptr,0);
    std::wstring result(n,L'\0');
    if(n)MultiByteToWideChar(CP_ACP,0,text.data(),static_cast<int>(text.size()),result.data(),n);
    return result.empty()?L"System error":result;
  }
}

struct App {
  fs::path exe=ExecutableFolder();
  Config config{exe/"motogp0910.toml"},preferences{exe/"motogp0910-launcher.toml"};
  Settings settings=Settings::Load(config.table);
  bool turkish=Get(preferences.table,"language",std::string("en"))=="tr";
  HWND window{},path{},size{},backend{},language{},status{},title{},subtitle{},folder_label{},
       fullscreen{},vsync{},keyboard{},mute{},browse{},save{},defaults{},logs{},report{},play{},
       adapter{},msaa{},post_effect{},shader_cache{},render_resolution{},effect{},sharpness{},sharp_value{},
       anisotropic{},letterbox{},dither{},volume{},volume_value{},tooltip{},prompt_style{},game_language{},refresh_rate{},fps_limit{},fsr_quality{},fsr_notice{},aspect{};
  std::array<HWND,6> navigation{};
  std::array<HWND,kEngineEffects.size()> engine_choices{};
  HWND adaptive_triggers{},crash_diagnostics{},shadow_distance{},lan_enabled{},lan_player{},lan_room{},lan_adapter{},lan_host{},online_server{};
  std::vector<int> shadow_distances{kShadowDistances.begin(),kShadowDistances.end()};
  std::array<HWND,kBindings.size()> binding_fields{},binding_buttons{};
  struct Label {HWND control;const wchar_t* en;const wchar_t* tr;};
  struct Placement {HWND control;int x,y,w,h;};
  std::vector<Label> labels;
  std::vector<Placement> placements;
  std::array<std::vector<HWND>,6> pages;
  std::map<HWND,bool> checks;
  std::array<std::string,kBindings.size()> edited_bindings=settings.bindings;
  std::vector<Adapter> adapters=HardwareAdapters();
  std::pair<int,int> display_maximum=HardwareDisplayMaximum(Get<int64_t>(config.table,"monitor",0));
  std::vector<std::pair<int,int>> dimensions=WindowSizesFor(display_maximum);
  std::vector<int> refresh_rates{0},fps_limits{0,30,60,90,120,144,165,180,240};
  int current_page=3,binding_index=0;
  bool filling_binding=false,capturing=false;
  HFONT font{},heading{},small_font{},bold{},logo{};
  HBRUSH background=CreateSolidBrush(kBackground),card=CreateSolidBrush(kCard),field=CreateSolidBrush(kField);
  HANDLE process{};
  double scale=1;
  std::wstring status_en,status_tr;
  bool status_positive=false;
  #include "caption.inc"
  #include "achievements_page.inc"
  ~App(){verify_cancel=true;if(verify_future.valid())verify_future.wait();if(catalog_future.valid())catalog_future.wait();if(process)CloseHandle(process);DeleteFonts();DeleteObject(background);DeleteObject(card);DeleteObject(field);}
  const wchar_t* T(const wchar_t* en,const wchar_t* tr)const{return turkish?tr:en;}
  int S(int value)const{return static_cast<int>(value*scale+0.5);}
  void DeleteFonts(){for(auto f:{font,heading,small_font,bold,logo})if(f)DeleteObject(f);}
  void CreateFonts(){
    auto make=[&](int size,int weight){return CreateFontW(-S(size),0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");};
    font=make(13,FW_NORMAL);heading=make(28,FW_BOLD);small_font=make(12,FW_NORMAL);bold=make(13,FW_SEMIBOLD);logo=make(36,FW_BOLD);
  }
  HWND Control(const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int w,int h,int id=0,HWND parent=nullptr){
    bool button=std::wstring_view(cls)==L"BUTTON";
    if(std::wstring_view(cls)==L"STATIC")style|=SS_NOPREFIX;
    bool checkbox=button && (style&BS_TYPEMASK)==BS_AUTOCHECKBOX;
    if(button)style=(style&~BS_TYPEMASK)|BS_OWNERDRAW;
    if(std::wstring_view(cls)==L"COMBOBOX")style|=CBS_OWNERDRAWFIXED|CBS_HASSTRINGS;
    auto c=CreateWindowExW(0,cls,text,WS_CHILD|WS_VISIBLE|style,S(x),S(y+(parent?0:kCaptionHeight)),S(w),S(h),parent?parent:window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
    if(!c)throw std::runtime_error("Cannot create launcher control");
    placements.push_back({c,x,y,w,h});
    if(checkbox){checks.emplace(c,false);SetWindowSubclass(c,CheckProc,1,reinterpret_cast<DWORD_PTR>(this));}
    SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);
    if(std::wstring_view(cls)==L"EDIT")SetWindowTheme(c,L"",L"");
    if(std::wstring_view(cls)==L"COMBOBOX")SetWindowSubclass(c,ComboFrameProc,8,reinterpret_cast<DWORD_PTR>(this));
    return c;
  }
  static LRESULT CALLBACK CheckProc(HWND control,UINT message,WPARAM wparam,LPARAM lparam,UINT_PTR id,DWORD_PTR data){
    auto& app=*reinterpret_cast<App*>(data);
    if(message==BM_GETCHECK)return app.Checked(control)?BST_CHECKED:BST_UNCHECKED;
    if(message==BM_SETCHECK){app.SetCheck(control,wparam==BST_CHECKED);return 0;}
    if(message==WM_NCDESTROY)RemoveWindowSubclass(control,CheckProc,id);
    return DefSubclassProc(control,message,wparam,lparam);
  }
  static LRESULT CALLBACK BindingProc(HWND control,UINT message,WPARAM wparam,LPARAM lparam,UINT_PTR id,DWORD_PTR data){
    if(message==WM_CONTEXTMENU)return SendMessageW(reinterpret_cast<App*>(data)->window,message,reinterpret_cast<WPARAM>(control),lparam);
    if(message==WM_NCDESTROY)RemoveWindowSubclass(control,BindingProc,id);
    return DefSubclassProc(control,message,wparam,lparam);
  }
  HWND PageControl(int page,const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int w,int h,int id=0){
    auto c=Control(cls,text,style,x,y,w,h,id,page_view);pages[page].push_back(c);
    page_extent[page]=std::max(page_extent[page],y+(std::wstring_view(cls)==L"COMBOBOX"?28:h)+8);
    SetWindowSubclass(c,PageChildProc,7,reinterpret_cast<DWORD_PTR>(this));return c;
  }
  HWND LabelControl(int page,const wchar_t* en,const wchar_t* tr,int x,int y,int w,int h=22){
    auto c=PageControl(page,L"STATIC",L"",0,x,y,w,h);labels.push_back({c,en,tr});return c;
  }
  std::wstring Text(HWND c)const{
    std::wstring v(GetWindowTextLengthW(c)+1,L'\0');v.resize(GetWindowTextW(c,v.data(),static_cast<int>(v.size())));return v;
  }
  fs::path GameRoot()const{
    auto v=Text(path);if(v.empty())return exe/L"game";
    auto root=fs::path(v);return fs::absolute(root.is_absolute()?root:exe/root).lexically_normal();
  }
  void SetCheck(HWND c,bool value){checks[c]=value;InvalidateRect(c,nullptr,FALSE);}
  bool Checked(HWND c)const{return checks.at(c);}
  void Status(std::wstring en,std::wstring tr,bool positive=false){
    status_positive=positive;InvalidateRect(window,nullptr,FALSE);
    status_en=std::move(en);status_tr=std::move(tr);SetWindowTextW(status,(turkish?status_tr:status_en).c_str());
  }
  void ErrorBox(const std::exception& error){
    if(auto* e=dynamic_cast<const Error*>(&error)){
      Status(e->english,e->turkish);MessageBoxW(window,(turkish?e->turkish:e->english).c_str(),T(L"MotoGP 09/10 — Error",L"MotoGP 09/10 — Hata"),MB_OK|MB_ICONERROR);
    }else{
      auto detail=ErrorDetails(error);Status(L"Could not complete the action. "+detail,L"İşlem tamamlanamadı. "+detail);
      MessageBoxW(window,(turkish?status_tr:status_en).c_str(),T(L"Error",L"Hata"),MB_OK|MB_ICONERROR);
    }
  }
  void FillCombo(HWND c,std::initializer_list<std::wstring> items){
    auto index=SendMessageW(c,CB_GETCURSEL,0,0);SendMessageW(c,CB_RESETCONTENT,0,0);
    for(auto& item:items)SendMessageW(c,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(item.c_str()));
    SendMessageW(c,CB_SETCURSEL,index<0?0:index,0);
  }
  void NetworkControls(){
    const auto mode=SendMessageW(lan_enabled,CB_GETCURSEL,0,0);
    for(auto control:{lan_player,lan_room})EnableWindow(control,!process && mode>0);
    for(auto control:{lan_adapter,lan_host})EnableWindow(control,!process && mode==1);
    EnableWindow(online_server,!process && mode==2);
  }
  void CancelCapture(){
    capturing=false;for(auto c:binding_buttons)if(c)SetWindowTextW(c,L"…");
  }
  void ShowPage(int page){
    CancelCapture();current_page=page;
    for(int i=0;i<6;++i)for(auto c:pages[i])ShowWindow(c,i==page?SW_SHOW:SW_HIDE);
    ScrollPage(page_scroll[page]);Localize();if(page==5){if(!catalog_error.empty())catalog_key.clear();SetTimer(window,2,500,nullptr);PollAchievements();}else if(!catalog_future.valid())KillTimer(window,2);InvalidateRect(window,nullptr,FALSE);
  }
  void UpdateSharpness(){
    const auto selected=SendMessageW(effect,CB_GETCURSEL,0,0);
    const bool temporal=selected==3 || selected==4;
    if(fsr_quality)EnableWindow(fsr_quality,temporal && !process);
    if(fsr_notice)SetWindowTextW(fsr_notice,temporal?T(L"Experimental: neutral motion/depth inputs.\nMay fall back to FSR 1; no frame generation.",L"Deneysel: nötr hareket/derinlik verileri.\nFSR 1’e dönebilir; kare üretimi yok."):T(L"Higher resolutions use more GPU memory.",L"Yüksek çözünürlük daha fazla GPU belleği kullanır."));
    EnableWindow(sharpness,!process && SendMessageW(effect,CB_GETCURSEL,0,0)!=0);
    SetWindowTextW(sharp_value,(std::to_wstring(SendMessageW(sharpness,TBM_GETPOS,0,0))+L"%").c_str());
  }
  void RefreshRates(int preferred=-1){
    if(!refresh_rate || !size)return;
    if(preferred<0){auto index=SendMessageW(refresh_rate,CB_GETCURSEL,0,0);preferred=index>=0 && index<static_cast<LRESULT>(refresh_rates.size())?refresh_rates[index]:0;}
    auto index=SendMessageW(size,CB_GETCURSEL,0,0);auto output=index>=0 && index<static_cast<LRESULT>(dimensions.size())?dimensions[index]:std::pair{0,0};
    refresh_rates=HardwareRefreshRates(output.first,output.second,Get<int64_t>(config.table,"monitor",0));
    SendMessageW(refresh_rate,CB_RESETCONTENT,0,0);
    for(auto rate:refresh_rates){auto label=rate?std::to_wstring(rate)+L" Hz":T(L"Automatic",L"Otomatik");SendMessageW(refresh_rate,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}
    const bool exclusive=SendMessageW(fullscreen,CB_GETCURSEL,0,0)==0;
    auto found=std::find(refresh_rates.begin(),refresh_rates.end(),exclusive?preferred:0);
    SendMessageW(refresh_rate,CB_SETCURSEL,found==refresh_rates.end()?0:std::distance(refresh_rates.begin(),found),0);
    EnableWindow(refresh_rate,exclusive && !process);
    EnableWindow(size,SendMessageW(fullscreen,CB_GETCURSEL,0,0)!=1 && !process);
  }
  void FillRenderPresets(int selection=-1){
    if(selection<0)selection=static_cast<int>(SendMessageW(render_resolution,CB_GETCURSEL,0,0));
    SendMessageW(render_resolution,CB_RESETCONTENT,0,0);
    const std::array<std::wstring,3> names{
      T(L"1x - 1280 x 720 (native)",L"1x - 1280 x 720 (yerel)"),
      L"2x - 2560 x 1440",L"3x - 3840 x 2160"};
    for(const auto& text:names)SendMessageW(render_resolution,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));
    if(MatchingRenderPreset(settings)<0){
      auto [w,h]=RenderDimensions(settings);auto text=std::wstring(T(L"Custom — ",L"Özel — "))+std::to_wstring(w)+L" × "+std::to_wstring(h);
      SendMessageW(render_resolution,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));
    }
    SendMessageW(render_resolution,CB_SETCURSEL,selection<0?0:selection,0);
  }
  void Localize(){
    SetWindowTextW(window,L"MotoGP 09/10 Launcher");
    SendMessageW(language,CB_SETCURSEL,turkish?1:0,0);
    SetWindowTextW(title,current_page==0?T(L"Graphics",L"Grafik"):current_page==1?T(L"Audio",L"Ses"):current_page==2?T(L"Controls",L"Kontroller"):current_page==3?T(L"General",L"Genel"):current_page==4?T(L"Advanced",L"Gelişmiş"):T(L"Achievements",L"Başarımlar"));
    SetWindowTextW(subtitle,current_page==0?T(L"Display, detail and upscaling.",L"Görüntü, detay ve ölçekleme."):
        current_page==1?T(L"Set the sound of your race.",L"Yarışın sesini ayarla."):current_page==2?T(L"Your controller. Your keys.",L"Senin kontrolcün. Senin tuşların."):current_page==3?T(L"Your game preferences.",L"Oyun tercihlerin."):current_page==4?T(L"Fine-tune MotoGP engine effects.",L"MotoGP motor efektlerini ayarla."):T(L"Your milestones. Every race counts.",L"Her yarış, yeni bir hedef."));
    SetWindowTextW(folder_label,T(L"GAME FOLDER",L"OYUN KLASÖRÜ"));SetWindowTextW(browse,T(L"Browse",L"Gözat"));
    for(auto& label:labels)SetWindowTextW(label.control,T(label.en,label.tr));
    for(int i=0;i<6;++i)SetWindowTextW(navigation[i],i==0?T(L"Graphics",L"Grafik"):i==1?T(L"Audio",L"Ses"):i==2?T(L"Controls",L"Kontroller"):i==3?T(L"General",L"Genel"):i==4?T(L"Advanced",L"Gelişmiş"):T(L"Achievements",L"Başarımlar"));
    LocalizeAchievements();
    FillCombo(fullscreen,{T(L"Fullscreen",L"Tam ekran"),T(L"Borderless",L"Kenarlıksız"),T(L"Windowed",L"Pencereli")});SetWindowTextW(vsync,L"VSync");
    SetWindowTextW(letterbox,T(L"Letterbox",L"Siyah kenarlar"));
    SetWindowTextW(msaa,T(L"Native 2× MSAA",L"Yerel 2× MSAA"));
    SetWindowTextW(shader_cache,T(L"Shader cache",L"Shader önbelleği"));
    SetWindowTextW(dither,T(L"Reduce color banding",L"Renk bantlaşmasını azalt"));
    SetWindowTextW(adaptive_triggers,T(L"DualSense adaptive triggers (Experimental)…",L"DualSense adaptif tetikler (Deneysel)…"));
    SetWindowTextW(keyboard,T(L"Enable keyboard",L"Klavyeyi etkinleştir"));
    SetWindowTextW(crash_diagnostics,T(L"Extended crash diagnostics",L"Gelişmiş çökme tanılaması"));
    FillCombo(lan_enabled,{T(L"Offline",L"Çevrimdışı"),L"LAN / Radmin",T(L"Online — community server (Experimental)",L"Çevrimiçi — topluluk sunucusu (Deneysel)")});
    SetWindowTextW(mute,T(L"Mute game audio",L"Oyun sesini kapat"));
    SetWindowTextW(save,T(L"Save",L"Kaydet"));
    SetWindowTextW(defaults,T(L"Defaults",L"Varsayılan"));
    SetWindowTextW(logs,T(L"Logs",L"Loglar"));SetWindowTextW(report,T(L"Report",L"Rapor"));
    SetWindowTextW(verify_files,verifying?T(L"Cancel check",L"Kontrolü iptal et"):T(L"Verify game files",L"Dosyaları doğrula"));
    SetWindowTextW(play,T(process?L"GAME RUNNING":L"PLAY",process?L"OYUN ÇALIŞIYOR":L"OYUNU BAŞLAT"));
    FillCombo(post_effect,{T(L"Off",L"Kapalı"),L"FXAA",L"FXAA Extreme"});
    FillCombo(backend,{L"SDL — Xbox / PlayStation / USB",T(L"XInput — Xbox-compatible",L"XInput — Xbox uyumlu")});
    FillCombo(prompt_style,{T(L"Automatic",L"Otomatik"),L"PlayStation 3",L"Xbox 360",T(L"Keyboard",L"Klavye")});
    FillCombo(effect,{T(L"Bilinear (default)",L"Bilinear (varsayılan)"),L"AMD FSR 1.0",L"AMD CAS",T(L"AMD FSR 2 (Experimental)",L"AMD FSR 2 (Deneysel)"),T(L"AMD FSR 3 (Experimental)",L"AMD FSR 3 (Deneysel)")});
    FillCombo(fsr_quality,{T(L"Auto",L"Otomatik"),T(L"Native AA",L"Yerel AA"),T(L"Quality",L"Kalite"),T(L"Balanced",L"Dengeli"),T(L"Performance",L"Performans"),T(L"Ultra Performance",L"Ultra Performans")});
    FillCombo(aspect,{T(L"Auto / 16:9",L"Otomatik / 16:9"),L"16:9",T(L"21:9 (Experimental)",L"21:9 (Deneysel)"),T(L"32:9 (Experimental)",L"32:9 (Deneysel)")});
    FillCombo(anisotropic,{T(L"Game default",L"Oyunun ayarı"),T(L"Off",L"Kapalı"),L"1×",L"2×",L"4×",L"8×",L"16×"});
    for(auto c:engine_choices)FillCombo(c,{T(L"Game default",L"Oyunun ayarı"),T(L"Off",L"Kapalı")});
    auto distance_index=SendMessageW(shadow_distance,CB_GETCURSEL,0,0);SendMessageW(shadow_distance,CB_RESETCONTENT,0,0);
    for(int distance:shadow_distances){auto text=distance?std::to_wstring(distance):T(L"Game default",L"Oyunun ayarı");SendMessageW(shadow_distance,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));}
    SendMessageW(shadow_distance,CB_SETCURSEL,distance_index<0?0:distance_index,0);
    FillRenderPresets();
    auto fps_selection=SendMessageW(fps_limit,CB_GETCURSEL,0,0);SendMessageW(fps_limit,CB_RESETCONTENT,0,0);
    for(auto rate:fps_limits){auto label=rate?std::to_wstring(rate)+L" FPS":T(L"No added limit",L"Ek sınır yok");SendMessageW(fps_limit,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}
    SendMessageW(fps_limit,CB_SETCURSEL,fps_selection<0?0:fps_selection,0);
    auto gpu=SendMessageW(adapter,CB_GETCURSEL,0,0);SendMessageW(adapter,CB_RESETCONTENT,0,0);
    SendMessageW(adapter,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(T(L"Automatic (recommended)",L"Otomatik (önerilen)")));
    for(const auto& item:adapters)SendMessageW(adapter,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(item.name.c_str()));
    SendMessageW(adapter,CB_SETCURSEL,gpu<0?0:gpu,0);
    auto selected=SendMessageW(size,CB_GETCURSEL,0,0);SendMessageW(size,CB_RESETCONTENT,0,0);
    for(auto [w,h]:dimensions){auto text=(w||h)?std::to_wstring(w)+L" × "+std::to_wstring(h):T(L"Automatic / desktop",L"Otomatik / masaüstü");
      SendMessageW(size,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));}
    SendMessageW(size,CB_SETCURSEL,selected<0?0:selected,0);
    SetWindowTextW(status,(turkish?status_tr:status_en).c_str());UpdateSharpness();RefreshRates();
    NetworkControls();RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);
  }
  bool CaptureKey(const MSG& message){
    if(!capturing || (message.message!=WM_KEYDOWN && message.message!=WM_SYSKEYDOWN))return false;
    auto vk=static_cast<UINT>(message.wParam);
    if(vk==VK_ESCAPE){CancelCapture();return true;}
    if(vk==VK_SHIFT || vk==VK_CONTROL || vk==VK_MENU)return true;
    auto key=KeyName(vk);if(key.empty())return true;
    std::string value;if(GetKeyState(VK_CONTROL)&0x8000)value+="Ctrl+";
    if(GetKeyState(VK_MENU)&0x8000)value+="Alt+";if(GetKeyState(VK_SHIFT)&0x8000)value+="Shift+";
    value+=key;CancelCapture();SetWindowTextW(binding_fields[binding_index],Wide(value).c_str());return true;
  }
  void SetControls(const Settings& s){
    SendMessageW(lan_enabled,CB_SETCURSEL,s.network_mode=="online"?2:s.network_mode=="lan"?1:0,0);
    SetWindowTextW(online_server,Wide(s.online_server).c_str());
    SetWindowTextW(lan_player,Wide(s.lan_player_name).c_str());SetWindowTextW(lan_room,Wide(s.lan_room_name).c_str());
    SetWindowTextW(lan_adapter,Wide(s.lan_bind_ip).c_str());SetWindowTextW(lan_host,Wide(s.lan_host_ip).c_str());
    NetworkControls();
    SendMessageW(aspect,CB_SETCURSEL,s.aspect=="21:9"?2:s.aspect=="32:9"?3:s.aspect=="16:9"?1:0,0);
    auto found=std::find(dimensions.begin(),dimensions.end(),std::pair{s.width,s.height});
    if(found==dimensions.end() && WindowSizeFits({s.width,s.height},display_maximum)){dimensions.emplace_back(s.width,s.height);Localize();found=dimensions.end()-1;}
    if(found==dimensions.end())found=dimensions.begin();
    SendMessageW(size,CB_SETCURSEL,std::distance(dimensions.begin(),found),0);
    for(size_t i=0;i<engine_choices.size();++i)SendMessageW(engine_choices[i],CB_SETCURSEL,s.engine_effects[i]=="off"?1:0,0);
    auto shadow=std::find(shadow_distances.begin(),shadow_distances.end(),s.shadow_distance);
    if(shadow==shadow_distances.end()){shadow_distances.push_back(s.shadow_distance);auto text=std::to_wstring(s.shadow_distance);SendMessageW(shadow_distance,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));shadow=shadow_distances.end()-1;}
    SendMessageW(shadow_distance,CB_SETCURSEL,std::distance(shadow_distances.begin(),shadow),0);
    const auto mode=s.display_mode=="auto"?(s.fullscreen?"borderless":"windowed"):s.display_mode;
    SendMessageW(fullscreen,CB_SETCURSEL,mode=="fullscreen"?0:mode=="windowed"?2:1,0);
    RefreshRates(s.refresh_rate);
    auto fps=std::find(fps_limits.begin(),fps_limits.end(),s.fps_limit);
    if(fps==fps_limits.end()){fps_limits.push_back(s.fps_limit);auto text=std::to_wstring(s.fps_limit)+L" FPS";SendMessageW(fps_limit,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));fps=fps_limits.end()-1;}
    SendMessageW(fps_limit,CB_SETCURSEL,std::distance(fps_limits.begin(),fps),0);
    int gpu_index=0;
    if(s.adapter!=-1){auto gpu=std::find_if(adapters.begin(),adapters.end(),[&](const Adapter& v){return v.index==s.adapter;});
      if(gpu==adapters.end()){adapters.push_back({s.adapter,L"DXGI adapter #"+std::to_wstring(s.adapter)});
        SendMessageW(adapter,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(adapters.back().name.c_str()));gpu=adapters.end()-1;}
      gpu_index=static_cast<int>(std::distance(adapters.begin(),gpu))+1;}
    SendMessageW(adapter,CB_SETCURSEL,gpu_index,0);
    SendMessageW(backend,CB_SETCURSEL,s.backend=="xinput"?1:0,0);
    SendMessageW(game_language,CB_SETCURSEL,s.game_language==4?1:s.game_language==3?2:s.game_language==5?3:s.game_language==6?4:s.game_language==1000?5:0,0);
    SendMessageW(prompt_style,CB_SETCURSEL,s.prompt_style=="playstation"?1:s.prompt_style=="xbox"?2:s.prompt_style=="keyboard"?3:0,0);
    auto preset=MatchingRenderPreset(s);FillRenderPresets(preset<0?static_cast<int>(kRenderPresets.size()):preset);
    SendMessageW(effect,CB_SETCURSEL,s.effect=="fsr"?1:s.effect=="cas"?2:s.effect=="fsr2"?3:s.effect=="fsr3"?4:0,0);
    constexpr std::array<const char*,6> qualities{"auto","nativeaa","quality","balanced","performance","ultra_performance"};
    auto quality=std::find(qualities.begin(),qualities.end(),s.fsr_quality);
    SendMessageW(fsr_quality,CB_SETCURSEL,quality==qualities.end()?0:std::distance(qualities.begin(),quality),0);
    SendMessageW(post_effect,CB_SETCURSEL,s.post_effect=="fxaa"?1:s.post_effect=="fxaa_extreme"?2:0,0);
    SendMessageW(anisotropic,CB_SETCURSEL,std::clamp(s.anisotropic,-1,5)+1,0);
    SendMessageW(volume,TBM_SETPOS,TRUE,std::clamp(s.volume,0,100));
    SetWindowTextW(volume_value,(std::to_wstring(std::clamp(s.volume,0,100))+L"%").c_str());
    SendMessageW(sharpness,TBM_SETPOS,TRUE,s.effect=="cas"?static_cast<int>((std::isfinite(s.cas_sharpness)?std::clamp(s.cas_sharpness,0.0,1.0):0.0)*100):static_cast<int>(100-(std::isfinite(s.fsr_reduction)?std::clamp(s.fsr_reduction,0.0,2.0):0.2)*50));
    for(auto [c,value]:{std::pair{vsync,s.vsync},{keyboard,s.keyboard},{mute,s.mute},
        {letterbox,s.letterbox},{dither,s.dither},{shader_cache,s.shader_cache},{msaa,s.msaa}})SetCheck(c,value);
    CancelCapture();filling_binding=true;edited_bindings=s.bindings;
    for(size_t i=0;i<kBindings.size();++i)SetWindowTextW(binding_fields[i],Wide(edited_bindings[i]).c_str());
    filling_binding=false;UpdateSharpness();
  }
  Settings GetControls()const{
    Settings s=settings;
    s.network_mode=SendMessageW(lan_enabled,CB_GETCURSEL,0,0)==2?"online":SendMessageW(lan_enabled,CB_GETCURSEL,0,0)==1?"lan":"offline";
    s.lan_enabled=s.network_mode=="lan";s.online_server=Utf8(Text(online_server));s.lan_player_name=Utf8(Text(lan_player));s.lan_room_name=Utf8(Text(lan_room));
    s.lan_bind_ip=Utf8(Text(lan_adapter));s.lan_host_ip=Utf8(Text(lan_host));
    const auto aspect_index=SendMessageW(aspect,CB_GETCURSEL,0,0);if(aspect_index<0 || aspect_index>3)throw std::runtime_error("Invalid aspect ratio selection");s.aspect=std::array{"auto","16:9","21:9","32:9"}[aspect_index];auto index=SendMessageW(size,CB_GETCURSEL,0,0);
    if(index<0 || static_cast<size_t>(index)>=dimensions.size())throw std::runtime_error("Invalid window size selection");
    std::tie(s.width,s.height)=dimensions[index];auto gpu=SendMessageW(adapter,CB_GETCURSEL,0,0);
    if(gpu<0 || gpu>static_cast<LRESULT>(adapters.size()))throw std::runtime_error("Invalid GPU selection");
    s.adapter=gpu==0?-1:adapters[gpu-1].index;s.backend=SendMessageW(backend,CB_GETCURSEL,0,0)==1?"xinput":"sdl";
    auto preset=SendMessageW(render_resolution,CB_GETCURSEL,0,0);
    if(preset<0)throw std::runtime_error("Invalid render resolution");
    if(preset<static_cast<LRESULT>(kRenderPresets.size()))ApplyRenderPreset(s,static_cast<int>(preset));
    auto mode=SendMessageW(effect,CB_GETCURSEL,0,0);s.effect=mode==1?"fsr":mode==2?"cas":mode==3?"fsr2":mode==4?"fsr3":"bilinear";
    constexpr std::array<const char*,6> qualities{"auto","nativeaa","quality","balanced","performance","ultra_performance"};
    auto quality=SendMessageW(fsr_quality,CB_GETCURSEL,0,0);if(quality<0 || quality>=6)throw std::runtime_error("Invalid FSR quality mode");s.fsr_quality=qualities[quality];
    s.anisotropic=SendMessageW(anisotropic,CB_GETCURSEL,0,0)-1;s.volume=SendMessageW(volume,TBM_GETPOS,0,0);
    if(mode==1 || mode==3 || mode==4)s.fsr_reduction=2.0-SendMessageW(sharpness,TBM_GETPOS,0,0)/50.0;
    if(mode==2)s.cas_sharpness=SendMessageW(sharpness,TBM_GETPOS,0,0)/100.0;
    auto display=SendMessageW(fullscreen,CB_GETCURSEL,0,0);
    if(display<0 || display>2)throw std::runtime_error("Invalid display mode");
    s.display_mode=display==0?"fullscreen":display==1?"borderless":"windowed";s.fullscreen=display!=2;
    const auto refresh=SendMessageW(refresh_rate,CB_GETCURSEL,0,0),fps=SendMessageW(fps_limit,CB_GETCURSEL,0,0);
    if(refresh<0 || refresh>=static_cast<LRESULT>(refresh_rates.size()) || fps<0 || fps>=static_cast<LRESULT>(fps_limits.size()))throw std::runtime_error("Invalid refresh rate or frame limit");
    s.refresh_rate=display==0?refresh_rates[refresh]:0;s.fps_limit=fps_limits[fps];
    s.vsync=Checked(vsync);s.keyboard=Checked(keyboard);s.mute=Checked(mute);
    s.letterbox=Checked(letterbox);s.dither=Checked(dither);s.msaa=Checked(msaa);s.shader_cache=Checked(shader_cache);
    auto aa=SendMessageW(post_effect,CB_GETCURSEL,0,0);s.post_effect=aa==1?"fxaa":aa==2?"fxaa_extreme":"none";
    const auto language_index=SendMessageW(game_language,CB_GETCURSEL,0,0);
    constexpr std::array<int,6> language_ids{1,4,3,5,6,1000};
    if(language_index<0 || language_index>=static_cast<LRESULT>(language_ids.size()))throw std::runtime_error("Invalid game language selection");
    s.game_language=language_ids[language_index];
    const auto glyph_mode=SendMessageW(prompt_style,CB_GETCURSEL,0,0);
    if(glyph_mode<0 || glyph_mode>3)throw std::runtime_error("Invalid prompt style selection");
    s.prompt_style=glyph_mode==1?"playstation":glyph_mode==2?"xbox":glyph_mode==3?"keyboard":"auto";
    for(size_t i=0;i<engine_choices.size();++i){auto choice=SendMessageW(engine_choices[i],CB_GETCURSEL,0,0);if(choice<0 || choice>1)throw std::runtime_error("Invalid engine effect selection");s.engine_effects[i]=choice==1?"off":"game";}
    auto distance_index=SendMessageW(shadow_distance,CB_GETCURSEL,0,0);if(distance_index<0 || distance_index>=static_cast<LRESULT>(shadow_distances.size()))throw std::runtime_error("Invalid shadow distance");s.shadow_distance=shadow_distances[distance_index];
    s.bindings=edited_bindings;s.Validate();return s;
  }
  void ReadyStatus(){try{Validate(exe,GameRoot());Status(L"Ready to play · Title update 0.0.1.1",L"Başlatmaya hazır · Title update 0.0.1.1",true);}
    catch(const Error& e){Status(e.english,e.turkish);}catch(const std::exception& e){Status(L"Check game folder: "+ErrorDetails(e),L"Oyun klasörünü kontrol et: "+ErrorDetails(e));}}
  #include "compact_layout.inc"
  COLORREF BackgroundFor(HWND)const{return kBackground;}
  void Rounded(HDC dc,RECT r,COLORREF fill,COLORREF border,int radius=8){
    auto brush=CreateSolidBrush(fill);auto pen=CreatePen(PS_SOLID,S(1),border);
    auto old_b=SelectObject(dc,brush),old_p=SelectObject(dc,pen);
    RoundRect(dc,r.left,r.top,r.right,r.bottom,S(radius),S(radius));SelectObject(dc,old_b);SelectObject(dc,old_p);DeleteObject(brush);DeleteObject(pen);
  }
  void DrawButton(const DRAWITEMSTRUCT& item){
    const auto c=item.hwndItem;RECT r=item.rcItem;
    bool disabled=(item.itemState&ODS_DISABLED)!=0,pressed=(item.itemState&ODS_SELECTED)!=0;
    bool is_check=checks.contains(c),primary=c==play,nav=std::find(navigation.begin(),navigation.end(),c)!=navigation.end();
    auto hdc=item.hDC;SetBkMode(hdc,TRANSPARENT);SelectObject(hdc,bold);
    if(is_check){
      auto brush=CreateSolidBrush(BackgroundFor(c));FillRect(hdc,&r,brush);DeleteObject(brush);
      RECT box{r.left+S(1),r.top+S(6),r.left+S(17),r.top+S(22)};
      Rounded(hdc,box,Checked(c)?kAccent:kField,(item.itemState&ODS_FOCUS)?kAccent:kBorder,4);
      if(Checked(c)){auto pen=CreatePen(PS_SOLID,S(2),kBackground);auto old=SelectObject(hdc,pen);
        MoveToEx(hdc,box.left+S(4),box.top+S(8),nullptr);LineTo(hdc,box.left+S(7),box.top+S(11));LineTo(hdc,box.left+S(12),box.top+S(5));SelectObject(hdc,old);DeleteObject(pen);}
      r.left+=S(29);SetTextColor(hdc,disabled?kMuted:kText);auto text=Text(c);DrawTextW(hdc,text.c_str(),-1,&r,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);return;
    }
    bool active=nav && c==navigation[current_page];
    if(nav){
      FillRect(hdc,&r,background);SetTextColor(hdc,active?kText:kMuted);auto text=Text(c);
      DrawTextW(hdc,text.c_str(),-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
      if(active){RECT line{r.left+S(4),r.bottom-S(2),r.right-S(4),r.bottom};auto b=CreateSolidBrush(kAccent);FillRect(hdc,&line,b);DeleteObject(b);}
      if(item.itemState&ODS_FOCUS){InflateRect(&r,-S(3),-S(3));DrawFocusRect(hdc,&r);}return;
    }
    auto fill=primary?(pressed?RGB(227,139,29):kAccent):active?RGB(44,36,30):pressed?RGB(51,55,65):nav?kSidebar:kField;
    if(disabled)fill=RGB(40,43,50);
    auto clear=CreateSolidBrush(nav?kSidebar:BackgroundFor(c));FillRect(hdc,&r,clear);DeleteObject(clear);
    Rounded(hdc,r,fill,(item.itemState&ODS_FOCUS)?kAccent:primary?fill:nav?fill:kBorder,10);
    if(active){RECT line{r.left,r.top+S(9),r.left+S(3),r.bottom-S(9)};auto b=CreateSolidBrush(kRed);FillRect(hdc,&line,b);DeleteObject(b);}
    SetTextColor(hdc,disabled?kMuted:primary?kBackground:active?kAccent:kText);
    auto text=Text(c);if(nav)r.left+=S(18);
    DrawTextW(hdc,text.c_str(),-1,&r,(nav?DT_LEFT:DT_CENTER)|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
  }
  void DrawCombo(const DRAWITEMSTRUCT& item){
    RECT r=item.rcItem;auto b=CreateSolidBrush((item.itemState&ODS_SELECTED)?RGB(57,47,35):kField);FillRect(item.hDC,&r,b);DeleteObject(b);
    SetBkMode(item.hDC,TRANSPARENT);SetTextColor(item.hDC,(item.itemState&ODS_DISABLED)?kMuted:kText);SelectObject(item.hDC,font);r.left+=S(8);
    std::wstring text;if(item.itemID!=UINT(-1)){auto len=SendMessageW(item.hwndItem,CB_GETLBTEXTLEN,item.itemID,0);
      if(len!=CB_ERR){text.resize(len+1);SendMessageW(item.hwndItem,CB_GETLBTEXT,item.itemID,reinterpret_cast<LPARAM>(text.data()));text.resize(len);}}
    DrawTextW(item.hDC,text.c_str(),-1,&r,DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
  }
  LRESULT DrawSlider(NMCUSTOMDRAW& draw){
    if(draw.dwDrawStage!=CDDS_PREPAINT)return CDRF_DODEFAULT;
    auto c=draw.hdr.hwndFrom;RECT r;GetClientRect(c,&r);auto b=CreateSolidBrush(kBackground);FillRect(draw.hdc,&r,b);DeleteObject(b);
    RECT channel{},thumb{};SendMessageW(c,TBM_GETCHANNELRECT,0,reinterpret_cast<LPARAM>(&channel));SendMessageW(c,TBM_GETTHUMBRECT,0,reinterpret_cast<LPARAM>(&thumb));
    const int cy=(r.top+r.bottom)/2,cx=(thumb.left+thumb.right)/2;
    RECT track{channel.left,cy-S(2),channel.right,cy+S(2)};Rounded(draw.hdc,track,kBorder,kBorder,4);
    track.right=cx;Rounded(draw.hdc,track,IsWindowEnabled(c)?kAccent:kMuted,IsWindowEnabled(c)?kAccent:kMuted,4);
    auto brush=CreateSolidBrush(IsWindowEnabled(c)?kAccent:kMuted);auto old=SelectObject(draw.hdc,brush),pen=SelectObject(draw.hdc,GetStockObject(NULL_PEN));
    Ellipse(draw.hdc,cx-S(6),cy-S(6),cx+S(6),cy+S(6));SelectObject(draw.hdc,old);SelectObject(draw.hdc,pen);DeleteObject(brush);
    if(GetFocus()==c){RECT focus=r;InflateRect(&focus,-S(2),-S(2));DrawFocusRect(draw.hdc,&focus);}return CDRF_SKIPDEFAULT;
  }
  void Paint(HDC target){
    RECT client;GetClientRect(window,&client);auto dc=CreateCompatibleDC(target);
    auto bitmap=CreateCompatibleBitmap(target,std::max<LONG>(1,client.right),std::max<LONG>(1,client.bottom));auto old=SelectObject(dc,bitmap);
    DrawBackground(dc);SetBkMode(dc,TRANSPARENT);SelectObject(dc,logo);
    SetTextColor(dc,kText);RECT r{S(28),S(3+kCaptionHeight),S(194),S(50+kCaptionHeight)};
    DrawTextW(dc,L"MotoGP",-1,&r,DT_LEFT|DT_SINGLELINE|DT_NOPREFIX);
    SetTextColor(dc,kAccent);r={S(194),S(3+kCaptionHeight),S(328),S(50+kCaptionHeight)};DrawTextW(dc,L"09/10",-1,&r,DT_LEFT|DT_SINGLELINE|DT_NOPREFIX);
    SelectObject(dc,small_font);SetTextColor(dc,kMuted);r={S(30),S(55+kCaptionHeight),S(520),S(76+kCaptionHeight)};
    DrawTextW(dc,T(L"P C   R E C O M P I L A T I O N",L"P C   Y E N İ D E N   D E R L E M E"),-1,&r,DT_LEFT|DT_SINGLELINE|DT_NOPREFIX);
    const auto pen=CreatePen(PS_SOLID,S(1),kBorder);auto old_pen=SelectObject(dc,pen);
    MoveToEx(dc,S(28),S(129+kCaptionHeight),nullptr);LineTo(dc,S(548),S(129+kCaptionHeight));SelectObject(dc,old_pen);DeleteObject(pen);
    const auto dot=CreateSolidBrush(status_positive?RGB(80,198,112):kMuted);auto old_brush=SelectObject(dc,dot);auto old_dot_pen=SelectObject(dc,GetStockObject(NULL_PEN));
    Ellipse(dc,S(28),S(475+kCaptionHeight),S(35),S(482+kCaptionHeight));SelectObject(dc,old_brush);SelectObject(dc,old_dot_pen);DeleteObject(dot);
    DrawCaption(dc);BitBlt(target,0,0,client.right,client.bottom,dc,0,0,SRCCOPY);SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);
  }
  void Rescale(double value){
    DeleteFonts();scale=value;CreateFonts();
    for(auto& p:placements){MoveWindow(p.control,S(p.x),S(p.y+(GetParent(p.control)==page_view?0:kCaptionHeight)),S(p.w),S(p.h),TRUE);SendMessageW(p.control,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);
      wchar_t cls[32];GetClassNameW(p.control,cls,32);if(std::wstring_view(cls)==L"COMBOBOX"){SendMessageW(p.control,CB_SETITEMHEIGHT,0,S(22));SendMessageW(p.control,CB_SETITEMHEIGHT,WPARAM(-1),S(22));}}
    ScrollPage(page_scroll[current_page]);
    SendMessageW(title,WM_SETFONT,reinterpret_cast<WPARAM>(heading),TRUE);
    for(auto c:{folder_label,status})SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(small_font),TRUE);
    RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);
  }
  void BindingMenu(HWND control,POINT point){
    auto it=std::find(binding_fields.begin(),binding_fields.end(),control);if(it==binding_fields.end())return;
    binding_index=static_cast<int>(std::distance(binding_fields.begin(),it));CancelCapture();
    if(point.x==-1 && point.y==-1){RECT r;GetWindowRect(control,&r);point={r.left,r.bottom};}
    auto menu=CreatePopupMenu();AppendMenuW(menu,MF_STRING,kCapture,T(L"Capture key…",L"Tuş yakala…"));
    AppendMenuW(menu,MF_STRING,kClearBinding,T(L"Unbind",L"Tuşu kaldır"));AppendMenuW(menu,MF_STRING,kResetBinding,T(L"Restore action default",L"Eylemi varsayılana döndür"));
    auto command=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,point.x,point.y,0,window,nullptr);DestroyMenu(menu);
    if(command)Command(command,BN_CLICKED);
  }
  void SaveSettings() {
    settings=GetControls(); settings.Store(config.table);
    Put(config.table,"game_data_root",Utf8(GameRoot().wstring()));
    config.Save();
    Put(preferences.table,"crash_monitor",Checked(crash_diagnostics));preferences.Save();
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
    fs::path native_crash;fs::file_time_type crash_time=fs::file_time_type::min();
    for(const auto& entry:fs::directory_iterator(exe/L"logs")){
      if(!entry.is_directory() || !entry.path().filename().wstring().starts_with(L"crash-") || !fs::is_regular_file(entry.path()/L"report.txt"))continue;
      const auto modified=entry.last_write_time();if(modified>=crash_time){crash_time=modified;native_crash=entry.path();}
    }
    if(!native_crash.empty()){fs::copy(native_crash,folder/L"native-crash",fs::copy_options::recursive);output<<"Native crash evidence: native-crash/\n";}
    output.close(); if(!output) throw std::runtime_error("Could not write report");
    Status(L"Report created: "+folder.wstring(),L"Rapor oluşturuldu: "+folder.wstring());
    OpenFolder(folder);
  }
  void Running(bool value) {
    CancelCapture();
    for(auto control:{path,browse,save,defaults,play}) EnableWindow(control,!value);
    for(size_t i=0;i<pages.size();++i) for(auto control:pages[i]) EnableWindow(control,i==5||!value);
    Localize();
  }
  void Command(int id,int event){
    try{
      if(id>=kBindingBase && id<kBindingBase+static_cast<int>(kBindings.size()) && event==EN_CHANGE && !filling_binding){
        edited_bindings[id-kBindingBase]=Utf8(Text(binding_fields[id-kBindingBase]));
      }else if(id==kAchievementSearch && event==EN_CHANGE){FilterAchievements();
      }else if(id==kAchievementFilter && event==CBN_SELCHANGE){FilterAchievements();
      }else if(id==kLanEnabled && event==CBN_SELCHANGE){NetworkControls();
      }else if(id==kLanguage && event==CBN_SELCHANGE){
        turkish=SendMessageW(language,CB_GETCURSEL,0,0)==1;Localize();Put(preferences.table,"language",std::string(turkish?"tr":"en"));preferences.Save();
      }else if((id==kFullscreen || id==kSize) && event==CBN_SELCHANGE){RefreshRates();
      }else if(id==kEffect && event==CBN_SELCHANGE){auto mode=SendMessageW(effect,CB_GETCURSEL,0,0);
        SendMessageW(sharpness,TBM_SETPOS,TRUE,mode==2?static_cast<int>(settings.cas_sharpness*100):static_cast<int>(100-settings.fsr_reduction*50));UpdateSharpness();
      }else if(id==kPath && event==EN_CHANGE){Status(L"Select your game folder, then press PLAY.",L"Oyun klasörünü seç ve OYUNU BAŞLAT'a bas.");
      }else if(event==BN_CLICKED){
        auto c=FindControl(id);if(checks.contains(c)){if(id==kAchievementPopup||id==kAchievementSound){SaveAchievementOption(id);return;}SetCheck(c,!Checked(c));return;}
        if(id>=kNavGraphics && id<=kNavAchievements){ShowPage(id-kNavGraphics);return;}
        if(id>=kCaptureBase && id<kCaptureBase+static_cast<int>(kBindings.size())){binding_index=id-kCaptureBase;id=kCapture;}
        switch(id){
          case kAdaptiveTriggers:AdaptiveDialog(window,font,turkish,scale,settings.adaptive).Show();break;
          case kCapture:CancelCapture();capturing=true;SetFocus(binding_buttons[binding_index]);SetWindowTextW(binding_buttons[binding_index],L"?");
            Status(L"Press a key or chord. Escape cancels.",L"Bir tuşa veya tuş birleşimine bas. Escape iptal eder.");break;
          case kClearBinding:SetWindowTextW(binding_fields[binding_index],L"");break;
          case kResetBinding:SetWindowTextW(binding_fields[binding_index],Wide(kBindings[binding_index].defaults).c_str());break;
          case kVerifyFiles:StartVerification();break;
          case kBrowse:Browse();break;case kSave:SaveSettings();break;
          case kDefaults:settings=Settings{};SetControls(settings);Status(L"Defaults selected. Save or press PLAY to apply.",L"Varsayılanlar seçildi. Kaydet veya oyunu başlat.");break;
          case kLogs:OpenFolder(exe/L"logs");break;case kReport:CreateReport();break;
          case kPlay:if(process || verifying)break;Validate(exe,GameRoot());SaveSettings();process=Start(exe,Arguments(GameRoot(),settings,config.table),Checked(crash_diagnostics));Running(true);SetTimer(window,1,700,nullptr);
            Status(L"Game running. Closing the launcher leaves the game open.",L"Oyun çalışıyor. Launcher'ı kapatman oyunu kapatmaz.");break;
        }
      }
    }catch(const std::exception& e){ErrorBox(e);}
  }
  void Poll(){
    if(!process || WaitForSingleObject(process,0)!=WAIT_OBJECT_0)return;
    DWORD code=0;GetExitCodeProcess(process,&code);CloseHandle(process);process=nullptr;KillTimer(window,1);Running(false);
    // Controls were disabled while the game ran. Re-read live settings saved
    // through F1, so PLAY cannot silently restore the old launcher's snapshot.
    try{Config updated(config.path);auto loaded=Settings::Load(updated.table);loaded.Validate();config=std::move(updated);settings=std::move(loaded);SetControls(settings);}
    catch(const std::exception& e){ErrorBox(e);return;}
    if(code)Status(L"Game exited (code "+std::to_wstring(code)+L"). Open logs for the crash report.",L"Oyun kapandı (kod "+std::to_wstring(code)+L"). Çökme raporu için logları aç.");
    else Status(L"Game closed. You can launch again.",L"Oyun kapandı. Tekrar başlatabilirsin.");
  }
};
LRESULT CALLBACK WindowProc(HWND window,UINT message,WPARAM wparam,LPARAM lparam){
  auto* app=reinterpret_cast<App*>(GetWindowLongPtrW(window,GWLP_USERDATA));
  if(message==WM_NCCREATE){app=static_cast<App*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);app->window=window;SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(app));}
  if(!app)return DefWindowProcW(window,message,wparam,lparam);
  switch(message){
    case WM_CREATE:try{app->BuildControls();app->SetupCaption();}catch(const std::exception& e){app->ErrorBox(e);return -1;}return 0;
    case WM_SETTEXT:case WM_SETICON:{const auto result=DefWindowProcW(window,message,wparam,lparam);app->PaintCaption();return result;}
    case WM_NCHITTEST:{POINT p{static_cast<short>(LOWORD(lparam)),static_cast<short>(HIWORD(lparam))};if(const auto hit=app->CaptionHit(p))return hit;break;}
    case WM_NCMOUSEMOVE:{app->CaptionMove({static_cast<short>(LOWORD(lparam)),static_cast<short>(HIWORD(lparam))});TRACKMOUSEEVENT track{sizeof(track),TME_LEAVE|TME_NONCLIENT,window,0};TrackMouseEvent(&track);return 0;}
    case WM_NCMOUSELEAVE:app->caption_hover=0;app->PaintCaption();return 0;
    case WM_NCLBUTTONDOWN:if(wparam==HTMINBUTTON||wparam==HTCLOSE){app->caption_pressed=static_cast<int>(wparam);SetCapture(window);app->PaintCaption();return 0;}break;
    case WM_NCLBUTTONDBLCLK:if(wparam==HTCAPTION||wparam==HTMAXBUTTON)return 0;break;
    case WM_MOUSEMOVE:if(app->caption_pressed){POINT p;GetCursorPos(&p);app->CaptionMove(p);return 0;}break;
    case WM_LBUTTONUP:if(app->caption_pressed){POINT p;GetCursorPos(&p);app->CaptionRelease(p);return 0;}break;
    case WM_CAPTURECHANGED:app->caption_pressed=0;app->PaintCaption();return 0;
    case WM_SYSCOMMAND:if((wparam&0xFFF0)==SC_MAXIMIZE)return 0;break;
    case WM_COMMAND:app->Command(LOWORD(wparam),HIWORD(wparam));return 0;
    case WM_CONTEXTMENU:app->BindingMenu(reinterpret_cast<HWND>(wparam),{static_cast<short>(LOWORD(lparam)),static_cast<short>(HIWORD(lparam))});return 0;
    case WM_TIMER:if(wparam==2)app->PollAchievements();else if(wparam==3)app->PollVerification();else app->Poll();return 0;
    case WM_DRAWITEM:{auto& draw=*reinterpret_cast<DRAWITEMSTRUCT*>(lparam);if(draw.CtlType==ODT_BUTTON)app->DrawButton(draw);else if(draw.CtlType==ODT_COMBOBOX)app->DrawCombo(draw);return TRUE;}
    case WM_MEASUREITEM:reinterpret_cast<MEASUREITEMSTRUCT*>(lparam)->itemHeight=app->S(22);return TRUE;
    case WM_NOTIFY:{auto* n=reinterpret_cast<NMHDR*>(lparam);if(n->code==NM_CUSTOMDRAW && (n->hwndFrom==app->sharpness || n->hwndFrom==app->volume))return app->DrawSlider(*reinterpret_cast<NMCUSTOMDRAW*>(lparam));return 0;}
    case WM_VSCROLL:if(app->achievement_list && (!lparam||reinterpret_cast<HWND>(lparam)==app->achievement_list))return SendMessageW(app->achievement_list,WM_VSCROLL,wparam,lparam);break;
    case WM_MOUSEWHEEL:return SendMessageW(app->page_view,message,wparam,lparam);
    case WM_HSCROLL:{if(reinterpret_cast<HWND>(lparam)==app->volume)SetWindowTextW(app->volume_value,(std::to_wstring(SendMessageW(app->volume,TBM_GETPOS,0,0))+L"%").c_str());
      if(reinterpret_cast<HWND>(lparam)==app->sharpness){auto value=SendMessageW(app->sharpness,TBM_GETPOS,0,0),mode=SendMessageW(app->effect,CB_GETCURSEL,0,0);
        if(mode==1)app->settings.fsr_reduction=2.0-value/50.0;if(mode==2)app->settings.cas_sharpness=value/100.0;app->UpdateSharpness();}return 0;}
    case WM_CTLCOLOREDIT:case WM_CTLCOLORLISTBOX:SetTextColor(reinterpret_cast<HDC>(wparam),kText);SetBkColor(reinterpret_cast<HDC>(wparam),kField);return reinterpret_cast<LRESULT>(app->field);
    case WM_CTLCOLORSTATIC:case WM_CTLCOLORBTN:{auto c=reinterpret_cast<HWND>(lparam);auto bg=app->BackgroundFor(c);SetTextColor(reinterpret_cast<HDC>(wparam),c==app->title?kText:c==app->status&&app->status_positive?RGB(107,205,132):kMuted);
      SetBkColor(reinterpret_cast<HDC>(wparam),bg);return reinterpret_cast<LRESULT>(bg==kCard?app->card:app->background);}
    case WM_ERASEBKGND:return 1;
    case WM_PAINT:{PAINTSTRUCT paint;auto dc=BeginPaint(window,&paint);app->Paint(dc);EndPaint(window,&paint);return 0;}
    case WM_DPICHANGED:{auto* proposed=reinterpret_cast<RECT*>(lparam);
      MONITORINFO monitor{sizeof(monitor)};GetMonitorInfoW(MonitorFromRect(proposed,MONITOR_DEFAULTTONEAREST),&monitor);
      const auto& work=monitor.rcWork;
      app->Rescale(std::max(.55,std::min({HIWORD(wparam)/96.0,(work.right-work.left-40)/double(kWidth),(work.bottom-work.top-50)/double(kHeight+kCaptionHeight)})));
      RECT r{0,0,app->S(kWidth),app->S(kHeight+kCaptionHeight)};AdjustWindowRectExForDpi(&r,static_cast<DWORD>(GetWindowLongPtrW(window,GWL_STYLE)),FALSE,0,HIWORD(wparam));
      const int width=r.right-r.left,height=r.bottom-r.top;
      SetWindowPos(window,nullptr,std::clamp<int>(proposed->left,work.left,std::max<int>(work.left,work.right-width)),
          std::clamp<int>(proposed->top,work.top,std::max<int>(work.top,work.bottom-height)),width,height,SWP_NOZORDER|SWP_NOACTIVATE);return 0;}
    case WM_CLOSE:app->verify_cancel=true;DestroyWindow(window);return 0;
    case WM_DESTROY:PostQuitMessage(0);return 0;
  }return DefWindowProcW(window,message,wparam,lparam);
}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int show){
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
  Gdiplus::GdiplusStartupInput gdip;ULONG_PTR gdip_token{};Gdiplus::GdiplusStartup(&gdip_token,&gdip,nullptr);
  INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_STANDARD_CLASSES|ICC_BAR_CLASSES};InitCommonControlsEx(&controls);
  int result=0;
  try{
    App app;auto mutex_name=L"Local\\MotoGP0910Launcher-"+std::to_wstring(std::hash<std::wstring>{}(app.exe.wstring()));
    HANDLE mutex=CreateMutexW(nullptr,FALSE,mutex_name.c_str());if(!mutex)throw std::runtime_error("Cannot initialize launcher lock");
    if(GetLastError()==ERROR_ALREADY_EXISTS){CloseHandle(mutex);MessageBoxW(nullptr,app.T(L"The launcher is already open.",L"Launcher zaten açık."),L"MotoGP 09/10",MB_OK);Gdiplus::GdiplusShutdown(gdip_token);CoUninitialize();return 0;}
    RECT work{};SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);
    app.scale=std::max(0.55,std::min({GetDpiForSystem()/96.0,(work.right-work.left-40)/double(kWidth),(work.bottom-work.top-50)/double(kHeight+kCaptionHeight)}));
    WNDCLASSW cls{};cls.lpfnWndProc=WindowProc;cls.hInstance=instance;cls.lpszClassName=L"MotoGP0910Launcher";cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.hIcon=LoadIconW(instance,MAKEINTRESOURCEW(motogp::kIconResource));RegisterClassW(&cls);
    RECT size{0,0,app.S(kWidth),app.S(kHeight+kCaptionHeight)};constexpr DWORD style=WS_POPUP|WS_SYSMENU|WS_MINIMIZEBOX;
    AdjustWindowRectEx(&size,style,FALSE,0);
    auto window=CreateWindowExW(0,cls.lpszClassName,L"MotoGP 09/10 Launcher",style,(work.right-size.right+size.left)/2,(work.bottom-size.bottom+size.top)/2,
        size.right-size.left,size.bottom-size.top,nullptr,nullptr,instance,&app);
    if(!window){CloseHandle(mutex);throw std::runtime_error("Cannot create launcher window");}
    motogp::ApplyWindowIcon(window);
    BOOL dark=TRUE;DwmSetWindowAttribute(window,20,&dark,sizeof(dark));
    ShowWindow(window,show);UpdateWindow(window);MSG message;
    while(GetMessageW(&message,nullptr,0,0)>0){if(app.CaptureKey(message))continue;if(!IsDialogMessageW(window,&message)){TranslateMessage(&message);DispatchMessageW(&message);}}
    CloseHandle(mutex);
  }catch(const std::exception& e){MessageBoxW(nullptr,(L"Cannot open launcher / Launcher açılamadı:\n"+ErrorDetails(e)).c_str(),L"MotoGP 09/10",MB_OK|MB_ICONERROR);result=1;}
  Gdiplus::GdiplusShutdown(gdip_token);CoUninitialize();return result;
}
