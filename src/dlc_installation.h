#pragma once
#include "dlc_paths.h"
#include "launcher/dlc_packages.h"
#include <rex/system/xam/content_manager.h>
#include <algorithm>
#include <array>
#include <fstream>
#include <string>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace motogp::dlc {
// Shared local launch prerequisite; no server request or per-frame scanning.
inline bool InstallationLink(const std::filesystem::path& path){
#ifdef _WIN32
  const auto flags=GetFileAttributesW(path.c_str());
  return flags!=INVALID_FILE_ATTRIBUTES&&(flags&FILE_ATTRIBUTE_REPARSE_POINT);
#else
  return std::filesystem::is_symlink(std::filesystem::symlink_status(path));
#endif
}
inline bool Installed(const std::filesystem::path& root,const launcher::dlc::Package& spec){
  namespace fs=std::filesystem;
  try{
    const auto folder=Packages(root)/spec.id,header=Headers(root)/(std::string(spec.id)+".header");
    for(const auto& target:{folder,header}){
      auto current=root;if(InstallationLink(current))return false;
      for(const auto& part:target.lexically_relative(root)){current/=part;if(InstallationLink(current))return false;}
    }
    if(!fs::is_directory(folder)||!fs::is_regular_file(header))return false;
    rex::system::xam::XCONTENT_AGGREGATE_DATA data{};
    if(fs::file_size(header)!=sizeof(data)+(spec.license?sizeof(uint32_t):0))return false;
    std::ifstream metadata(header,std::ios::binary);metadata.read(reinterpret_cast<char*>(&data),sizeof(data));
    const std::string filename(data.file_name_raw,std::find(std::begin(data.file_name_raw),std::end(data.file_name_raw),'\0'));
    if(!metadata||data.title_id!=0x434307F0||data.content_type!=rex::system::XContentType::kMarketplaceContent||data.xuid!=0||filename!=spec.id)return false;
    if(spec.license){uint32_t mask=0;metadata.read(reinterpret_cast<char*>(&mask),sizeof(mask));if(!metadata||mask!=spec.license)return false;}
    const auto marker=folder/("DLCPack0"+std::to_string(spec.marker)+".meta");
    if(InstallationLink(marker)||!fs::is_regular_file(marker)||fs::file_size(marker)!=15)return false;
    std::ifstream input(marker,std::ios::binary);std::string text((std::istreambuf_iterator<char>(input)),{});if(text!="DLCPACK1PADDING")return false;
    uint64_t bytes=0;size_t count=0,entries=0;
    for(auto it=fs::recursive_directory_iterator(folder);it!=fs::recursive_directory_iterator();++it){
      if(++entries>4096||it.depth()>32||InstallationLink(it->path()))return false;
      if(it->is_regular_file()){
        const auto size=it->file_size();if(size>spec.payload_bytes-bytes)return false;
        bytes+=size;++count;
      }
    }
    return count==spec.files&&bytes==spec.payload_bytes;
  }catch(const std::exception&){return false;}
}
using Installation=std::array<bool,launcher::dlc::kPackages.size()>;
inline Installation InspectInstallation(const std::filesystem::path& root){
  Installation installed{};
  for(size_t i=0;i<installed.size();++i)installed[i]=Installed(root,launcher::dlc::kPackages[i]);
  return installed;
}
inline bool Complete(const Installation& installed){return std::all_of(installed.begin(),installed.end(),[](bool value){return value;});}
inline bool AllInstalled(const std::filesystem::path& root){return Complete(InspectInstallation(root));}
}
