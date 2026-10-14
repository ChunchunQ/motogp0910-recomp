#include <atomic>
#include <cstdint>
#if defined(_WIN32)
#define MOTOGP_EXPORT __declspec(dllexport)
#else
#define MOTOGP_EXPORT __attribute__((visibility("default")))
#endif
namespace {std::atomic<uint64_t> aspect{0};}
extern "C" MOTOGP_EXPORT void Motogp_SetPresentationAspect(uint32_t x,uint32_t y) {
  aspect.store(x && y?uint64_t(x)<<32|y:0,std::memory_order_release);
}
extern "C" MOTOGP_EXPORT void Motogp_GetPresentationAspect(uint32_t* x,uint32_t* y) {
  const auto value=aspect.load(std::memory_order_acquire);
  if(value){*x=value>>32;*y=uint32_t(value);}
}
