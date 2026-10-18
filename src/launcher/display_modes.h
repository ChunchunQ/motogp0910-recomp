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
inline std::vector<std::pair<int,int>> WindowSizesFor(std::pair<int,int> maximum,
    const std::vector<std::pair<int,int>>& reported={}) {
  std::vector<std::pair<int,int>> result{{0,0}};
  const auto add=[&](std::pair<int,int> size){if(WindowSizeFits(size,maximum)&&std::find(result.begin(),result.end(),size)==result.end())result.push_back(size);};
  for(auto size:std::array{std::pair{640,480},std::pair{800,600},std::pair{1024,576},std::pair{1024,768},
      std::pair{1152,864},std::pair{1280,720},std::pair{1280,768},std::pair{1280,800},std::pair{1280,960},std::pair{1280,1024},
      std::pair{1360,768},std::pair{1366,768},std::pair{1440,900},std::pair{1600,900},std::pair{1600,1024},std::pair{1600,1200},
      std::pair{1680,1050},std::pair{1920,1080},std::pair{1920,1200},std::pair{1920,1440},std::pair{2048,1152},std::pair{2048,1536},
      std::pair{2560,1080},std::pair{2560,1440},std::pair{2560,1600},std::pair{2880,1620},std::pair{3200,1800},std::pair{3440,1440},
      std::pair{3840,1080},std::pair{3840,1600},std::pair{3840,2160},std::pair{4096,2160},std::pair{5120,1440},std::pair{5120,2160},
      std::pair{5120,2880},std::pair{5760,1080},std::pair{6016,3384},std::pair{7680,2160},std::pair{7680,4320}})add(size);
  for(auto size:reported)add(size);
  add(maximum);
  std::sort(result.begin()+1,result.end(),[](auto a,auto b){return a.first*a.second==b.first*b.second?a.first<b.first:a.first*a.second<b.first*b.second;});
  return result;
}
}
