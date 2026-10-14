#pragma once
#include "protocol.h"
#include <chrono>
#include <functional>
#include <memory>
namespace motogp::lan {
struct Adapter {std::string name;uint32_t ip=0,mask=0;bool radmin=false;};
std::vector<Adapter> Adapters();
struct Options {
  std::string bind_ip; // empty selects Radmin, then an active LAN interface
  uint16_t port=0; // zero selects a free port in the discovery range
  // Test/explicit network override; normal discovery uses the adapter broadcast.
  std::vector<uint32_t> discovery_targets;
  std::chrono::milliseconds expiry{3500};
};
struct Datagram {uint64_t peer=0;uint16_t source_port=0,destination_port=0;std::vector<uint8_t> bytes;};
class Node {
  struct Impl;std::unique_ptr<Impl> impl_;
 public:
  explicit Node(Options options={});~Node();
  Node(const Node&)=delete;Node& operator=(const Node&)=delete;
  Endpoint Address()const;uint64_t Identity()const;std::string AdapterName()const;
  Room Host(std::string name,uint32_t capacity=2,uint32_t flags=0,
            std::vector<uint8_t> properties={},std::vector<uint8_t> qos={});
  void CloseRoom();void UpdateRoom(const Room&);
  void Search(const std::string& direct_ip={}); // nonblocking; direct and automatic share validation
  std::vector<Room> Rooms()const;std::optional<Room> CurrentRoom()const;
  bool Join(const Room&,std::chrono::milliseconds timeout=std::chrono::milliseconds(1200));
  void Leave();uint32_t Members()const;
  std::optional<Endpoint> Peer(uint64_t)const;
  bool Connected(uint64_t)const;
  void RememberPeer(uint64_t,Endpoint);
  bool SendData(uint64_t peer,uint16_t source,uint16_t destination,std::span<const uint8_t>);
  std::optional<Datagram> ReceiveData();
};
}
