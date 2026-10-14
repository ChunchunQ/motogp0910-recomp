// Export the production keycap geometry for a visual review, outside the repo.
#include "../src/button_prompt_geometry.h"
#include <iomanip>
#include <iostream>

int main(){
  const std::array labels{"Space","Backspace","Enter","Tab","Shift","Ctrl+K","WASD","Arrows","Q","E","F","1"};
  std::cout<<std::setprecision(8)<<'[';
  bool first=true;
  for(auto label:labels){
    if(!first)std::cout<<',';first=false;
    const auto aspect=motogp::prompts::LabelAspect(label);
    const auto g=motogp::prompts::MakeKeyboardGeometry(label,aspect,0xFFFFFFFF);
    std::cout<<"{\"label\":\""<<label<<"\",\"aspect\":"<<aspect<<",\"quads\":[";
    for(size_t i=0;i<g.quads.size();++i){
      if(i)std::cout<<',';const auto& q=g.quads[i];
      std::cout<<"{\"color\":"<<q.color<<",\"points\":[";
      for(unsigned v=0;v<4;++v){if(v)std::cout<<',';std::cout<<'['<<q.points[v].x<<','<<q.points[v].y<<']';}
      std::cout<<"]}";
    }
    std::cout<<"]}";
  }
  std::cout<<"]\n";
}
