#pragma once
#include "guest_bridge.h"
#include <rex/ppc/context.h>
namespace motogp::lan {
// Payload bytes the title has sent through the LAN/online bridge since start
// (XNET_OPTID_CALLER_XMIT_BYTES).
uint64_t TitleBytesSent();
// All guest socket I/O is serialized with close/cancel. The receiver is joined
// before teardown can destroy guest memory. Session/port state is copied.
class SocketBridge {
  struct Impl;
  std::unique_ptr<Impl> impl_;
 public:
  explicit SocketBridge(GuestBridge&);~SocketBridge();
  SocketBridge(const SocketBridge&)=delete;
  bool Handle(std::string_view import,PPCContext&,uint8_t*);
  bool Owns(uint32_t handle)const;
  void CancelSessionReceives();
  void Shutdown();
};
}
