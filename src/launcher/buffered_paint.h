#pragma once
#include <windows.h>

namespace launcher {
// Present the complete background, glyphs and text in one blit, including
// synchronous WM_DRAWITEM calls made outside the control's WM_PAINT handler.
template<class Draw> void PaintBuffered(HDC target,RECT area,Draw&& draw){
  const int width=area.right-area.left,height=area.bottom-area.top;
  if(width<=0||height<=0)return;
  const auto memory=CreateCompatibleDC(target);
  const auto bitmap=memory?CreateCompatibleBitmap(target,width,height):nullptr;
  if(!bitmap){if(memory)DeleteDC(memory);draw(target,area);return;}
  const auto previous=SelectObject(memory,bitmap);
  draw(memory,RECT{0,0,width,height});
  BitBlt(target,area.left,area.top,width,height,memory,0,0,SRCCOPY);
  SelectObject(memory,previous);DeleteObject(bitmap);DeleteDC(memory);
}
}
