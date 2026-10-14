#pragma once

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
#include "button_prompt_types.h"
#include "keyboard_font_data.h"

namespace motogp::prompts {
inline float KeyboardTextWidth(const std::string& label) {
  float width=0;
  for(unsigned char code:label)width+=font::kGlyphs[(code>=32 && code<=126?code:'?')-32].advance/64.f;
  return width;
}
inline float LabelAspect(const std::string& label) {
  return std::clamp(.38f+.74f*KeyboardTextWidth(label)/font::kHeight,1.f,12.f);
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
  void RoundedRect(float x,float y,float w,float h,float radius,float aspect,uint32_t color) {
    radius=std::min({radius,h*.5f,w*aspect*.5f});
    Rect(x,y+radius,w,h-2*radius,color);
    constexpr unsigned slices=8;
    for(unsigned i=0;i<slices;++i){
      const float a=radius*i/slices,b=radius*(i+1)/slices;
      const float dy=radius-(a+b)*.5f;
      const float inset=(radius-std::sqrt(std::max(0.f,radius*radius-dy*dy)))/aspect;
      Rect(x+inset,y+a,w-2*inset,b-a,color);
      Rect(x+inset,y+h-b,w-2*inset,b-a,color);
    }
  }
  static uint32_t Blend(uint32_t background,uint32_t foreground,unsigned coverage) {
    uint32_t result=foreground&0xFF000000;
    for(unsigned shift:{0u,8u,16u}){
      const auto b=(background>>shift)&255,f=(foreground>>shift)&255;
      result|=((b*(15-coverage)+f*coverage+7)/15)<<shift;
    }
    return result;
  }
  void Text(const std::string& label,float aspect,uint32_t color,uint32_t background,float height_fraction=.74f) {
    const float text_width=KeyboardTextWidth(label);
    const float scale=std::min(height_fraction/font::kHeight,(aspect-.30f)/std::max(1.f,text_width));
    float x=(aspect-text_width*scale)*.5f;
    const float y=(1-font::kHeight*scale)*.5f-.015f;
    for(unsigned char code:label){
      const auto glyph=font::kGlyphs[(code>=32 && code<=126?code:'?')-32];
      for(unsigned i=0;i<glyph.count;++i){
        const auto run=font::kRuns[glyph.first+i];
        Rect((x+run.x*scale)/aspect,y+run.y*scale,run.width*scale/aspect,run.height*scale,
             Blend(background,color,run.coverage));
      }
      x+=glyph.advance/64.f*scale;
    }
  }
};
inline Geometry MakeKeyboardGeometry(const std::string& label,float aspect,uint32_t color) {
  Geometry g;
  if(!std::isfinite(aspect) || aspect<=0)return g;
  const auto alpha=color&0xFF000000;
  const auto face=alpha|0x242A32,edge=alpha|0x4C545F;
  g.RoundedRect(.025f,.17f,.95f,.76f,.10f,aspect,alpha|0x090C10);
  g.RoundedRect(.01f,.075f,.98f,.79f,.11f,aspect,edge);
  g.RoundedRect(.025f,.10f,.95f,.72f,.09f,aspect,face);
  // A compact warm accent follows the cover/launcher palette. Avoid a thick
  // bright frame, which competes with the action text at native 720p.
  const float accent=std::min(.34f/aspect,.50f);
  g.Rect((1-accent)*.5f,.112f,accent,.032f,alpha|0xFFAE24);
  g.Text(label,aspect,alpha|0xF3F4F6,face);
  return g;
}
}
