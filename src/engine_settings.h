#pragma once
#include <rex/memory.h>
#include <bit>
#include <cstdint>
namespace motogp::engine {
inline constexpr uint32_t kDofSettings=0x8337BE34;
inline constexpr uint32_t kSpeedBlurSettings=0x8337BE54;
inline constexpr uint32_t kShadowEnabled=0x83487A6C;
inline constexpr uint32_t kShadowDistance=0x83487CA0;
// Native registry objects keep their initialized value pointer at +12 and
// embedded value at +16. Do not touch an uninitialized or redirected object.
class PropertyOverride {
 public:
  PropertyOverride(uint8_t* base,uint32_t object,uint32_t value,bool byte,bool enabled):base_(base),byte_(byte){
    if(!enabled || rex::memory::load_and_swap<uint32_t>(base+object+12)!=object+16)return;
    address_=object+16;
    saved_=byte?base[address_]:rex::memory::load_and_swap<uint32_t>(base+address_);
    if(byte)base[address_]=static_cast<uint8_t>(value);else rex::memory::store_and_swap<uint32_t>(base+address_,value);
  }
  ~PropertyOverride(){if(address_){if(byte_)base_[address_]=static_cast<uint8_t>(saved_);else rex::memory::store_and_swap<uint32_t>(base_+address_,saved_);}}
  PropertyOverride(const PropertyOverride&)=delete;
  PropertyOverride& operator=(const PropertyOverride&)=delete;
 private:
  uint8_t* base_;bool byte_;uint32_t address_=0,saved_=0;
};
}
