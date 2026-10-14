#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>
namespace motogp::lan {
constexpr uint32_t kTitle = 0x434307F0;
// Protocol title tag: build 1, qfe 1 (not the packed XEX version word).
constexpr uint32_t kGame = 0x101;
constexpr uint64_t kCompatibility = 0x4D4750394C414E02ull;
constexpr uint16_t kFirstPort = 30810, kPortCount = 8;
constexpr size_t kMaxPacket = 1400, kMaxProperties = 768, kMaxQos = 256;
enum class Kind : uint16_t { Discover=1, Advert, Join, Joined, Leave, Left, Heartbeat, Reject, Data, Roster };
struct Endpoint {
  uint32_t ip=0; uint16_t port=0;
  friend bool operator==(const Endpoint&,const Endpoint&)=default;
};
struct Room {
  uint64_t id=0, host=0;
  Endpoint endpoint;
  std::array<uint8_t,16> key{};
  std::string name;
  uint32_t flags=0, capacity=2, members=1, state=0;
  uint32_t private_slots=0, private_members=0;
  std::vector<uint8_t> properties, qos;
};
struct Packet {
  Kind kind=Kind::Discover;
  uint32_t request=0;
  uint64_t peer=0, session=0;
  std::vector<uint8_t> payload;
};
class Writer {
 public:
  std::vector<uint8_t> bytes;
  void U8(uint8_t v){bytes.push_back(v);}
  void U16(uint16_t v){U8(v>>8);U8(v);}
  void U32(uint32_t v){U16(v>>16);U16(v);}
  void U64(uint64_t v){U32(v>>32);U32(v);}
  void Raw(std::span<const uint8_t> v){bytes.insert(bytes.end(),v.begin(),v.end());}
  void Blob(std::span<const uint8_t> v){U16(static_cast<uint16_t>(v.size()));Raw(v);}
  void Text(const std::string& v){Blob({reinterpret_cast<const uint8_t*>(v.data()),v.size()});}
};
class Reader {
  std::span<const uint8_t> bytes_; size_t position_=0;
 public:
  explicit Reader(std::span<const uint8_t> bytes):bytes_(bytes){}
  uint8_t U8(); uint16_t U16(); uint32_t U32(); uint64_t U64();
  std::span<const uint8_t> Raw(size_t count);
  std::vector<uint8_t> Blob(size_t maximum);
  std::string Text(size_t maximum);
  bool End()const{return position_==bytes_.size();}
};
std::vector<uint8_t> Encode(const Packet&);
std::optional<Packet> Decode(std::span<const uint8_t>);
std::vector<uint8_t> EncodeRoom(const Room&);
std::optional<Room> DecodeRoom(std::span<const uint8_t>);
uint64_t RandomId();
void RandomBytes(std::span<uint8_t>);
std::optional<uint32_t> ParseIPv4(const std::string&);
std::string IPv4(uint32_t);
}
