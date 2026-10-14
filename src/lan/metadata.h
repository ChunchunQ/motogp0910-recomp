#pragma once
#include "protocol.h"
#include <map>
namespace motogp::lan {
struct Metadata {
  std::map<uint32_t,uint32_t> contexts;
  // Property type is encoded in the property's upper nibble. Values are copied
  // console bytes; pointer-bearing XUSER_DATA structs never cross the network.
  std::map<uint32_t,std::vector<uint8_t>> properties;
};
std::vector<uint8_t> EncodeMetadata(const Metadata&);
std::optional<Metadata> DecodeMetadata(std::span<const uint8_t>);
bool ValidProperty(uint32_t,std::span<const uint8_t>);
}
