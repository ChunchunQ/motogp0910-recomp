#pragma once
#include <cstdint>
namespace rex {class Runtime;}
namespace motogp::options {
inline constexpr uint32_t kVtable=0x8203411C;
inline constexpr uint32_t kGameSettingsVtable=0x820338B4,kMultiplayerVtable=0x8203A0C4;
inline constexpr uint32_t kCreateVtable=0x82035884,kCustomVtable=0x82035E0C,kOnlineCreateVtable=0x8203C464;
inline constexpr uint32_t kOnlineHandle=0x8288D258;
void Initialize(rex::Runtime* runtime);
uint32_t OnlineLabel();
bool RemoveStorage(uint8_t* base,uint32_t menu,uint32_t screen);
bool RemoveParty(uint8_t* base,uint32_t menu,uint32_t screen);
bool RemovePremium(uint8_t* base,uint32_t menu,uint32_t screen);
// Restrict the native Options list to its five functional rows, before init.
bool RemoveMarketplace(uint8_t* base,uint32_t menu,uint32_t screen);
}
