// Real packaged kernel/VFS checks for the Motegi QA log's optional I/O probes.
#include "motogp0910_init.h"
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/filesystem/vfs.h>
#include <rex/filesystem/devices/host_path_device.h>
#include <rex/system/xfile.h>
#include <rex/system/info/file.h>
#include <rex/system/info/volume.h>
#include <filesystem>
#include <array>
#include <fstream>
#include <iostream>
#include <cstring>
REX_EXTERN(__imp__NtQueryInformationFile);
REX_EXTERN(__imp__NtQueryVolumeInformationFile);
REX_EXTERN(__imp__IoDismountVolumeByFileHandle);
using rex::X_STATUS;
namespace {
unsigned checks=0;
void Check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);++checks;}
struct FileOwner {
 rex::system::XFile* value;
 ~FileOwner(){value->ReleaseHandle();value->Release();}
};
}
int main(int argc,char** argv){try{
 Check(argc==2,"Usage: file_compat_probe COMPLETE_GAME_ROOT");
 rex::Runtime runtime(std::filesystem::canonical(argv[1]));
 Check(!XFAILED(runtime.Setup(PPCImageConfig,{.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true})),"Real runtime initialized");
 auto* vfs=runtime.file_system();auto* base=runtime.virtual_membase();
 Check(!vfs->ResolvePath("ShaderDumpxe:\\CompareBackEnds"),"Optional developer shader comparison directory remains absent without a warning");
 auto* game=vfs->ResolvePath("game:\\");Check(game,"Original game volume resolves");
 vfs->RegisterSymbolicLink("\\Device\\Image",game->absolute_path());
 for(const std::string& path:std::array<std::string,4>{"\\Device\\Image","\\device\\image","\\Device\\Image\\",game->absolute_path()}){
  rex::filesystem::File* f=nullptr;rex::filesystem::FileAction action{};
  Check(!XFAILED(vfs->OpenFile(nullptr,path,rex::filesystem::FileDisposition::kOpen,rex::filesystem::FileAccess::kFileReadData,true,false,&f,&action))&&f,"Exact image/device root opens through real VFS OpenFile");
  Check(f->entry()==game,"Image alias points at the actual game root rather than a fabricated file");f->Destroy();
 }
 const auto fixture=runtime.memory()->SystemHeapAlloc(4096);Check(fixture,"Guest buffers allocated");
 auto query=[&](rex::system::XFile* file,bool volume,uint32_t cls,uint32_t length,uint32_t handle=0){
  PPCContext c{};c.r3.u32=handle?handle:file->handle();c.r4.u32=fixture;c.r5.u32=fixture+32;c.r6.u32=length;c.r7.u32=cls;
  if(volume)__imp__NtQueryVolumeInformationFile(c,base);else __imp__NtQueryInformationFile(c,base);
  return c.r3.u32;
 };
 auto open=[&](const std::string& path){rex::filesystem::File* f=nullptr;rex::filesystem::FileAction action{};
  Check(!XFAILED(vfs->OpenFile(nullptr,path,rex::filesystem::FileDisposition::kOpen,rex::filesystem::FileAccess::kFileReadData,false,true,&f,&action))&&f,"Actual file opens");return new rex::system::XFile(runtime.kernel_state(),f,true);};
 {
  FileOwner file{open("game:\\default.xex")};file.value->set_position(17);
  for(int i=0;i<702;++i){
   std::memset(base+fixture,0xA5,96);
   Check(query(file.value,false,rex::system::XFileXctdCompressionInformation,4)==X_STATUS_INVALID_PARAMETER,"Uncompressed host file returns the native negative XCTD capability result");
   Check(rex::memory::load_and_swap<uint32_t>(base+fixture)==X_STATUS_INVALID_PARAMETER&&rex::memory::load_and_swap<uint32_t>(base+fixture+4)==0,"Negative compression probe reports zero output bytes");
   Check(file.value->position()==17&&rex::memory::load_and_swap<uint32_t>(base+fixture+32)==0,"Compression query preserves file position and initializes output");
  }
  std::memset(base+fixture,0xA5,96);
  Check(query(file.value,false,rex::system::XFileXctdCompressionInformation,3)==X_STATUS_INFO_LENGTH_MISMATCH,"Short compression buffer is rejected");
  Check(rex::memory::load_and_swap<uint32_t>(base+fixture+32)==0xA5A5A5A5,"Short buffer cannot overwrite caller data");
  Check(query(file.value,false,rex::system::XFileXctdCompressionInformation,4,0xDEAD1234)==X_STATUS_INVALID_HANDLE,"Invalid compression handle remains rejected");
  Check(query(file.value,true,rex::system::XFileFsDeviceInformation,8)==X_STATUS_SUCCESS,"Host volume device information implemented");
  Check(rex::memory::load_and_swap<uint32_t>(base+fixture+32)==7,"Host-backed volume identifies as a disk");
  Check(rex::memory::load_and_swap<uint32_t>(base+fixture+36)==(0x20u|(file.value->device()->is_read_only()?2u:0u)),"Volume characteristics reflect mount/read-only state");
  Check(query(file.value,true,rex::system::XFileFsDeviceInformation,7)==X_STATUS_INFO_LENGTH_MISMATCH,"Short volume buffer is rejected");
  Check(query(file.value,true,rex::system::XFileFsDeviceInformation,8,0xDEAD1234)==X_STATUS_INVALID_HANDLE,"Invalid volume handle is rejected");
  Check(query(file.value,false,rex::system::XFileNetworkOpenInformation,56)==X_STATUS_SUCCESS,"Other file information classes retain real metadata");
  Check(rex::memory::load_and_swap<uint64_t>(base+fixture+72)==std::filesystem::file_size(std::filesystem::path(argv[1])/"default.xex"),"File size remains real");
  uint8_t before[4]{},after[4]{};size_t got=0;Check(!XFAILED(file.value->file()->ReadSync(before,0,&got))&&got==4,"Read before virtual dismount");
  PPCContext dismount{};dismount.r3.u32=file.value->handle();__imp__IoDismountVolumeByFileHandle(dismount,base);Check(dismount.r3.u32==X_STATUS_SUCCESS,"Host-directory dismount compatibility succeeds without physical host changes");
  Check(!XFAILED(file.value->file()->ReadSync(after,0,&got))&&got==4&&!std::memcmp(before,after,4),"Dismount does not invalidate shared files or alter data");
  dismount.r3.u32=0xDEAD1234;__imp__IoDismountVolumeByFileHandle(dismount,base);Check(dismount.r3.u32==X_STATUS_INVALID_HANDLE,"Invalid dismount handle is no longer reported successful");
 }
 for(const char* path:{"\\Device\\Harddisk0\\Partition0","\\Device\\Harddisk0\\Cache0","\\Device\\Harddisk0\\Cache1"}){
  FileOwner file{open(path)};
  Check(query(file.value,true,rex::system::XFileFsDeviceInformation,8)==X_STATUS_SUCCESS,"Raw virtual cache volume metadata is implemented");
  Check(rex::memory::load_and_swap<uint32_t>(base+fixture+32)==7&&rex::memory::load_and_swap<uint32_t>(base+fixture+36)==0x20,"Virtual cache reports its writable disk contract");
  PPCContext c{};c.r3.u32=file.value->handle();__imp__IoDismountVolumeByFileHandle(c,base);Check(c.r3.u32==X_STATUS_SUCCESS,"Virtual cache dismount preserves the SDK null-device contract");
 }
 std::cout<<checks<<" real kernel, volume-root and uncompressed-file compatibility checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<"File compatibility probe failed: "<<e.what()<<'\n';return 1;}}
