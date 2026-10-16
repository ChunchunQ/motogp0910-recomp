// Read-only localization inventory from the supplied title image. Outputs are
// development artifacts only; never include original title memory in a release.
#include "motogp0910_init.h"
#include "startup_initializers.h"
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/memory.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstring>
#include <toml++/toml.hpp>
int main(int argc,char** argv){try{
 if(argc!=3)return 2;
 rex::Runtime runtime(std::filesystem::canonical(argv[1]));rex::RuntimeConfig config{.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true};
 if(XFAILED(runtime.Setup(PPCImageConfig,std::move(config)))||XFAILED(runtime.LoadXexImage("game:\\default.xex")))return 3;
 auto* base=runtime.virtual_membase();std::filesystem::path out=argv[2];std::filesystem::create_directories(out);
 std::ofstream image(out/"image.bin",std::ios::binary);image.write(reinterpret_cast<char*>(base+0x82000000),0x870000);
 toml::array achievements;
 for(const auto& info:runtime.kernel_state()->loaded_achievements())achievements.push_back(toml::table{{"id",info.id},{"en",info.label},{"description",info.description},{"locked",info.unachieved_description}});
 std::ofstream catalog(out/"achievements.toml");catalog<<toml::table{{"achievements",std::move(achievements)}};
 for(auto& row:kStartupInitializers){std::cout<<std::hex<<row.address<<" "<<row.payload<<" "<<row.size<<" "<<reinterpret_cast<char*>(base+row.name)<<'\n';}
 return image?0:4;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
