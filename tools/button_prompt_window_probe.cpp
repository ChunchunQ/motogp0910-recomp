// Exercise the production observer with real SDK window callbacks and real
// SDL virtual gamepads. Include the implementation to inspect the private
// selection without adding a test/debug API to the shipped game executable.
#include "../src/button_prompts.cpp"
#include <rex/ui/window_sdl.h>
#include <rex/ui/windowed_app_context_sdl.h>
#include <iostream>
#include <stdexcept>

using namespace motogp::prompts;
int main() {
  int checks=0;
  auto require=[&](bool ok,const char* message) {
    if(!ok) throw std::runtime_error(message);
    ++checks;
  };
  try {
    rex::cvar::SetFlagByName("fullscreen","false");
    rex::cvar::SetFlagByName("mnk_mode","true");
    rex::cvar::SetFlagByName("mnk_mouse","false");
    rex::cvar::SetFlagByName("input_backend","sdl");
    rex::cvar::SetFlagByName("button_prompt_style","auto");
    rex::ui::SDLWindowedAppContext context;
    require(context.Initialize(),"SDK desktop initialization");
    auto window=rex::ui::Window::Create(context,"Button prompt transition probe");
    require(window && window->Open(),"Real SDK window opens");
    auto* native=static_cast<rex::ui::WindowSDL*>(window.get());
    auto focus=[&](bool value) {
      SDL_Event e{};e.type=value?SDL_EVENT_WINDOW_FOCUS_GAINED:SDL_EVENT_WINDOW_FOCUS_LOST;
      native->HandleWindowEvent(e);
    };
    auto key=[&](SDL_Scancode code,bool down=true) {
      SDL_Event e{};e.type=down?SDL_EVENT_KEY_DOWN:SDL_EVENT_KEY_UP;
      e.key.scancode=code;e.key.down=down;
      native->HandleKeyEvent(e);
    };
    auto mouse=[&](float dx) {
      SDL_Event e{};e.type=SDL_EVENT_MOUSE_MOTION;e.motion.xrel=dx;
      native->HandleMouseEvent(e);
    };
    require(motogp::shared_sdl::Init(SDL_INIT_GAMEPAD),"Initialize runtime gamepads");
    SDL_VirtualJoystickDesc desc{};SDL_INIT_INTERFACE(&desc);
    desc.type=SDL_JOYSTICK_TYPE_GAMEPAD;desc.naxes=6;desc.nbuttons=15;
    desc.axis_mask=0x3F;desc.button_mask=0x7FFF;
    desc.vendor_id=0x054C;desc.product_id=0x0CE6;desc.name="Window probe DualSense";
    const auto ps=motogp::shared_sdl::AttachVirtualJoystick(&desc);
    auto* ps_pad=motogp::shared_sdl::OpenGamepad(ps);
    require(ps && ps_pad,"Virtual Sony connected");
    desc.vendor_id=0x045E;desc.product_id=0x02EA;desc.name="Window probe Xbox";
    const auto xbox=motogp::shared_sdl::AttachVirtualJoystick(&desc);
    auto* xbox_pad=motogp::shared_sdl::OpenGamepad(xbox);
    require(xbox && xbox_pad,"Virtual Xbox connected");
    auto pump=[] {
      motogp::shared_sdl::UpdateJoysticks();
      SDL_Event e;while(motogp::shared_sdl::PollEvent(&e)){}
    };
    auto press=[&](SDL_Gamepad* pad) {
      auto* stick=motogp::shared_sdl::GetGamepadJoystick(pad);
      require(motogp::shared_sdl::SetJoystickVirtualButton(stick,SDL_GAMEPAD_BUTTON_SOUTH,false),"Virtual release");pump();
      require(motogp::shared_sdl::SetJoystickVirtualButton(stick,SDL_GAMEPAD_BUTTON_SOUTH,true),"Virtual press");pump();
    };
    StartInputObserver(window.get());
    struct Cleanup {~Cleanup(){StopInputObserver();}} cleanup;
    focus(true);pump();
    key(SDL_SCANCODE_SPACE);require(SelectedStyle()==Style::Keyboard,"Real keyboard listener selects keyboard");
    for(unsigned cycle=0;cycle<8;++cycle) {
      press(ps_pad);require(SelectedStyle()==Style::PlayStation,"Keyboard to Sony");
      press(xbox_pad);require(SelectedStyle()==Style::Xbox,"Sony to Xbox");
      key(SDL_SCANCODE_SPACE);require(SelectedStyle()==Style::Keyboard,"Xbox to keyboard");
      press(xbox_pad);require(SelectedStyle()==Style::Xbox,"Keyboard to Xbox");
      press(ps_pad);require(SelectedStyle()==Style::PlayStation,"Xbox to Sony");
      key(SDL_SCANCODE_SPACE);require(SelectedStyle()==Style::Keyboard,"Sony to keyboard");
      pump();require(SelectedStyle()==Style::Keyboard,"Held pads do not steal keyboard prompts");
    }
    std::cout<<"PASS: all six transitions repeated eight times through production callbacks\n";
    press(ps_pad);
    for(auto code:{SDL_SCANCODE_LSHIFT,SDL_SCANCODE_LCTRL,SDL_SCANCODE_LALT}) {
      key(code);require(SelectedStyle()==Style::PlayStation,"Bare modifier preserves controller style");
      key(code,false);require(SelectedStyle()==Style::PlayStation,"Key release preserves controller style");
    }
    mouse(15);require(SelectedStyle()==Style::PlayStation,"Disabled mouse does not steal prompts");
    rex::cvar::SetFlagByName("mnk_mouse","true");
    mouse(0);require(SelectedStyle()==Style::PlayStation,"Zero mouse delta ignored");
    mouse(15);require(SelectedStyle()==Style::Keyboard,"Enabled mouse selects keyboard");
    press(ps_pad);focus(false);
    key(SDL_SCANCODE_SPACE);mouse(15);press(xbox_pad);
    require(SelectedStyle()==Style::PlayStation,"Unfocused key/mouse/gamepad input ignored");
    focus(true);key(SDL_SCANCODE_SPACE);
    require(SelectedStyle()==Style::Keyboard,"Focus return permits keyboard transition");
    press(ps_pad);rex::cvar::SetFlagByName("mnk_mode","false");
    key(SDL_SCANCODE_SPACE);mouse(15);
    require(SelectedStyle()==Style::PlayStation,"Disabled keyboard input preserves pad prompts");
    rex::cvar::SetFlagByName("mnk_mode","true");
    for(auto entry:{std::pair{"xbox",Style::Xbox},std::pair{"playstation",Style::PlayStation},std::pair{"keyboard",Style::Keyboard}}) {
      rex::cvar::SetFlagByName("button_prompt_style",entry.first);
      press(ps_pad);require(SelectedStyle()==entry.second,"Fixed style ignores Sony input");
      press(xbox_pad);require(SelectedStyle()==entry.second,"Fixed style ignores Xbox input");
      key(SDL_SCANCODE_SPACE);require(SelectedStyle()==entry.second,"Fixed style ignores keyboard input");
    }
    rex::cvar::SetFlagByName("button_prompt_style","auto");
    require(SelectedStyle()==Style::Keyboard,"Automatic mode resumes latest input");
    StopInputObserver();press(ps_pad);key(SDL_SCANCODE_SPACE);
    require(SelectedStyle()==Style::Keyboard,"Shutdown removes gamepad and window observers");
    motogp::shared_sdl::CloseGamepad(ps_pad);motogp::shared_sdl::DetachVirtualJoystick(ps);
    motogp::shared_sdl::CloseGamepad(xbox_pad);motogp::shared_sdl::DetachVirtualJoystick(xbox);
    std::cout<<"Passed "<<checks<<" production window/input transition checks\n";
    return 0;
  } catch(const std::exception& e) {std::cerr<<"FAIL after "<<checks<<" checks: "<<e.what()<<'\n';return 1;}
}
