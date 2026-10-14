// Run with UBSan: dates outside POSIX nanosecond system_clock's range must
// retain FILETIME's 100 ns precision, including the 1601 epoch and year 9999.
#include <rex/chrono/chrono.h>
#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>
int main() { try {
  using Clock=rex::chrono::WinSystemClock;
  using namespace std::chrono;
  const auto epoch=sys_days{year{1601}/1/1};
  unsigned checks=0;
  auto require=[&](bool ok){if(!ok)throw std::runtime_error("FILETIME range or precision mismatch");++checks;};
  for(int y : {1601,1677,1970,2000,2262,9999}) {
    const auto date=sys_days{year{y}/1/1};
    const auto ticks=duration_cast<Clock::duration>(date-epoch+hours{12}+milliseconds{345}).count()+7;
    const auto native=Clock::from_file_time(static_cast<uint64_t>(ticks));
    const auto sys=Clock::to_sys(native);
    require(floor<days>(sys)==date);
    require(Clock::to_file_time(Clock::from_sys(sys))==static_cast<uint64_t>(ticks));
    require(duration_cast<Clock::duration>(sys-floor<days>(sys)).count()==duration_cast<Clock::duration>(hours{12}+milliseconds{345}).count()+7);
  }
  require(Clock::to_file_time(Clock::from_sys(sys_days{year{1601}/1/1}))==0);
  std::cout<<checks<<" FILETIME range/precision checks passed\n";
  return 0;
} catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;} }
