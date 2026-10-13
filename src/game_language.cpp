#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/memory.h>
#include <mutex>

REX_EXTERN(__imp__sub_8236DC30);
REX_HOOK_RAW(sub_8236DC30) {
  const auto language=rex::cvar::Query<uint32_t>("user_language");
  // Preserve the game's own override on direct launch without an explicit
  // language. A launcher choice takes priority over that engine tweak.
  if(rex::cvar::GetFlagSource("user_language")==rex::cvar::Source::kDefault ||
     (language!=1 && language!=3 && language!=4 && language!=5 && language!=6)) {
    __imp__sub_8236DC30(ctx,base);return;
  }
  static std::mutex selector_mutex;
  const std::lock_guard lock(selector_mutex);
  const auto property=rex::memory::load_and_swap<uint32_t>(base+0x83374448);
  if(!property){__imp__sub_8236DC30(ctx,base);return;}
  const auto value=rex::memory::load_and_swap<uint32_t>(base+property);
  struct Restore {
    uint8_t* address;uint32_t value;
    ~Restore(){rex::memory::store_and_swap<uint32_t>(address,value);}
  } restore{base+property,value};
  rex::memory::store_and_swap<uint32_t>(base+property,language);
  // Reuse the title's own enum-to-localization-file mapping.
  __imp__sub_8236DC30(ctx,base);
}
