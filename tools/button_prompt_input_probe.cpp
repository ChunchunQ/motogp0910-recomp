#include "../src/button_prompt_input.h"
#include "../src/button_prompt_mapping.h"
#include <iostream>
#include <stdexcept>
#include <string>

using namespace motogp::prompts;
int main() {
  int checks=0;
  auto require=[&](bool condition,const char* message) {if(!condition)throw std::runtime_error(message);++checks;};
  try {
    require(PrimaryLabel("Semicolon,Space")=="Space","Default confirm binding");
    require(PrimaryLabel("Quote,Backspace")=="Backspace","Default back binding");
    require(PrimaryLabel("X,Return")=="Enter","Default pause binding");
    require(PrimaryLabel("Z,Tab")=="Tab","Default view binding");
    require(PrimaryLabel(" Ctrl+K , Space ")=="Ctrl+K","Custom primary binding");
    require(PrimaryLabel("InvalidKey,Shift+Up")=="Shift+Up","Skip unusable alternative");
    require(PrimaryLabel("Shift,Ctrl,Unknown")=="Unbound","Unusable modifier-only bindings");
    require(PrimaryLabel("")=="Unbound","Unbound action");
    require(FindGlyph('R')->bindings[0]==std::string("keybind_right_shoulder"),"Uppercase shoulder glyph");
    require(FindGlyph('r')->bindings[0]==std::string("keybind_right_trigger"),"Lowercase trigger glyph");
    require(FindGlyph('S')->bindings[0]==std::string("keybind_back"),"View/Create glyph");
    const auto arrows=[](const char* name) {
      const std::string flag=name;
      return flag.ends_with("_up")?std::string("Shift+Up"):flag.ends_with("_left")?std::string("Shift+Left"):
          flag.ends_with("_down")?std::string("Shift+Down"):std::string("Shift+Right");
    };
    require(LabelFor(*FindGlyph('P'),Style::Keyboard,arrows)=="Shift+Arrows","Compact actual D-pad bindings");
    require(LabelFor(*FindGlyph('T'),Style::Keyboard,[](const char* name) {
      const std::string flag=name;
      return flag.ends_with("_up")?std::string("Ctrl+I"):flag.ends_with("_left")?std::string("Ctrl+J"):
          flag.ends_with("_down")?std::string("Ctrl+K"):std::string("Ctrl+L");
    })=="Ctrl+IJKL","Compact custom movement bindings");
    require(motogp::shared_sdl::Init(SDL_INIT_GAMEPAD),motogp::shared_sdl::GetError());
#ifdef _WIN32
    HMODULE owner=nullptr;
    const auto& api=motogp::shared_sdl::Api();
    require(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(api.GetGamepads),&owner) && owner==GetModuleHandleW(L"rexruntime.dll"),
        "Gamepad enumeration belongs to the actual SDK runtime");
    require(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(api.AddEventWatch),&owner) && owner==GetModuleHandleW(L"rexruntime.dll"),
        "Event observer belongs to the actual SDK runtime");
#endif
    SDL_VirtualJoystickDesc desc{};SDL_INIT_INTERFACE(&desc);
    desc.type=SDL_JOYSTICK_TYPE_GAMEPAD;desc.naxes=6;desc.nbuttons=15;
    desc.axis_mask=0x3F;desc.button_mask=0x7FFF;
    desc.vendor_id=0x054C;desc.product_id=0x0CE6;desc.name="Prompt probe DualSense";
    const auto ps=motogp::shared_sdl::AttachVirtualJoystick(&desc);
    require(ps!=0,motogp::shared_sdl::GetError());
    auto* ps_pad=motogp::shared_sdl::OpenGamepad(ps);require(ps_pad!=nullptr,motogp::shared_sdl::GetError());
    auto* ps_stick=motogp::shared_sdl::GetGamepadJoystick(ps_pad);
    desc.vendor_id=0x045E;desc.product_id=0x02EA;desc.name="Prompt probe Xbox";
    const auto xbox=motogp::shared_sdl::AttachVirtualJoystick(&desc);
    require(xbox!=0,motogp::shared_sdl::GetError());
    auto* xbox_pad=motogp::shared_sdl::OpenGamepad(xbox);require(xbox_pad!=nullptr,motogp::shared_sdl::GetError());
    auto* xbox_stick=motogp::shared_sdl::GetGamepadJoystick(xbox_pad);
    ActiveInput tracker;tracker.Initialize(true);
    struct Watch {ActiveInput* tracker;std::atomic<bool> focused{true};} watch{&tracker};
    const SDL_EventFilter observer=[](void* context,SDL_Event* event) {
      auto& state=*static_cast<Watch*>(context);
      state.tracker->Observe(*event,state.focused.load());return true;
    };
    require(motogp::shared_sdl::AddEventWatch(observer,&watch),motogp::shared_sdl::GetError());
    struct Registration {
      SDL_EventFilter observer;void* context;bool active=true;
      void Stop(){if(active){motogp::shared_sdl::RemoveEventWatch(observer,context);active=false;}}
      ~Registration(){Stop();}
    } registration{observer,&watch};
    auto pump=[&](bool focused=true) {watch.focused.store(focused);motogp::shared_sdl::UpdateJoysticks();SDL_Event event;while(motogp::shared_sdl::PollEvent(&event)){};};
    pump();
    require(ActiveInput::Classify(ps)==Style::PlayStation,"Actual SDL Sony classification");
    require(ActiveInput::Classify(xbox)==Style::Xbox,"Actual SDL Xbox classification");
    tracker.Keyboard();require(tracker.style()==Style::Keyboard,"Keyboard takes priority over idle connected pads");
    require(motogp::shared_sdl::SetJoystickVirtualAxis(ps_stick,SDL_GAMEPAD_AXIS_LEFTX,500),motogp::shared_sdl::GetError());pump();
    require(tracker.style()==Style::Keyboard,"Ignore stick drift");
    require(motogp::shared_sdl::SetJoystickVirtualButton(ps_stick,SDL_GAMEPAD_BUTTON_SOUTH,true),motogp::shared_sdl::GetError());pump();
    require(tracker.style()==Style::PlayStation,"Sony press selects PlayStation");
    tracker.Keyboard();require(tracker.style()==Style::Keyboard,"Keyboard replaces held controller prompts");
    pump();require(tracker.style()==Style::Keyboard,"Held controller does not steal prompts back");
    require(motogp::shared_sdl::SetJoystickVirtualButton(xbox_stick,SDL_GAMEPAD_BUTTON_SOUTH,true),motogp::shared_sdl::GetError());pump();
    require(tracker.style()==Style::Xbox,"Most recently used Xbox selects Xbox");
    require(motogp::shared_sdl::SetJoystickVirtualButton(ps_stick,SDL_GAMEPAD_BUTTON_SOUTH,false),motogp::shared_sdl::GetError());pump();
    require(tracker.style()==Style::Xbox,"Sony release is not new input");
    require(motogp::shared_sdl::SetJoystickVirtualAxis(ps_stick,SDL_GAMEPAD_AXIS_LEFTX,15000),motogp::shared_sdl::GetError());pump();
    require(tracker.style()==Style::PlayStation,"Meaningful Sony stick input selects PlayStation");
    tracker.Keyboard();
    require(motogp::shared_sdl::SetJoystickVirtualAxis(xbox_stick,SDL_GAMEPAD_AXIS_RIGHTX,15000),motogp::shared_sdl::GetError());pump(false);
    require(tracker.style()==Style::Keyboard,"Unfocused game does not change prompts");
    require(motogp::shared_sdl::SetJoystickVirtualAxis(ps_stick,SDL_GAMEPAD_AXIS_LEFT_TRIGGER,5000),motogp::shared_sdl::GetError());pump();
    require(tracker.style()==Style::PlayStation,"Meaningful trigger selects PlayStation");
    for(const auto axis:{SDL_GAMEPAD_AXIS_LEFTX,SDL_GAMEPAD_AXIS_LEFTY,SDL_GAMEPAD_AXIS_RIGHTX,SDL_GAMEPAD_AXIS_RIGHTY}) {
      const int threshold=axis>=SDL_GAMEPAD_AXIS_RIGHTX?8689:7849;
      tracker.Keyboard();
      require(motogp::shared_sdl::SetJoystickVirtualAxis(ps_stick,axis,threshold),motogp::shared_sdl::GetError());pump();
      require(tracker.style()==Style::Keyboard,"Positive deadzone boundary preserves keyboard");
      require(motogp::shared_sdl::SetJoystickVirtualAxis(ps_stick,axis,threshold+1),motogp::shared_sdl::GetError());pump();
      require(tracker.style()==Style::PlayStation,"Positive input beyond boundary selects pad");
      tracker.Keyboard();
      require(motogp::shared_sdl::SetJoystickVirtualAxis(ps_stick,axis,-threshold),motogp::shared_sdl::GetError());pump();
      require(tracker.style()==Style::Keyboard,"Negative deadzone boundary preserves keyboard");
      require(motogp::shared_sdl::SetJoystickVirtualAxis(ps_stick,axis,-threshold-1),motogp::shared_sdl::GetError());pump();
      require(tracker.style()==Style::PlayStation,"Negative input beyond boundary selects pad");
    }
    tracker.Keyboard();
    desc.name="Prompt probe idle hotplug";
    const auto idle=motogp::shared_sdl::AttachVirtualJoystick(&desc);
    require(idle!=0,motogp::shared_sdl::GetError());
    auto* idle_pad=motogp::shared_sdl::OpenGamepad(idle);require(idle_pad!=nullptr,motogp::shared_sdl::GetError());pump();
    require(tracker.style()==Style::Keyboard,"Idle hotplug does not steal keyboard prompts");
    motogp::shared_sdl::CloseGamepad(idle_pad);
    require(motogp::shared_sdl::DetachVirtualJoystick(idle),motogp::shared_sdl::GetError());pump();
    require(tracker.style()==Style::Keyboard,"Removing inactive controller preserves keyboard");
    require(motogp::shared_sdl::SetJoystickVirtualButton(ps_stick,SDL_GAMEPAD_BUTTON_SOUTH,true),motogp::shared_sdl::GetError());pump();
    require(tracker.style()==Style::PlayStation,"Sony can reclaim prompts after hotplug");
    watch.focused.store(false);
    motogp::shared_sdl::CloseGamepad(ps_pad);require(motogp::shared_sdl::DetachVirtualJoystick(ps),motogp::shared_sdl::GetError());pump(false);
    require(tracker.style()==Style::Xbox,"Active removal selects remaining controller even while unfocused");
    motogp::shared_sdl::CloseGamepad(xbox_pad);require(motogp::shared_sdl::DetachVirtualJoystick(xbox),motogp::shared_sdl::GetError());pump(false);
    require(tracker.style()==Style::Keyboard,"Last controller removal selects keyboard");
    desc.vendor_id=0x054C;desc.product_id=0x0CE6;desc.name="Prompt probe reconnected Sony";
    const auto reconnected=motogp::shared_sdl::AttachVirtualJoystick(&desc);
    require(reconnected!=0,motogp::shared_sdl::GetError());
    auto* connected_pad=motogp::shared_sdl::OpenGamepad(reconnected);require(connected_pad!=nullptr,motogp::shared_sdl::GetError());pump();
    require(tracker.style()==Style::Keyboard,"Reconnection waits for meaningful input");
    require(motogp::shared_sdl::SetJoystickVirtualButton(motogp::shared_sdl::GetGamepadJoystick(connected_pad),SDL_GAMEPAD_BUTTON_SOUTH,true),motogp::shared_sdl::GetError());pump();
    require(tracker.style()==Style::PlayStation,"Reconnected Sony input selects PlayStation");
    tracker.SetKeyboardEnabled(false);tracker.Keyboard();
    require(tracker.style()==Style::PlayStation,"Disabling keyboard preserves active Sony");
    motogp::shared_sdl::CloseGamepad(connected_pad);
    require(motogp::shared_sdl::DetachVirtualJoystick(reconnected),motogp::shared_sdl::GetError());pump();
    require(tracker.style()==Style::Xbox,"No pads and disabled keyboard fall back to Xbox");
    tracker.Initialize(false);tracker.Keyboard();
    require(tracker.style()==Style::Xbox,"Disabled keyboard cannot select keyboard prompts");
    registration.Stop();
    motogp::shared_sdl::Quit();
    std::cout<<"Passed "<<checks<<" binding and real SDL virtual-controller checks\n";
    return 0;
  } catch(const std::exception& error) {
    std::cerr<<error.what()<<'\n';try{motogp::shared_sdl::Quit();}catch(...){};return 1;
  }
}
