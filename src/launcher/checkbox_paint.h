#pragma once
#include <windows.h>
#include <objidl.h>
#include <propidl.h>
#include <gdiplus.h>

namespace launcher {
inline void PaintCheckbox(HDC dc,const RECT& bounds,bool checked,bool enabled,bool pressed){
  using namespace Gdiplus;
  Graphics g(dc);g.SetSmoothingMode(SmoothingModeAntiAlias);
  const float width=float(bounds.right-bounds.left),height=float(bounds.bottom-bounds.top);
  const float x=float(bounds.left)+.5f,y=float(bounds.top)+.5f,w=width-1,h=height-1,r=width*.22f;
  GraphicsPath shape;
  shape.AddArc(x,y,r*2,r*2,180,90);shape.AddArc(x+w-r*2,y,r*2,r*2,270,90);
  shape.AddArc(x+w-r*2,y+h-r*2,r*2,r*2,0,90);shape.AddArc(x,y+h-r*2,r*2,r*2,90,90);shape.CloseFigure();
  const Color fill=checked?(enabled?(pressed?Color(255,207,49,39):Color(255,239,66,47)):Color(255,91,58,59)):Color(255,24,28,35);
  SolidBrush brush(fill);g.FillPath(&brush,&shape);
  Pen border(checked?fill:Color(255,66,73,85),1.f);g.DrawPath(&border,&shape);
  if(checked){
    Pen tick(enabled?Color(255,255,255,255):Color(255,159,165,176),width*.12f);
    tick.SetStartCap(LineCapRound);tick.SetEndCap(LineCapRound);tick.SetLineJoin(LineJoinRound);
    PointF points[]{{x+width*.23f,y+height*.50f},{x+width*.43f,y+height*.69f},{x+width*.78f,y+height*.29f}};
    g.DrawLines(&tick,points,3);
  }
}
}
