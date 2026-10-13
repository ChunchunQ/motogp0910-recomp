#pragma once

#include "button_prompt_types.h"
#include "shared_sdl.h"
#include <atomic>
#include <cmath>

namespace motogp::prompts {
// Observe meaningful input changes rather than polling held input every
// frame. A connected or drifting controller must not steal keyboard prompts.
class ActiveInput {
 public:
  static Style Classify(SDL_JoystickID id) {
    const auto type=motogp::shared_sdl::GetGamepadTypeForID(id);
    return type==SDL_GAMEPAD_TYPE_PS3 || type==SDL_GAMEPAD_TYPE_PS4 ||
        type==SDL_GAMEPAD_TYPE_PS5 || motogp::shared_sdl::GetGamepadVendorForID(id)==0x054C
        ?Style::PlayStation:Style::Xbox;
  }
  void Initialize(bool keyboard_enabled) {
    keyboard_enabled_=keyboard_enabled;
    int count=0;auto* pads=motogp::shared_sdl::GetGamepads(&count);
    if(count>0) SelectPad(pads[0]);
    else {pad_.store(0);style_.store(keyboard_enabled?Style::Keyboard:Style::Xbox);}
    motogp::shared_sdl::Free(pads);
  }
  Style style() const {return style_.load(std::memory_order_relaxed);}
  void SetKeyboardEnabled(bool enabled) {keyboard_enabled_.store(enabled,std::memory_order_relaxed);}
  void Keyboard() {
    if(!keyboard_enabled_.load(std::memory_order_relaxed)) return;
    pad_.store(0,std::memory_order_relaxed);style_.store(Style::Keyboard,std::memory_order_relaxed);
  }
  void Xbox() {pad_.store(0,std::memory_order_relaxed);style_.store(Style::Xbox,std::memory_order_relaxed);}
  void Observe(const SDL_Event& event,bool focused) {
    if(event.type==SDL_EVENT_GAMEPAD_REMOVED && event.gdevice.which==pad_.load(std::memory_order_relaxed)) {
      Initialize(keyboard_enabled_.load(std::memory_order_relaxed));return;
    }
    if(!focused) return;
    if(event.type==SDL_EVENT_GAMEPAD_BUTTON_DOWN) SelectPad(event.gbutton.which);
    else if(event.type==SDL_EVENT_GAMEPAD_AXIS_MOTION) {
      const bool trigger=event.gaxis.axis>=SDL_GAMEPAD_AXIS_LEFT_TRIGGER;
      const int threshold=trigger?3855:(event.gaxis.axis>=SDL_GAMEPAD_AXIS_RIGHTX?8689:7849);
      if(trigger?int(event.gaxis.value)>threshold:std::abs(int(event.gaxis.value))>threshold)
        SelectPad(event.gaxis.which);
    }
  }
 private:
  void SelectPad(SDL_JoystickID id) {
    pad_.store(id,std::memory_order_relaxed);style_.store(Classify(id),std::memory_order_relaxed);
  }
  std::atomic<Style> style_{Style::Keyboard};
  std::atomic<SDL_JoystickID> pad_{0};
  std::atomic<bool> keyboard_enabled_{true};
};
}
