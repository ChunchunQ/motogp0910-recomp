#include "frame_pacing.h"
#include <rex/cvar.h>
#include <thread>
REXCVAR_DEFINE_INT32(fps_limit,0,"Display","Maximum guest presentation rate; 0 adds no frame limit")
    .range(0,1000);
namespace motogp {
void PaceGameFrame() {
  thread_local FrameSchedule schedule;
  const auto now=FrameSchedule::Clock::now();
  const auto target=schedule.Next(now,rex::cvar::Query<int32_t>("fps_limit"));
  if(target>now)std::this_thread::sleep_until(target);
}
}
