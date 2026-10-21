#pragma once
#include <mutex>

namespace motogp {
// The pinned InputSystem rebuilds its device/owner vectors during state,
// capability, vibration and keystroke calls without an internal mutex. All
// application-owned polling/import entry points share this lock for the full
// runtime call. It does not replace locking inside runtime-owned XAM dialogs.
// It also serializes MnK binding edits against the runtime's string readers.
// Keep this separate from settings so quit/input-only consumers share it too.
inline std::mutex& InputStateMutex(){
  static std::mutex mutex;
  return mutex;
}
}  // namespace motogp
