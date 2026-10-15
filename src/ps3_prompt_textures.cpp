#include "ps3_prompt_textures.h"
#include <rex/graphics/xenos.h>
#include <array>
#include <cstring>
#include <fstream>
#include <map>
#include <stdexcept>
#include <vector>

namespace motogp::prompts {
namespace {
struct Texture {uint32_t index,physical,wrapper,descriptor;bool configured=false;};
std::vector<Texture> textures;
rex::memory::Memory* guest_memory=nullptr;
Texture controller{};
uint32_t Index(char code) {
  switch(code) {
    case 'A':return 106;case 'B':return 107;case 'Y':return 108;case 'X':return 109;
    case 'T':case 'o':return 110;case 'O':return 112;
    case 'R':return 113;case 'L':return 114;case 'r':return 115;case 'l':return 116;
    case 'S':return 117;case 'P':return 135;
    case '1':return 136;case '3':return 137;case '4':return 138;case '2':return 139;
    case 'z':return 131;case 'Z':return 132;case 'Q':return 133;case 'q':return 134;
    default:return 0;
  }
}
}
void LoadPS3Artwork(rex::memory::Memory* memory,const std::filesystem::path& file) {
  StopPS3Artwork();
  std::ifstream stream(file,std::ios::binary);
  if(!stream)throw std::runtime_error("Missing resources/controller/ps3-prompts.bin. Extract the complete build ZIP.");
  std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(stream)),{});
  if(bytes.size()!=12+42*(16+4096) || std::memcmp(bytes.data(),"MGPS3BTN",8) ||
     rex::memory::load_and_swap<uint32_t>(bytes.data()+8)!=42)
    throw std::runtime_error("Invalid PS3 controller artwork pack");
  // Validate the entire pack before creating any GPU-visible allocation.
  for(unsigned i=0;i<42;++i) {
    const auto p=bytes.data()+12+i*(16+4096);
    if(rex::memory::load_and_swap<uint32_t>(p)!=102+i ||
       rex::memory::load_and_swap<uint16_t>(p+8)!=64 ||
       rex::memory::load_and_swap<uint16_t>(p+10)!=64 ||
       rex::memory::load_and_swap<uint32_t>(p+12)!=4096)
      throw std::runtime_error("Unsupported PS3 controller artwork record");
  }
  guest_memory=memory;
  for(unsigned i=0;i<42;++i) {
    const auto p=bytes.data()+12+i*(16+4096);
    // The GPU loader aligns compressed rows to 32 blocks even for linear
    // base textures. A 64-pixel BC3 image has 16 source blocks per row, but
    // needs 32 * 16 = 512 guest bytes per row (128-pixel pitch).
    const auto physical=memory->SystemHeapAlloc(8192,4096,rex::memory::kSystemHeapPhysical);
    const auto objects=memory->SystemHeapAlloc(128);
    if(!physical || !objects)throw std::runtime_error("Controller texture allocation failed");
    auto* destination=memory->TranslateVirtual(physical);
    std::memset(destination,0,8192);
    for(unsigned row=0;row<16;++row)
      std::memcpy(destination+row*512,p+16+row*256,256);
    textures.push_back({102+i,physical,objects,objects+64});
  }
}
void StopPS3Artwork() {
  // Guest memory owns these allocations. Keep them alive until runtime/GPU
  // shutdown, since previously submitted UI batches may still reference them.
  textures.clear();controller={};guest_memory=nullptr;
}
namespace {
uint32_t Configure(Texture& texture,uint32_t original,uint8_t* base,uint32_t width,uint32_t height,uint32_t pitch){
    if(!texture.configured) {
      const auto native=rex::memory::load_and_swap<uint32_t>(base+original);
      if(!native)return 0;
      std::memcpy(base+texture.wrapper,base+original,64);
      std::memcpy(base+texture.descriptor,base+native,52);
      // BC3 blocks retain PC/PS3 byte order; small textures have padded rows.
      rex::graphics::xenos::xe_gpu_texture_fetch_t fetch{};
      fetch.type=rex::graphics::xenos::FetchConstantType::kTexture;
      fetch.pitch=pitch/32; // Xenos stores pitch in units of 32 texels.
      fetch.format=rex::graphics::xenos::TextureFormat::k_DXT4_5;
      fetch.base_address=guest_memory->GetPhysicalAddress(texture.physical)>>12;
      fetch.size_2d.width=width-1;fetch.size_2d.height=height-1;
      fetch.swizzle=0x688; // XYZW, matching the game's RGBA UI textures.
      fetch.dimension=rex::graphics::xenos::DataDimension::k2DOrStacked;
      for(unsigned i=0;i<6;++i)
        rex::memory::store_and_swap<uint32_t>(base+texture.descriptor+28+4*i,reinterpret_cast<uint32_t*>(&fetch)[i]);
      rex::memory::store_and_swap<uint32_t>(base+texture.wrapper,texture.descriptor);
      rex::memory::store_and_swap<uint32_t>(base+texture.wrapper+4,2); // Native BC3 UI material.
      rex::memory::store_and_swap<uint32_t>(base+texture.wrapper+8,width);
      rex::memory::store_and_swap<uint32_t>(base+texture.wrapper+12,height);
      texture.configured=true;
    }
    return texture.wrapper;
}
}
void LoadPS3ControlArtwork(rex::memory::Memory* memory,const std::filesystem::path& file){
  std::ifstream input(file,std::ios::binary);std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)),{});
  if(bytes.size()!=28+524288||std::memcmp(bytes.data(),"MGPS3CTL",8)||rex::memory::load_and_swap<uint32_t>(bytes.data()+8)!=1||
     rex::memory::load_and_swap<uint32_t>(bytes.data()+12)!=251||rex::memory::load_and_swap<uint16_t>(bytes.data()+20)!=1024||
     rex::memory::load_and_swap<uint16_t>(bytes.data()+22)!=512||rex::memory::load_and_swap<uint32_t>(bytes.data()+24)!=524288)
    throw std::runtime_error("Missing or invalid resources/controller/ps3-controls.bin. Extract the complete build ZIP.");
  const auto physical=memory->SystemHeapAlloc(524288,4096,rex::memory::kSystemHeapPhysical),objects=memory->SystemHeapAlloc(128);
  if(!physical||!objects)throw std::runtime_error("Controller diagram allocation failed");
  std::memcpy(memory->TranslateVirtual(physical),bytes.data()+28,524288);
  guest_memory=memory;controller={251,physical,objects,objects+64};
}
bool PS3ControlArtworkReady(){return guest_memory&&controller.physical;}
uint32_t PS3ControlTexture(uint32_t original,uint8_t* base){return original&&PS3ControlArtworkReady()?Configure(controller,original,base,1024,512,1024):0;}
uint32_t PS3Texture(char glyph,uint32_t original,uint8_t* base) {
  const auto index=Index(glyph);
  if(!index || !guest_memory || !original)return 0;
  for(auto& texture:textures)if(texture.index==index)return Configure(texture,original,base,64,64,128);
  return 0;
}
}
