#include <rex/perf/counter.h>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
extern "C" uint32_t Motogp_ReadGpuCounters(uint32_t,int64_t*,uint32_t);
int main(int argc,char** argv){try{
 unsigned checks=0;auto check=[&](bool ok,const char* what){if(!ok)throw std::runtime_error(what);++checks;};
 using C=rex::perf::CounterId;constexpr auto n=static_cast<uint32_t>(C::kCount);std::array<int64_t,n> values;values.fill(-99);
 check(!Motogp_ReadGpuCounters(2,values.data(),n),"Unknown ABI version is rejected");check(values[0]==-99,"Rejected ABI leaves output untouched");
 check(!Motogp_ReadGpuCounters(1,nullptr,n),"Null output is rejected");check(!Motogp_ReadGpuCounters(1,values.data(),n-1),"Short output cannot be overwritten");
 rex::perf::Init();rex::perf::SetCounter(C::kDrawCalls,654);rex::perf::SetCounter(C::kFrameTimeUs,16667);rex::perf::ResetFrameCounters();
 check(Motogp_ReadGpuCounters(1,values.data(),n)==1,"Supported snapshot ABI succeeds");
 check(values[static_cast<size_t>(C::kDrawCalls)]==654&&values[static_cast<size_t>(C::kFrameTimeUs)]==16667,"Snapshot reports the owning module's actual counters");
 check(rex::perf::GetSnapshotCounter(C::kDrawCalls)==654,"Reading a snapshot never resets the source counters");
 check(argc==2,"Supply the packaged GPU plugin path");
#ifdef _WIN32
 const auto module=LoadLibraryA(argv[1]);
 const auto api=module?reinterpret_cast<uint32_t(*)(uint32_t,int64_t*,uint32_t)>(GetProcAddress(module,"Motogp_ReadGpuCounters")):nullptr;
#else
 const auto module=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
 const auto api=module?reinterpret_cast<uint32_t(*)(uint32_t,int64_t*,uint32_t)>(dlsym(module,"Motogp_ReadGpuCounters")):nullptr;
#endif
 check(api!=nullptr,"The actual packaged GPU module exports the diagnostic ABI");
 check(api(1,values.data(),n)==1,"Release GPU module has real counter instrumentation enabled");
 values.fill(-99);check(!api(2,values.data(),n)&&values[0]==-99,"Packaged GPU module rejects an incompatible ABI without writing output");
 check(!api(1,nullptr,n)&&!api(1,values.data(),n-1),"Packaged GPU module validates output capacity");
#ifdef _WIN32
 FreeLibrary(module);
#else
 dlclose(module);
#endif
 std::cout<<checks<<" GPU diagnostic ABI checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
