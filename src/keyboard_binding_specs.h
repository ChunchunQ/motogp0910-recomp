#pragma once
#include <array>
#include <string_view>
namespace launcher {
inline constexpr bool IsLegacyDefaultBinding(unsigned index,std::string_view value){
  switch(index){
    case 0:return value=="Semicolon,Space";
    case 1:return value=="Quote,Backspace";
    case 12:return value=="F";
    case 13:return value=="Up";
    case 14:return value=="Down";
    case 15:return value=="Left";
    case 16:return value=="Right";
    case 17:return value=="K";
    default:return false;
  }
}
struct BindingSpec { const char* key; const char* defaults; const wchar_t* english; const wchar_t* turkish; };
inline constexpr std::array<BindingSpec,25> kBindings{{
  {"keybind_a", "Space", L"Tuck in / Confirm (A/Cross)", L"Kapan / Onayla (A/Cross)"},
  {"keybind_b", "Backspace", L"Gear down / Back (B/Circle)", L"Vites küçült / Geri (B/Circle)"},
  {"keybind_x", "L", L"Rear brake (X/Square)", L"Arka fren (X/Square)"},
  {"keybind_y", "P", L"Gear up (Y/Triangle)", L"Vites büyüt (Y/Triangle)"},
  {"keybind_left_trigger", "Q,I", L"Brake (LT/L2)", L"Ön fren (LT/L2)"},
  {"keybind_right_trigger", "E,O", L"Accelerate (RT/R2)", L"Gaz (RT/R2)"},
  {"keybind_left_shoulder", "1", L"Look behind (LB/L1)", L"Arkaya bak (LB/L1)"},
  {"keybind_right_shoulder", "3", L"Change view (RB/R1)", L"Kamera değiştir (RB/R1)"},
  {"keybind_lstick_up", "W", L"Lean rider forward (LS Up)", L"Sürücüyü öne eğ (LS Yukarı)"},
  {"keybind_lstick_down", "S", L"Lean rider back (LS Down)", L"Sürücüyü geriye eğ (LS Aşağı)"},
  {"keybind_lstick_left", "A", L"Turn left (LS Left)", L"Sola dön (LS Sol)"},
  {"keybind_lstick_right", "D", L"Turn right (LS Right)", L"Sağa dön (LS Sağ)"},
  {"keybind_lstick_press", "", L"Unassigned (LS/L3)", L"Atanmamış (LS/L3)"},
  {"keybind_rstick_up", "", L"Unassigned (RS Up)", L"Atanmamış (RS Yukarı)"},
  {"keybind_rstick_down", "", L"Unassigned (RS Down)", L"Atanmamış (RS Aşağı)"},
  {"keybind_rstick_left", "", L"Unassigned (RS Left)", L"Atanmamış (RS Sol)"},
  {"keybind_rstick_right", "", L"Unassigned (RS Right)", L"Atanmamış (RS Sağ)"},
  {"keybind_rstick_press", "", L"Unassigned (RS/R3)", L"Atanmamış (RS/R3)"},
  {"keybind_dpad_up", "Shift+Up", L"Celebration gesture (D-pad Up)", L"Kutlama hareketi (D-pad Yukarı)"},
  {"keybind_dpad_down", "Shift+Down", L"Change view (D-pad Down)", L"Kamera değiştir (D-pad Aşağı)"},
  {"keybind_dpad_left", "Shift+Left", L"Apology gesture (D-pad Left)", L"Özür hareketi (D-pad Sol)"},
  {"keybind_dpad_right", "Shift+Right", L"Angry gesture (D-pad Right)", L"Öfke hareketi (D-pad Sağ)"},
  {"keybind_back", "Z,Tab", L"Second Chance (Back/Select)", L"İkinci Şans (Back/Select)"},
  {"keybind_start", "X,Return", L"Pause (Start/Start)", L"Duraklat (Start/Start)"},
  {"keybind_guide", "", L"Unassigned (Guide/PS)", L"Atanmamış (Guide/PS)"},
}};
}
