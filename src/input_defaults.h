#pragma once

#include <rex/cvar.h>
#include "keyboard_binding_specs.h"

// Called after config/environment/CLI parsing, before input initialization.
// Double-clicking the game should provide the same keyboard controls as the
// former batch file, while respecting an explicit user preference.
inline void ApplyMotogpInputDefaults() {
  if (rex::cvar::GetFlagSource("mnk_mode") == rex::cvar::Source::kDefault) {
    rex::cvar::SetFlagByName("mnk_mode", "true");
  }
  for(unsigned i=0;i<2;++i){const auto& binding=launcher::kBindings[i];
    const auto source=rex::cvar::GetFlagSource(binding.key);
    if(source==rex::cvar::Source::kDefault||(source==rex::cvar::Source::kConfig&&launcher::IsLegacyDefaultBinding(i,rex::cvar::GetFlagByName(binding.key))))rex::cvar::SetFlagByName(binding.key,binding.defaults);
  }
}
