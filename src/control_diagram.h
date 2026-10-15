#pragma once
#include "button_prompt_types.h"
#include "button_prompt_geometry.h"
#include <rex/ppc/context.h>
#include <array>
#include <string>
namespace motogp::controls {
// Native rows are physical input slots; their action labels come from the
// selected Default / Alternative A / B mapping, not hard-coded English names.
inline constexpr std::array<unsigned,9> kPS3LeftRows{0,1,2,5,6,7,8,3,4};
bool IsDrawing();
std::array<std::string,18> KeyboardRows();
motogp::prompts::Geometry KeyboardDiagram(const std::array<std::string,18>& labels,uint32_t color);
bool DrawDiagram(PPCContext& context,uint8_t* base,motogp::prompts::Style style);
// Only the temporary native label array on the Controls renderer's own stack
// is reordered. The profile mapping and original game data remain untouched.
bool PrepareLabels(PPCContext& context,uint8_t* base,motogp::prompts::Style style);
}
