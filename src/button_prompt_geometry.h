#pragma once

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
#include "button_prompt_types.h"
#include "../third_party/stb/stb_easy_font.h"

namespace motogp::prompts {
inline float LabelAspect(const std::string& label) {
  return std::clamp(.35f+float(label.size())*.39f,1.f,12.f);
}
struct Point {float x,y;};
struct Quad {std::array<Point,4> points;uint32_t color;};

// Solid quads go through the game's own untextured UI material. This keeps
// native clipping, layering, fades, render scaling and presentation effects.
class Geometry {
 public:
  std::vector<Quad> quads;
  void Rect(float x,float y,float w,float h,uint32_t color) {
    quads.push_back({{{{x,y},{x+w,y},{x+w,y+h},{x,y+h}}},color});
  }
  void Line(Point a,Point b,float thickness,uint32_t color,float aspect=1) {
    const float dx=(b.x-a.x)*aspect,dy=b.y-a.y;
    const float length=std::hypot(dx,dy);
    if(length==0) return;
    const float ox=-dy/length*thickness*.5f/aspect,oy=dx/length*thickness*.5f;
    quads.push_back({{{{a.x-ox,a.y-oy},{b.x-ox,b.y-oy},{b.x+ox,b.y+oy},{a.x+ox,a.y+oy}}},color});
  }
  void Circle(Point center,float radius,float thickness,uint32_t color) {
    constexpr unsigned count=24;
    for(unsigned i=0;i<count;++i) {
      const float a=i*6.2831853f/count,b=(i+1)*6.2831853f/count;
      Line({center.x+std::cos(a)*radius,center.y+std::sin(a)*radius},
           {center.x+std::cos(b)*radius,center.y+std::sin(b)*radius},thickness,color);
    }
  }
  void Disc(Point center,float radius,uint32_t color) {
    constexpr unsigned count=24;
    for(unsigned i=0;i<count;++i) {
      const float a=i*6.2831853f/count,b=(i+1)*6.2831853f/count;
      quads.push_back({{{center,{center.x+std::cos(a)*radius,center.y+std::sin(a)*radius},
          {center.x+std::cos(b)*radius,center.y+std::sin(b)*radius},center}},color});
    }
  }
  void Text(std::string label,float aspect,uint32_t color,float height_fraction=.62f) {
    // stb_easy_font supplies ASCII stroke geometry, not a platform font or
    // texture. It therefore also works in the native Xbox UI draw batch.
    struct Vertex {float x,y,z;uint8_t color[4];};
    std::array<Vertex,2048> vertices{};
    const float text_width=float(stb_easy_font_width(label.data()));
    const float text_height=12;
    const float scale=std::min(height_fraction/text_height,(aspect-.22f)/std::max(1.f,text_width));
    const float offset_x=(aspect-text_width*scale)*.5f,offset_y=(1-text_height*scale)*.5f;
    const int count=stb_easy_font_print(0,0,label.data(),nullptr,vertices.data(),sizeof(vertices));
    for(int q=0;q<count;++q) {
      Quad quad{{},color};
      for(unsigned v=0;v<4;++v)quad.points[v]={(offset_x+vertices[q*4+v].x*scale)/aspect,offset_y+vertices[q*4+v].y*scale};
      quads.push_back(quad);
    }
  }
};
inline Geometry MakeGeometry(char glyph,Style style,const std::string& label,float aspect,uint32_t color) {
  Geometry g;
  const uint32_t background=(color&0xFF000000)|0x15181D;
  if(style==Style::Keyboard) {
    g.Rect(.01f,.13f,.98f,.74f,background);
    g.Rect(.01f,.13f,.98f,.045f,color);g.Rect(.01f,.825f,.98f,.045f,color);
    g.Rect(.01f,.13f,.045f/aspect,.74f,color);g.Rect(.99f-.045f/aspect,.13f,.045f/aspect,.74f,color);
    g.Text(label,aspect,color);
  } else {
    if(glyph=='A'||glyph=='B'||glyph=='X'||glyph=='Y'||glyph=='S')g.Disc({.5f,.5f},.43f,background);
    switch(glyph) {
      case 'A': g.Line({.23f,.23f},{.77f,.77f},.075f,color);g.Line({.77f,.23f},{.23f,.77f},.075f,color);break;
      case 'B': g.Circle({.5f,.5f},.31f,.075f,color);break;
      case 'X':
        g.Line({.23f,.23f},{.77f,.23f},.075f,color);g.Line({.77f,.23f},{.77f,.77f},.075f,color);
        g.Line({.77f,.77f},{.23f,.77f},.075f,color);g.Line({.23f,.77f},{.23f,.23f},.075f,color);break;
      case 'Y':
        g.Line({.5f,.19f},{.81f,.77f},.075f,color);g.Line({.81f,.77f},{.19f,.77f},.075f,color);
        g.Line({.19f,.77f},{.5f,.19f},.075f,color);break;
      case 'S':
        // DualSense Create / DualShock Share symbol: three outward rays.
        g.Line({.5f,.42f},{.5f,.12f},.075f,color);
        g.Line({.4f,.49f},{.19f,.29f},.075f,color);
        g.Line({.6f,.49f},{.81f,.29f},.075f,color);
        g.Rect(.38f,.64f,.24f,.11f,color);break;
      default:
        g.Rect(.05f,.18f,.9f,.64f,background);
        g.Line({.05f,.18f},{.95f,.18f},.045f,color,aspect);
        g.Line({.95f,.18f},{.95f,.82f},.045f,color,aspect);
        g.Line({.95f,.82f},{.05f,.82f},.045f,color,aspect);
        g.Line({.05f,.82f},{.05f,.18f},.045f,color,aspect);
        g.Text(label,aspect,color,.48f);break;
    }
  }
  return g;
}
}
