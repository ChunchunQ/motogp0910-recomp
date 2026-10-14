#pragma once
#include <rex/ui/window.h>
#include <rex/ppc/context.h>
namespace motogp::wide {
void Start(rex::ui::Window* window);
void Stop();
void SetRatio(uint32_t x,uint32_t y);
float HorizontalScale();
void DrawUiQuad(PPCContext& ctx,uint8_t* base);
}
