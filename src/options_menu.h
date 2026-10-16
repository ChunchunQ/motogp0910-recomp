#pragma once
#include <cstdint>
namespace motogp::options {
inline constexpr uint32_t kVtable=0x8203411C;
// Restrict the native Options list to its five functional rows, before init.
bool RemoveMarketplace(uint8_t* base,uint32_t menu,uint32_t screen);
}
