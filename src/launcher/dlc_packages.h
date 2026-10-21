#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
namespace launcher::dlc {
struct Package {
  const char* id;const char* sha256;const wchar_t* name;
  uint64_t bytes,payload_bytes;size_t files;unsigned marker;uint32_t license;
};
// Fingerprints of the owner's three original Xbox 360 marketplace packages.
// No game assets or content keys are distributed with this catalog.
inline constexpr std::array<Package,3> kPackages{{
  {"BED5A7F648EE9FEF5E279DDD21189E0B665DB3C543","a1b9e79b369f7fe0440042043839b50e37c9980552ffc856816c413fdecfaf5a",L"Free GP Class Season 2010 Update",708038656,703278545,187,1,0},
  {"FBFA3148C2716C4E4ADAFC7325F7162244ECE0BE43","2044bea000da158ca863568b668544778427d1b91e78f1545063f56103129501",L"Twin Track Pack",74858496,74310655,20,2,1},
  {"1C2755211DB925ADFCFD6BFEFB5639A270769ABA43","2d29e3154c0441f915a3d7d045b75fa605f00d0f12ee333023afb3013a73d60a",L"Free 2010 Moto2 and 125 Update",930828288,924258775,341,3,1},
}};
inline const Package* ByName(std::string_view name){for(const auto& p:kPackages)if(name==p.id)return &p;return nullptr;}
inline const Package* BySize(uint64_t bytes){for(const auto& p:kPackages)if(bytes==p.bytes)return &p;return nullptr;}
}
