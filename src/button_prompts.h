#pragma once

#include <rex/ui/window.h>
#include <rex/ppc/context.h>
#include "button_prompt_types.h"

namespace motogp::prompts {
// Replacement geometry uses the game's original UI draw batch and layout.
void StartInputObserver(rex::ui::Window* window);
void StopInputObserver();
// Shared by the native draw hook and the integration probe that checks real
// generated UI batching, guest vertex restoration and the guest calling ABI.
bool DrawReplacement(PPCContext& context,uint8_t* base,Style style,bool inline_glyph=false);
float MeasureButtonHalfWidth(char code,float original,Style style);
}
