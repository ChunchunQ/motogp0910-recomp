#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace rex {class Runtime;}
namespace motogp::online::lap_times {
inline constexpr uint32_t kChampionship=0x10000000;
enum class Label : uint8_t {
 Native,Unknown,QuickRace,Preview,Training,Tutorial,SplitScreen,
 OnlineSplitScreen,Lan,Online,Demo,Replay,Count
};
// Preserve the title's localized Championship/Career/Arcade/Time Trial paths.
// Unknown or missing metadata must not silently become an Arcade result.
constexpr Label ModeLabel(uint32_t mode){
 if((mode&~kChampionship)>16)return Label::Unknown;
 if((mode&kChampionship)!=0)return Label::Native;
 switch(mode){
  case 2:case 3:case 4:case 5:case 6:case 10:return Label::Native;
  case 1:return Label::QuickRace;
  case 7:return Label::Preview;
  case 8:return Label::Training;
  case 9:return Label::Tutorial;
  case 11:return Label::SplitScreen;
  case 12:return Label::OnlineSplitScreen;
  case 13:return Label::Lan;
  case 14:return Label::Online;
  case 15:return Label::Demo;
  case 16:return Label::Replay;
  default:return Label::Unknown;
 }
}
inline constexpr std::array<std::array<std::string_view,static_cast<std::size_t>(Label::Count)>,2> kLabels{{
 {"","-","QUICK RACE","PREVIEW","TRAINING","TUTORIAL","SPLITSCREEN",
  "ONLINE SPLITSCREEN","LAN","ONLINE","DEMO","REPLAY"},
 {"","-","HIZLI YARIŞ","ÖNİZLEME","ANTRENMAN","EĞİTİM","BÖLÜNMÜŞ EKRAN",
  "ÇEVRİMİÇİ / BÖLÜNMÜŞ","LAN","ÇEVRİMİÇİ","DEMO","TEKRAR"}
}};
void Initialize(rex::Runtime* runtime);
}
