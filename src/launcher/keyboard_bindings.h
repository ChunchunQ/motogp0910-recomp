#pragma once
#include <windows.h>
#include "../keyboard_binding_specs.h"
#include <string>
#include <string_view>
#include <stdexcept>

namespace launcher {
// Canonical key names accepted by ReXGlue v0.10.0. Keep the parser case-sensitive
// like the SDK. Win32 VK codes match the SDK's VirtualKey enum.
inline std::string KeyName(UINT vk) {
  if(vk>='A' && vk<='Z') return std::string(1,static_cast<char>(vk));
  if(vk>='0' && vk<='9') return std::string(1,static_cast<char>(vk));
  if(vk>=VK_F1 && vk<=VK_F24) return "F"+std::to_string(vk-VK_F1+1);
  if(vk>=VK_NUMPAD0 && vk<=VK_NUMPAD9) return "Numpad"+std::to_string(vk-VK_NUMPAD0);
  switch(vk) {
    case VK_OEM_3: return "Backtick";
    case VK_OEM_MINUS: return "Minus";
    case VK_OEM_PLUS: return "Plus";
    case VK_OEM_COMMA: return "Comma";
    case VK_OEM_PERIOD: return "Period";
    case VK_OEM_1: return "Semicolon";
    case VK_OEM_2: return "Slash";
    case VK_OEM_5: return "Backslash";
    case VK_OEM_4: return "LBracket";
    case VK_OEM_6: return "RBracket";
    case VK_OEM_7: return "Quote";
    case VK_ESCAPE: return "Escape";
    case VK_RETURN: return "Return";
    case VK_SPACE: return "Space";
    case VK_TAB: return "Tab";
    case VK_BACK: return "Backspace";
    case VK_DELETE: return "Delete";
    case VK_INSERT: return "Insert";
    case VK_HOME: return "Home";
    case VK_END: return "End";
    case VK_PRIOR: return "PageUp";
    case VK_NEXT: return "PageDown";
    case VK_LEFT: return "Left";
    case VK_RIGHT: return "Right";
    case VK_UP: return "Up";
    case VK_DOWN: return "Down";
    case VK_SHIFT: return "Shift";
    case VK_CONTROL: return "Control";
    case VK_MENU: return "Alt";
    case VK_ADD: return "NumpadPlus";
    case VK_SUBTRACT: return "NumpadMinus";
    case VK_MULTIPLY: return "NumpadStar";
    case VK_DIVIDE: return "NumpadSlash";
    case VK_SNAPSHOT: return "PrintScreen";
    case VK_PAUSE: return "Pause";
    case VK_CAPITAL: return "CapsLock";
    case VK_NUMLOCK: return "NumLock";
    case VK_SCROLL: return "ScrollLock";
    case VK_LBUTTON: return "LMB";
    case VK_RBUTTON: return "RMB";
    case VK_MBUTTON: return "MMB";
    default: return {};
  }
}
inline bool KnownKey(std::string_view key) {
  if(key.empty()) return false;
  if(key=="NumpadEnter") return true;
  for(UINT vk=1;vk<256;++vk) if(KeyName(vk)==key) return true;
  return false;
}
inline std::string_view TrimBinding(std::string_view value) {
  while(!value.empty() && value.front()==' ') value.remove_prefix(1);
  while(!value.empty() && value.back()==' ') value.remove_suffix(1);
  return value;
}
inline bool ValidBinding(std::string_view value) {
  if(value.empty()) return true; // Explicitly unbound.
  for(;;) {
    auto comma=value.find(',');
    auto token=TrimBinding(value.substr(0,comma));
    if(token.empty()) return false;
    for(;;) {
      auto plus=token.find('+');
      if(plus==std::string_view::npos) break;
      auto prefix=token.substr(0,plus);
      if(prefix!="Shift" && prefix!="Ctrl" && prefix!="Control" && prefix!="Alt") return false;
      token.remove_prefix(plus+1);
    }
    // Bare modifiers cannot fire in the SDK because matching requires the
    // exact modifier mask; reject a mapping that would silently do nothing.
    if(!KnownKey(token) || token=="Shift" || token=="Control" || token=="Alt") return false;
    if(comma==std::string_view::npos) return true;
    value.remove_prefix(comma+1);
  }
}
} // namespace launcher
