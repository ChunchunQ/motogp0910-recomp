#include <rex/hook.h>
#include <rex/memory.h>

REX_EXTERN(__imp__sub_822FC460);

namespace {
// These are frontend modes (FEMODE_*), not the separate leaderboard/race-mode
// IDs. The native name table at 828814D0 identifies Career=3 and SS_Career=10.
bool OpenRiderSelection(uint32_t mode){
  switch(mode){
    case 1: // Quick race.
    case 2: // Arcade, also selected by non-career multiplayer setup.
    case 4: // Vanilla championship.
    case 5: // Time trial.
    case 9: // Local split screen.
      return true;
    default:return false;
  }
}
}

REX_HOOK_RAW(sub_822FC460){
  const auto caller=ctx.lr;
  __imp__sub_822FC460(ctx,base);
  // 822586D8 is the native rider selector. At this exact return site a null
  // unlock record means "available"; a real record reads its locked byte at
  // +32. Only change this query result, never the record or profile progress.
  // Other callers, unknown modes and both career modes retain native behavior.
  if(caller==0x82258A40 && ctx.r3.u32 &&
      OpenRiderSelection(rex::memory::load_and_swap<uint32_t>(base+0x828DA060))){
    ctx.r3.u64=0;
  }
}
