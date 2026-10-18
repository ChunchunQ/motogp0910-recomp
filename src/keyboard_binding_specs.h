#pragma once
#include <array>
#include <string_view>
namespace launcher {
inline constexpr bool IsLegacyDefaultBinding(unsigned index,std::string_view value){
  return (index==0&&value=="Semicolon,Space")||(index==1&&value=="Quote,Backspace");
}
struct BindingSpec { const char* key; const char* defaults; const wchar_t* english; const wchar_t* turkish; };
inline constexpr std::array<BindingSpec,25> kBindings{{
  {"keybind_a", "Space", L"Gear up / Confirm (A/Cross)", L"Vites büyüt / Onayla (A/Cross)"},
  {"keybind_b", "Backspace", L"Gear down / Back (B/Circle)", L"Vites küçült / Geri (B/Circle)"},
  {"keybind_x", "L", L"Rear brake (X/Square)", L"Arka fren (X/Square)"},
  {"keybind_y", "P", L"Celebration gesture (Y/Triangle)", L"Kutlama hareketi (Y/Triangle)"},
  {"keybind_left_trigger", "Q,I", L"Front brake (LT/L2)", L"Ön fren (LT/L2)"},
  {"keybind_right_trigger", "E,O", L"Accelerate (RT/R2)", L"Gaz (RT/R2)"},
  {"keybind_left_shoulder", "1", L"Tuck in (LB/L1)", L"Kapan (LB/L1)"},
  {"keybind_right_shoulder", "3", L"Change view (RB/R1)", L"Kamera değiştir (RB/R1)"},
  {"keybind_lstick_up", "W", L"Lean forward (LS/Left stick)", L"Öne eğil (LS/Left stick)"},
  {"keybind_lstick_down", "S", L"Lean backward (LS/Left stick)", L"Arkaya eğil (LS/Left stick)"},
  {"keybind_lstick_left", "A", L"Turn left (LS/Left stick)", L"Sola dön (LS/Left stick)"},
  {"keybind_lstick_right", "D", L"Turn right (LS/Left stick)", L"Sağa dön (LS/Left stick)"},
  {"keybind_lstick_press", "F", L"Look behind (LS/L3)", L"Arkaya bak (LS/L3)"},
  {"keybind_rstick_up", "Up", L"Look around: up (RS/Right stick)", L"Etrafa bak: yukarı (RS/Right stick)"},
  {"keybind_rstick_down", "Down", L"Look around: down (RS/Right stick)", L"Etrafa bak: aşağı (RS/Right stick)"},
  {"keybind_rstick_left", "Left", L"Look around: left (RS/Right stick)", L"Etrafa bak: sol (RS/Right stick)"},
  {"keybind_rstick_right", "Right", L"Look around: right (RS/Right stick)", L"Etrafa bak: sağ (RS/Right stick)"},
  {"keybind_rstick_press", "K", L"Unassigned (RS/R3)", L"Atanmamış (RS/R3)"},
  {"keybind_dpad_up", "Shift+Up", L"Apology gesture (D-pad Up)", L"Özür hareketi (D-pad Yukarı)"},
  {"keybind_dpad_down", "Shift+Down", L"Change view (D-pad Down)", L"Kamera değiştir (D-pad Aşağı)"},
  {"keybind_dpad_left", "Shift+Left", L"Angry gesture (D-pad Left)", L"Öfke hareketi (D-pad Sol)"},
  {"keybind_dpad_right", "Shift+Right", L"Skip music track (D-pad Right)", L"Sonraki müzik (D-pad Sağ)"},
  {"keybind_back", "Z,Tab", L"Second Chance (Back/Select)", L"İkinci Şans (Back/Select)"},
  {"keybind_start", "X,Return", L"Pause (Start/Start)", L"Duraklat (Start/Start)"},
  {"keybind_guide", "", L"Unassigned (Guide/PS)", L"Atanmamış (Guide/PS)"},
}};
}
