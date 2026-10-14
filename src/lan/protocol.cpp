#include "protocol.h"
#include <algorithm>
#include <charconv>
#include <limits>
#include <random>
#include <stdexcept>
#ifdef _WIN32
#include <Windows.h>
#include <bcrypt.h>
#endif
namespace motogp::lan {
std::span<const uint8_t> Reader::Raw(size_t n){
  if(n>bytes_.size()-position_)throw std::runtime_error("Truncated LAN message");
  const auto result=bytes_.subspan(position_,n);position_+=n;return result;
}
uint8_t Reader::U8(){return Raw(1)[0];}
uint16_t Reader::U16(){const auto v=Raw(2);return uint16_t(v[0])<<8|v[1];}
uint32_t Reader::U32(){const auto a=U16();return uint32_t(a)<<16|U16();}
uint64_t Reader::U64(){const auto a=U32();return uint64_t(a)<<32|U32();}
std::vector<uint8_t> Reader::Blob(size_t maximum){const auto n=U16();if(n>maximum)throw std::runtime_error("Oversized LAN field");const auto v=Raw(n);return {v.begin(),v.end()};}
std::string Reader::Text(size_t maximum){const auto v=Blob(maximum);if(std::find(v.begin(),v.end(),0)!=v.end())throw std::runtime_error("NUL in LAN name");return {v.begin(),v.end()};}
std::vector<uint8_t> Encode(const Packet& p){
  if(p.payload.size()>kMaxPacket-48 || !p.peer)throw std::runtime_error("Invalid LAN packet");
  Writer w;w.U32(0x4D475039);w.U16(1);w.U16(static_cast<uint16_t>(p.kind));
  w.U32(kTitle);w.U32(kGame);w.U64(kCompatibility);w.U32(p.request);
  w.U64(p.peer);w.U64(p.session);w.U32(static_cast<uint32_t>(p.payload.size()));w.Raw(p.payload);return w.bytes;
}
std::optional<Packet> Decode(std::span<const uint8_t> bytes){try{
  if(bytes.size()<48 || bytes.size()>kMaxPacket)return {};
  Reader r(bytes);if(r.U32()!=0x4D475039 || r.U16()!=1)return {};
  const auto kind=r.U16();if(kind<1 || kind>static_cast<uint16_t>(Kind::Roster))return {};
  if(r.U32()!=kTitle || r.U32()!=kGame || r.U64()!=kCompatibility)return {};
  Packet p;p.kind=static_cast<Kind>(kind);p.request=r.U32();p.peer=r.U64();p.session=r.U64();
  const auto payload=r.Raw(r.U32());if(!p.peer || !r.End())return {};
  p.payload.assign(payload.begin(),payload.end());return p;
}catch(const std::exception&){return {};}}
std::vector<uint8_t> EncodeRoom(const Room& room){
  if(!room.id || !room.host || room.name.empty() || room.name.size()>31 ||
     room.name.find('\0')!=std::string::npos || !room.capacity || room.capacity>20 ||
     room.members>room.capacity || room.private_slots>room.capacity || room.private_members>room.private_slots ||
     room.private_members>room.members || room.members-room.private_members>room.capacity-room.private_slots ||
     room.state>1 || room.properties.size()>kMaxProperties || room.qos.size()>kMaxQos)
    throw std::runtime_error("Invalid LAN room");
  Writer w;w.U64(room.id);w.U64(room.host);w.U32(room.flags);w.U32(room.capacity);w.U32(room.members);w.U32(room.state);
  w.U32(room.private_slots);w.U32(room.private_members);
  w.Raw(room.key);w.Text(room.name);w.Blob(room.properties);w.Blob(room.qos);return w.bytes;
}
std::optional<Room> DecodeRoom(std::span<const uint8_t> bytes){try{
  Reader r(bytes);Room room;room.id=r.U64();room.host=r.U64();room.flags=r.U32();room.capacity=r.U32();room.members=r.U32();room.state=r.U32();
  room.private_slots=r.U32();room.private_members=r.U32();
  const auto key=r.Raw(16);std::copy(key.begin(),key.end(),room.key.begin());room.name=r.Text(31);
  room.properties=r.Blob(kMaxProperties);room.qos=r.Blob(kMaxQos);
  if(!r.End())return {};EncodeRoom(room);return room;
}catch(const std::exception&){return {};}}
void RandomBytes(std::span<uint8_t> bytes){
#ifdef _WIN32
  if(BCryptGenRandom(nullptr,bytes.data(),static_cast<ULONG>(bytes.size()),BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0)
    throw std::runtime_error("LAN random generation failed");
#else
  std::random_device random;for(auto& byte:bytes)byte=static_cast<uint8_t>(random());
#endif
}
uint64_t RandomId(){uint64_t value=0;while(!value)RandomBytes({reinterpret_cast<uint8_t*>(&value),sizeof(value)});return value;}
std::optional<uint32_t> ParseIPv4(const std::string& text){
  uint32_t value=0;size_t position=0;
  for(unsigned part=0;part<4;++part){const auto end=text.find('.',position);const auto stop=end==std::string::npos?text.size():end;
    unsigned byte=0;const auto result=std::from_chars(text.data()+position,text.data()+stop,byte);
    if(position==stop || result.ec!=std::errc{} || result.ptr!=text.data()+stop || byte>255 || (part<3)==(end==std::string::npos))return {};
    value=value<<8|byte;position=stop+1;
  }
  return value;
}
std::string IPv4(uint32_t ip){return std::to_string(ip>>24)+"."+std::to_string((ip>>16)&255)+"."+std::to_string((ip>>8)&255)+"."+std::to_string(ip&255);}
}
