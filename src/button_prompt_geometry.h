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
inline Geometry MakeKeyboardGeometry(const std::string& label,float aspect,uint32_t color) {
  Geometry g;
  const uint32_t background=(color&0xFF000000)|0x15181D;
  g.Rect(.01f,.13f,.98f,.74f,background);
  g.Rect(.01f,.13f,.98f,.045f,color);g.Rect(.01f,.825f,.98f,.045f,color);
  g.Rect(.01f,.13f,.045f/aspect,.74f,color);g.Rect(.99f-.045f/aspect,.13f,.045f/aspect,.74f,color);
  g.Text(label,aspect,color);
  return g;
}
}
