#pragma once
#include "lan/protocol.h"
#include <array>
#include <optional>

namespace motogp::online {
// Online uses an authenticated envelope around the unchanged LAN02 protocol.
// The key is negotiated over HTTPS and never sent in a UDP datagram.
struct Ticket {
  lan::Endpoint endpoint;
  uint64_t peer=0;
  std::array<uint8_t,16> id{};
  std::array<uint8_t,32> key{};
};
constexpr size_t kEnvelopeOverhead=68;
lan::Endpoint VirtualEndpoint(uint64_t peer);
uint64_t VirtualPeer(lan::Endpoint endpoint);
std::vector<uint8_t> Seal(const Ticket&,uint64_t sequence,uint64_t destination,
                          std::span<const uint8_t> packet,bool server=false);
struct Envelope {uint64_t sequence=0,destination=0;std::vector<uint8_t> packet;};
std::optional<Envelope> Open(const Ticket&,std::span<const uint8_t>,bool server=false);
// 128-packet reordering window, updated only after MAC verification.
class ReplayWindow {
  uint64_t top_=0,low_=0,high_=0;
 public:
  bool Accept(uint64_t sequence);
};
}
