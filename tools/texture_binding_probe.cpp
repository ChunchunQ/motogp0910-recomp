// Exercise the actual patched SDK CPU texture-key parser with captured descriptors.
#include <rex/graphics/pipeline/texture/cache.h>
#include <rex/cvar.h>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
struct Access:rex::graphics::TextureCache {
 using TextureCache::TextureKey;
 using TextureCache::BindingInfoFromFetchConstant;
};
int main(){try{
 unsigned checks=0;auto check=[&](bool ok,const char* what){if(!ok)throw std::runtime_error(what);++checks;};
 check(rex::cvar::SetFlagByName("gpu_allow_invalid_fetch_constants","false"),"Keep strict resource validation enabled");
 const std::array samples={
  std::array<uint32_t,6>{0x00000000u,0x00000000u,0x00000000u,0x01000000u,0x00000000u,0x00000000u},
  std::array<uint32_t,6>{0x00024800u,0x00000000u,0x00000000u,0x01280000u,0x00000003u,0x00000000u},
  std::array<uint32_t,6>{0x01000000u,0x0FED0052u,0x140FE07Fu,0x00A80D10u,0x00000003u,0x00000600u},
  std::array<uint32_t,6>{0x01000000u,0x183CF054u,0x140FE07Fu,0x00A80D10u,0x00000003u,0x00000600u},
  std::array<uint32_t,6>{0x01000000u,0x18F96054u,0x003FE07Fu,0x04A80D10u,0x00000CC3u,0x18F86A00u},
  std::array<uint32_t,6>{0x01024800u,0x1B4C0054u,0x001FE03Fu,0x00A80D10u,0x00000083u,0x1B4B8A00u},
  std::array<uint32_t,6>{0x02000000u,0x0F1F0052u,0x000FE0FFu,0x00A80D10u,0x000000C3u,0x0F1EAA00u},
  std::array<uint32_t,6>{0x02000000u,0x0F60F052u,0x141FE0FFu,0x00A80D10u,0x00000003u,0x00000600u},
  std::array<uint32_t,6>{0x02000000u,0x0FA73052u,0x141FE0FFu,0x00A80D10u,0x00000003u,0x00000600u},
  std::array<uint32_t,6>{0x02000000u,0x1045E052u,0x141FE0FFu,0x00A80D10u,0x00000003u,0x00000600u},
  std::array<uint32_t,6>{0x02000000u,0x110FD052u,0x141FE0FFu,0x00A80D10u,0x00000003u,0x00000600u},
  std::array<uint32_t,6>{0x02000000u,0x11DAF052u,0x007FE0FFu,0x00A80D10u,0x00000103u,0x11D9FA00u},
  std::array<uint32_t,6>{0x02000000u,0x11EA9052u,0x007FE0FFu,0x00A80D10u,0x00000103u,0x11E99A00u},
  std::array<uint32_t,6>{0x02000000u,0x11F41052u,0x001FE0FFu,0x00A80D10u,0x00000103u,0x11F39A00u},
  std::array<uint32_t,6>{0x02000000u,0x1281B052u,0x000FE0FFu,0x00A80D10u,0x000000C3u,0x12815A00u},
  std::array<uint32_t,6>{0x02000000u,0x12893052u,0x000FE0FFu,0x00A80D10u,0x000000C3u,0x1288DA00u},
  std::array<uint32_t,6>{0x02000000u,0x1437D052u,0x000FE0FFu,0x00A80D10u,0x000000C3u,0x14377A00u},
  std::array<uint32_t,6>{0x02000000u,0x15087052u,0x000FE0FFu,0x00A80D10u,0x000000C3u,0x15081A00u},
  std::array<uint32_t,6>{0x02000000u,0x15D8F052u,0x000FE0FFu,0x00A80D10u,0x000000C3u,0x15D89A00u},
  std::array<uint32_t,6>{0x02000000u,0x15E24052u,0x000FE0FFu,0x00A80D10u,0x000000C3u,0x15E1EA00u},
  std::array<uint32_t,6>{0x02000000u,0x18E2A052u,0x001FE0FFu,0x04A80D10u,0x00000D03u,0x18E22A00u},
  std::array<uint32_t,6>{0x02000000u,0x1AD42052u,0x001FE0FFu,0x04A80D10u,0x00000D03u,0x1AD3AA00u},
  std::array<uint32_t,6>{0x02000000u,0x1B488052u,0x141FE0FFu,0x00A80D10u,0x00000003u,0x00000600u},
  std::array<uint32_t,6>{0x04000000u,0x0FEA6052u,0x003FE1FFu,0x00A80D10u,0x00000143u,0x0FE96A00u},
  std::array<uint32_t,6>{0x04000000u,0x0FED6052u,0x003FE1FFu,0x00A80D10u,0x00000143u,0x0FEC6A00u},
  std::array<uint32_t,6>{0x04000000u,0x10362052u,0x003FE1FFu,0x04A80D10u,0x00000D43u,0x10352A00u},
  std::array<uint32_t,6>{0x04000000u,0x10C92052u,0x001FE1FFu,0x04A80D10u,0x00000D03u,0x10C88A00u},
  std::array<uint32_t,6>{0x04000000u,0x11C18052u,0x003FE1FFu,0x00A80D10u,0x00000143u,0x11C08A00u},
  std::array<uint32_t,6>{0x04000000u,0x12903052u,0x003FE1FFu,0x04A80D10u,0x00000D43u,0x128F3A00u},
  std::array<uint32_t,6>{0x04000000u,0x1884A052u,0x003FE1FFu,0x00A80D10u,0x00000143u,0x1883AA00u},
  std::array<uint32_t,6>{0x04000000u,0x18A19052u,0x003FE1FFu,0x00A80D10u,0x00000143u,0x18A09A00u},
  std::array<uint32_t,6>{0x04000000u,0x18B78052u,0x001FE1FFu,0x04A80D10u,0x00000D03u,0x18B6EA00u},
  std::array<uint32_t,6>{0x04000000u,0x18B98052u,0x003FE1FFu,0x04A80D10u,0x00000D43u,0x18B88A00u},
  std::array<uint32_t,6>{0x04000000u,0x18F12052u,0x003FE1FFu,0x00A80D10u,0x00000143u,0x18F02A00u},
  std::array<uint32_t,6>{0x04000000u,0x1A330054u,0x007FE1FFu,0x00A80D10u,0x00000283u,0x1A2FCA00u},
  std::array<uint32_t,6>{0x08000000u,0x1A464054u,0x00FFE3FFu,0x00A80D10u,0x000002C3u,0x1A3B0A00u},
  std::array<uint32_t,6>{0x10000000u,0x14385052u,0x007FE7FFu,0x04A80D10u,0x00000D83u,0x1432BA00u},
  std::array<uint32_t,6>{0x82824800u,0x1DE4C086u,0x0016613Fu,0x01280C14u,0x00000003u,0x00000200u},
 };
 for(const auto& words:samples){
  rex::graphics::xenos::xe_gpu_texture_fetch_t fetch{};std::memcpy(&fetch,words.data(),sizeof(fetch));
  Access::TextureKey key{};key.is_valid=1;uint8_t signs=0xFF;
  check(fetch.type==rex::graphics::xenos::FetchConstantType::kInvalidTexture,"Captured descriptor is unbound type zero");
  Access::BindingInfoFromFetchConstant(fetch,key,&signs);
  check(!key.is_valid&&signs==0,"Unbound/stale descriptor remains null with unsigned default signs");
 }
 rex::graphics::xenos::xe_gpu_texture_fetch_t valid{};
 const uint32_t words[6]={0x04000002,0x18A19052,0x003FE1FF,0x00A80D10,0x00000143,0x18A09A00};std::memcpy(&valid,words,sizeof(valid));
 Access::TextureKey key{};Access::BindingInfoFromFetchConstant(valid,key,nullptr);
 check(key.is_valid,"A genuinely bound texture is still validated and receives a real key");
 const auto page=key.base_page;valid.type=rex::graphics::xenos::FetchConstantType::kInvalidTexture;
 Access::BindingInfoFromFetchConstant(valid,key,nullptr);check(!key.is_valid,"Unbinding a previous real texture cannot reuse its stale key");
 valid.type=rex::graphics::xenos::FetchConstantType::kTexture;Access::BindingInfoFromFetchConstant(valid,key,nullptr);check(key.is_valid&&key.base_page==page,"Rebinding the same resource restores the original key");
 std::cout<<checks<<" real texture-key parser checks passed for "<<samples.size()<<" unique captured null descriptors\n";return 0;
}catch(const std::exception& e){std::cerr<<"Texture binding probe failed: "<<e.what()<<'\n';return 1;}}
