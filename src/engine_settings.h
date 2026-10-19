#pragma once
#include <rex/memory.h>
#include <bit>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <optional>
namespace motogp::engine {
inline constexpr uint32_t kDofSettings=0x8337BE34;
inline constexpr uint32_t kSpeedBlurSettings=0x8337BE54;
inline constexpr uint32_t kShadowEnabled=0x83487A6C;
inline constexpr uint32_t kShadowDistance=0x83487CA0;
inline constexpr uint32_t kShadowFilter=0x83487A8C;
inline constexpr uint32_t kSoftShadowDistance=0x83487CC4;
inline constexpr uint32_t kParticleDensity=0x829177BC;
inline constexpr uint32_t kFarClipScale=0x834F6CA8;
inline constexpr uint32_t kBloomObject=0x82892540;
inline constexpr uint32_t kBloomCurrentIntensity=kBloomObject+20;
inline constexpr uint32_t kVignetteAlpha=kSpeedBlurSettings+16;
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
// Scale only during the consuming pass. Restoring the original bits prevents
// repeated frames from compounding the multiplier or altering camera presets.
class FloatScale {
 public:
  FloatScale(uint8_t* base,uint32_t address,int percent):base_(base){
    if(percent<0)return;
    const auto saved=rex::memory::load_and_swap<uint32_t>(base+address);
    const auto value=std::bit_cast<float>(saved);
    if(!std::isfinite(value) || value<0)return;
    const auto scaled=value*(float(percent)/100.f);
    if(!std::isfinite(scaled))return;
    address_=address;saved_=saved;
    rex::memory::store_and_swap<float>(base+address,scaled);
  }
  ~FloatScale(){if(address_)rex::memory::store_and_swap<uint32_t>(base_+address_,saved_);}
  FloatScale(const FloatScale&)=delete;FloatScale& operator=(const FloatScale&)=delete;
 private:
  uint8_t* base_;uint32_t address_=0,saved_=0;
};
class ScaledPropertyOverride {
 public:
  ScaledPropertyOverride(uint8_t* base,uint32_t object,int percent){
    if(percent<0 || rex::memory::load_and_swap<uint32_t>(base+object+12)!=object+16)return;
    const auto read=[&](uint32_t offset){return rex::memory::load_and_swap<float>(base+object+offset);};
    const auto value=read(16),minimum=read(20),maximum=read(24);
    if(!std::isfinite(value)||!std::isfinite(minimum)||!std::isfinite(maximum)||value<0||minimum>maximum)return;
    const auto scaled=std::clamp(value*(float(percent)/100.f),minimum,maximum);
    value_.emplace(base,object,std::bit_cast<uint32_t>(scaled),false,true);
  }
 private:
  std::optional<PropertyOverride> value_;
};
}
