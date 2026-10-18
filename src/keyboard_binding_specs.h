#pragma once
#include <array>
namespace launcher {
struct BindingSpec { const char* key; const char* defaults; const wchar_t* english; const wchar_t* turkish; };
inline constexpr std::array<BindingSpec,25> kBindings{{
  {"keybind_a", "Semicolon,Space", L"A button (A/Cross)", L"A / Onayla (A/Cross)"},
  {"keybind_b", "Quote,Backspace", L"B button (B/Circle)", L"B / Geri (B/Circle)"},
  {"keybind_x", "L", L"X button (X/Square)", L"X düğmesi (X/Square)"},
  {"keybind_y", "P", L"Y button (Y/Triangle)", L"Y düğmesi (Y/Triangle)"},
  {"keybind_left_trigger", "Q,I", L"Left trigger (LT/L2)", L"Sol tetik (LT/L2)"},
  {"keybind_right_trigger", "E,O", L"Right trigger (RT/R2)", L"Sağ tetik (RT/R2)"},
  {"keybind_left_shoulder", "1", L"Left shoulder (LB/L1)", L"Sol omuz (LB/L1)"},
  {"keybind_right_shoulder", "3", L"Right shoulder (RB/R1)", L"Sağ omuz (RB/R1)"},
  {"keybind_lstick_up", "W", L"Left stick up (LS/Left stick)", L"Sol çubuk: yukarı (LS/Left stick)"},
  {"keybind_lstick_down", "S", L"Left stick down (LS/Left stick)", L"Sol çubuk: aşağı (LS/Left stick)"},
  {"keybind_lstick_left", "A", L"Left stick left (LS/Left stick)", L"Sol çubuk: sol (LS/Left stick)"},
  {"keybind_lstick_right", "D", L"Left stick right (LS/Left stick)", L"Sol çubuk: sağ (LS/Left stick)"},
  {"keybind_lstick_press", "F", L"Left stick press (LS/L3)", L"Sol çubuk: bas (LS/L3)"},
  {"keybind_rstick_up", "Up", L"Right stick up (RS/Right stick)", L"Sağ çubuk: yukarı (RS/Right stick)"},
  {"keybind_rstick_down", "Down", L"Right stick down (RS/Right stick)", L"Sağ çubuk: aşağı (RS/Right stick)"},
  {"keybind_rstick_left", "Left", L"Right stick left (RS/Right stick)", L"Sağ çubuk: sol (RS/Right stick)"},
  {"keybind_rstick_right", "Right", L"Right stick right (RS/Right stick)", L"Sağ çubuk: sağ (RS/Right stick)"},
  {"keybind_rstick_press", "K", L"Right stick press (RS/R3)", L"Sağ çubuk: bas (RS/R3)"},
  {"keybind_dpad_up", "Shift+Up", L"D-pad up (D-pad)", L"Yön: yukarı (D-pad)"},
  {"keybind_dpad_down", "Shift+Down", L"D-pad down (D-pad)", L"Yön: aşağı (D-pad)"},
  {"keybind_dpad_left", "Shift+Left", L"D-pad left (D-pad)", L"Yön: sol (D-pad)"},
  {"keybind_dpad_right", "Shift+Right", L"D-pad right (D-pad)", L"Yön: sağ (D-pad)"},
  {"keybind_back", "Z,Tab", L"Back button (Back/Select)", L"Geri / Seç (Back/Select)"},
  {"keybind_start", "X,Return", L"Start button (Start/Start)", L"Başlat / Duraklat (Start/Start)"},
  {"keybind_guide", "", L"Guide button (Guide/PS)", L"Rehber düğmesi (Guide/PS)"},
}};
}
