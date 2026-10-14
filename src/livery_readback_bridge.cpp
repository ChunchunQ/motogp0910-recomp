// Compiled into the pinned SDK GPU plugin, not into the application.
#include <rex/system/gpu_plugin.h>
#include <atomic>
#include <cstdint>

namespace {
std::atomic<uint32_t> livery_scopes{0};
}
extern "C" bool Motogp_IsLiveryReadbackRequired() {
  return livery_scopes.load(std::memory_order_acquire)!=0;
}
// Small C ABI: +1 acquires, -1 releases, 0 queries. Nesting is supported.
extern "C" REX_GPU_PLUGIN_EXPORT uint32_t Motogp_LiveryReadbackScope(int32_t delta) {
  if(delta==1)return livery_scopes.fetch_add(1,std::memory_order_acq_rel)+1;
  auto count=livery_scopes.load(std::memory_order_acquire);
  if(delta==-1)while(count && !livery_scopes.compare_exchange_weak(count,count-1,std::memory_order_acq_rel)) {}
  return delta==-1 && count?count-1:count;
}
