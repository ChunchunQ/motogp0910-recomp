// Actual Windows DLL, CVar, presenter, input, persistence and ImGui regression.
#include "motogp0910_init.h"
#include "../src/in_game_settings.h"
#include "../src/adaptive_triggers.h"
#include "../src/audio_settings.h"
#include "../src/button_prompt_options.h"
#include "../src/engine_settings_options.h"
#include "../src/frame_pacing.h"
#include "../src/launcher/core.h"
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/input/input_system.h>
#include <rex/input/mnk/mnk_input_driver.h>
#include <rex/audio/downmix.h>
#include <rex/ui/window.h>
#include <rex/ui/windowed_app_context.h>
#include <rex/ui/window_listener.h>
#include <rex/ui/keybinds.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <fstream>
#include <iostream>
#include <deque>
#include <set>
using namespace motogp::settings;
using namespace rex::input;
using rex::X_RESULT;using rex::X_STATUS;
REX_EXTERN(__imp__XamInputGetKeystrokeEx);
REX_EXTERN(__imp__XamInputGetState);
namespace {
int checks=0;
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);++checks;}
class UIContext final:public rex::ui::WindowedAppContext {void NotifyUILoopOfPendingFunctions()override{}void PlatformQuitFromUIThread()override{}};
class Driver final:public InputDriver {
 public:
  Driver():InputDriver(nullptr,0){}
  X_INPUT_STATE state{};std::deque<X_INPUT_KEYSTROKE> queue;
  X_STATUS Setup()override{return X_STATUS_SUCCESS;}
  void EnumerateDevices(std::vector<DeviceInfo>& out)override{DeviceInfo info{};info.id=static_cast<DeviceId>(1);out.push_back(info);}
  X_RESULT GetDeviceState(DeviceId,X_INPUT_STATE* out)override{*out=state;return X_ERROR_SUCCESS;}
  X_RESULT GetDeviceCapabilities(DeviceId,uint32_t,X_INPUT_CAPABILITIES* out)override{*out={};return X_ERROR_SUCCESS;}
  X_RESULT SetDeviceVibration(DeviceId,X_INPUT_VIBRATION*)override{return X_ERROR_SUCCESS;}
  X_RESULT GetDeviceKeystroke(DeviceId,uint32_t,X_INPUT_KEYSTROKE* out)override{if(queue.empty())return X_ERROR_EMPTY;*out=queue.front();queue.pop_front();return X_ERROR_SUCCESS;}
};
struct Dimensions:rex::ui::Surface {
  TypeIndex GetType()const override{return kTypeIndex_Win32Hwnd;}
  bool GetSizeImpl(uint32_t& w,uint32_t& h)const override{w=1920;h=1080;return true;}
};
struct PaintPlanner:rex::ui::Presenter {
  PaintPlanner():Presenter({}){}~PaintPlanner()override{SetWindowSurfaceFromUIThread(nullptr,nullptr);}
  using Presenter::GuestOutputProperties;using Presenter::GetGuestOutputPaintFlow;using Presenter::GuestOutputPaintEffect;
  rex::ui::Surface::TypeFlags GetSupportedSurfaceTypes()const override{return rex::ui::Surface::kTypeFlag_Win32Hwnd;}
  bool CaptureGuestOutput(rex::ui::RawImage&)override{return false;}
  SurfacePaintConnectResult ConnectOrReconnectPaintingToSurfaceFromUIThread(rex::ui::Surface&,uint32_t,uint32_t,bool,bool& implicit)override{implicit=false;return SurfacePaintConnectResult::kSuccess;}
  void DisconnectPaintingFromSurfaceFromUIThreadImpl()override{}
  bool RefreshGuestOutputImpl(uint32_t,uint32_t,uint32_t,std::function<bool(GuestOutputRefreshContext&)>,bool&)override{throw std::runtime_error("GPU not part of this test");}
  PaintResult PaintAndPresentImpl(bool)override{throw std::runtime_error("GPU not part of this test");}
};
void Dump(const std::filesystem::path& stem) {
  unsigned char* pixels;int w,h;ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
  std::ofstream texture(stem.string()+".rgba",std::ios::binary);texture.write(reinterpret_cast<char*>(pixels),w*h*4);
  auto* data=ImGui::GetDrawData();std::ofstream out(stem.string()+".json");
  out<<"{\"size\":["<<data->DisplaySize.x<<','<<data->DisplaySize.y<<"],\"texture\":["<<w<<','<<h<<"],\"lists\":[";
  for(int l=0;l<data->CmdListsCount;++l) {
    if(l)out<<',';const auto* list=data->CmdLists[l];out<<"{\"v\":[";
    for(int i=0;i<list->VtxBuffer.Size;++i) {if(i)out<<',';auto& v=list->VtxBuffer[i];out<<'['<<v.pos.x<<','<<v.pos.y<<','<<v.uv.x<<','<<v.uv.y<<','<<v.col<<']';}
    out<<"],\"i\":[";for(int i=0;i<list->IdxBuffer.Size;++i) {if(i)out<<',';out<<list->IdxBuffer[i];}
    out<<"],\"cmd\":[";
    for(int i=0;i<list->CmdBuffer.Size;++i) {if(i)out<<',';auto& c=list->CmdBuffer[i];out<<'['<<c.ClipRect.x<<','<<c.ClipRect.y<<','<<c.ClipRect.z<<','<<c.ClipRect.w<<','<<c.IdxOffset<<','<<c.ElemCount<<','<<c.VtxOffset<<']';}
    out<<"]}";
  }out<<"]}";
}
}
int main(int argc,char** argv){try{
  Check(argc==4,"Usage: in_game_settings_probe GAME_ROOT FONT OUTPUT");
  const auto out=std::filesystem::path(argv[3]);std::filesystem::create_directories(out);
  const auto config_path=out/"config.json";
  {std::ofstream f(config_path);f<<motogp::config::Serialize(toml::parse("game_data_root = 'original-game'\nuser_data_root = 'original-saves'\nlan_enabled = true\nresolution_scale = 3\ncustom_option = 'preserve'\n[audio]\nmaster_volume = 73\n[launcher]\nlanguage='tr'\ncrash_monitor=false\n"),config_path);}
  Check(LoadLibraryW((launcher::ExecutableFolder()/L"rexgpu-xenos.dll").c_str())!=nullptr,"Load shipped GPU plugin and its real flags");
  Driver* pad=nullptr;rex::Runtime runtime(std::filesystem::canonical(argv[1]));rex::RuntimeConfig config{.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true};
  config.input_factory=[&](bool){auto input=std::make_unique<InputSystem>(nullptr);auto driver=std::make_unique<Driver>();pad=driver.get();input->AddDriver(std::move(driver));input->SetDeviceAssignment(std::make_unique<SlotAssignment>());return input;};
  Check(!XFAILED(runtime.Setup(PPCImageConfig,std::move(config)))&&!XFAILED(runtime.LoadXexImage("game:\\default.xex")),"Load actual runtime, patched game and input system");
  auto* input=static_cast<InputSystem*>(runtime.input_system());
  WatchMotogpAudioSettings();
  Check(rex::cvar::SetFlagByName("audio_master_volume","73"),"Launcher/CLI startup volume");
  rex::cvar::SetFlagByName("keybind_a","Space");rex::cvar::SetFlagByName("fps_limit","0");rex::cvar::SetFlagByName("mnk_mode","true");
  UIContext ctx;auto window=rex::ui::Window::Create(ctx,"Settings lifecycle probe");Dimensions dimensions;PaintPlanner planner;planner.SetWindowSurfaceFromUIThread(window.get(),&dimensions);using Paint=rex::ui::Presenter::GuestOutputPaintConfig;
  auto model=std::make_shared<Model>(config_path,&planner);
  Check(model->Value("audio_master_volume")=="73"&&!model->Unsaved(),"Menu opens with actual runtime values, not hardcoded defaults");
  Check(model->Set("audio_master_volume","37")&&rex::cvar::Query<int32_t>("audio_master_volume")==37,"Volume overrides startup immediately");
  Check(std::abs(rex::audio::GetOutputGain()-.37f)<.001f,"Actual stereo/surround audio gain updates, not only registry");
  Check(!model->Set("audio_master_volume","1000")&&!model->Set("fps_limit","NaN")&&!model->Set("fps_limit","60.5"),"Out-of-range and non-integral settings rejected");
  for(auto key:{"draw_resolution_scale_x","resolution_scale","d3d12_adapter","store_shaders","host_refresh_rate","display_mode","user_language","game_data_root","lan_enabled","input_backend","mnk_mode","anisotropic_override","swap_post_effect"}){
    auto before=rex::cvar::GetFlagByName(key);Check(!model->Set(key,"1")&&before==rex::cvar::GetFlagByName(key),"Startup-only/unverified controls absent and immutable through live menu");
  }
  for(auto key:{"vsync","present_letterbox","audio_mute"}){Check(model->Set(key,"false")&&!rex::cvar::Query<bool>(key),"Live Boolean off reaches actual registry");Check(model->Set(key,"true")&&rex::cvar::Query<bool>(key),"Live Boolean on reaches actual registry");}
  Check(model->Set("fps_limit","144")&&rex::cvar::Query<int32_t>("fps_limit")==144,"FPS limit applies without a restart");
  Check(model->Set("button_prompt_style","playstation")&&model->Set("button_prompt_style","keyboard")&&model->Set("button_prompt_style","auto"),"Prompt styles can switch live");
  for(auto key:{"engine_speed_blur","engine_depth_of_field","engine_dynamic_shadows"})Check(model->Set(key,"off")&&model->Set(key,"game"),"Native engine hook choices remain live");
  Check(model->Set("engine_shadow_distance","45"),"Shadow distance accepted live");
  const auto* effect=std::find_if(Catalog().begin(),Catalog().end(),[](const Spec& s){return s.key=="present_effect";}).operator->();
  PaintPlanner::GuestOutputProperties props;props.frontbuffer_width=1280;props.frontbuffer_height=720;props.display_aspect_ratio_x=16;props.display_aspect_ratio_y=9;props.is_8bpc=true;
  int index=0;for(const auto& c:effect->choices){Check(model->Set("present_effect",c.value),"Actual live upscale selection accepted");auto paint=planner.GetGuestOutputPaintConfigFromUIThread();Check(int(paint.GetEffect())==index++,"Presenter receives new effect immediately, not just CVar");
    auto flow=planner.GetGuestOutputPaintFlow(props,1920,1080,16384,16384,paint);Check(flow.effect_count>0,"Updated config drives actual SDK paint planning");}
  for(auto [value,mode]:std::array<std::pair<const char*,int>,6>{{{"auto",0},{"nativeaa",1},{"quality",2},{"balanced",3},{"performance",4},{"ultra_performance",5}}})Check(model->Set("present_fsr_quality_mode",value)&&int(planner.GetGuestOutputPaintConfigFromUIThread().GetFsrQualityMode())==mode,"FSR quality reaches actual live presenter");
  Check(model->Set("present_cas_additional_sharpness","0.7")&&std::abs(planner.GetGuestOutputPaintConfigFromUIThread().GetCasAdditionalSharpness()-.7f)<.001f,"CAS coefficient changes immediately");
  Check(model->Set("present_fsr_sharpness_reduction","0.8")&&std::abs(planner.GetGuestOutputPaintConfigFromUIThread().GetFsrSharpnessReduction()-.8f)<.001f,"FSR coefficient changes immediately");
  Check(model->Set("present_dither","true")&&planner.GetGuestOutputPaintConfigFromUIThread().GetDither(),"Dither directly applied to presenter");
  auto copy=planner.GetGuestOutputPaintConfigFromUIThread();auto previous=copy.GetEffect();Check(!PatchPaintConfig(copy,"present_effect","bogus")&&copy.GetEffect()==previous,"Invalid paint update does not corrupt config");
  {Model trigger_model(out/"trigger-settings.toml",&planner);const auto original=motogp::adaptive::ReadProfile();
    Check(trigger_model.Set("dualsense_adaptive_triggers","true")&&trigger_model.Set("dualsense_rt_strength","8"),"Adaptive trigger enable and strength apply live");
    Check(!trigger_model.Set("dualsense_rt_start","9")&&!trigger_model.Set("dualsense_rt_mode","vibration")&&!trigger_model.Set("dualsense_lt_strength","9"),"Invalid Weapon intervals, unsupported modes and excessive strength rejected");
    Check(trigger_model.Set("dualsense_rt_end","9")&&trigger_model.Set("dualsense_rt_start","8"),"Valid separated Weapon interval accepted");
    Check(trigger_model.Revert()&&motogp::adaptive::ReadProfile()==original&&!trigger_model.Unsaved(),"Revert restores whole profile without invalid intermediate ranges");
    Check(trigger_model.Set("dualsense_adaptive_triggers","true")&&trigger_model.Set("dualsense_rt_mode","feedback")&&trigger_model.Set("dualsense_rt_start","4")&&trigger_model.Set("dualsense_rt_end","7")&&trigger_model.Set("dualsense_rt_strength","6")&&trigger_model.Set("dualsense_lt_end","5")&&trigger_model.Set("dualsense_lt_start","1")&&trigger_model.Set("dualsense_lt_strength","7")&&trigger_model.Set("dualsense_lt_mode","weapon"),"All nine trigger controls accept independent live values");
    rex::cvar::SetFlagByName("input_backend","xinput");
    Check(!motogp::adaptive::LiveAvailable()&&!trigger_model.Set("dualsense_rt_strength","2"),"Live DualSense controls are unavailable under a backend that cannot apply them");rex::cvar::SetFlagByName("input_backend","sdl");
    const auto changed=motogp::adaptive::ReadProfile();Check(trigger_model.Save()&&launcher::Settings::Load(toml::parse_file((out/"trigger-settings.toml").string())).adaptive==changed,"F1 save writes all nine values for the launcher with correct types");
    Check(motogp::adaptive::WriteProfile(original),"Restore initial trigger profile without disturbing other live settings");
  }
  Check(model->Set("keybind_a","Ctrl+K,Space")&&!model->Set("keybind_b","Ctrl+")&&model->Set("keybind_guide",""),"Live binding alternatives and explicit unbind validated");
  for(const auto& b:launcher::kBindings)Check(ValidBinding(b.defaults)==launcher::ValidBinding(b.defaults),"Canonical launcher bindings accepted identically");
  {
    rex::input::mnk::MnkInputDriver keyboard(nullptr,0);Check(keyboard.Setup()==X_STATUS_SUCCESS,"Real MnK driver initializes");
    std::vector<DeviceInfo> devices;keyboard.EnumerateDevices(devices);Check(devices.size()==1,"Real keyboard device enumerates");
    auto key=[&](rex::ui::VirtualKey vk,bool down){rex::ui::KeyEvent e(nullptr,vk,1,false,false,false,false,false);if(down)keyboard.OnKeyDown(e);else keyboard.OnKeyUp(e);};
    auto buttons=[&]{X_INPUT_STATE s{};Check(keyboard.GetDeviceState(devices[0].id,&s)==0,"Real keyboard state query");return s.gamepad.buttons;};
    model->Set("keybind_a","Space");key(rex::ui::VirtualKey::kSpace,true);Check(buttons()&X_INPUT_GAMEPAD_A,"Original live key produces A");
    Check(model->Set("keybind_a","J")&&!(buttons()&X_INPUT_GAMEPAD_A),"Old held binding stops producing A immediately");key(rex::ui::VirtualKey::kSpace,false);
    key(rex::ui::VirtualKey::kJ,true);Check(buttons()&X_INPUT_GAMEPAD_A,"New binding works without recreating keyboard driver");key(rex::ui::VirtualKey::kJ,false);
    // The pinned SDK ignores releases while MnK is disabled. It is not safe
    // to expose that toggle as a live control until the driver is repaired.
    key(rex::ui::VirtualKey::kJ,true);rex::cvar::SetFlagByName("mnk_mode","false");key(rex::ui::VirtualKey::kJ,false);rex::cvar::SetFlagByName("mnk_mode","true");
    Check(buttons()&X_INPUT_GAMEPAD_A,"Confirm pinned SDK held-key limitation behind omitted keyboard-enable toggle");key(rex::ui::VirtualKey::kJ,false);
    model->Set("keybind_a","Ctrl+K,Space");
  }
  Check(model->Save()&&!model->Unsaved(),"Save clears only successfully persisted live changes");
  auto t=motogp::config::Load(config_path);auto loaded=launcher::Settings::Load(t);
  Check(loaded.volume==37&&loaded.fps_limit==144&&loaded.bindings[0]=="Ctrl+K,Space"&&loaded.effect=="fsr3","Launcher reads saved live values with correct JSON types");
  Check(t["launcher"]["language"].value_or(std::string{})=="tr"&&!t["launcher"]["crash_monitor"].value_or(true),"Live game saves preserve preferences in the shared launcher namespace");
  Check(launcher::Get(t,"game_data_root",std::string{})=="original-game"&&launcher::Get(t,"user_data_root",std::string{})=="original-saves"&&launcher::Get(t,"resolution_scale",int64_t{})==3&&launcher::Get(t,"lan_enabled",false)&&launcher::Get(t,"custom_option",std::string{})=="preserve","Unknown, restart-only, LAN and save paths preserved");
  Check(model->Set("audio_master_volume","41"),"Second live edit");
  {launcher::Config other(config_path);launcher::Put(other.table,"other_writer",std::string("keep"));other.Save();}
  Check(model->Save()&&launcher::Get(motogp::config::Load(config_path),"other_writer",std::string{})=="keep","Unrelated external edits merge without loss");
  Check(model->Set("audio_master_volume","51"),"Conflict candidate remains live");
  {launcher::Config other(config_path);launcher::Put(other.table,"audio_master_volume",62);other.Save();}
  Check(!model->Save()&&rex::cvar::Query<int32_t>("audio_master_volume")==51,"Conflicting external edit rejected without losing live value");
  Check(launcher::Get(motogp::config::Load(config_path),"audio_master_volume",int64_t{})==62,"Conflicting file was not overwritten");
  Check(model->Revert()&&!model->Unsaved(),"Revert restores last saved runtime values");
  auto lock_path=config_path;lock_path+=".tmp";{std::ofstream f(lock_path);f<<"unrelated";}
  Check(model->Set("fps_limit","60")&&!model->Save()&&std::filesystem::file_size(lock_path)==9,"Existing temporary file cannot be clobbered");std::filesystem::remove(lock_path);model->Revert();
  Model no_presenter(out/"headless.toml",nullptr);Check(!no_presenter.Set("present_effect","cas")&&no_presenter.Set("fps_limit","120"),"Headless runtimes do not advertise nonfunctional image controls");
  auto gate=std::make_shared<InputGate>();gate->Initialize(input);gate->SetFocused(true);SetGuestGate(gate.get());motogp::SetFrameObserver(PollGuestInput);
  rex::cvar::SetFlagByName("mnk_mouse","true");pad->state.gamepad.buttons=X_INPUT_GAMEPAD_A;gate->Open();Check(!rex::cvar::Query<bool>("mnk_mouse"),"Mouse-look capture is temporarily released");X_INPUT_STATE state{};Check(input->GetState(0,&state)==0&&!state.gamepad.buttons,"Open blocks gameplay immediately before next guest frame");
  PPCContext call{};call.r3.u32=0;call.r4.u32=0;call.r5.u32=runtime.memory()->SystemHeapAlloc(64);__imp__XamInputGetKeystrokeEx(call,runtime.virtual_membase());Check(call.r3.u32==X_ERROR_EMPTY,"Actual imported keystroke wrapper blocks overlay input");
  X_INPUT_KEYSTROKE event{};pad->queue.push_back(event);motogp::PaceGameFrame();Check(gate->Buttons()&X_INPUT_GAMEPAD_A&&pad->queue.empty(),"Guest frame samples UI and drains stale keystrokes");
  gate->Close();motogp::PaceGameFrame();Check(gate->Blocking(),"Closing while A is held cannot leak confirm into game");
  gate->Open();gate->Close();pad->state.gamepad.buttons=0;motogp::PaceGameFrame();Check(!gate->Blocking(),"Reopen/close balances exactly one blocker");Check(rex::cvar::Query<bool>("mnk_mouse"),"Mouse-look preference restored after close");
  pad->state.gamepad.buttons=X_INPUT_GAMEPAD_A;input->GetState(0,&state);Check(state.gamepad.buttons&X_INPUT_GAMEPAD_A,"Fresh gameplay input resumes after release");pad->state.gamepad.buttons=0;
  gate->Open();gate->SetFocused(false);motogp::PaceGameFrame();Check(!gate->Buttons(),"Unfocused menu cannot act on gamepad input");gate->SetFocused(true);Check(!gate->NeutralReady(),"UI-first focus regain cannot arm before a fresh guest sample");pad->state.gamepad.buttons=X_INPUT_GAMEPAD_A;motogp::PaceGameFrame();Check(!gate->NeutralReady(),"Held A on focus regain stays disarmed");pad->state.gamepad.buttons=0;motogp::PaceGameFrame();Check(gate->NeutralReady(),"Fresh neutral sample rearms gamepad navigation");gate->Close();motogp::PaceGameFrame();
  {
    PPCContext state_call{};state_call.r3.u32=0;state_call.r4.u32=0;state_call.r5.u32=runtime.memory()->SystemHeapAlloc(sizeof(X_INPUT_STATE));
    pad->state.gamepad.buttons=X_INPUT_GAMEPAD_X;__imp__XamInputGetState(state_call,runtime.virtual_membase());
    Check(state_call.r3.u32==0&&rex::memory::load_and_swap<uint16_t>(runtime.virtual_membase()+state_call.r5.u32+4)==X_INPUT_GAMEPAD_X,"Actual synchronized GetState import preserves original pad output");
    pad->state.gamepad.buttons=0;
  }
  // Closed original export still validates its arguments (not our empty result).
  call={};call.r5.u32=0;__imp__XamInputGetKeystrokeEx(call,runtime.virtual_membase());Check(call.r3.u32==X_ERROR_BAD_ARGUMENTS,"Original runtime keystroke export restored when closed");
  rex::ui::ImGuiDrawer drawer(window.get(),64);auto& io=drawer.GetIO();
  io.IniFilename=nullptr;io.LogFilename=nullptr;io.DeltaTime=1.f/60;static const ImWchar ranges[]={0x20,0x1ff,0};
  auto* font=io.Fonts->AddFontFromFileTTF(argv[2],17,nullptr,ranges);auto* heading=io.Fonts->AddFontFromFileTTF(argv[2],30,nullptr,ranges);Check(font&&heading,"Actual packaged fonts load");
  unsigned char* pixels;int width,height;io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);io.Fonts->SetTexID(1);
  View view;model->Set("present_effect","fsr");model->Set("fps_limit","0");
  for(bool tr:{false,true})for(ImVec2 size:{ImVec2(1280,720),ImVec2(1920,1080),ImVec2(2560,1080),ImVec2(640,480)})for(int page=0;page<5;++page){
    io.DisplaySize=size;view.page=Page(page);for(int frame=0;frame<2;++frame){ImGui::NewFrame();Check(!DrawMenu(*model,view,tr,font,heading),"Rendering settings never changes state or closes spontaneously");ImGui::Render();}
    auto* win=ImGui::FindWindowByName("##motogp_settings");Check(win&&win->Pos.x>=0&&win->Pos.y>=0&&win->Pos.x+win->Size.x<=size.x+1&&win->Pos.y+win->Size.y<=size.y+1,"All pages stay within viewport");
    Check(win->ContentSize.y<=win->InnerRect.GetHeight()+1,"Header, navigation and footer do not overlap or clip");
    auto* context=ImGui::GetCurrentContext();Check(context->ColorStack.Size==0&&context->StyleVarStack.Size==0&&context->FontStack.Size==0,"No style/font stack leaks into achievements or SDK UI");
    if(size.x==1280)Dump(out/((tr?"settings-turkish-":"settings-english-")+std::to_string(page)));
  }
  view.page=Page::Controls;view.capture="keybind_a";view.capture_wait=false;io.DisplaySize=ImVec2(1280,720);
  io.AddKeyEvent(ImGuiKey_J,true);ImGui::NewFrame();Check(!DrawMenu(*model,view,false,font,heading),"Key capture does not close menu");ImGui::Render();Check(model->Value("keybind_a")=="J"&&view.capture.empty(),"Production key capture writes current binding");
  io.AddKeyEvent(ImGuiKey_J,false);ImGui::NewFrame();DrawMenu(*model,view,false,font,heading);ImGui::Render();
  view.capture="keybind_a";view.capture_wait=false;io.AddKeyEvent(ImGuiKey_Escape,true);ImGui::NewFrame();Check(!DrawMenu(*model,view,false,font,heading),"Esc cancels capture rather than closing menu");ImGui::Render();Check(view.capture.empty()&&model->Value("keybind_a")=="J","Canceled capture leaves binding unchanged");
  io.AddKeyEvent(ImGuiKey_Escape,false);ImGui::NewFrame();DrawMenu(*model,view,false,font,heading);ImGui::Render();
  rex::ui::RegisterBind("bind_settings","F4","SDK settings binding fixture",[]{});
  io.DisplaySize=ImVec2(1280,720);auto menu=CreateMenu(&drawer,model,gate,font,heading,false,window.get());auto* listener=dynamic_cast<rex::ui::WindowInputListener*>(menu.get());Check(listener,"Production menu installs keyboard interception");
  auto frame=[&]{motogp::PaceGameFrame();drawer.GetIO();ImGui::NewFrame();menu->Draw();ImGui::Render();};
  auto key=[&](rex::ui::VirtualKey key){rex::ui::KeyEvent e(window.get(),key,1,false,false,false,false,false);listener->OnKeyDown(e);return e.is_handled();};
  Check(rex::cvar::Query<std::string>("bind_settings")=="F1"&&!key(rex::ui::VirtualKey::kF4),"F4 migrates to F1 and no longer opens either settings overlay");
  Check(key(rex::ui::VirtualKey::kF1)&&gate->IsOpen(),"F1 opens custom menu before SDK default handler");frame();frame();
  Check(key(rex::ui::VirtualKey::kK),"Editing keys cannot also reach virtual controller");
  io.AddKeyEvent(ImGuiKey_Escape,true);frame();Check(!gate->IsOpen(),"Escape returns to game without deleting app-owned dialog");io.AddKeyEvent(ImGuiKey_Escape,false);frame();motogp::PaceGameFrame();
  Check(key(rex::ui::VirtualKey::kF1)&&gate->IsOpen(),"F1 reopens safely");frame();Check(key(rex::ui::VirtualKey::kF1)&&!gate->IsOpen(),"F1 toggles closed without duplicate binding registration");frame();motogp::PaceGameFrame();
  rex::cvar::SetFlagByName("bind_settings","F8");Check(!key(rex::ui::VirtualKey::kF1)&&key(rex::ui::VirtualKey::kF8)&&gate->IsOpen(),"Existing user-remapped settings binding is preserved");
  frame();frame();frame();Check(io.NavActive,"Actual ImGui navigation activates for keyboard/controller");
  pad->state.gamepad.buttons=X_INPUT_GAMEPAD_DPAD_DOWN;frame();frame();Check(ImGui::IsKeyDown(ImGuiKey_GamepadDpadDown),"Guest pad sample reaches actual ImGui navigation input");pad->state.gamepad.buttons=0;frame();frame();
  pad->state.gamepad.buttons=X_INPUT_GAMEPAD_B;frame();Check(!gate->IsOpen(),"Controller B closes settings");pad->state.gamepad.buttons=0;frame();
  menu.reset();motogp::PaceGameFrame();Check(!gate->Blocking(),"Dialog destruction balances input blocker");
  {
    auto blocked=CreateMenu(&drawer,model,gate,font,heading,false,window.get(),[]{return false;});
    auto* blocked_listener=dynamic_cast<rex::ui::WindowInputListener*>(blocked.get());
    rex::ui::KeyEvent e(window.get(),rex::ui::VirtualKey::kF8,1,false,false,false,false,false);blocked_listener->OnKeyDown(e);
    Check(e.is_handled()&&!gate->IsOpen(),"Pending exit modal prevents opening a second settings dialog or SDK fallback");
  }
  motogp::SetFrameObserver(nullptr);SetGuestGate(nullptr);rex::ui::UnregisterBind("bind_settings");
  std::cout<<checks<<" live settings, presenter, persistence, input and UI checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
