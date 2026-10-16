#pragma once
#include "node.h"
#include "online/client.h"
#include "metadata.h"
#include <rex/system/kernel_state.h>
#include <map>
#include <mutex>
namespace motogp::lan {
// Owns no guest buffers across calls. Session handles belong to the kernel's
// object table; native room records contain copied values, never guest pointers.
class GuestBridge {
  rex::system::KernelState* kernel_;
  std::shared_ptr<Node> node_;
  std::shared_ptr<online::Client> online_;
  std::string direct_ip_, room_name_;
  mutable std::mutex mutex_;
  Metadata metadata_;
  // Keep the copied, bounded search result until the next successful search.
  // The title can leave its result screen open longer than discovery expiry.
  std::vector<Room> search_results_;
 public:
  GuestBridge(rex::system::KernelState*,std::shared_ptr<Node>,std::string direct_ip={},
              std::string room_name="MotoGP room",std::shared_ptr<online::Client> service={});
  bool Valid(uint32_t,size_t,bool write=false)const;
  uint32_t CreateHandle(uint32_t output);
  uint32_t Reference(uint32_t handle,uint32_t output);
  std::optional<uint32_t> Dispatch(uint32_t message,uint32_t buffer,uint32_t size);
  std::function<uint32_t()> StatisticsOperation(uint32_t message,uint32_t buffer,uint32_t size);
  void WriteAddress(uint32_t output,uint64_t identity,Endpoint)const;
  void WriteInfo(uint32_t output,const Room&)const;
  std::vector<Room> KnownRooms()const;
  Node& Network()const{return *node_;}
  const std::shared_ptr<online::Client>& Online()const{return online_;}
  rex::system::KernelState* Kernel()const{return kernel_;}
};
}
