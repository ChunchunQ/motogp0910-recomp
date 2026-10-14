#pragma once
#include "node.h"
#include <rex/system/kernel_state.h>
#include <map>
#include <mutex>
namespace motogp::lan {
// Owns no guest buffers across calls. Session handles belong to the kernel's
// object table; native room records contain copied values, never guest pointers.
class GuestBridge {
  rex::system::KernelState* kernel_;
  std::shared_ptr<Node> node_;
  std::string direct_ip_, room_name_;
  std::mutex mutex_;
  std::map<uint32_t,uint32_t> contexts_;
 public:
  GuestBridge(rex::system::KernelState*,std::shared_ptr<Node>,std::string direct_ip={},
              std::string room_name="MotoGP room");
  bool Valid(uint32_t,size_t,bool write=false)const;
  uint32_t CreateHandle(uint32_t output);
  uint32_t Reference(uint32_t handle,uint32_t output);
  std::optional<uint32_t> Dispatch(uint32_t message,uint32_t buffer,uint32_t size);
  void WriteAddress(uint32_t output,uint64_t identity,Endpoint)const;
  void WriteInfo(uint32_t output,const Room&)const;
  Node& Network()const{return *node_;}
};
}
