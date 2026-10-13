#pragma once

#include "button_prompt_types.h"
#include <rex/ui/virtual_key.h>
#include <rex/ui/keybinds.h>
#include <array>
#include <string>
#include <string_view>

namespace motogp::prompts {
struct Glyph {
  char code;
  const char* playstation;
  bool shared;
  std::array<const char*,4> bindings;
};
// Verified against the native texture table, with the glyph ID written into
// the same rendered test frame. Case distinguishes shoulders from triggers.
inline constexpr std::array<Glyph,21> kGlyphs{{
  {'A',"",false,{"keybind_a"}}, {'B',"",false,{"keybind_b"}},
  {'Y',"",false,{"keybind_y"}}, {'X',"",false,{"keybind_x"}},
  {'T',"L",true,{"keybind_lstick_up","keybind_lstick_left","keybind_lstick_down","keybind_lstick_right"}},
  {'o',"L",true,{"keybind_lstick_up","keybind_lstick_left","keybind_lstick_down","keybind_lstick_right"}},
  {'O',"R",true,{"keybind_rstick_up","keybind_rstick_left","keybind_rstick_down","keybind_rstick_right"}},
  {'R',"R1",false,{"keybind_right_shoulder"}}, {'L',"L1",false,{"keybind_left_shoulder"}},
  {'r',"R2",false,{"keybind_right_trigger"}}, {'l',"L2",false,{"keybind_left_trigger"}},
  {'S',"Create",false,{"keybind_back"}},
  {'P',"D-pad",true,{"keybind_dpad_up","keybind_dpad_left","keybind_dpad_down","keybind_dpad_right"}},
  {'1',"Up",true,{"keybind_dpad_up"}}, {'3',"Down",true,{"keybind_dpad_down"}},
  {'4',"Left",true,{"keybind_dpad_left"}}, {'2',"Right",true,{"keybind_dpad_right"}},
  {'z',"L Up",true,{"keybind_lstick_up"}}, {'Z',"L Down",true,{"keybind_lstick_down"}},
  {'Q',"L Left",true,{"keybind_lstick_left"}}, {'q',"L Right",true,{"keybind_lstick_right"}},
}};
inline const Glyph* FindGlyph(char code) {
  for(const auto& glyph:kGlyphs)if(glyph.code==code)return &glyph;
  return nullptr;
}
inline std::string PrimaryLabel(std::string value) {
  if(value=="Semicolon,Space")value="Space";
  else if(value=="Quote,Backspace")value="Backspace";
  else if(value=="X,Return")value="Return";
  else if(value=="Z,Tab")value="Tab";
  for(size_t start=0;start<=value.size();) {
    const auto end=value.find(',',start);
    auto token=value.substr(start,end==std::string::npos?value.size()-start:end-start);
    const auto first=token.find_first_not_of(' '),last=token.find_last_not_of(' ');
    if(first!=std::string::npos) {
      token=token.substr(first,last-first+1);
      std::string_view key=token;
      while(true) {
        const auto plus=key.find('+');
        if(plus==std::string_view::npos)break;
        const auto modifier=key.substr(0,plus);
        if(modifier!="Shift" && modifier!="Ctrl" && modifier!="Control" && modifier!="Alt")break;
        key.remove_prefix(plus+1);
      }
      const auto vk=rex::ui::ParseVirtualKey(key);
      if(vk!=rex::ui::VirtualKey::kNone && vk!=rex::ui::VirtualKey::kShift &&
         vk!=rex::ui::VirtualKey::kControl && vk!=rex::ui::VirtualKey::kMenu) {
        const std::string suffix=key=="Return"?"Enter":key=="Semicolon"?";":key=="Quote"?"'":
            key=="PageUp"?"PgUp":key=="PageDown"?"PgDn":std::string(key);
        token.resize(token.size()-key.size());token+=suffix;return token;
      }
    }
    if(end==std::string::npos)break;
    start=end+1;
  }
  return "Unbound";
}
template<class ReadBinding>
std::string LabelFor(const Glyph& glyph,Style style,ReadBinding&& read) {
  if(style==Style::PlayStation)return glyph.playstation;
  std::array<std::string,4> labels;unsigned count=0;
  bool compact=true;
  for(auto* name:glyph.bindings)if(name) {
    labels[count]=PrimaryLabel(read(name));
    compact &= labels[count].size()==1;++count;
  }
  if(count==4) {
    std::array<std::string,4> keys;
    std::string prefix;bool same_prefix=true,single_keys=true;
    for(unsigned i=0;i<4;++i) {
      const auto plus=labels[i].find_last_of('+');
      const auto current=plus==std::string::npos?std::string{}:labels[i].substr(0,plus+1);
      if(i==0)prefix=current;else same_prefix &= current==prefix;
      keys[i]=labels[i].substr(current.size());single_keys &= keys[i].size()==1;
    }
    if(same_prefix && keys==std::array<std::string,4>{"Up","Left","Down","Right"})return prefix+"Arrows";
    if(same_prefix && single_keys)return prefix+keys[0]+keys[1]+keys[2]+keys[3];
  }
  std::string result;
  for(unsigned i=0;i<count;++i) {if(i&&!compact)result+='/';result+=labels[i];}
  return result;
}
}
