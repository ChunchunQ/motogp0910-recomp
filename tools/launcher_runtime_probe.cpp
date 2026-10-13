// CPU integration checks against the shipped DLL, not a GPU/gameplay test.
#include "../src/launcher/core.h"
#include "../src/audio_settings.h"
#include "../src/video_settings.h"
#include <rex/kernel/xboxkrnl/video.h>
#include <rex/audio/conversion.h>
#include <rex/input/mnk/mnk_input_driver.h>
#include <rex/ui/presenter.h>
#include <rex/ui/keybinds.h>
#include <rex/ui/windowed_app_context_sdl.h>
#include <iostream>
#include <bit>
#include <cmath>

using namespace launcher;
using rex::X_STATUS;
using rex::X_RESULT;
using namespace rex::input;
int checks=0;
void Check(bool pass,const char* text) {
  if(!pass) throw std::runtime_error(text);
  ++checks;
}
// The subclass exposes the SDK's real CPU pass planner; GPU entry points are
// deliberately unused. No shader execution is claimed by this probe.
struct Dimensions : rex::ui::Surface {
  uint32_t width=1920,height=1080;
  TypeIndex GetType() const override {return kTypeIndex_Win32Hwnd;}
  bool GetSizeImpl(uint32_t& w,uint32_t& h) const override {w=width;h=height;return true;}
};
struct PaintPlanner : rex::ui::Presenter {
  PaintPlanner():Presenter({}){}
  ~PaintPlanner() override {SetWindowSurfaceFromUIThread(nullptr,nullptr);}
  using Presenter::GuestOutputProperties;
  using Presenter::GuestOutputPaintEffect;
  using Presenter::GuestOutputPaintFlow;
  using Presenter::GetGuestOutputPaintFlow;
  using Presenter::FsrEasuConstants;
  using Presenter::FsrRcasConstants;
  rex::ui::Surface::TypeFlags GetSupportedSurfaceTypes() const override {return rex::ui::Surface::kTypeFlag_Win32Hwnd;}
  bool CaptureGuestOutput(rex::ui::RawImage&) override {return false;}
  SurfacePaintConnectResult ConnectOrReconnectPaintingToSurfaceFromUIThread(
      rex::ui::Surface&,uint32_t,uint32_t,bool,bool& implicit) override {implicit=false;return SurfacePaintConnectResult::kSuccess;}
  void DisconnectPaintingFromSurfaceFromUIThreadImpl() override {}
  bool RefreshGuestOutputImpl(uint32_t,uint32_t,uint32_t,
      std::function<bool(GuestOutputRefreshContext&)>,bool&) override {throw std::runtime_error("GPU not part of this test");}
  PaintResult PaintAndPresentImpl(bool) override {throw std::runtime_error("GPU not part of this test");}
};
int main() {
  try {
    auto gpu=LoadLibraryW((ExecutableFolder()/L"rexgpu-xenos.dll").c_str());
    Check(gpu!=nullptr,"Load matching GPU plugin");
    Check(rex::cvar::SetFlagByName("present_effect","fsr"),"DLL accepts FSR1 selection");
    Check(rex::cvar::SetFlagByName("present_fsr_sharpness_reduction","0.54"),"DLL accepts RCAS tuning");
    rex::ui::SDLWindowedAppContext context;Check(context.Initialize(),"Real SDL desktop initialized");
    auto window=rex::ui::Window::Create(context,"FSR CPU planner test",800,500);
    Check(window && window->Open(),"Real SDK host window opened");
    Dimensions dimensions;PaintPlanner planner;
    planner.SetWindowSurfaceFromUIThread(window.get(),&dimensions);
    PaintPlanner::GuestOutputProperties props;
    props.frontbuffer_width=1280;props.frontbuffer_height=720;
    props.display_aspect_ratio_x=16;props.display_aspect_ratio_y=9;props.is_8bpc=true;
    rex::cvar::SetFlagByName("present_letterbox","true");
    PaintPlanner::GuestOutputPaintConfig config;config.SetEffect(decltype(config)::Effect::kFsr);
    auto flow=planner.GetGuestOutputPaintFlow(props,1920,1080,16384,16384,config);
    Check(flow.effect_count==2,"720p to 1080p needs EASU and RCAS");
    Check(flow.effects[0]==PaintPlanner::GuestOutputPaintEffect::kFsrEasu &&
          flow.effects[1]==PaintPlanner::GuestOutputPaintEffect::kFsrRcas,"Actual FSR1 pass sequence");
    Check(flow.effect_output_sizes[0]==std::pair<uint32_t,uint32_t>{1920,1080},"FSR output dimensions");
    config.SetFsrSharpnessReduction(0.0f);
    PaintPlanner::FsrRcasConstants sharp;sharp.Initialize(flow,1,config);
    config.SetFsrSharpnessReduction(2.0f);
    PaintPlanner::FsrRcasConstants soft;soft.Initialize(flow,1,config);
    Check(std::abs(sharp.sharpness_post_setup-1.f)<0.0001f &&
          std::abs(soft.sharpness_post_setup-0.25f)<0.0001f,"Sharpness changes RCAS coefficient");
    config.SetEffect(decltype(config)::Effect::kBilinear);
    auto basic=planner.GetGuestOutputPaintFlow(props,1920,1080,16384,16384,config);
    Check(basic.effect_count==1 && basic.effects[0]==PaintPlanner::GuestOutputPaintEffect::kBilinear,"Bilinear fallback retained");
    config.SetEffect(decltype(config)::Effect::kFsr);
    dimensions.width=2560;planner.OnSurfaceResizeFromUIThread();
    auto ultrawide=planner.GetGuestOutputPaintFlow(props,2560,1080,16384,16384,config);
    Check(ultrawide.letterbox_clear_rectangle_count==2,"Ultrawide aspect ratio retained");
    rex::cvar::SetFlagByName("window_width","1920");rex::cvar::SetFlagByName("window_height","1080");
    ApplyMotogpVideoSettings();rex::system::X_VIDEO_MODE mode{};
    rex::kernel::xboxkrnl::VdQueryVideoMode(&mode);
    Check(mode.display_width==1280 && mode.display_height==720,"Direct EXE separates native source and 1080p output");
    rex::cvar::ResetToDefault("video_mode_width");rex::cvar::ResetToDefault("video_mode_height");
    rex::cvar::SetFlagFromCommandLine("video_mode_width","1280");rex::cvar::SetFlagFromCommandLine("video_mode_height","720");
    rex::kernel::xboxkrnl::VdQueryVideoMode(&mode);
    Check(mode.display_width==1280 && mode.display_height==720,"Explicit default-valued native mode respected");
    rex::cvar::SetFlagByName("video_mode_width","1600");rex::cvar::SetFlagByName("video_mode_height","900");
    ApplyMotogpVideoSettings();rex::kernel::xboxkrnl::VdQueryVideoMode(&mode);
    Check(mode.display_width==1600 && mode.display_height==900,"Custom guest mode preserved");
    rex::cvar::SetFlagByName("audio_master_volume","37");ApplyMotogpAudioSettings();WatchMotogpAudioSettings();
    Check(std::abs(rex::audio::GetOutputGain()-0.37f)<0.0001f,"Game volume controls real output gain");
    rex::cvar::SetFlagByName("audio_master_volume","0");
    Check(rex::audio::GetOutputGain()==0.f,"Runtime volume callback reaches silence");
    rex::cvar::SetFlagByName("audio_master_volume","50");
    std::array<float,24> input{};
    for(auto& sample:input) sample=std::bit_cast<float>(rex::byte_swap(std::bit_cast<uint32_t>(0.25f)));
    std::array<float,24> half{},unity{};
    rex::audio::conversion::sequential_6_BE_to_interleaved_6_LE(half.data(),input.data(),4,{},rex::audio::GetOutputGain());
    rex::audio::conversion::sequential_6_BE_to_interleaved_6_LE(unity.data(),input.data(),4,{},1.f);
    Check(half[0]==unity[0]*0.5f && half[23]==unity[23]*0.5f,"Surround samples actually attenuated");
    rex::cvar::SetFlagByName("mnk_mode","true");
    for(const auto& spec:kBindings) rex::cvar::SetFlagByName(spec.key,"");
    rex::cvar::SetFlagByName("keybind_a","Ctrl+K,Space");
    rex::cvar::SetFlagByName("keybind_dpad_up","Shift+Up");
    rex::input::mnk::MnkInputDriver driver(nullptr,0);
    Check(driver.Setup()==X_STATUS_SUCCESS,"Real keyboard driver setup");
    std::vector<rex::input::DeviceInfo> devices;driver.EnumerateDevices(devices);
    Check(devices.size()==1 && devices[0].synthetic,"Keyboard controller enumerated");
    auto key=[&](rex::ui::VirtualKey vk,bool down) {
      rex::ui::KeyEvent event(nullptr,vk,1,false,false,false,false,false);
      if(down) driver.OnKeyDown(event);else driver.OnKeyUp(event);
    };
    auto state=[&]() {
      rex::input::X_INPUT_STATE result{};
      Check(driver.GetDeviceState(devices[0].id,&result)==X_ERROR_SUCCESS,"Keyboard state read");return result;
    };
    using V=rex::ui::VirtualKey;
    key(V::kK,true);Check((state().gamepad.buttons & X_INPUT_GAMEPAD_A)==0,"Modifier required");
    key(V::kControl,true);Check((state().gamepad.buttons & X_INPUT_GAMEPAD_A)!=0,"Custom Ctrl+K reaches guest A");
    key(V::kK,false);key(V::kControl,false);key(V::kSpace,true);
    Check((state().gamepad.buttons & X_INPUT_GAMEPAD_A)!=0,"Alternative Space reaches guest A");
    key(V::kSpace,false);key(V::kUp,true);Check((state().gamepad.buttons & X_INPUT_GAMEPAD_DPAD_UP)==0,"D-pad modifier required");
    key(V::kShift,true);Check((state().gamepad.buttons & X_INPUT_GAMEPAD_DPAD_UP)!=0,"Shift+Up reaches guest D-pad");
    for(UINT vk=1;vk<256;++vk) {
      auto name=KeyName(vk);
      if(!name.empty()) Check(rex::ui::ParseVirtualKey(name)!=V::kNone,"Launcher key name accepted by SDK");
    }
    std::cout<<checks<<" runtime integration checks passed\n";return 0;
  }catch(const std::exception& error) {std::cerr<<"FAIL after "<<checks<<" checks: "<<error.what()<<'\n';return 1;}
}
