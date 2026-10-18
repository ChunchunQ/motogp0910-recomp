// Native CPU integration: real title MSG relocation/lookup/UTF-8 decoder,
// font metric parser, Options list traversal, and SDK read-only VFS routing.
#include "motogp0910_init.h"
#include "../src/localization.h"
#include "../src/options_menu.h"
#include "../src/keyboard_binding_specs.h"
#include <rex/cvar.h>
#include <rex/kernel/init.h>
#include <rex/runtime.h>
#include <rex/filesystem/vfs.h>
#include <rex/filesystem/file.h>
#include <array>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
REX_EXTERN(sub_826EDB80);
REX_EXTERN(sub_826EDB38);
REX_EXTERN(sub_826EC558);
REX_EXTERN(sub_822D7A70);
REX_EXTERN(sub_8227F318);
REX_EXTERN(sub_8227D3E8);
REX_EXTERN(sub_822F0AD0);
namespace {
int checks=0;
void Check(bool ok,const char* what){if(!ok)throw std::runtime_error(what);++checks;}
std::vector<uint8_t> Read(rex::filesystem::VirtualFileSystem* vfs,const std::string& path){
 auto* entry=vfs->ResolvePath(path);Check(entry,"VFS entry exists");rex::filesystem::File* file=nullptr;
 Check(!XFAILED(entry->Open(rex::filesystem::FileAccess::kFileReadData,&file))&&file,"VFS read opens");
 std::vector<uint8_t> bytes(entry->size());size_t read=0;const auto status=file->ReadSync(bytes,0,&read);file->Destroy();
 Check(!XFAILED(status)&&read==bytes.size(),"VFS reads complete overlay bytes");return bytes;
}
std::vector<uint8_t> OpenRead(rex::filesystem::VirtualFileSystem* vfs,const std::string& path,rex::filesystem::Entry* root=nullptr){
 rex::filesystem::File* file=nullptr;rex::filesystem::FileAction action{};
 const auto opened=vfs->OpenFile(root,path,rex::filesystem::FileDisposition::kOpen,rex::filesystem::FileAccess::kFileReadData,false,true,&file,&action);
 if(XFAILED(opened)||!file)throw std::runtime_error("Guest OpenFile failed for "+path+" status="+std::to_string(opened));
 ++checks;
 std::vector<uint8_t> bytes(file->entry()->size());size_t read=0;const auto status=file->ReadSync(bytes,0,&read);file->Destroy();
 Check(!XFAILED(status)&&read==bytes.size(),"Actual guest open reads complete bytes");return bytes;
}
uint32_t U32(const uint8_t* p){return rex::memory::load_and_swap<uint32_t>(p);}
}
int main(int argc,char** argv){try{
 Check(argc==3,"Usage: localization_probe PATCHED_COMPLETE_GAME_ROOT OVERLAY_ROOT");
 rex::Runtime runtime(std::filesystem::canonical(argv[1]));rex::RuntimeConfig config{.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true};
 Check(!XFAILED(runtime.Setup(PPCImageConfig,std::move(config)))&&!XFAILED(runtime.LoadXexImage("game:\\default.xex")),"Load actual patched game");
 auto* base=runtime.virtual_membase();auto* mem=runtime.memory();auto* vfs=runtime.file_system();
 const auto write=[&](uint32_t at,uint32_t v){rex::memory::store_and_swap<uint32_t>(base+at,v);};
 const auto read=[&](uint32_t at){return U32(base+at);};
 Check(!motogp::localization::Turkish(),"Default language uses original data");
 const auto original=Read(vfs,"game:\\xenondata\\text\\english.msg");
 const auto original_path=vfs->ResolvePath("game:\\xenondata\\text\\english.msg")->absolute_path();
 const auto fallback=Read(vfs,"game:\\xenondata\\scripts\\bike_setup.english.lua");
 motogp::localization::Install(&runtime,"does-not-exist");
 Check(Read(vfs,"game:\\xenondata\\text\\english.msg")==original,"Disabled localization does not redirect assets");
 Check(rex::cvar::SetFlagByName("user_language","4")&&rex::cvar::SetFlagByName("game_text_language","turkish"),"Language flags register");
 motogp::localization::ApplyStartupLanguage();Check(rex::cvar::Query<uint32_t>("user_language")==1,"Turkish keeps supported English voice enum");
 const auto before=runtime.kernel_state()->loaded_achievements();Check(before.size()==50,"Original achievement count");
 const auto unlocked=runtime.kernel_state()->achievements().IsUnlocked(1);
 bool rejected=false;try{motogp::localization::Install(&runtime,"does-not-exist");}catch(const std::exception&){rejected=true;}
 Check(rejected&&Read(vfs,"game:\\xenondata\\text\\english.msg")==original,"Incomplete overlay fails before routing changes");
 motogp::localization::Install(&runtime,std::filesystem::canonical(argv[2]));
 const auto translated=Read(vfs,"game:\\xenondata\\text\\english.msg");
 Check(translated!=original&&U32(translated.data())==2102,"Turkish bank replaces only virtual text path");
 Check(OpenRead(vfs,"game:\\xenondata\\text\\english.msg")==translated,"Native OpenFile must read Turkish, not bypass a leaf symlink");
 Check(OpenRead(vfs,original_path)==translated,"Physical path alias uses the same translated bank");
 for(const char* directory:{"game:\\","game:\\xenondata","game:\\xenondata\\text"}){
  rex::filesystem::File* folder=nullptr;rex::filesystem::FileAction action{};
  Check(!XFAILED(vfs->OpenFile(nullptr,directory,rex::filesystem::FileDisposition::kOpen,rex::filesystem::FileAccess::kFileReadData,true,false,&folder,&action))&&folder,"Guest directory handle opens");
  const auto relative=std::string_view(directory)=="game:\\"?"xenondata\\text\\english.msg":std::string_view(directory)=="game:\\xenondata"?"text\\english.msg":"ENGLISH.MSG";
  const auto bytes=OpenRead(vfs,relative,folder->entry());folder->Destroy();
  Check(bytes==translated,"Directory-handle relative opens retain Turkish routing");
 }
 Check(OpenRead(vfs,"game:\\xenondata\\scripts\\bike_setup.english.lua")==fallback,"Nontranslated files fall back byte-exactly to original game data");
 for(const char* path:{"scripts/riders.english.lua","music/track_1_2009.ogg"}){
  // Read the expectation from the host, without populating the original VFS
  // entry first: this catches lost lazy children under directory symlinks.
  std::ifstream input(std::filesystem::path(argv[1])/"xenondata"/path,std::ios::binary);
  const std::vector<uint8_t> expected((std::istreambuf_iterator<char>(input)),{});
  std::string guest=std::string("game:\\xenondata\\")+path;std::replace(guest.begin(),guest.end(),'/','\\');
  Check(!expected.empty()&&OpenRead(vfs,guest)==expected,"Previously unopened scripts/audio remain byte-exact and accessible");
 }
 for(const char* path:{"scripts/bikes.english.lua","scripts/tracks.english.lua","scripts/upgrades.english.lua","scripts/unlockables.english.lua","scripts/credits.english.lua","fonts/impact_17pt.font","fonts/impact_small.font","fonts/arial_bold_17pt.font","fonts/arial_bold_small.font"}){
  std::ifstream input(std::filesystem::path(argv[2])/path,std::ios::binary);
  const std::vector<uint8_t> expected((std::istreambuf_iterator<char>(input)),{});
  std::string guest=std::string("game:\\xenondata\\")+path;std::replace(guest.begin(),guest.end(),'/','\\');
  Check(!expected.empty()&&OpenRead(vfs,guest)==expected,"All translated scripts/fonts load through actual guest OpenFile");
 }
 Check(Read(vfs,"GAME:\\XENONDATA\\TEXT\\ENGLISH.MSG")==translated,"Case-insensitive game paths select the overlay");
 const auto ach=runtime.kernel_state()->loaded_achievements();Check(ach.size()==before.size()&&ach[0].label=="Zafer","Actual runtime achievement store uses Turkish names");
 Check(runtime.kernel_state()->achievements().IsUnlocked(1)==unlocked,"Localization preserves achievement unlock state");
 for(size_t i=0;i<ach.size();++i)Check(ach[i].id==before[i].id&&ach[i].image_id==before[i].image_id&&ach[i].gamerscore==before[i].gamerscore,"Achievement IDs/icons/scores preserved");
 const auto fixture=mem->SystemHeapAlloc(0x20000),bank=mem->SystemHeapAlloc(static_cast<uint32_t>(translated.size()));Check(fixture&&bank,"Allocate guest fixtures");
 std::memset(base+fixture,0,0x20000);std::memcpy(base+bank,translated.data(),translated.size());
 const auto stack=fixture+0x1f000,handle=fixture+0x1000,iterator=fixture+0x1100;
 PPCContext relocate{};relocate.r3.u32=bank;relocate.r4.u32=0;sub_826EDB80(relocate,base);
 for(unsigned id=0;id<2102;++id){
  write(handle,id);PPCContext text{};text.r3.u32=handle;sub_826EDB38(text,base);
  const auto offset=U32(translated.data()+4+4*id)+4;
  Check(text.r3.u32==bank+offset,"Actual native MSG lookup returns correct Turkish string");
  write(iterator,text.r3.u32);
  // Compare every Unicode scalar against an independent host UTF-8 decode.
  const auto* p=translated.data()+offset;
  do{
   uint32_t cp=*p++;if(cp>=0xc0){unsigned tails=cp<0xe0?1:cp<0xf0?2:3;cp&=0x7f>>(tails+1);while(tails--)cp=(cp<<6)|(*p++&0x3f);}
   PPCContext decode{};decode.r3.u32=iterator;sub_826EC558(decode,base);Check(decode.r3.u32==cp,"Native UTF8 decoder preserves Turkish and inline button codepoints");
   if(!cp)break;
  }while(true);
 }
 const auto menu=fixture,screen=fixture+0x600;
 Check(read(motogp::options::kVtable+4)==0x8223B4E0&&read(motogp::options::kVtable+24)==0x8223B950,"Actual Options vtable matches hooks");
 write(menu,motogp::options::kVtable);for(unsigned i=0;i<6;++i)write(menu+32+4*i,i);write(menu+56,UINT32_MAX);write(screen+24,5);
 Check(motogp::options::RemoveMarketplace(base,menu,screen)&&read(menu+52)==UINT32_MAX&&read(screen+24)==4,"Native Marketplace ID terminates Options; old cursor clamps to Credits");
 Check(motogp::options::RemoveMarketplace(base,menu,screen),"Removal is idempotent");
 write(screen+24,UINT32_MAX);motogp::options::RemoveMarketplace(base,menu,screen);Check(read(screen+24)==UINT32_MAX,"Unselected animation sentinel remains untouched");
 for(unsigned i=0;i<6;++i){const auto row=menu+60+i*40;write(row+4,1);write(row+16,i<5?1:0);write(row+28,i<5?bank+U32(translated.data()+4+4*i)+4:0);}
 auto move=[&](int index,int delta){write(screen+24,index);PPCContext c{};c.r1.u32=stack;c.lr=0x12345678;c.r30.u64=0x1122334455667788;c.r31.u64=0x8877665544332211;c.r3.u32=menu+60;c.r4.u32=screen+24;c.r5.s64=delta;c.r6.u32=0;sub_8227F318(c,base);Check(c.r1.u32==stack&&c.lr==0x12345678&&c.r30.u64==0x1122334455667788&&c.r31.u64==0x8877665544332211,"Native navigation ABI unchanged");return read(screen+24);};
 Check(move(3,1)==4&&move(4,1)==0&&move(0,-1)==4,"Native navigation visits five functional Options rows and wraps");
 write(menu,0);Check(!motogp::options::RemoveMarketplace(base,menu,screen),"Unrelated menus unchanged");
 // Exercise the real Default layout constructor, independently of launcher labels.
 PPCContext defaults{};defaults.r3.u32=menu;sub_822F0AD0(defaults,base);
 constexpr std::array<uint32_t,24> default_actions{7,8,0,0,6,5,21,4,14,3,0,0,10,9,23,24,22,5,2,26,0,0,0,0};
 for(unsigned i=0;i<default_actions.size();++i)Check(read(menu+4*i)==default_actions[i],"Default native physical slots match the documented action table");
 Check(std::wstring_view(launcher::kBindings[0].english).starts_with(L"Gear up")&&std::wstring_view(launcher::kBindings[1].english).starts_with(L"Gear down"),"Launcher A/B descriptions agree with Default rather than Alternative A");
 motogp::options::Initialize(&runtime);write(motogp::options::kOnlineHandle,731);
 PPCContext online{};online.r3.u32=motogp::options::kOnlineHandle;sub_826EDB38(online,base);
 Check(online.r3.u32==motogp::options::OnlineLabel()&&std::strcmp(reinterpret_cast<char*>(base+online.r3.u32),"Online")==0,"Only the native multiplayer Online label is replaced, without rewriting original message files");
 for(auto [vtable,renderer]:{std::pair{motogp::options::kGameSettingsVtable,0x8223B120u},std::pair{motogp::options::kMultiplayerVtable,0x822483E0u},std::pair{motogp::options::kCreateVtable,0x82240570u},std::pair{motogp::options::kCustomVtable,0x822409F0u},std::pair{motogp::options::kOnlineCreateVtable,0x8224EEF8u}})
   Check(read(vtable+4)==renderer,"Actual title vtables match each guarded PC menu cleanup hook");
 const auto setup_menu=[&](uint32_t vtable,std::initializer_list<uint32_t> ids,unsigned selected){
   std::memset(base+menu,0,0x600);write(menu,vtable);unsigned i=0;for(auto id:ids)write(menu+32+4*i++,id);write(menu+32+4*i,UINT32_MAX);write(screen+24,selected);
 };
 setup_menu(motogp::options::kMultiplayerVtable,{0,1,2,3},2);
 Check(motogp::options::RemoveParty(base,menu,screen)&&read(menu+32)==0&&read(menu+36)==2&&read(menu+40)==3&&read(menu+44)==UINT32_MAX&&read(screen+24)==1,"Party removal compacts the list while retaining Splitscreen focus and original action IDs");
 Check(motogp::options::RemoveParty(base,menu,screen)&&read(screen+24)==1,"Party removal is idempotent");
 setup_menu(motogp::options::kGameSettingsVtable,{0,1,2,3,4,5},5);
 Check(motogp::options::RemoveStorage(base,menu,screen)&&read(menu+52)==UINT32_MAX&&read(menu+24)==6&&read(screen+24)==4,"Storage selector removal updates the dynamic native count and clamps old focus");
 for(auto vtable:{motogp::options::kCreateVtable,motogp::options::kCustomVtable,motogp::options::kOnlineCreateVtable}){
   setup_menu(vtable,{0,1,2,3,7,6,8},5);
   Check(motogp::options::RemovePremium(base,menu,screen)&&read(menu+48)==6&&read(menu+52)==8&&read(menu+56)==UINT32_MAX&&read(screen+24)==4,"Premium removal retains AI/private-slot IDs and moves their existing focus to the compacted row");
   for(unsigned i=0;i<7;++i){const auto row=menu+152+i*40;write(row+4,1);write(row+16,i<6?1:0);write(row+28,i<6?bank+U32(translated.data()+4+4*i)+4:0);}
   write(screen+24,5);PPCContext navigate{};navigate.r1.u32=stack;navigate.r3.u32=menu+152;navigate.r4.u32=screen+24;navigate.r5.s64=1;navigate.r6.u32=0;sub_8227F318(navigate,base);
   Check(read(screen+24)==0,"Real native navigation wraps past the removed Premium row without leaving a blank selectable slot");
 }
 setup_menu(0,{0,1,2,3,7,6,8},4);Check(!motogp::options::RemovePremium(base,menu,screen)&&read(menu+48)==7,"Unrelated menus retain all original rows");
 const auto font_table=fixture+0x1400,zone_table=fixture+0x1410,zone=fixture+0x1420,font_data=fixture+0x1440,texture=fixture+0x1460,client_font=fixture+0x1480;
 constexpr uint32_t fonts_slot=0x8292CD18,zones_slot=0x8292CD14;
 const auto saved_fonts=read(fonts_slot),saved_zones=read(zones_slot);
 write(fonts_slot,font_table);write(zones_slot,zone_table);write(font_table,font_data);write(zone_table,zone);write(zone+12,0xffffffe0);
 write(font_data,texture);write(font_data+4,368);write(font_data+8,fixture+0x6000);write(texture+8,1024);write(client_font,std::bit_cast<uint32_t>(1.f));
 // Exercise the title's real eight-float font parser with its memory-reader
 // fast path. This verifies serialization, axis reorder, and finite bearings.
 for(const char* name:{"impact_17pt","impact_small","arial_bold_17pt","arial_bold_small"}){
  const auto bytes=Read(vfs,std::string("game:\\xenondata\\fonts\\")+name+".font");
  const auto h=rex::memory::load_and_swap<uint16_t>(bytes.data()+18);Check(h==512||h==2048,"Extended native atlas height");
  const auto metric_offset=bytes.size()-368*32;Check(U32(bytes.data()+metric_offset-4)==368,"Native font glyph count preserved");
  const auto raw=fixture+0x2000,stream=fixture+0x1200,parsed_all=fixture+0x6000;
  std::memcpy(base+raw,bytes.data()+metric_offset,368*32);write(stream+8,raw);
  for(unsigned i=0;i<368;++i){const auto parsed=parsed_all+32*i;write(stream+12,i*32);PPCContext c{};c.r1.u32=stack;c.lr=0x12345678;c.r3.u32=parsed;c.r4.u32=stream;sub_822D7A70(c,base);
   constexpr unsigned order[]={0,2,1,3,4,5,6,7};for(unsigned j=0;j<8;++j)Check(read(parsed+4*j)==U32(bytes.data()+metric_offset+32*i+4*order[j]),"Native font parser reads exact generated metrics");
  }
  for(unsigned cp=32;cp<400;++cp){
   if(cp==130||cp==160)continue; // Native zero-width control / nonbreaking-space special cases.
   PPCContext c{};c.r1.u32=stack;c.lr=0x12345678;c.r3.u32=client_font;c.r4.u32=cp;sub_8227D3E8(c,base);
   const auto* m=bytes.data()+metric_offset+(cp-32)*32;const auto f=[&](unsigned j){return std::bit_cast<float>(U32(m+4*j));};
   Check(std::isfinite(c.f1.f64)&&std::abs(c.f1.f64-(f(1)-f(0)+f(4)+f(5))*1024)<.001,"Native font lookup and advance match the enlarged atlas");
  }
  for(unsigned cp:{199u,231u,286u,287u,304u,305u,214u,246u,350u,351u,220u,252u}){
   const auto* m=bytes.data()+metric_offset+(cp-32)*32;const auto f=[&](unsigned j){return std::bit_cast<float>(U32(m+4*j));};
   Check((f(1)-f(0))*1024>10&&(f(3)-f(2))*h>20&&f(2)>=.5f,"Turkish glyph has a real full-size cell in appended atlas");
  }
 }
 write(fonts_slot,saved_fonts);write(zones_slot,saved_zones);
 std::cout<<checks<<" native Turkish text, UTF8, font, VFS, achievement and Options navigation checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<"Localization probe failed: "<<e.what()<<'\n';return 1;}}
