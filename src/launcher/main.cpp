#include "core.h"
#include "hardware.h"
#include "ui_ids.h"
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
constexpr int kWidth=1120,kHeight=860;
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
       anisotropic{},letterbox{},dither{},volume{},volume_value{},tooltip{};
  std::array<HWND,3> navigation{};
  std::array<HWND,kBindings.size()> binding_fields{},binding_buttons{};
  struct Label {HWND control;const wchar_t* en;const wchar_t* tr;};
  struct Placement {HWND control;int x,y,w,h;};
  std::vector<Label> labels;
  std::vector<Placement> placements;
  std::array<std::vector<HWND>,3> pages;
  std::map<HWND,bool> checks;
  std::array<std::string,kBindings.size()> edited_bindings=settings.bindings;
  std::vector<Adapter> adapters=HardwareAdapters();
  std::vector<std::pair<int,int>> dimensions{{0,0},{1280,720},{1600,900},{1920,1080},{2560,1440},{3840,2160}};
  int current_page=0,binding_index=0;
  bool filling_binding=false,capturing=false;
  HFONT font{},heading{},small_font{},bold{},logo{};
  HBRUSH background=CreateSolidBrush(kBackground),card=CreateSolidBrush(kCard),field=CreateSolidBrush(kField);
  HANDLE process{};
  double scale=1;
  std::wstring status_en,status_tr;
  ~App(){if(process)CloseHandle(process);DeleteFonts();DeleteObject(background);DeleteObject(card);DeleteObject(field);}
  const wchar_t* T(const wchar_t* en,const wchar_t* tr)const{return turkish?tr:en;}
  int S(int value)const{return static_cast<int>(value*scale+0.5);}
  void DeleteFonts(){for(auto f:{font,heading,small_font,bold,logo})if(f)DeleteObject(f);}
  void CreateFonts(){
    auto make=[&](int size,int weight){return CreateFontW(-S(size),0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");};
    font=make(14,FW_NORMAL);heading=make(34,FW_BOLD);small_font=make(12,FW_NORMAL);bold=make(14,FW_SEMIBOLD);logo=make(31,FW_BOLD);
  }
  HWND Control(const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int w,int h,int id=0){
    bool button=std::wstring_view(cls)==L"BUTTON";
    if(std::wstring_view(cls)==L"STATIC")style|=SS_NOPREFIX;
    bool checkbox=button && (style&BS_TYPEMASK)==BS_AUTOCHECKBOX;
    if(button)style=(style&~BS_TYPEMASK)|BS_OWNERDRAW;
    if(std::wstring_view(cls)==L"COMBOBOX")style|=CBS_OWNERDRAWFIXED|CBS_HASSTRINGS;
    auto c=CreateWindowExW(0,cls,text,WS_CHILD|WS_VISIBLE|style,S(x),S(y),S(w),S(h),window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
    if(!c)throw std::runtime_error("Cannot create launcher control");
    placements.push_back({c,x,y,w,h});
    if(checkbox){checks.emplace(c,false);SetWindowSubclass(c,CheckProc,1,reinterpret_cast<DWORD_PTR>(this));}
    SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);
    if(std::wstring_view(cls)==L"EDIT")SetWindowTheme(c,L"",L"");
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
    auto c=Control(cls,text,style,x,y,w,h,id);pages[page].push_back(c);return c;
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
  void Status(std::wstring en,std::wstring tr){
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
  void CancelCapture(){
    capturing=false;for(auto c:binding_buttons)if(c)SetWindowTextW(c,L"…");
  }
  void ShowPage(int page){
    CancelCapture();current_page=page;
    for(int i=0;i<3;++i)for(auto c:pages[i])ShowWindow(c,i==page?SW_SHOW:SW_HIDE);
    Localize();InvalidateRect(window,nullptr,FALSE);
  }
  void UpdateSharpness(){
    EnableWindow(sharpness,!process && SendMessageW(effect,CB_GETCURSEL,0,0)!=0);
    SetWindowTextW(sharp_value,(std::to_wstring(SendMessageW(sharpness,TBM_GETPOS,0,0))+L"%").c_str());
  }
  void FillRenderPresets(int selection=-1){
    if(selection<0)selection=static_cast<int>(SendMessageW(render_resolution,CB_GETCURSEL,0,0));
    SendMessageW(render_resolution,CB_RESETCONTENT,0,0);
    const std::array<std::wstring,3> names{
      T(L"720p — 1280 × 720 (native)",L"720p — 1280 × 720 (yerel)"),
      L"2K / 1440p — 2560 × 1440",L"4K / 2160p — 3840 × 2160"};
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
    SetWindowTextW(title,current_page==0?T(L"Graphics",L"Grafik"):current_page==1?T(L"Audio",L"Ses"):T(L"Controls",L"Kontroller"));
    SetWindowTextW(subtitle,current_page==0?T(L"Display, detail and upscaling.",L"Görüntü, detay ve ölçekleme."):
        current_page==1?T(L"Set the sound of your race.",L"Yarışın sesini ayarla."):T(L"Your controller. Your keys.",L"Senin kontrolcün. Senin tuşların."));
    SetWindowTextW(folder_label,T(L"GAME FOLDER",L"OYUN KLASÖRÜ"));SetWindowTextW(browse,T(L"Browse",L"Gözat"));
    for(auto& label:labels)SetWindowTextW(label.control,T(label.en,label.tr));
    for(int i=0;i<3;++i)SetWindowTextW(navigation[i],i==0?T(L"Graphics",L"Grafik"):i==1?T(L"Audio",L"Ses"):T(L"Controls",L"Kontroller"));
    SetWindowTextW(fullscreen,T(L"Fullscreen",L"Tam ekran"));SetWindowTextW(vsync,L"VSync");
    SetWindowTextW(letterbox,T(L"Keep aspect ratio",L"En-boy oranını koru"));
    SetWindowTextW(msaa,T(L"Native 2× MSAA",L"Yerel 2× MSAA"));
    SetWindowTextW(shader_cache,T(L"Shader cache",L"Shader önbelleği"));
    SetWindowTextW(dither,T(L"Reduce color banding",L"Renk bantlaşmasını azalt"));
    SetWindowTextW(keyboard,T(L"Enable keyboard",L"Klavyeyi etkinleştir"));
    SetWindowTextW(mute,T(L"Mute game audio",L"Oyun sesini kapat"));
    SetWindowTextW(save,T(L"Save settings",L"Ayarları kaydet"));
    SetWindowTextW(defaults,T(L"Restore defaults",L"Varsayılanlar"));
    SetWindowTextW(logs,T(L"Open logs",L"Log klasörü"));SetWindowTextW(report,T(L"Error report",L"Hata raporu"));
    SetWindowTextW(play,T(process?L"GAME RUNNING":L"PLAY",process?L"OYUN ÇALIŞIYOR":L"OYUNU BAŞLAT"));
    FillCombo(post_effect,{T(L"Off",L"Kapalı"),L"FXAA",L"FXAA Extreme"});
    FillCombo(backend,{L"SDL — Xbox / PlayStation / USB",T(L"XInput — Xbox-compatible",L"XInput — Xbox uyumlu")});
    FillCombo(effect,{T(L"Bilinear (default)",L"Bilinear (varsayılan)"),L"AMD FSR 1.0",L"AMD CAS"});
    FillCombo(anisotropic,{T(L"Game default",L"Oyunun ayarı"),T(L"Off",L"Kapalı"),L"1×",L"2×",L"4×",L"8×",L"16×"});
    FillRenderPresets();
    auto gpu=SendMessageW(adapter,CB_GETCURSEL,0,0);SendMessageW(adapter,CB_RESETCONTENT,0,0);
    SendMessageW(adapter,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(T(L"Automatic (recommended)",L"Otomatik (önerilen)")));
    for(const auto& item:adapters)SendMessageW(adapter,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(item.name.c_str()));
    SendMessageW(adapter,CB_SETCURSEL,gpu<0?0:gpu,0);
    auto selected=SendMessageW(size,CB_GETCURSEL,0,0);SendMessageW(size,CB_RESETCONTENT,0,0);
    for(auto [w,h]:dimensions){auto text=(w||h)?std::to_wstring(w)+L" × "+std::to_wstring(h):T(L"Automatic / desktop",L"Otomatik / masaüstü");
      SendMessageW(size,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));}
    SendMessageW(size,CB_SETCURSEL,selected<0?0:selected,0);
    SetWindowTextW(status,(turkish?status_tr:status_en).c_str());UpdateSharpness();
    RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);
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
    auto found=std::find(dimensions.begin(),dimensions.end(),std::pair{s.width,s.height});
    if(found==dimensions.end()){dimensions.emplace_back(s.width,s.height);Localize();found=dimensions.end()-1;}
    SendMessageW(size,CB_SETCURSEL,std::distance(dimensions.begin(),found),0);
    int gpu_index=0;
    if(s.adapter!=-1){auto gpu=std::find_if(adapters.begin(),adapters.end(),[&](const Adapter& v){return v.index==s.adapter;});
      if(gpu==adapters.end()){adapters.push_back({s.adapter,L"DXGI adapter #"+std::to_wstring(s.adapter)});
        SendMessageW(adapter,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(adapters.back().name.c_str()));gpu=adapters.end()-1;}
      gpu_index=static_cast<int>(std::distance(adapters.begin(),gpu))+1;}
    SendMessageW(adapter,CB_SETCURSEL,gpu_index,0);
    SendMessageW(backend,CB_SETCURSEL,s.backend=="xinput"?1:0,0);
    auto preset=MatchingRenderPreset(s);FillRenderPresets(preset<0?static_cast<int>(kRenderPresets.size()):preset);
    SendMessageW(effect,CB_SETCURSEL,s.effect=="fsr"?1:s.effect=="cas"?2:0,0);
    SendMessageW(post_effect,CB_SETCURSEL,s.post_effect=="fxaa"?1:s.post_effect=="fxaa_extreme"?2:0,0);
    SendMessageW(anisotropic,CB_SETCURSEL,std::clamp(s.anisotropic,-1,5)+1,0);
    SendMessageW(volume,TBM_SETPOS,TRUE,std::clamp(s.volume,0,100));
    SetWindowTextW(volume_value,(std::to_wstring(std::clamp(s.volume,0,100))+L"%").c_str());
    SendMessageW(sharpness,TBM_SETPOS,TRUE,s.effect=="cas"?static_cast<int>((std::isfinite(s.cas_sharpness)?std::clamp(s.cas_sharpness,0.0,1.0):0.0)*100):static_cast<int>(100-(std::isfinite(s.fsr_reduction)?std::clamp(s.fsr_reduction,0.0,2.0):0.2)*50));
    for(auto [c,value]:{std::pair{fullscreen,s.fullscreen},{vsync,s.vsync},{keyboard,s.keyboard},{mute,s.mute},
        {letterbox,s.letterbox},{dither,s.dither},{shader_cache,s.shader_cache},{msaa,s.msaa}})SetCheck(c,value);
    CancelCapture();filling_binding=true;edited_bindings=s.bindings;
    for(size_t i=0;i<kBindings.size();++i)SetWindowTextW(binding_fields[i],Wide(edited_bindings[i]).c_str());
    filling_binding=false;UpdateSharpness();
  }
  Settings GetControls()const{
    Settings s=settings;auto index=SendMessageW(size,CB_GETCURSEL,0,0);
    if(index<0 || static_cast<size_t>(index)>=dimensions.size())throw std::runtime_error("Invalid window size selection");
    std::tie(s.width,s.height)=dimensions[index];auto gpu=SendMessageW(adapter,CB_GETCURSEL,0,0);
    if(gpu<0 || gpu>static_cast<LRESULT>(adapters.size()))throw std::runtime_error("Invalid GPU selection");
    s.adapter=gpu==0?-1:adapters[gpu-1].index;s.backend=SendMessageW(backend,CB_GETCURSEL,0,0)==1?"xinput":"sdl";
    auto preset=SendMessageW(render_resolution,CB_GETCURSEL,0,0);
    if(preset<0)throw std::runtime_error("Invalid render resolution");
    if(preset<static_cast<LRESULT>(kRenderPresets.size()))ApplyRenderPreset(s,static_cast<int>(preset));
    auto mode=SendMessageW(effect,CB_GETCURSEL,0,0);s.effect=mode==1?"fsr":mode==2?"cas":"bilinear";
    s.anisotropic=SendMessageW(anisotropic,CB_GETCURSEL,0,0)-1;s.volume=SendMessageW(volume,TBM_GETPOS,0,0);
    if(mode==1)s.fsr_reduction=2.0-SendMessageW(sharpness,TBM_GETPOS,0,0)/50.0;
    if(mode==2)s.cas_sharpness=SendMessageW(sharpness,TBM_GETPOS,0,0)/100.0;
    s.fullscreen=Checked(fullscreen);s.vsync=Checked(vsync);s.keyboard=Checked(keyboard);s.mute=Checked(mute);
    s.letterbox=Checked(letterbox);s.dither=Checked(dither);s.msaa=Checked(msaa);s.shader_cache=Checked(shader_cache);
    auto aa=SendMessageW(post_effect,CB_GETCURSEL,0,0);s.post_effect=aa==1?"fxaa":aa==2?"fxaa_extreme":"none";
    s.bindings=edited_bindings;s.Validate();return s;
  }
  void ReadyStatus(){try{Validate(exe,GameRoot());Status(L"Ready to play · Title update 0.0.1.1",L"Başlatmaya hazır · Title update 0.0.1.1");}
    catch(const Error& e){Status(e.english,e.turkish);}catch(const std::exception& e){Status(L"Check game folder: "+ErrorDetails(e),L"Oyun klasörünü kontrol et: "+ErrorDetails(e));}}
  void BuildControls(){
    CreateFonts();title=Control(L"STATIC",L"",0,242,28,610,46);SendMessageW(title,WM_SETFONT,reinterpret_cast<WPARAM>(heading),TRUE);
    subtitle=Control(L"STATIC",L"",0,244,80,590,24);
    auto lang_label=Control(L"STATIC",L"Language / Dil",0,878,29,210,22);SendMessageW(lang_label,WM_SETFONT,reinterpret_cast<WPARAM>(small_font),TRUE);
    language=Control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,878,56,210,160,kLanguage);
    SendMessageW(language,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"English"));SendMessageW(language,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Türkçe"));SendMessageW(language,CB_SETCURSEL,turkish?1:0,0);
    for(int i=0;i<3;++i)navigation[i]=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,20,216+i*52,170,42,kNavGraphics+i);
    folder_label=Control(L"STATIC",L"",0,260,130,700,22);SendMessageW(folder_label,WM_SETFONT,reinterpret_cast<WPARAM>(small_font),TRUE);
    path=Control(L"EDIT",ResolveGameRoot(exe,config.table).c_str(),WS_TABSTOP|ES_AUTOHSCROLL|WS_BORDER,260,160,658,28,kPath);SendMessageW(path,EM_SETLIMITTEXT,32760,0);
    browse=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,932,158,138,32,kBrowse);
    auto combo=[&](int page,int x,int y,int w,int id){return PageControl(page,L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,x,y,w,220,id);};
    LabelControl(0,L"DISPLAY",L"GÖRÜNTÜ",260,247,374);
    LabelControl(0,L"Graphics adapter",L"Ekran kartı",260,290,374);adapter=combo(0,260,316,374,kAdapter);
    LabelControl(0,L"Output window size",L"Çıkış pencere boyutu",260,364,374);size=combo(0,260,390,374,kSize);
    fullscreen=PageControl(0,L"BUTTON",L"",WS_TABSTOP|BS_AUTOCHECKBOX,260,443,175,28,kFullscreen);
    vsync=PageControl(0,L"BUTTON",L"",WS_TABSTOP|BS_AUTOCHECKBOX,456,443,175,28,kVsync);
    letterbox=PageControl(0,L"BUTTON",L"",WS_TABSTOP|BS_AUTOCHECKBOX,260,488,374,28,kLetterbox);
    dither=PageControl(0,L"BUTTON",L"",WS_TABSTOP|BS_AUTOCHECKBOX,260,533,374,28,kDither);
    shader_cache=PageControl(0,L"BUTTON",L"",WS_TABSTOP|BS_AUTOCHECKBOX,260,578,374,28,kShaderCache);
    LabelControl(0,L"Fullscreen uses your desktop resolution.\nInternal resolution controls the rendered detail.",L"Tam ekran masaüstü çözünürlüğünü kullanır.\nİç çözünürlük, işlenen detay düzeyini belirler.",260,655,374,48);
    LabelControl(0,L"RENDER QUALITY",L"GÖRÜNTÜ KALİTESİ",696,247,374);
    LabelControl(0,L"Internal resolution",L"İç çözünürlük",696,290,374);render_resolution=combo(0,696,316,374,kRenderResolution);
    LabelControl(0,L"Texture filtering",L"Doku filtreleme",696,364,374);anisotropic=combo(0,696,390,374,kAnisotropic);
    LabelControl(0,L"Anti-aliasing",L"Kenar yumuşatma",696,438,374);post_effect=combo(0,696,464,374,kPostEffect);
    LabelControl(0,L"Upscaling / sharpening",L"Ölçekleme / keskinleştirme",696,512,374);effect=combo(0,696,538,374,kEffect);
    LabelControl(0,L"Sharpness",L"Keskinlik",696,578,290);sharp_value=PageControl(0,L"STATIC",L"90%",SS_RIGHT,1004,578,66,22);
    sharpness=PageControl(0,TRACKBAR_CLASSW,L"",WS_TABSTOP|TBS_HORZ|TBS_NOTICKS,690,602,386,30,kSharpness);SendMessageW(sharpness,TBM_SETRANGE,TRUE,MAKELPARAM(0,100));
    msaa=PageControl(0,L"BUTTON",L"",WS_TABSTOP|BS_AUTOCHECKBOX,696,653,374,28,kMsaa);
    LabelControl(0,L"Higher resolutions use more GPU memory.",L"Yüksek çözünürlük daha fazla GPU belleği kullanır.",696,696,374,22);
    LabelControl(1,L"GAME AUDIO",L"OYUN SESİ",260,247,798);
    LabelControl(1,L"One level for music, engines and menu sounds.",L"Müzik, motor ve menü sesleri için toplam ses düzeyi.",260,292,798);
    LabelControl(1,L"Master volume",L"Ana ses düzeyi",260,343,700);volume_value=PageControl(1,L"STATIC",L"100%",SS_RIGHT,994,343,76,24);
    volume=PageControl(1,TRACKBAR_CLASSW,L"",WS_TABSTOP|TBS_HORZ|TBS_NOTICKS,254,380,822,40,kVolume);SendMessageW(volume,TBM_SETRANGE,TRUE,MAKELPARAM(0,100));
    mute=PageControl(1,L"BUTTON",L"",WS_TABSTOP|BS_AUTOCHECKBOX,260,450,798,28,kMute);
    LabelControl(1,L"Use the in-game options for separate music and effects levels.\nChanges apply the next time you start the game.",L"Müzik ve efektleri ayrı ayarlamak için oyun içi seçenekleri kullan.\nDeğişiklikler oyunu sonraki başlatışında uygulanır.",260,513,798,52);
    LabelControl(2,L"Controller",L"Kontrolcü",260,249,110);backend=combo(2,386,246,356,kBackend);
    keyboard=PageControl(2,L"BUTTON",L"",WS_TABSTOP|BS_AUTOCHECKBOX,766,246,304,28,kKeyboard);
    LabelControl(2,L"BUTTONS & MOVEMENT",L"DÜĞMELER VE HAREKET",260,335,374);
    LabelControl(2,L"CAMERA & MENUS",L"KAMERA VE MENÜLER",696,335,374);
    for(size_t i=0;i<kBindings.size();++i){bool right=i>=13;int x=right?696:260,y=365+static_cast<int>(right?i-13:i)*28;
      LabelControl(2,kBindings[i].english,kBindings[i].turkish,x,y+3,172,23);
      binding_fields[i]=PageControl(2,L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL|WS_BORDER,x+180,y,162,25,kBindingBase+i);SendMessageW(binding_fields[i],EM_SETLIMITTEXT,512,0);
      SetWindowSubclass(binding_fields[i],BindingProc,1,reinterpret_cast<DWORD_PTR>(this));
      binding_buttons[i]=PageControl(2,L"BUTTON",L"…",WS_TABSTOP|BS_PUSHBUTTON,x+350,y,24,25,kCaptureBase+i);
    }
    LabelControl(2,L"Type a key or use … to capture. Right-click a field to unbind or reset. Esc cancels capture.",L"Tuşu yaz veya … ile yakala. Sağ tık: tuşu kaldır / sıfırla. Esc: yakalamayı iptal et.",242,736,846,22);
    status=Control(L"STATIC",L"",0,242,765,846,32);SendMessageW(status,WM_SETFONT,reinterpret_cast<WPARAM>(small_font),TRUE);
    save=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,242,806,158,38,kSave);
    defaults=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,410,806,156,38,kDefaults);
    logs=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,576,806,126,38,kLogs);
    report=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,712,806,130,38,kReport);
    play=Control(L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,858,800,230,50,kPlay);
    Localize();SetControls(settings);ShowPage(0);ReadyStatus();
  }
  COLORREF BackgroundFor(HWND control)const{
    RECT r;GetWindowRect(control,&r);MapWindowPoints(nullptr,window,reinterpret_cast<POINT*>(&r),2);
    return r.top>=S(118) && r.top<S(732)?kCard:kBackground;
  }
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
    auto c=draw.hdr.hwndFrom;RECT r;GetClientRect(c,&r);auto b=CreateSolidBrush(kCard);FillRect(draw.hdc,&r,b);DeleteObject(b);
    RECT channel{},thumb{};SendMessageW(c,TBM_GETCHANNELRECT,0,reinterpret_cast<LPARAM>(&channel));SendMessageW(c,TBM_GETTHUMBRECT,0,reinterpret_cast<LPARAM>(&thumb));
    const int cy=(r.top+r.bottom)/2,cx=(thumb.left+thumb.right)/2;
    RECT track{channel.left,cy-S(2),channel.right,cy+S(2)};Rounded(draw.hdc,track,kBorder,kBorder,4);
    track.right=cx;Rounded(draw.hdc,track,IsWindowEnabled(c)?kAccent:kMuted,IsWindowEnabled(c)?kAccent:kMuted,4);
    auto brush=CreateSolidBrush(IsWindowEnabled(c)?kAccent:kMuted);auto old=SelectObject(draw.hdc,brush),pen=SelectObject(draw.hdc,GetStockObject(NULL_PEN));
    Ellipse(draw.hdc,cx-S(6),cy-S(6),cx+S(6),cy+S(6));SelectObject(draw.hdc,old);SelectObject(draw.hdc,pen);DeleteObject(brush);
    if(GetFocus()==c){RECT focus=r;InflateRect(&focus,-S(2),-S(2));DrawFocusRect(draw.hdc,&focus);}return CDRF_SKIPDEFAULT;
  }
  void Paint(HDC target){
    RECT client;GetClientRect(window,&client);auto dc=CreateCompatibleDC(target);auto bitmap=CreateCompatibleBitmap(target,client.right,client.bottom);auto old=SelectObject(dc,bitmap);
    FillRect(dc,&client,background);RECT side{0,0,S(210),client.bottom};auto b=CreateSolidBrush(kSidebar);FillRect(dc,&side,b);DeleteObject(b);
    auto box=[&](int x,int y,int w,int h){RECT r{S(x),S(y),S(x+w),S(y+h)};Rounded(dc,r,kCard,kBorder,16);};
    box(242,118,846,84);
    if(current_page==0){box(242,228,410,504);box(678,228,410,504);}
    else if(current_page==1)box(242,228,846,368);
    else{box(242,228,846,72);box(242,322,410,410);box(678,322,410,410);}
    {Gdiplus::Graphics g(dc);g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);g.ScaleTransform(static_cast<float>(scale),static_cast<float>(scale));
      Gdiplus::GraphicsPath red;red.AddBezier(0.f,854.f,194.f,828.f,72.f,683.f,210.f,573.f);red.AddLine(210.f,573.f,210.f,636.f);red.AddBezier(210.f,636.f,144.f,726.f,195.f,828.f,28.f,860.f);red.CloseFigure();
      Gdiplus::SolidBrush ribbon(Gdiplus::Color(255,126,37,33));g.FillPath(&ribbon,&red);
      Gdiplus::Pen glow(Gdiplus::Color(255,255,170,48),3.f),line(Gdiplus::Color(255,239,66,47),1.5f);
      g.DrawBezier(&glow,0.f,833.f,202.f,766.f,41.f,697.f,209.f,548.f);
      g.DrawBezier(&line,0.f,801.f,169.f,738.f,32.f,661.f,208.f,519.f);
    }
    SetBkMode(dc,TRANSPARENT);SelectObject(dc,logo);SetTextColor(dc,kText);RECT r{S(24),S(36),S(197),S(78)};DrawTextW(dc,L"MotoGP",-1,&r,DT_LEFT|DT_SINGLELINE);
    SetTextColor(dc,kAccent);r={S(24),S(78),S(197),S(120)};DrawTextW(dc,L"09/10",-1,&r,DT_LEFT|DT_SINGLELINE);
    SelectObject(dc,small_font);SetTextColor(dc,kMuted);r={S(25),S(138),S(197),S(157)};DrawTextW(dc,T(L"PC RECOMPILATION",L"PC YENİDEN DERLEME"),-1,&r,DT_LEFT|DT_SINGLELINE);
    r={S(26),S(184),S(197),S(204)};DrawTextW(dc,T(L"SETTINGS",L"AYARLAR"),-1,&r,DT_LEFT|DT_SINGLELINE);
    r={S(25),S(429),S(185),S(483)};DrawTextW(dc,T(L"Make it yours.\nThen take the grid.",L"Kendine göre ayarla.\nSonra piste çık."),-1,&r,DT_LEFT);
    BitBlt(target,0,0,client.right,client.bottom,dc,0,0,SRCCOPY);SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);
  }
  void Rescale(double value){
    DeleteFonts();scale=value;CreateFonts();
    for(auto& p:placements){MoveWindow(p.control,S(p.x),S(p.y),S(p.w),S(p.h),TRUE);SendMessageW(p.control,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);
      wchar_t cls[32];GetClassNameW(p.control,cls,32);if(std::wstring_view(cls)==L"COMBOBOX"){SendMessageW(p.control,CB_SETITEMHEIGHT,0,S(22));SendMessageW(p.control,CB_SETITEMHEIGHT,WPARAM(-1),S(22));}}
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
    CancelCapture();
    for(auto control:{path,browse,save,defaults,play}) EnableWindow(control,!value);
    for(auto& page:pages) for(auto control:page) EnableWindow(control,!value);
    Localize();
  }
  void Command(int id,int event){
    try{
      if(id>=kBindingBase && id<kBindingBase+static_cast<int>(kBindings.size()) && event==EN_CHANGE && !filling_binding){
        edited_bindings[id-kBindingBase]=Utf8(Text(binding_fields[id-kBindingBase]));
      }else if(id==kLanguage && event==CBN_SELCHANGE){
        turkish=SendMessageW(language,CB_GETCURSEL,0,0)==1;Localize();Put(preferences.table,"language",std::string(turkish?"tr":"en"));preferences.Save();
      }else if(id==kEffect && event==CBN_SELCHANGE){auto mode=SendMessageW(effect,CB_GETCURSEL,0,0);
        SendMessageW(sharpness,TBM_SETPOS,TRUE,mode==2?static_cast<int>(settings.cas_sharpness*100):static_cast<int>(100-settings.fsr_reduction*50));UpdateSharpness();
      }else if(id==kPath && event==EN_CHANGE){Status(L"Select your game folder, then press PLAY.",L"Oyun klasörünü seç ve OYUNU BAŞLAT'a bas.");
      }else if(event==BN_CLICKED){
        auto c=GetDlgItem(window,id);if(checks.contains(c)){SetCheck(c,!Checked(c));return;}
        if(id>=kNavGraphics && id<=kNavControls){ShowPage(id-kNavGraphics);return;}
        if(id>=kCaptureBase && id<kCaptureBase+static_cast<int>(kBindings.size())){binding_index=id-kCaptureBase;id=kCapture;}
        switch(id){
          case kCapture:CancelCapture();capturing=true;SetFocus(binding_buttons[binding_index]);SetWindowTextW(binding_buttons[binding_index],L"?");
            Status(L"Press a key or chord. Escape cancels.",L"Bir tuşa veya tuş birleşimine bas. Escape iptal eder.");break;
          case kClearBinding:SetWindowTextW(binding_fields[binding_index],L"");break;
          case kResetBinding:SetWindowTextW(binding_fields[binding_index],Wide(kBindings[binding_index].defaults).c_str());break;
          case kBrowse:Browse();break;case kSave:SaveSettings();break;
          case kDefaults:settings=Settings{};SetControls(settings);Status(L"Defaults selected. Save or press PLAY to apply.",L"Varsayılanlar seçildi. Kaydet veya oyunu başlat.");break;
          case kLogs:OpenFolder(exe/L"logs");break;case kReport:CreateReport();break;
          case kPlay:if(process)break;Validate(exe,GameRoot());SaveSettings();process=Start(exe,Arguments(GameRoot(),settings,config.table));Running(true);SetTimer(window,1,700,nullptr);
            Status(L"Game running. Closing the launcher leaves the game open.",L"Oyun çalışıyor. Launcher'ı kapatman oyunu kapatmaz.");break;
        }
      }
    }catch(const std::exception& e){ErrorBox(e);}
  }
  void Poll(){
    if(!process || WaitForSingleObject(process,0)!=WAIT_OBJECT_0)return;
    DWORD code=0;GetExitCodeProcess(process,&code);CloseHandle(process);process=nullptr;KillTimer(window,1);Running(false);
    if(code)Status(L"Game exited (code "+std::to_wstring(code)+L"). Open logs or create an error report.",L"Oyun kapandı (kod "+std::to_wstring(code)+L"). Logları aç veya hata raporu oluştur.");
    else Status(L"Game closed. You can launch again.",L"Oyun kapandı. Tekrar başlatabilirsin.");
  }
};
LRESULT CALLBACK WindowProc(HWND window,UINT message,WPARAM wparam,LPARAM lparam){
  auto* app=reinterpret_cast<App*>(GetWindowLongPtrW(window,GWLP_USERDATA));
  if(message==WM_NCCREATE){app=static_cast<App*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);app->window=window;SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(app));}
  if(!app)return DefWindowProcW(window,message,wparam,lparam);
  switch(message){
    case WM_CREATE:try{app->BuildControls();}catch(const std::exception& e){app->ErrorBox(e);return -1;}return 0;
    case WM_COMMAND:app->Command(LOWORD(wparam),HIWORD(wparam));return 0;
    case WM_CONTEXTMENU:app->BindingMenu(reinterpret_cast<HWND>(wparam),{static_cast<short>(LOWORD(lparam)),static_cast<short>(HIWORD(lparam))});return 0;
    case WM_TIMER:app->Poll();return 0;
    case WM_DRAWITEM:{auto& draw=*reinterpret_cast<DRAWITEMSTRUCT*>(lparam);if(draw.CtlType==ODT_BUTTON)app->DrawButton(draw);else if(draw.CtlType==ODT_COMBOBOX)app->DrawCombo(draw);return TRUE;}
    case WM_MEASUREITEM:reinterpret_cast<MEASUREITEMSTRUCT*>(lparam)->itemHeight=app->S(22);return TRUE;
    case WM_NOTIFY:{auto* n=reinterpret_cast<NMHDR*>(lparam);if(n->code==NM_CUSTOMDRAW && (n->hwndFrom==app->sharpness || n->hwndFrom==app->volume))return app->DrawSlider(*reinterpret_cast<NMCUSTOMDRAW*>(lparam));return 0;}
    case WM_HSCROLL:{if(reinterpret_cast<HWND>(lparam)==app->volume)SetWindowTextW(app->volume_value,(std::to_wstring(SendMessageW(app->volume,TBM_GETPOS,0,0))+L"%").c_str());
      if(reinterpret_cast<HWND>(lparam)==app->sharpness){auto value=SendMessageW(app->sharpness,TBM_GETPOS,0,0),mode=SendMessageW(app->effect,CB_GETCURSEL,0,0);
        if(mode==1)app->settings.fsr_reduction=2.0-value/50.0;if(mode==2)app->settings.cas_sharpness=value/100.0;app->UpdateSharpness();}return 0;}
    case WM_CTLCOLOREDIT:case WM_CTLCOLORLISTBOX:SetTextColor(reinterpret_cast<HDC>(wparam),kText);SetBkColor(reinterpret_cast<HDC>(wparam),kField);return reinterpret_cast<LRESULT>(app->field);
    case WM_CTLCOLORSTATIC:case WM_CTLCOLORBTN:{auto c=reinterpret_cast<HWND>(lparam);auto bg=app->BackgroundFor(c);SetTextColor(reinterpret_cast<HDC>(wparam),c==app->title?kText:kMuted);
      SetBkColor(reinterpret_cast<HDC>(wparam),bg);return reinterpret_cast<LRESULT>(bg==kCard?app->card:app->background);}
    case WM_ERASEBKGND:return 1;
    case WM_PAINT:{PAINTSTRUCT paint;auto dc=BeginPaint(window,&paint);app->Paint(dc);EndPaint(window,&paint);return 0;}
    case WM_DPICHANGED:{auto* proposed=reinterpret_cast<RECT*>(lparam);
      MONITORINFO monitor{sizeof(monitor)};GetMonitorInfoW(MonitorFromRect(proposed,MONITOR_DEFAULTTONEAREST),&monitor);
      const auto& work=monitor.rcWork;
      app->Rescale(std::max(.55,std::min({HIWORD(wparam)/96.0,(work.right-work.left-40)/double(kWidth),(work.bottom-work.top-50)/double(kHeight)})));
      RECT r{0,0,app->S(kWidth),app->S(kHeight)};AdjustWindowRectExForDpi(&r,static_cast<DWORD>(GetWindowLongPtrW(window,GWL_STYLE)),FALSE,0,HIWORD(wparam));
      const int width=r.right-r.left,height=r.bottom-r.top;
      SetWindowPos(window,nullptr,std::clamp<int>(proposed->left,work.left,std::max<int>(work.left,work.right-width)),
          std::clamp<int>(proposed->top,work.top,std::max<int>(work.top,work.bottom-height)),width,height,SWP_NOZORDER|SWP_NOACTIVATE);return 0;}
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
    app.scale=std::max(0.55,std::min({GetDpiForSystem()/96.0,(work.right-work.left-40)/double(kWidth),(work.bottom-work.top-50)/double(kHeight)}));
    WNDCLASSW cls{};cls.lpfnWndProc=WindowProc;cls.hInstance=instance;cls.lpszClassName=L"MotoGP0910Launcher";cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.hIcon=LoadIconW(nullptr,IDI_APPLICATION);RegisterClassW(&cls);
    RECT size{0,0,app.S(kWidth),app.S(kHeight)};constexpr DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX;
    AdjustWindowRectEx(&size,style,FALSE,0);
    auto window=CreateWindowExW(0,cls.lpszClassName,L"MotoGP 09/10 Launcher",style,(work.right-size.right+size.left)/2,(work.bottom-size.bottom+size.top)/2,
        size.right-size.left,size.bottom-size.top,nullptr,nullptr,instance,&app);
    if(!window){CloseHandle(mutex);throw std::runtime_error("Cannot create launcher window");}
    BOOL dark=TRUE;DwmSetWindowAttribute(window,20,&dark,sizeof(dark));
    ShowWindow(window,show);UpdateWindow(window);MSG message;
    while(GetMessageW(&message,nullptr,0,0)>0){if(app.CaptureKey(message))continue;if(!IsDialogMessageW(window,&message)){TranslateMessage(&message);DispatchMessageW(&message);}}
    CloseHandle(mutex);
  }catch(const std::exception& e){MessageBoxW(nullptr,(L"Cannot open launcher / Launcher açılamadı:\n"+ErrorDetails(e)).c_str(),L"MotoGP 09/10",MB_OK|MB_ICONERROR);result=1;}
  Gdiplus::GdiplusShutdown(gdip_token);CoUninitialize();return result;
}
