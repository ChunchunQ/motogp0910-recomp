#pragma once
#include <dxgi1_2.h>
#include <string>
#include <vector>

namespace launcher {
struct Adapter { int index; std::wstring name; };
inline std::vector<Adapter> HardwareAdapters() {
  std::vector<Adapter> result;
  IDXGIFactory1* factory=nullptr;
  if(FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1),reinterpret_cast<void**>(&factory)))) return result;
  // ReXGlue's D3D12 provider also uses EnumAdapters1. Preserve the original
  // index when skipping software adapters; never renumber the visible list.
  for(UINT index=0;;++index) {
    IDXGIAdapter1* adapter=nullptr;
    if(factory->EnumAdapters1(index,&adapter)!=S_OK) break;
    DXGI_ADAPTER_DESC1 info{};
    if(SUCCEEDED(adapter->GetDesc1(&info)) && !(info.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)) {
      std::wstring name=info.Description;
      if(info.DedicatedVideoMemory) {
        auto mib=info.DedicatedVideoMemory/(1024*1024);
        name+=L" ("+std::to_wstring(mib)+L" MB)";
      }
      result.push_back({static_cast<int>(index),std::move(name)});
    }
    adapter->Release();
  }
  factory->Release();return result;
}
} // namespace launcher
