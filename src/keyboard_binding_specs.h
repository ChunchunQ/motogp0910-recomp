#pragma once
#include <array>
namespace launcher {
struct BindingSpec { const char* key; const char* defaults; const wchar_t* english; const wchar_t* turkish; };
inline constexpr std::array<BindingSpec,25> kBindings{{
  {"keybind_a", "Semicolon,Space", L"A button", L"A / Onayla"},
  {"keybind_b", "Quote,Backspace", L"B button", L"B / Geri"},
  {"keybind_x", "L", L"X button", L"X düğmesi"},
  {"keybind_y", "P", L"Y button", L"Y düğmesi"},
  {"keybind_left_trigger", "Q,I", L"Left trigger", L"Sol tetik (LT)"},
  {"keybind_right_trigger", "E,O", L"Right trigger", L"Sağ tetik (RT)"},
  {"keybind_left_shoulder", "1", L"Left shoulder", L"Sol omuz (LB)"},
  {"keybind_right_shoulder", "3", L"Right shoulder", L"Sağ omuz (RB)"},
  {"keybind_lstick_up", "W", L"Left stick up", L"Sol çubuk: yukarı"},
  {"keybind_lstick_down", "S", L"Left stick down", L"Sol çubuk: aşağı"},
  {"keybind_lstick_left", "A", L"Left stick left", L"Sol çubuk: sol"},
  {"keybind_lstick_right", "D", L"Left stick right", L"Sol çubuk: sağ"},
  {"keybind_lstick_press", "F", L"Left stick press", L"Sol çubuk: bas"},
  {"keybind_rstick_up", "Up", L"Right stick up", L"Sağ çubuk: yukarı"},
  {"keybind_rstick_down", "Down", L"Right stick down", L"Sağ çubuk: aşağı"},
  {"keybind_rstick_left", "Left", L"Right stick left", L"Sağ çubuk: sol"},
  {"keybind_rstick_right", "Right", L"Right stick right", L"Sağ çubuk: sağ"},
  {"keybind_rstick_press", "K", L"Right stick press", L"Sağ çubuk: bas"},
  {"keybind_dpad_up", "Shift+Up", L"D-pad up", L"D-pad: yukarı"},
  {"keybind_dpad_down", "Shift+Down", L"D-pad down", L"D-pad: aşağı"},
  {"keybind_dpad_left", "Shift+Left", L"D-pad left", L"D-pad: sol"},
  {"keybind_dpad_right", "Shift+Right", L"D-pad right", L"D-pad: sağ"},
  {"keybind_back", "Z,Tab", L"Back button", L"Back / Seç"},
  {"keybind_start", "X,Return", L"Start button", L"Start / Duraklat"},
  {"keybind_guide", "", L"Guide button", L"Guide düğmesi"},
}};
}
