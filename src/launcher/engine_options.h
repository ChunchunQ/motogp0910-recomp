#pragma once
#include <array>
#include <span>
namespace launcher {
enum class EngineKind {Toggle,Strength,Filter,Distance,DrawDistance};
struct EngineEffect {const char* key;const wchar_t* english;const wchar_t* turkish;EngineKind kind=EngineKind::Toggle;};
inline constexpr std::array<EngineEffect,9> kEngineEffects{{
  {"engine_speed_blur",L"Motion blur (speed)",L"Hareket bulanıklığı (hız)"},
  {"engine_depth_of_field",L"Depth of field",L"Alan derinliği"},
  {"engine_dynamic_shadows",L"Dynamic shadows",L"Dinamik gölgeler"},
  {"engine_vignette",L"Vignette intensity",L"Kenar kararması şiddeti",EngineKind::Strength},
  {"engine_bloom",L"Bloom intensity",L"Işık yayılması şiddeti",EngineKind::Strength},
  {"engine_shadow_filter",L"Shadow filtering",L"Gölge filtreleme",EngineKind::Filter},
  {"engine_soft_shadow_distance",L"Soft shadow distance",L"Yumuşak gölge mesafesi",EngineKind::Distance},
  {"engine_particle_density",L"Particle density",L"Parçacık yoğunluğu",EngineKind::Strength},
  {"engine_draw_distance",L"Draw distance (Experimental)",L"Çizim mesafesi (Deneysel)",EngineKind::DrawDistance}
}};
inline constexpr std::array<const char*,2> kEngineToggleValues{"game","off"};
inline constexpr std::array<const char*,6> kEngineStrengthValues{"game","off","25","50","75","100"};
inline constexpr std::array<const char*,3> kEngineFilterValues{"game","off","on"};
inline constexpr std::array<const char*,4> kEngineDistanceValues{"game","45","90","180"};
inline constexpr std::array<const char*,6> kEngineDrawDistanceValues{"game","50","75","100","125","150"};
inline std::span<const char* const> EngineValues(const EngineEffect& effect){
  switch(effect.kind){
    case EngineKind::Strength:return kEngineStrengthValues;
    case EngineKind::Filter:return kEngineFilterValues;
    case EngineKind::Distance:return kEngineDistanceValues;
    case EngineKind::DrawDistance:return kEngineDrawDistanceValues;
    default:return kEngineToggleValues;
  }
}
inline constexpr std::array<int,4> kShadowDistances{0,45,90,180};
}
