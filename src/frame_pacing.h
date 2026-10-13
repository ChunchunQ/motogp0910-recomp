#pragma once
#include <chrono>
namespace motogp {
class FrameSchedule {
 public:
  using Clock=std::chrono::steady_clock;
  Clock::time_point Next(Clock::time_point now,int fps) {
    if(fps<=0 || fps>1000){rate_=0;deadline_={};return now;}
    const auto period=std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(1.0/fps));
    if(fps!=rate_){rate_=fps;deadline_=now;}
    const auto target=deadline_>now?deadline_:now;
    deadline_=target+period;
    return target;
  }
 private:
  int rate_=0;
  Clock::time_point deadline_{};
};
void PaceGameFrame();
}
