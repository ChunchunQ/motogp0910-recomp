#pragma once
#include <dxgi1_2.h>
#include <windows.h>
#include <utility>
#include <algorithm>
#include "display_modes.h"
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
// The native target mode avoids exposing virtual DSR/supersampling desktop
// sizes as the physical monitor's maximum. Fall back to its current mode.
inline std::pair<int,int> HardwareDisplayMaximum(int monitor_index=0) {
  std::vector<DISPLAY_DEVICEW> displays;
  for(DWORD i=0;;++i){DISPLAY_DEVICEW device{};device.cb=sizeof(device);
    if(!EnumDisplayDevicesW(nullptr,i,&device,0))break;
    if((device.StateFlags&DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) && !(device.StateFlags&DISPLAY_DEVICE_MIRRORING_DRIVER))displays.push_back(device);
  }
  std::stable_sort(displays.begin(),displays.end(),[](const auto& a,const auto& b){
    return bool(a.StateFlags&DISPLAY_DEVICE_PRIMARY_DEVICE)>bool(b.StateFlags&DISPLAY_DEVICE_PRIMARY_DEVICE);});
  if(displays.empty())return {GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN)};
  const auto index=monitor_index>0 && monitor_index<=static_cast<int>(displays.size())?monitor_index-1:0;
  const auto& device=displays[index];
  for(unsigned retry=0;retry<3;++retry){
    UINT32 path_count=0,mode_count=0;
    if(GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS,&path_count,&mode_count)!=ERROR_SUCCESS)break;
    std::vector<DISPLAYCONFIG_PATH_INFO> paths(path_count);std::vector<DISPLAYCONFIG_MODE_INFO> modes(mode_count);
    auto result=QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS,&path_count,paths.data(),&mode_count,modes.data(),nullptr);
    if(result==ERROR_INSUFFICIENT_BUFFER)continue;
    if(result!=ERROR_SUCCESS)break;
    for(UINT32 i=0;i<path_count;++i){const auto& path=paths[i];
      DISPLAYCONFIG_SOURCE_DEVICE_NAME source{};
      source.header.type=DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;source.header.size=sizeof(source);
      source.header.adapterId=path.sourceInfo.adapterId;source.header.id=path.sourceInfo.id;
      if(DisplayConfigGetDeviceInfo(&source.header)!=ERROR_SUCCESS || std::wstring(source.viewGdiDeviceName)!=device.DeviceName)continue;
      DISPLAYCONFIG_TARGET_PREFERRED_MODE target{};
      target.header.type=DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_PREFERRED_MODE;target.header.size=sizeof(target);
      target.header.adapterId=path.targetInfo.adapterId;target.header.id=path.targetInfo.id;
      if(DisplayConfigGetDeviceInfo(&target.header)==ERROR_SUCCESS && target.width && target.height){
        int width=static_cast<int>(target.width),height=static_cast<int>(target.height);
        if(path.targetInfo.rotation==DISPLAYCONFIG_ROTATION_ROTATE90 || path.targetInfo.rotation==DISPLAYCONFIG_ROTATION_ROTATE270)std::swap(width,height);
        return {width,height};
      }
    }
    break;
  }
  DEVMODEW mode{};mode.dmSize=sizeof(mode);
  if(EnumDisplaySettingsW(device.DeviceName,ENUM_CURRENT_SETTINGS,&mode))return {static_cast<int>(mode.dmPelsWidth),static_cast<int>(mode.dmPelsHeight)};
  return {GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN)};
}
inline std::vector<int> HardwareRefreshRates(int width,int height,int monitor_index=0) {
  std::vector<DISPLAY_DEVICEW> displays;
  for(DWORD i=0;;++i){DISPLAY_DEVICEW device{};device.cb=sizeof(device);
    if(!EnumDisplayDevicesW(nullptr,i,&device,0))break;
    if((device.StateFlags&DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) && !(device.StateFlags&DISPLAY_DEVICE_MIRRORING_DRIVER))displays.push_back(device);}
  std::stable_sort(displays.begin(),displays.end(),[](const auto& a,const auto& b){return bool(a.StateFlags&DISPLAY_DEVICE_PRIMARY_DEVICE)>bool(b.StateFlags&DISPLAY_DEVICE_PRIMARY_DEVICE);});
  std::vector<int> rates{0};if(displays.empty())return rates;
  const auto index=monitor_index>0 && monitor_index<=static_cast<int>(displays.size())?monitor_index-1:0;
  DEVMODEW desktop{};desktop.dmSize=sizeof(desktop);
  if(!EnumDisplaySettingsW(displays[index].DeviceName,ENUM_CURRENT_SETTINGS,&desktop))return rates;
  if(!width)width=desktop.dmPelsWidth;if(!height)height=desktop.dmPelsHeight;
  for(DWORD i=0;;++i){DEVMODEW mode{};mode.dmSize=sizeof(mode);
    if(!EnumDisplaySettingsW(displays[index].DeviceName,i,&mode))break;
    if(mode.dmPelsWidth==static_cast<DWORD>(width) && mode.dmPelsHeight==static_cast<DWORD>(height) && mode.dmDisplayFrequency>=24 && mode.dmDisplayFrequency<=1000 &&
       !(mode.dmDisplayFlags&DM_INTERLACED))rates.push_back(mode.dmDisplayFrequency);}
  std::sort(rates.begin(),rates.end());rates.erase(std::unique(rates.begin(),rates.end()),rates.end());return rates;
}
} // namespace launcher
