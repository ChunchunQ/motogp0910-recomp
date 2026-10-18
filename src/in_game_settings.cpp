#include "in_game_settings.h"
#include "adaptive_triggers.h"
#include "overlay_theme.h"
#include "keyboard_binding_specs.h"
#include <rex/cvar.h>
#include <rex/input/input_system.h>
#include <rex/ui/keybinds.h>
#include <rex/ui/window.h>
#include <rex/ui/window_listener.h>
#include <toml++/toml.hpp>
#include <imgui.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <fstream>
#include <optional>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif
namespace motogp::settings {
std::mutex& InputValueMutex(){static std::mutex mutex;return mutex;}
namespace {
using Paint=rex::ui::Presenter::GuestOutputPaintConfig;
const Spec* Find(std::string_view key){for(const auto& s:Catalog())if(s.key==key)return &s;return nullptr;}
bool Numeric(std::string_view s,double& value){if(s.empty())return false;auto r=std::from_chars(s.data(),s.data()+s.size(),value);return r.ec==std::errc{}&&r.ptr==s.data()+s.size()&&std::isfinite(value);}
bool IsPaint(std::string_view key){return key=="present_effect"||key=="present_fsr_quality_mode"||key=="present_fsr_sharpness_reduction"||key=="present_cas_additional_sharpness"||key=="present_dither";}
const toml::node* Node(const toml::table& t,std::string_view name,std::string prefix={}){
  for(const auto& [key,n]:t){auto full=prefix.empty()?std::string(key.str()):prefix+"_"+std::string(key.str());
    if(full==name&&!n.is_table())return &n;if(n.is_table())if(auto* found=Node(*n.as_table(),name,full))return found;}return nullptr;
}
void Erase(toml::table& t,std::string_view name,std::string prefix={}){
  for(auto it=t.begin();it!=t.end();){auto full=prefix.empty()?std::string(it->first.str()):prefix+"_"+std::string(it->first.str());
    if(full==name&&!it->second.is_table())it=t.erase(it);else{if(it->second.is_table())Erase(*it->second.as_table(),name,full);++it;}}
}
std::optional<std::string> Read(const std::filesystem::path& path){
  std::error_code ec;if(!std::filesystem::exists(path,ec)){if(ec)throw std::runtime_error("Cannot inspect settings file");return {};}
  std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("Cannot read settings file");std::string text((std::istreambuf_iterator<char>(f)),{});if(f.bad())throw std::runtime_error("Cannot read settings file");return text;
}
std::string Serialized(const toml::node* n){if(!n)return "<absent>";std::ostringstream out;out<<toml::toml_formatter{*n};return out.str();}
void Store(toml::table& t,const Spec& spec,const std::string& value){
  Erase(t,spec.key);
  if(spec.kind==Kind::Toggle)t.insert_or_assign(spec.key,value=="true");
  else if(spec.kind==Kind::Integer)t.insert_or_assign(spec.key,std::stoll(value));
  else if(spec.kind==Kind::Number)t.insert_or_assign(spec.key,std::stod(value));
  else t.insert_or_assign(spec.key,value);
}
}
const std::vector<Spec>& Catalog(){static const auto all=[] {
  using K=Kind;using P=Page;
  std::vector<Spec> s={
    {"button_prompt_style","Button prompts","Tuş simgeleri",P::General,K::Choice,0,0,{{"auto","Automatic","Otomatik"},{"xbox","Xbox 360","Xbox 360"},{"playstation","PlayStation 3","PlayStation 3"},{"keyboard","Keyboard","Klavye"}}},
    {"fps_limit","Frame rate limit","FPS sınırı",P::Graphics,K::Integer,0,1000},
    {"vsync","VSync","VSync",P::Graphics,K::Toggle},
    {"present_letterbox","Keep aspect ratio","En-boy oranını koru",P::Graphics,K::Toggle},
    {"present_effect","Upscaling / sharpening","Yükseltme / keskinleştirme",P::Graphics,K::Choice,0,0,{{"bilinear","Bilinear","Bilinear"},{"cas","AMD CAS","AMD CAS"},{"fsr","AMD FSR 1","AMD FSR 1"},{"fsr2","AMD FSR 2 (Experimental)","AMD FSR 2 (Deneysel)"},{"fsr3","AMD FSR 3 (Experimental)","AMD FSR 3 (Deneysel)"}}},
    {"present_fsr_quality_mode","FSR quality mode","FSR kalite modu",P::Graphics,K::Choice,0,0,{{"auto","Automatic","Otomatik"},{"nativeaa","Native AA","Yerel AA"},{"quality","Quality","Kalite"},{"balanced","Balanced","Dengeli"},{"performance","Performance","Performans"},{"ultra_performance","Ultra performance","Ultra performans"}}},
    {"present_fsr_sharpness_reduction","FSR sharpness","FSR keskinliği",P::Graphics,K::Number,0,2},
    {"present_cas_additional_sharpness","CAS sharpness","CAS keskinliği",P::Graphics,K::Number,0,1},
    {"present_dither","Reduce color banding","Renk bantlaşmasını azalt",P::Graphics,K::Toggle},
    {"engine_speed_blur","Motion blur (speed)","Hareket bulanıklığı (hız)",P::Advanced,K::Choice,0,0,{{"game","Game default","Oyun varsayılanı"},{"off","Off","Kapalı"}}},
    {"engine_depth_of_field","Depth of field","Alan derinliği",P::Advanced,K::Choice,0,0,{{"game","Game default","Oyun varsayılanı"},{"off","Off","Kapalı"}}},
    {"engine_dynamic_shadows","Dynamic shadows","Dinamik gölgeler",P::Advanced,K::Choice,0,0,{{"game","Game default","Oyun varsayılanı"},{"off","Off","Kapalı"}}},
    {"engine_shadow_distance","Shadow distance (0 = game)","Gölge mesafesi (0 = oyun)",P::Advanced,K::Integer,0,180},
    {"audio_master_volume","Master volume","Ana ses düzeyi",P::Audio,K::Integer,0,100},
    {"audio_mute","Mute","Sessiz",P::Audio,K::Toggle},
  };
  s.push_back({"dualsense_adaptive_triggers","DualSense adaptive triggers (Experimental)","DualSense adaptif tetikler (Deneysel)",P::Controls,K::Toggle});
  for(int side=0;side<2;++side){const std::string key=side?"dualsense_lt_":"dualsense_rt_",en=side?"L2 · ":"R2 · ",tr=side?"L2 · ":"R2 · ";
    s.push_back({key+"mode",en+"Effect mode",tr+"Efekt modu",P::Controls,K::Choice,0,0,{{"off","Off","Kapalı"},{"feedback","Constant resistance","Sabit direnç"},{"weapon","Resistance zone","Direnç aralığı"}}});
    s.push_back({key+"start",en+"Start position",tr+"Başlangıç konumu",P::Controls,K::Integer,0,9});
    s.push_back({key+"end",en+"End position (zone only)",tr+"Bitiş konumu (direnç aralığı)",P::Controls,K::Integer,0,9});
    s.push_back({key+"strength",en+"Strength (0 = off)",tr+"Güç (0 = kapalı)",P::Controls,K::Integer,0,8});
  }
  for(const auto& b:launcher::kBindings){
    // UTF-8 UI names come from the SDK-independent canonical binding list.
    std::string en;for(auto* p=b.english;*p;++p)en+=char(*p);
    const char* tr[]={"A / Onayla","B / Geri","X düğmesi","Y düğmesi","Sol tetik (LT)","Sağ tetik (RT)","Sol omuz (LB)","Sağ omuz (RB)","Sol çubuk: yukarı","Sol çubuk: aşağı","Sol çubuk: sol","Sol çubuk: sağ","Sol çubuk: bas","Sağ çubuk: yukarı","Sağ çubuk: aşağı","Sağ çubuk: sol","Sağ çubuk: sağ","Sağ çubuk: bas","D-pad: yukarı","D-pad: aşağı","D-pad: sol","D-pad: sağ","Back / Seç","Start / Duraklat","Guide düğmesi"};
    s.push_back({b.key,en,tr[&b-launcher::kBindings.data()],P::Controls,K::Binding});
  }
  return s;
}();return all;}
bool ValidBinding(std::string_view value){
  if(value.empty())return true;
  for(;;){auto comma=value.find(',');auto token=value.substr(0,comma);while(!token.empty()&&token.front()==' ')token.remove_prefix(1);while(!token.empty()&&token.back()==' ')token.remove_suffix(1);
    if(token.empty())return false;
    for(;;){auto plus=token.find('+');if(plus==std::string_view::npos)break;auto mod=token.substr(0,plus);if(mod!="Shift"&&mod!="Ctrl"&&mod!="Control"&&mod!="Alt")return false;token.remove_prefix(plus+1);}
    if(token=="NumpadEnter"){}else if(token.empty()||token=="Shift"||token=="Control"||token=="Alt"||rex::ui::ParseVirtualKey(std::string(token))==rex::ui::VirtualKey::kNone)return false;
    if(comma==std::string_view::npos)return true;value.remove_prefix(comma+1);
  }
}
bool PatchPaintConfig(Paint& config,std::string_view key,std::string_view value){
  if(key=="present_effect"){
    using E=Paint::Effect;
    if(value=="bilinear")config.SetEffect(E::kBilinear);
    else if(value=="cas")config.SetEffect(E::kCas);else if(value=="fsr")config.SetEffect(E::kFsr);
    else if(value=="fsr2")config.SetEffect(E::kFsr2);else if(value=="fsr3")config.SetEffect(E::kFsr3);else return false;
  }else if(key=="present_fsr_quality_mode"){
    using Q=Paint::FsrQualityMode;
    if(value=="auto")config.SetFsrQualityMode(Q::kAuto);else if(value=="nativeaa")config.SetFsrQualityMode(Q::kNativeAa);
    else if(value=="quality")config.SetFsrQualityMode(Q::kQuality);else if(value=="balanced")config.SetFsrQualityMode(Q::kBalanced);
    else if(value=="performance")config.SetFsrQualityMode(Q::kPerformance);else if(value=="ultra_performance")config.SetFsrQualityMode(Q::kUltraPerformance);else return false;
  }else if(key=="present_dither"){if(value!="true"&&value!="false")return false;config.SetDither(value=="true");
  }else if(key=="present_cas_additional_sharpness"||key=="present_fsr_sharpness_reduction"){
    double n=0;if(!Numeric(value,n)||n<0||n>(key=="present_cas_additional_sharpness"?1:2))return false;
    if(key=="present_cas_additional_sharpness")config.SetCasAdditionalSharpness(float(n));else config.SetFsrSharpnessReduction(float(n));
  }else return false;return true;
}
struct Model::Disk {
  toml::table baseline;std::string load_error;
  explicit Disk(const std::filesystem::path& path){try{if(auto bytes=Read(path))baseline=toml::parse(*bytes);}catch(const std::exception& e){load_error=e.what();}}
};
Model::Model(std::filesystem::path config,rex::ui::Presenter* presenter):path_(std::move(config)),disk_(std::make_unique<Disk>(path_)),presenter_(presenter){
  for(const auto& s:Catalog())if(Available(s))saved_[s.key]=rex::cvar::GetFlagByName(s.key);
}
Model::~Model()=default;
bool Model::Available(const Spec& s)const{return rex::cvar::GetFlagInfo(s.key)&&(!IsPaint(s.key)||presenter_)&&(!s.key.starts_with("dualsense_")||adaptive::LiveAvailable());}
std::string Model::Value(std::string_view key)const{return rex::cvar::GetFlagByName(key);}
bool Model::Set(std::string_view key,std::string value){
  std::unique_lock<std::mutex> input_lock(InputValueMutex(),std::defer_lock);
  if(key.starts_with("keybind_"))input_lock.lock();
  const auto* spec=Find(key);if(!spec||!Available(*spec)){error_="This setting cannot be changed live.";return false;}
  double number=0;bool valid=true;
  if(spec->kind==Kind::Toggle)valid=value=="true"||value=="false";
  else if(spec->kind==Kind::Choice)valid=std::any_of(spec->choices.begin(),spec->choices.end(),[&](const Choice& c){return c.value==value;});
  else if(spec->kind==Kind::Binding)valid=ValidBinding(value);
  else valid=Numeric(value,number)&&number>=spec->minimum&&number<=spec->maximum&&(spec->kind!=Kind::Integer||std::trunc(number)==number);
  if(!valid){error_="Invalid value / Geçersiz değer: "+spec->en;return false;}
  if(key.starts_with("dualsense_")){
    auto p=adaptive::ReadProfile();auto& t=key.starts_with("dualsense_rt_")?p.right:p.left;
    if(key=="dualsense_adaptive_triggers")p.enabled=value=="true";
    else if(key.ends_with("mode"))t.mode=value;else if(key.ends_with("start"))t.start=static_cast<int>(number);else if(key.ends_with("end"))t.end=static_cast<int>(number);else if(key.ends_with("strength"))t.strength=static_cast<int>(number);
    if(!p.Valid()){error_="Resistance-zone end must exceed start / Direnç aralığının bitişi başlangıçtan büyük olmalı.";return false;}
    if(!adaptive::WriteProfile(p)){error_="Could not update DualSense profile.";return false;}error_.clear();return true;
  }
  if(Value(key)==value){error_.clear();return true;}
  Paint paint;if(IsPaint(key)){paint=presenter_->GetGuestOutputPaintConfigFromUIThread();if(!PatchPaintConfig(paint,key,value)){error_="Unsupported live image setting.";return false;}}
  if(!rex::cvar::SetFlagByName(key,value)){error_="The game rejected this setting: "+spec->en;return false;}
  if(IsPaint(key))presenter_->SetGuestOutputPaintConfigFromUIThread(paint);
  error_.clear();return true;
}
size_t Model::Unsaved()const{size_t n=0;for(const auto& [key,value]:saved_)n+=Value(key)!=value;return n;}
bool Model::Revert(){
  // Restore the complete trigger profile together: valid old and new Weapon
  // ranges can have invalid intermediate start/end values in map order.
  auto p=adaptive::ReadProfile();
  for(const auto& [key,value]:saved_){
    if(!key.starts_with("dualsense_")){if(Value(key)!=value&&!Set(key,value))return false;continue;}
    auto& t=key.starts_with("dualsense_lt_")?p.left:p.right;
    if(key=="dualsense_adaptive_triggers")p.enabled=value=="true";
    else if(key.ends_with("mode"))t.mode=value;else if(key.ends_with("start"))t.start=std::stoi(value);else if(key.ends_with("end"))t.end=std::stoi(value);else if(key.ends_with("strength"))t.strength=std::stoi(value);
  }
  if(!adaptive::WriteProfile(p)){error_="Could not restore DualSense profile.";return false;}
  error_.clear();return true;
}
bool Model::Save(){
  error_.clear();if(!Unsaved())return true;
  if(path_.empty()||!disk_->load_error.empty()){error_="Cannot save settings: "+disk_->load_error;return false;}
  // Hold an exclusive temporary file before reading/merging. Concurrent games
  // never overwrite each other's pending write; only changed managed keys move.
  auto temp=path_;temp+=".ingame.tmp";
#ifdef _WIN32
  HANDLE handle=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(handle==INVALID_HANDLE_VALUE){error_="Cannot save. Check folder access or close another settings writer.";return false;}
  struct Cleanup{HANDLE h;std::filesystem::path p;~Cleanup(){if(h!=INVALID_HANDLE_VALUE)CloseHandle(h);DeleteFileW(p.c_str());}} cleanup{handle,temp};
#else
  int handle=open(temp.c_str(),O_CREAT|O_EXCL|O_WRONLY,0600);
  if(handle<0){error_="Cannot save. Check folder access or close another settings writer.";return false;}
  struct Cleanup{int h;std::filesystem::path p;~Cleanup(){if(h>=0)close(h);std::error_code ec;std::filesystem::remove(p,ec);}} cleanup{handle,temp};
#endif
  try{
    auto original=Read(path_);auto table=original?toml::parse(*original):toml::table{};
    for(const auto& [key,old]:saved_)if(const auto current=Value(key);current!=old){
      if(Serialized(Node(table,key))!=Serialized(Node(disk_->baseline,key))){
        auto expected=table;Store(expected,*Find(key),current);
        if(Serialized(Node(table,key))!=Serialized(Node(expected,key)))throw std::runtime_error("Settings changed externally: "+key+". Current live values are kept; the file was not overwritten.");
      }
      Store(table,*Find(key),current);
    }
    std::ostringstream out;out<<toml::toml_formatter{table}<<'\n';const auto bytes=out.str();
#ifdef _WIN32
    DWORD written=0;if(!WriteFile(handle,bytes.data(),DWORD(bytes.size()),&written,nullptr)||written!=bytes.size()||!FlushFileBuffers(handle))throw std::runtime_error("Cannot write settings; original preserved.");
    CloseHandle(handle);cleanup.h=INVALID_HANDLE_VALUE;
#else
    if(write(handle,bytes.data(),bytes.size())!=static_cast<ssize_t>(bytes.size())||fsync(handle))throw std::runtime_error("Cannot write settings; original preserved.");close(handle);cleanup.h=-1;
#endif
    // Detect other writers, including a launcher already open before this game.
    if(Read(path_)!=original)throw std::runtime_error("Settings changed during save. Retry after closing the other writer.");
    auto backup=path_;backup+=".bak";if(original&&!std::filesystem::exists(backup))std::filesystem::copy_file(path_,backup);
#ifdef _WIN32
    if(!MoveFileExW(temp.c_str(),path_.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot replace settings; original preserved.");
#else
    std::filesystem::rename(temp,path_);
#endif
    disk_->baseline=std::move(table);for(auto& [key,value]:saved_)value=Value(key);return true;
  }catch(const std::exception& e){error_=e.what();return false;}
}
void InputGate::Initialize(rex::input::InputSystem* input){input_=input;open_=false;held_=false;buttons_=0;epoch_=0;}
void InputGate::Open(){std::lock_guard lock(transition_);if(!input_)return;neutral_epoch_=UINT64_MAX;open_=true;buttons_=0;if(!held_.exchange(true,std::memory_order_acq_rel)){
  input_->AddUIInputBlocker();
  // Mouse-look must not recapture/hide the pointer while editing the menu.
  // This temporary internal change is never written to the launcher file.
  restore_mouse_=rex::cvar::Query<bool>("mnk_mouse");
  if(restore_mouse_)rex::cvar::SetFlagByName("mnk_mouse","false");
}epoch_.fetch_add(1,std::memory_order_release);}
void InputGate::Close(){std::lock_guard lock(transition_);open_=false;buttons_=0;neutral_epoch_=UINT64_MAX;}
void InputGate::SetFocused(bool value){if(focused_.exchange(value)!=value){buttons_=0;neutral_epoch_=UINT64_MAX;epoch_.fetch_add(1,std::memory_order_release);}}
void InputGate::PollGuest(){
  if(!input_||!held_.load(std::memory_order_acquire))return;
  const std::lock_guard values_lock(InputValueMutex());
  // Open/close originate on UI, but release calls poll drivers and belongs here.
  const auto sample_epoch=epoch_.load(std::memory_order_acquire);
  uint16_t buttons=0;
  for(uint32_t user=0;user<4;++user){rex::input::X_INPUT_STATE state{};if(input_->GetStateForUI(user,&state)==0){
    buttons|=state.gamepad.buttons;
    if(state.gamepad.thumb_lx<-16000)buttons|=rex::input::X_INPUT_GAMEPAD_DPAD_LEFT;
    if(state.gamepad.thumb_lx>16000)buttons|=rex::input::X_INPUT_GAMEPAD_DPAD_RIGHT;
    if(state.gamepad.thumb_ly<-16000)buttons|=rex::input::X_INPUT_GAMEPAD_DPAD_DOWN;
    if(state.gamepad.thumb_ly>16000)buttons|=rex::input::X_INPUT_GAMEPAD_DPAD_UP;
  }}
  // Drain keystrokes as well as pad states. XamInputGetKeystrokeEx is gated
  // separately because the SDK's built-in blocker covers GetState only.
  for(uint32_t user=0;user<4;++user)for(int i=0;i<256;++i){rex::input::X_INPUT_KEYSTROKE event{};if(input_->GetKeystroke(user,0,&event)!=0)break;}
  buttons_.store(focused_.load(std::memory_order_acquire)?buttons:0,std::memory_order_release);
  if(!buttons&&focused_.load(std::memory_order_acquire))neutral_epoch_.store(sample_epoch,std::memory_order_release);
  if(!buttons){std::lock_guard lock(transition_);if(!open_.load(std::memory_order_acquire)&&held_.exchange(false)){
    input_->RemoveUIInputBlocker();
    if(restore_mouse_)rex::cvar::SetFlagByName("mnk_mouse","true");
    restore_mouse_=false;
  }}
}
namespace {
std::string CaptureKey(ImGuiKey key){
  if(key>=ImGuiKey_A&&key<=ImGuiKey_Z)return std::string(1,char('A'+key-ImGuiKey_A));
  if(key>=ImGuiKey_0&&key<=ImGuiKey_9)return std::string(1,char('0'+key-ImGuiKey_0));
  if(key>=ImGuiKey_F1&&key<=ImGuiKey_F24)return "F"+std::to_string(1+key-ImGuiKey_F1);
  if(key>=ImGuiKey_Keypad0&&key<=ImGuiKey_Keypad9)return "Numpad"+std::to_string(key-ImGuiKey_Keypad0);
  switch(key){
    case ImGuiKey_Space:return "Space";case ImGuiKey_Enter:return "Return";case ImGuiKey_Tab:return "Tab";case ImGuiKey_Backspace:return "Backspace";
    case ImGuiKey_Delete:return "Delete";case ImGuiKey_Insert:return "Insert";case ImGuiKey_Home:return "Home";case ImGuiKey_End:return "End";
    case ImGuiKey_PageUp:return "PageUp";case ImGuiKey_PageDown:return "PageDown";case ImGuiKey_LeftArrow:return "Left";case ImGuiKey_RightArrow:return "Right";
    case ImGuiKey_UpArrow:return "Up";case ImGuiKey_DownArrow:return "Down";case ImGuiKey_GraveAccent:return "Backtick";
    case ImGuiKey_Minus:return "Minus";case ImGuiKey_Equal:return "Plus";case ImGuiKey_LeftBracket:return "LBracket";case ImGuiKey_RightBracket:return "RBracket";
    case ImGuiKey_Backslash:return "Backslash";case ImGuiKey_Semicolon:return "Semicolon";case ImGuiKey_Apostrophe:return "Quote";
    case ImGuiKey_Comma:return "Comma";case ImGuiKey_Period:return "Period";case ImGuiKey_Slash:return "Slash";
    case ImGuiKey_KeypadEnter:return "NumpadEnter";case ImGuiKey_KeypadAdd:return "NumpadPlus";case ImGuiKey_KeypadSubtract:return "NumpadMinus";
    case ImGuiKey_KeypadMultiply:return "NumpadStar";case ImGuiKey_KeypadDivide:return "NumpadSlash";
    default:return {};
  }
}
void Capture(Model& model,View& view){
  if(view.capture.empty())return;
  if(view.capture_wait){if(!ImGui::IsMouseDown(0)&&!ImGui::IsKeyDown(ImGuiKey_Enter)&&!ImGui::IsKeyDown(ImGuiKey_Space))view.capture_wait=false;return;}
  if(ImGui::IsKeyPressed(ImGuiKey_Escape,false)){view.capture.clear();return;}
  std::string key;
  for(int k=ImGuiKey_NamedKey_BEGIN;k<ImGuiKey_GamepadStart;++k)if(ImGui::IsKeyPressed(ImGuiKey(k),false)){key=CaptureKey(ImGuiKey(k));if(!key.empty())break;}
  for(int b=0;b<3;++b)if(ImGui::IsMouseClicked(b))key=std::array{"LMB","RMB","MMB"}[b];
  if(key.empty())return;
  auto& io=ImGui::GetIO();if(io.KeyAlt)key="Alt+"+key;if(io.KeyCtrl)key="Ctrl+"+key;if(io.KeyShift)key="Shift+"+key;
  if(model.Set(view.capture,key)){view.edits.erase(view.capture);view.capture.clear();view.saved=false;}
}
void Widget(Model& model,View& view,const Spec& s,bool tr){
  using namespace motogp::overlay;
  const auto value=model.Value(s.key);ImGui::PushID(s.key.c_str());
  const bool columns=ImGui::GetContentRegionAvail().x>520;
  const bool table=columns&&ImGui::BeginTable("##row",2,ImGuiTableFlags_SizingStretchProp);
  if(table){ImGui::TableSetupColumn("##label",ImGuiTableColumnFlags_WidthStretch,.42f);ImGui::TableSetupColumn("##control",ImGuiTableColumnFlags_WidthStretch,.58f);ImGui::TableNextRow();ImGui::TableNextColumn();ImGui::AlignTextToFramePadding();}
  ImGui::TextWrapped("%s",(tr?s.tr:s.en).c_str());if(table)ImGui::TableNextColumn();ImGui::SetNextItemWidth(-1);
  bool changed=false;
  if(s.kind==Kind::Toggle){bool v=value=="true";if(ImGui::Checkbox(Tr(tr,"Enabled","Etkin"),&v))changed=model.Set(s.key,v?"true":"false");}
  else if(s.kind==Kind::Choice){
    const auto choice=std::find_if(s.choices.begin(),s.choices.end(),[&](const Choice& c){return c.value==value;});
    const auto label=choice==s.choices.end()?value:tr?choice->tr:choice->en;
    if(ImGui::BeginCombo("##value",label.c_str())){for(const auto& c:s.choices){const bool selected=c.value==value;if(ImGui::Selectable((tr?c.tr:c.en).c_str(),selected))changed=model.Set(s.key,c.value);if(selected)ImGui::SetItemDefaultFocus();}ImGui::EndCombo();}
  }else if(s.kind==Kind::Integer){int n=std::atoi(value.c_str());
    if(s.key=="fps_limit"){
      const auto label=n?std::to_string(n)+" FPS":Tr(tr,"Unlimited","Sınırsız");
      if(ImGui::BeginCombo("##value",label.c_str())){for(int fps:{0,30,60,90,120,144,165,180,240,360}){const auto text=fps?std::to_string(fps)+" FPS":Tr(tr,"Unlimited","Sınırsız");if(ImGui::Selectable(text.c_str(),n==fps))changed=model.Set(s.key,std::to_string(fps));if(n==fps)ImGui::SetItemDefaultFocus();}ImGui::EndCombo();}
    }else{const char* format=s.key=="audio_master_volume"?"%d%%":"%d";if(ImGui::SliderInt("##value",&n,int(s.minimum),int(s.maximum),format))changed=model.Set(s.key,std::to_string(n));}
  }else if(s.kind==Kind::Number){const double raw=std::atof(value.c_str());int n=int(std::lround((s.key=="present_fsr_sharpness_reduction"?1.0-raw/2.0:raw)*100));
    if(ImGui::SliderInt("##value",&n,0,100,"%d%%"))changed=model.Set(s.key,std::to_string(s.key=="present_fsr_sharpness_reduction"?2.0*(1.0-n/100.0):n/100.0));}
  else{
    auto [it,inserted]=view.edits.try_emplace(s.key,value);auto& text=it->second;
    char buffer[192]{};std::copy_n(text.data(),std::min(text.size(),sizeof(buffer)-1),buffer);
    const float button_width=std::min(150.f,ImGui::GetContentRegionAvail().x*.4f);
    ImGui::SetNextItemWidth(std::max(60.f,ImGui::GetContentRegionAvail().x-button_width-12));
    bool enter=ImGui::InputText("##binding",buffer,sizeof(buffer),ImGuiInputTextFlags_EnterReturnsTrue);text=buffer;
    if(enter||ImGui::IsItemDeactivatedAfterEdit())if(text!=value)changed=model.Set(s.key,text);
    const bool capture=view.capture==s.key;
    ImGui::SameLine();
    if(ImGui::Button(capture?Tr(tr,"Press a key...","Bir tuşa bas..."):Tr(tr,"Change key","Tuş değiştir"),ImVec2(button_width,0))){view.capture=s.key;view.capture_wait=true;}
    if(capture&&ImGui::IsItemHovered())ImGui::SetTooltip("%s",Tr(tr,"Esc cancels","Esc ile iptal"));
    if(ImGui::BeginPopupContextItem("##binding_actions")){
      if(ImGui::MenuItem(Tr(tr,"Unbind","Tuşu kaldır"))){changed=model.Set(s.key,"");view.edits.erase(s.key);}
      if(ImGui::MenuItem(Tr(tr,"Restore action default","Eylemi varsayılana döndür"))){for(const auto& b:launcher::kBindings)if(b.key==s.key)changed=model.Set(s.key,b.defaults);view.edits.erase(s.key);}
      ImGui::EndPopup();
    }
  }
  if(table)ImGui::EndTable();
  if(changed)view.saved=false;ImGui::Separator();ImGui::Spacing();ImGui::PopID();
}
}
bool DrawMenu(Model& model,View& view,bool tr,ImFont* font,ImFont* heading){
  using namespace motogp::overlay;Theme theme(font);
  ImGui::PushStyleColor(ImGuiCol_PopupBg,card);ImGui::PushStyleColor(ImGuiCol_HeaderHovered,ImVec4(.30f,.23f,.13f,1));ImGui::PushStyleColor(ImGuiCol_HeaderActive,ImVec4(.42f,.28f,.10f,1));
  ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(.125f,.14f,.175f,1));
  ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(.125f,.14f,.175f,1));
  ImGui::PushStyleColor(ImGuiCol_NavCursor,orange);
  const auto& io=ImGui::GetIO();const ImVec2 size(std::min(940.f,std::max(280.f,io.DisplaySize.x-32)),std::min(730.f,std::max(280.f,io.DisplaySize.y-32)));
  ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x*.5f,io.DisplaySize.y*.5f),ImGuiCond_Always,ImVec2(.5f,.5f));ImGui::SetNextWindowSize(size,ImGuiCond_Always);
  bool close=false;const bool was_capturing=!view.capture.empty();
  if(ImGui::Begin("##motogp_settings",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoMove)){
    ImGui::TextColored(orange,"MotoGP 09/10");if(heading)ImGui::PushFont(heading,30.f);
    ImGui::TextUnformatted(Tr(tr,"Settings","Ayarlar"));if(heading)ImGui::PopFont();
    ImGui::TextWrapped("%s",Tr(tr,"Changes apply immediately. Save to keep them in the launcher.","Değişiklikler anında uygulanır. Launcher'da da tutmak için kaydet."));
    ImGui::TextDisabled("%s",Tr(tr,"The race continues. Pause before adjusting settings.","Yarış devam eder. Ayarlamadan önce oyunu duraklat."));
    ImGui::Spacing();
    const char* en[]={"General","Graphics","Advanced","Audio","Controls"};const char* tu[]={"Genel","Grafik","Gelişmiş","Ses","Kontroller"};
    // A wrapping navigation row also fits 640x480 without horizontal scrolling.
    for(int i=0;i<5;++i){const char* label=tr?tu[i]:en[i];float w=ImGui::CalcTextSize(label).x+26;if(i&&ImGui::GetContentRegionAvail().x>w+12)ImGui::SameLine();
      bool selected=int(view.page)==i;if(selected)ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(.32f,.23f,.11f,1));
      if(ImGui::Button(label)){view.page=Page(i);view.capture.clear();}if(selected)ImGui::PopStyleColor();
    }
    ImGui::BeginChild("##settings_list",ImVec2(0,-130),ImGuiChildFlags_Borders);
    if(view.page==Page::Controls)ImGui::TextWrapped("%s",Tr(tr,"Click Change key, or edit alternatives separated by commas and press Enter. Right-click Change key to unbind or reset.","Tuş değiştir'e bas veya virgülle ayrılmış alternatifleri düzenleyip Enter'a bas. Kaldırmak / sıfırlamak için Tuş değiştir'e sağ tıkla."));
    if(view.page==Page::Controls){
      ImGui::TextWrapped("%s",Tr(tr,"DualSense / Edge + native SDL required; USB or Bluetooth. Static resistance profiles. Physical hardware testing is pending.","DualSense / Edge + yerel SDL gerekir; USB veya Bluetooth. Sabit direnç profilleri. Fiziksel donanım testi henüz yapılmadı."));
      if(!adaptive::LiveAvailable())ImGui::TextWrapped("%s",Tr(tr,"Select the SDL controller backend in the launcher to enable live DualSense controls.","Canlı DualSense ayarları için launcher’dan SDL kontrolcü arka ucunu seç."));
      ImGui::TextWrapped("%s",Tr(tr,"Trigger resistance stays fixed during play; it does not follow speed, engine RPM or braking load.","Tetik direnci oyun sırasında sabit kalır; hız, motor devri veya fren yükünü takip etmez."));
      ImGui::TextWrapped("%s",adaptive::Status(tr).c_str());ImGui::Separator();
    }
    if(view.page==Page::Graphics&&(model.Value("present_effect")=="fsr2"||model.Value("present_effect")=="fsr3")){
      ImGui::TextWrapped("%s",Tr(tr,"Experimental FSR 2/3: full temporal reconstruction is incomplete. No frame generation or scene render-cost reduction.","Deneysel FSR 2/3: tam zamansal yeniden oluşturma eksik. Kare üretimi veya sahne çizim yükü azalması sağlamaz."));ImGui::Separator();
    }
    for(const auto& s:Catalog())if(s.page==view.page&&model.Available(s)){
      const auto effect=model.Value("present_effect");
      if(s.key=="present_fsr_quality_mode"&&effect!="fsr2"&&effect!="fsr3")continue;
      if(s.key=="present_fsr_sharpness_reduction"&&effect!="fsr"&&effect!="fsr2"&&effect!="fsr3")continue;
      if(s.key=="present_cas_additional_sharpness"&&effect!="cas")continue;
      const bool trigger=s.key.starts_with("dualsense_")&&s.key!="dualsense_adaptive_triggers";
      const auto trigger_mode=trigger?model.Value(s.key.starts_with("dualsense_rt_")?"dualsense_rt_mode":"dualsense_lt_mode"):std::string{};
      const bool inactive=trigger&&(model.Value("dualsense_adaptive_triggers")!="true"||(!s.key.ends_with("_mode")&&(trigger_mode=="off"||(s.key.ends_with("_end")&&trigger_mode!="weapon"))));
      ImGui::BeginDisabled(inactive);Widget(model,view,s,tr);ImGui::EndDisabled();
    }
    ImGui::EndChild();
    ImGui::BeginChild("##settings_status",ImVec2(0,42),ImGuiChildFlags_None,ImGuiWindowFlags_NoBackground);
    if(!model.error().empty())ImGui::TextWrapped("%s",model.error().c_str());
    else if(model.Unsaved())ImGui::TextColored(orange,"%s: %zu",Tr(tr,"Unsaved changes","Kaydedilmemiş değişiklik"),model.Unsaved());
    else ImGui::TextDisabled("%s",Tr(tr,view.saved?"Saved. Launcher settings are up to date.":"All changes apply while you play.",view.saved?"Kaydedildi. Launcher ayarları güncel.":"Tüm değişiklikler oyun çalışırken uygulanır."));
    ImGui::EndChild();
    if(ImGui::Button(Tr(tr,"Save settings","Ayarları kaydet")))view.saved=model.Save();
    ImGui::SameLine();if(ImGui::Button(Tr(tr,"Revert unsaved","Kaydedilmeyenleri geri al"))){if(model.Revert()){view.edits.clear();view.saved=false;}}
    if(ImGui::GetContentRegionAvail().x>110)ImGui::SameLine();
    close=ImGui::Button(Tr(tr,"Return","Oyuna dön"));
    auto bind=rex::cvar::Query<std::string>("bind_settings");if(bind.empty())bind="F1";
    ImGui::TextDisabled(Tr(tr,"%s / Esc to return  ·  Live settings only","Oyuna dönmek için %s / Esc  ·  Yalnızca canlı ayarlar"),bind.c_str());
    const bool popup=ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId);
    if(!was_capturing&&!popup&&ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)&&ImGui::IsKeyPressed(ImGuiKey_Escape,false))close=true;
    Capture(model,view);
  }
  ImGui::End();ImGui::PopStyleColor(6);return close;
}
namespace {
std::atomic<InputGate*> guest_gate{nullptr};
class Menu final:public rex::ui::ImGuiDialog,public rex::ui::WindowInputListener {
 public:
  Menu(rex::ui::ImGuiDrawer* drawer,std::shared_ptr<Model> model,std::shared_ptr<InputGate> gate,ImFont* font,ImFont* heading,bool tr,rex::ui::Window* window,bool (*can_open)())
      :ImGuiDialog(drawer),model_(std::move(model)),gate_(std::move(gate)),font_(font),heading_(heading),tr_(tr),window_(window),can_open_(can_open){
    // Migrate the SDK's old F4 default, including an explicitly saved F4;
    // preserve a different user-defined shortcut. The SDK callback shares this
    // cvar, so F4 no longer opens its other settings dialog either.
    const auto binding=rex::cvar::Query<std::string>("bind_settings");
    if(binding.empty()||binding=="F4")rex::cvar::SetFlagByName("bind_settings","F1");
    if(window_)window_->AddInputListener(this,63);
  }
  ~Menu()override{if(window_)window_->RemoveInputListener(this);gate_->Close();if(nav_owned_){auto& io=GetIO();ClearPad(io);io.ConfigFlags=old_flags_;io.BackendFlags=old_backend_;}}
  void OnKeyDown(rex::ui::KeyEvent& e)override{
    const auto bind=rex::ui::ParseVirtualKey(rex::cvar::Query<std::string>("bind_settings"));
    if(bind!=rex::ui::VirtualKey::kNone&&e.virtual_key()==bind&&!e.is_alt_pressed()){
      if(!e.prev_state()){if(gate_->IsOpen())gate_->Close();else if(!can_open_||can_open_()){gate_->Open();view_.capture.clear();view_.edits.clear();}}
      e.set_handled(true);return;
    }
    // ImGui (priority 64) already received this event. Don't also enqueue a
    // key-down for the virtual controller or open a second SDK overlay.
    if(gate_->IsOpen()&&!(e.is_alt_pressed()&&e.virtual_key()==rex::ui::VirtualKey::kF4))e.set_handled(true);
  }
  // Key/mouse UP events still reach MnK so pre-opening held keys cannot stick.
  void OnKeyChar(rex::ui::KeyEvent& e)override{if(gate_->IsOpen())e.set_handled(true);}
  void OnMouseDown(rex::ui::MouseEvent& e)override{if(gate_->IsOpen())e.set_handled(true);}
 protected:
  void OnDraw(ImGuiIO& io)override{
    if(!gate_->IsOpen()){if(nav_owned_){ClearPad(io);io.ConfigFlags=old_flags_;io.BackendFlags=old_backend_;nav_owned_=false;}return;}
    if(!nav_owned_){old_flags_=io.ConfigFlags;old_backend_=io.BackendFlags;io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard|ImGuiConfigFlags_NavEnableGamepad;io.BackendFlags|=ImGuiBackendFlags_HasGamepad;nav_owned_=true;ImGui::SetNextWindowFocus();}
    if(epoch_!=gate_->Epoch()){epoch_=gate_->Epoch();armed_=false;previous_=0;ClearPad(io);}
    const auto buttons=gate_->Buttons();if(!armed_&&gate_->NeutralReady()&&!buttons)armed_=true;
    const auto value=armed_?buttons:0;
    using namespace rex::input;
    const std::array<std::pair<uint16_t,ImGuiKey>,6> keys={{{X_INPUT_GAMEPAD_DPAD_UP,ImGuiKey_GamepadDpadUp},{X_INPUT_GAMEPAD_DPAD_DOWN,ImGuiKey_GamepadDpadDown},{X_INPUT_GAMEPAD_DPAD_LEFT,ImGuiKey_GamepadDpadLeft},{X_INPUT_GAMEPAD_DPAD_RIGHT,ImGuiKey_GamepadDpadRight},{X_INPUT_GAMEPAD_A,ImGuiKey_GamepadFaceDown},{X_INPUT_GAMEPAD_B,ImGuiKey_GamepadFaceRight}}};
    for(auto [bit,key]:keys)if((value&bit)!=(previous_&bit))io.AddKeyEvent(key,bool(value&bit));
    const bool back=(value&X_INPUT_GAMEPAD_B)&&!(previous_&X_INPUT_GAMEPAD_B);previous_=value;
    const bool was_popup=ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId);
    if(DrawMenu(*model_,view_,tr_,font_,heading_)||(back&&view_.capture.empty()&&!was_popup)){view_.capture.clear();gate_->Close();}
  }
 private:
  void ClearPad(ImGuiIO& io){for(auto key:{ImGuiKey_GamepadDpadUp,ImGuiKey_GamepadDpadDown,ImGuiKey_GamepadDpadLeft,ImGuiKey_GamepadDpadRight,ImGuiKey_GamepadFaceDown,ImGuiKey_GamepadFaceRight})io.AddKeyEvent(key,false);previous_=0;}
  std::shared_ptr<Model> model_;std::shared_ptr<InputGate> gate_;View view_;
  ImFont* font_;ImFont* heading_;bool tr_;rex::ui::Window* window_;bool (*can_open_)();bool nav_owned_=false,armed_=false;
  ImGuiConfigFlags old_flags_=0;ImGuiBackendFlags old_backend_=0;uint64_t epoch_=0;uint16_t previous_=0;
};
}
void SetGuestGate(InputGate* gate){guest_gate.store(gate,std::memory_order_release);}
void PollGuestInput(){if(auto* gate=guest_gate.load(std::memory_order_acquire))gate->PollGuest();}
bool GuestInputBlocked(){auto* gate=guest_gate.load(std::memory_order_acquire);return gate&&gate->Blocking();}
std::unique_ptr<rex::ui::ImGuiDialog> CreateMenu(rex::ui::ImGuiDrawer* drawer,std::shared_ptr<Model> model,std::shared_ptr<InputGate> gate,ImFont* font,ImFont* heading,bool turkish,rex::ui::Window* window,bool (*can_open)()){return std::make_unique<Menu>(drawer,std::move(model),std::move(gate),font,heading,turkish,window,can_open);}
}
