#pragma once
#include "client.h"
#include <rex/system/kernel_state.h>
#include <optional>
#include <functional>
namespace motogp::lan {class GuestBridge;}
namespace motogp::online {
struct Spec {uint32_t view=0;std::vector<uint16_t> columns;};
// Xbox ABI: read results 8 bytes; views 16; rows 48; columns 24.
uint32_t ResultsSize(const std::vector<Spec>&,uint32_t rows);
void WriteResults(uint8_t* memory,uint32_t output,const std::vector<Spec>&,
                  const std::vector<Page>&);
uint32_t CreateEnumerator(lan::GuestBridge&,uint32_t mode,uint64_t first,
                           uint32_t count,uint32_t specs,uint32_t input,
                           uint32_t size_out,uint32_t handle_out);
std::optional<uint32_t> EnumeratorSize(rex::system::KernelState*,uint32_t handle);
uint32_t ReadStats(lan::GuestBridge&,uint32_t buffer,uint32_t size);
uint32_t WriteStats(lan::GuestBridge&,uint32_t buffer);
// Validate and copy inputs on the calling thread. Only the explicit read
// output/overlapped buffers follow the original asynchronous lifetime contract.
std::function<uint32_t()> PrepareWrite(lan::GuestBridge&,uint32_t buffer);
std::function<uint32_t()> PrepareRead(lan::GuestBridge&,uint32_t buffer,uint32_t size);
}
