#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <string>

namespace motogp::adaptive {
struct Trigger {
  std::string mode="off";
  int start=0,end=0,strength=0;
  bool Valid()const {
    return (mode=="off"||mode=="feedback"||mode=="weapon")&&start>=0&&start<=9&&
        end>=0&&end<=9&&strength>=0&&strength<=8&&(mode!="weapon"||end>start);
  }
  bool operator==(const Trigger&)const=default;
};
struct Profile {
  bool enabled=false;
  Trigger right{"weapon",3,6,5},left{"feedback",2,0,4};
  bool Valid()const{return right.Valid()&&left.Valid();}
  bool operator==(const Profile&)const=default;
};
using Report=std::array<uint8_t,47>;
// SDL DS5EffectsState_t common payload; SDL adds USB/BT framing and CRC.
// Legacy resistance modes use full-byte position/force, not raw UI steps.
inline void EncodeTrigger(uint8_t* out,const Trigger& t,bool enabled) {
  std::fill_n(out,11,uint8_t{0});
  if(!enabled||!t.Valid()||t.mode=="off"||!t.strength)return;
  out[0]=t.mode=="feedback"?0x01:0x02;
  out[1]=static_cast<uint8_t>((t.start*255+4)/9);
  if(t.mode=="feedback")out[2]=static_cast<uint8_t>((t.strength*255+4)/8);
  else {out[2]=static_cast<uint8_t>((t.end*255+4)/9);out[3]=static_cast<uint8_t>((t.strength*255+4)/8);}
}
inline Report Encode(const Profile& profile,bool focused=true) {
  Report report{};
  // Always write both trigger slots, including off: hardware retains its last
  // effect when an enable bit is absent. Leave rumble/audio/LED flags alone.
  report[0]=0x0C;
  EncodeTrigger(report.data()+10,profile.right,profile.enabled&&focused);
  EncodeTrigger(report.data()+21,profile.left,profile.enabled&&focused);
  return report;
}
}
