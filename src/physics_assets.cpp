#include <rex/hook.h>
#include <rex/memory.h>
#include <rex/logging.h>
#include <array>
#include <atomic>
#include <bit>

REX_EXTERN(__imp__sub_823559C8);
REX_EXTERN(__imp__sub_82355AA8);
namespace {
// Serialized collision floats are little-endian. Until reversed, their bits
// can resemble signaling NaNs. The generated lfs -> double -> stfs sequence
// quiets these NaNs and changes a bit in the eventual finite coordinate.
// Preserve the serialized bits; keep the original routine's integer fields,
// full call state and return value by repairing only its float-field stores.
template<size_t N>
void ConvertCollisionRecord(PPCContext& ctx,uint8_t* base,
    const std::array<unsigned,N>& offsets,void (*original)(PPCContext&,uint8_t*),const char* kind){
  const auto record=ctx.r3.u32;std::array<uint32_t,N> expected;
  for(size_t i=0;i<N;++i)expected[i]=std::byteswap(rex::memory::load_and_swap<uint32_t>(base+record+offsets[i]));
  original(ctx,base);
  unsigned changed=0;
  for(size_t i=0;i<N;++i){
    if(rex::memory::load_and_swap<uint32_t>(base+record+offsets[i])!=expected[i]){
      rex::memory::store_and_swap<uint32_t>(base+record+offsets[i],expected[i]);++changed;
    }
  }
  if(changed){
    static std::atomic<uint32_t> repaired{0};const auto count=++repaired;
    if(count<=8 || (count&(count-1))==0)
      REXLOG_DEBUG("PHYSICS_ASSET_FIX kind={} repaired_records={} corrected_fields={}",kind,count,changed);
  }
}
}
REX_HOOK_RAW(sub_823559C8){
  constexpr std::array<unsigned,14> fields{8,12,20,24,32,36,44,48,52,56,60,64,68,72};
  ConvertCollisionRecord(ctx,base,fields,__imp__sub_823559C8,"triangle");
}
REX_HOOK_RAW(sub_82355AA8){
  constexpr std::array<unsigned,4> fields{24,28,32,36};
  ConvertCollisionRecord(ctx,base,fields,__imp__sub_82355AA8,"shape");
}
