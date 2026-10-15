// Only the added native main-menu tile uses this white texture view. The
// original BC3 pixels, alpha, geometry, fades and all other menu tiles survive.
namespace {
rex::memory::Memory* accent_memory=nullptr;
thread_local uint32_t drawing_main_menu=0;
thread_local bool drawing_white_accent=false,binding_white_accent=false,drawing_quit_label=false;
std::mutex accent_mutex;
std::map<uint32_t,uint32_t> accent_descriptors,accent_materials;
std::set<uint32_t> white_materials;
void ResetAccent(rex::memory::Memory* memory){
  const std::lock_guard lock(accent_mutex);
  accent_memory=memory;accent_descriptors.clear();accent_materials.clear();white_materials.clear();
}
bool IsWhiteMaterial(uint32_t material){const std::lock_guard lock(accent_mutex);return white_materials.contains(material);}
uint32_t WhiteMaterial(uint8_t* base,uint32_t original){
  const std::lock_guard lock(accent_mutex);
  if(!accent_memory||!original)return 0;
  // Native mesh queue stores a material pointer for later use. Keep a separate
  // stable copy rather than changing shared material state around submission.
  if(const auto found=accent_materials.find(original);found!=accent_materials.end())return found->second;
  const auto copy=accent_memory->SystemHeapAlloc(144);if(!copy)return 0;
  std::memcpy(base+copy,base+original,144);accent_materials.emplace(original,copy);white_materials.insert(copy);return copy;
}
}
bool IsWhiteAccentDraw(uint8_t* base,uint32_t menu,uint32_t list,uint32_t index,uint64_t caller){
  return menu&&list&&Read(base,menu)==kMainMenuVtable&&index==kQuitIndex&&
      (caller==0x8228380C||caller==0x82283868)&&Read(base,list)==menu+kItemsOffset&&
      label_address&&Read(base,menu+kItemsOffset+kQuitIndex*kItemSize+28)==label_address;
}
bool IsQuitLabelDraw(uint8_t* base,uint32_t menu,uint32_t array,uint32_t index,uint64_t caller){
  return menu&&Read(base,menu)==kMainMenuVtable&&array==menu+kItemsOffset&&index==kQuitIndex&&
      caller==0x82283540&&label_address&&Read(base,array+index*kItemSize+28)==label_address;
}
bool IsWhiteCapPass(uint64_t caller){return caller==0x82280334||caller==0x822803A8;}
uint32_t WhiteAccentDescriptor(uint8_t* base,uint32_t original){
  const std::lock_guard lock(accent_mutex);
  if(!accent_memory||!original)return 0;
  if(const auto found=accent_descriptors.find(original);found!=accent_descriptors.end())return found->second;
  rex::graphics::xenos::xe_gpu_texture_fetch_t fetch{};
  for(unsigned i=0;i<6;++i)reinterpret_cast<uint32_t*>(&fetch)[i]=Read(base,original+28+4*i);
  if(fetch.type!=rex::graphics::xenos::FetchConstantType::kTexture)return 0;
  // Replicate the original visible blue channel: the blue cap becomes neutral
  // white/gray while retaining its brightness layers and original alpha.
  // Shared source pixels and the gray background passes are never modified.
  const unsigned blue=(fetch.swizzle>>6)&7;
  fetch.swizzle=(fetch.swizzle&0xE00)|blue|(blue<<3)|(blue<<6);
  const auto copy=accent_memory->SystemHeapAlloc(52);if(!copy)return 0;
  std::memcpy(base+copy,base+original,52);
  for(unsigned i=0;i<6;++i)Write(base,copy+28+4*i,reinterpret_cast<uint32_t*>(&fetch)[i]);
  accent_descriptors.emplace(original,copy);
  REXLOG_INFO("PC_QUIT_WHITE_ACCENT: native texture view created; original pixels and alpha retained");
  return copy;
}
