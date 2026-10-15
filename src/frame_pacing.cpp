#include "frame_pacing.h"
#include <rex/cvar.h>
#include <thread>
#include <atomic>
REXCVAR_DEFINE_INT32(fps_limit,0,"Display","Maximum guest presentation rate; 0 adds no frame limit")
    .range(0,1000);
namespace motogp {
namespace {std::atomic<void(*)()> frame_observer{nullptr};}
void SetFrameObserver(void (*observer)()){frame_observer.store(observer,std::memory_order_release);}
void PaceGameFrame() {
  if(auto observer=frame_observer.load(std::memory_order_acquire))observer();
  thread_local FrameSchedule schedule;
  const auto now=FrameSchedule::Clock::now();
  const auto target=schedule.Next(now,REXCVAR_GET(fps_limit));
  if(target>now)std::this_thread::sleep_until(target);
}
}
