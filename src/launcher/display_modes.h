#pragma once
#include <algorithm>
#include <array>
#include <utility>
#include <vector>
namespace launcher {
inline bool WindowSizeFits(std::pair<int,int> size,std::pair<int,int> maximum) {
  return size==std::pair{0,0} || (size.first>0 && size.second>0 &&
      size.first<=maximum.first && size.second<=maximum.second &&
      size.first<=8192 && size.second<=8192);
}
inline std::vector<std::pair<int,int>> WindowSizesFor(std::pair<int,int> maximum) {
  std::vector<std::pair<int,int>> result{{0,0}};
  for(auto size:std::array{std::pair{1280,720},std::pair{1600,900},std::pair{1920,1080},
                          std::pair{2560,1440},std::pair{3840,2160},maximum})
    if(WindowSizeFits(size,maximum) && std::find(result.begin(),result.end(),size)==result.end())result.push_back(size);
  std::sort(result.begin()+1,result.end(),[](auto a,auto b){return a.first*a.second<b.first*b.second;});
  return result;
}
}
