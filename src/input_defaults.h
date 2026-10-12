#pragma once

#include <rex/cvar.h>

// Called after config/environment/CLI parsing, before input initialization.
// Double-clicking the game should provide the same keyboard controls as the
// former batch file, while respecting an explicit user preference.
inline void ApplyMotogpInputDefaults() {
  if (rex::cvar::GetFlagSource("mnk_mode") == rex::cvar::Source::kDefault) {
    rex::cvar::SetFlagByName("mnk_mode", "true");
  }
}
