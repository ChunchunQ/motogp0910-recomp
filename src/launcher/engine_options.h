#pragma once
#include <array>
namespace launcher {
struct EngineEffect {const char* key;const wchar_t* english;const wchar_t* turkish;};
inline constexpr std::array<EngineEffect,3> kEngineEffects{{
  {"engine_speed_blur",L"Motion blur (speed)",L"Hareket bulanıklığı (hız)"},
  {"engine_depth_of_field",L"Depth of field",L"Alan derinliği"},
  {"engine_dynamic_shadows",L"Dynamic shadows",L"Dinamik gölgeler"}
}};
inline constexpr std::array<int,4> kShadowDistances{0,45,90,180};
}
