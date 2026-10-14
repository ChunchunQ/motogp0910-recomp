// Exercise the packaged runtime's TLS boundaries and concurrent guest IDs.
// No host/guest threads execute title code; guest memory is surrounded by guards.
#include "motogp0910_init.h"
#include <rex/cvar.h>
#include <rex/kernel/init.h>
#include <rex/kernel/xam/module.h>
#include <rex/runtime.h>
#include <rex/system/xthread.h>
#include <rex/system/user_module.h>
#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <set>
#include <stdexcept>
#include <thread>
#include <vector>
REX_EXTERN(__imp__XamLoaderLaunchTitle);
REX_EXTERN(__imp__RtlTimeToTimeFields);
REX_EXTERN(__imp__RtlTimeFieldsToTime);
namespace {
struct Thread final : rex::system::XThread {
  explicit Thread(rex::system::KernelState* state)
      : XThread(state, 0, 0, 0, 0, 0, false) {}
  void Allocate() {
    tls_static_address_ = memory()->SystemHeapAlloc(4096);
    if (!tls_static_address_) throw std::runtime_error("TLS fixture allocation failed");
    std::memset(memory()->TranslateVirtual(tls_static_address_), 0xCD, 4096);
  }
  void Layout(uint32_t total, uint32_t extended) {
    tls_total_size_ = total;
    tls_dynamic_address_ = tls_static_address_ + extended;
  }
};
struct Owner {
  Thread* value;
  ~Owner() { value->ReleaseHandle(); value->Release(); }
};
}
int main(int argc, char** argv) { try {
  if (argc != 2) throw std::runtime_error("Usage: runtime_stability_probe GAME_ROOT");
  rex::Runtime runtime(std::filesystem::canonical(argv[1]));
  if (XFAILED(runtime.Setup(PPCImageConfig, {.kernel_init=rex::kernel::InitializeKernel, .tool_mode=true})))
    throw std::runtime_error("Runtime setup failed");
  unsigned checks = 0;
  auto require = [&](bool ok, const char* message) { if (!ok) throw std::runtime_error(message); ++checks; };
  Owner thread{new Thread(runtime.kernel_state())};
  thread.value->Allocate();
  auto* bytes = runtime.memory()->TranslateVirtual<uint8_t*>(thread.value->tls_ptr());
  for (auto [total, extended] : std::array<std::array<uint32_t,2>,5>{{{128,0},{128,64},{128,128},{128,132},{0,0}}}) {
    thread.value->Layout(total, extended);
    const auto slots = extended <= total ? (total-extended)/4 : 0;
    if (slots) {
      uint32_t out = 0;
      require(thread.value->SetTLSValue(0,0x12345678),"First TLS slot write");
      require(thread.value->GetTLSValue(0,&out) && out==0x12345678,"First TLS slot read");
      require(thread.value->SetTLSValue(slots-1,0xAABBCCDD),"Last TLS slot write");
      require(thread.value->GetTLSValue(slots-1,&out) && out==0xAABBCCDD,"Last TLS slot read");
    }
    std::array<uint8_t,4096> saved{}; std::memcpy(saved.data(),bytes,saved.size());
    for (uint32_t slot : {slots, slots+1, 0x40000000u, 0x80000000u, 0xFFFFFFFFu}) {
      uint32_t out=0xFEEDFACE;
      require(!thread.value->GetTLSValue(slot,&out),"Out-of-range TLS read accepted");
      require(out==0xFEEDFACE,"Rejected TLS read altered output");
      require(!thread.value->SetTLSValue(slot,0x76543210),"Out-of-range TLS write accepted");
      require(std::memcmp(saved.data(),bytes,saved.size())==0,"TLS write changed a guard or another slot");
    }
  }
  // Invoke the actual exported guest kernel shims, including validation.
  const auto allocation=thread.value->tls_ptr();
  const auto fields=allocation+512, filetime=allocation+256;
  for(int year : {1601,1677,1970,2000,2262,9999}) {
    auto date=std::chrono::sys_days{std::chrono::year{year}/1/1};
    const auto epoch=std::chrono::sys_days{std::chrono::year{1601}/1/1};
    const auto expected=static_cast<uint64_t>(std::chrono::duration_cast<rex::chrono::hundrednanoseconds>(
        date-epoch+std::chrono::hours{12}+std::chrono::seconds{17}+std::chrono::milliseconds{345}).count());
    rex::memory::store_and_swap<uint64_t>(runtime.virtual_membase()+filetime,expected);
    PPCContext context{}; context.r3.u64=filetime; context.r4.u64=fields;
    __imp__RtlTimeToTimeFields(context,runtime.virtual_membase());
    const std::array<uint16_t,7> values{static_cast<uint16_t>(year),1,1,12,0,17,345};
    for(unsigned i=0;i<values.size();++i)
      require(rex::memory::load_and_swap<uint16_t>(runtime.virtual_membase()+fields+2*i)==values[i],"Kernel FILETIME calendar decomposition");
    context.r3.u64=fields; context.r4.u64=filetime;
    __imp__RtlTimeFieldsToTime(context,runtime.virtual_membase());
    require(context.r3.u32==1 && rex::memory::load_and_swap<uint64_t>(runtime.virtual_membase()+filetime)==expected,"Kernel FILETIME calendar round trip");
  }
  // Reject February 30 and leave the output untouched.
  rex::memory::store_and_swap<uint16_t>(runtime.virtual_membase()+fields,2000);
  rex::memory::store_and_swap<uint16_t>(runtime.virtual_membase()+fields+2,2);
  rex::memory::store_and_swap<uint16_t>(runtime.virtual_membase()+fields+4,30);
  rex::memory::store_and_swap<uint64_t>(runtime.virtual_membase()+filetime,0x123456789ABCDEF0);
  PPCContext invalid{};invalid.r3.u64=fields;invalid.r4.u64=filetime;
  __imp__RtlTimeFieldsToTime(invalid,runtime.virtual_membase());
  require(invalid.r3.u32==0 && rex::memory::load_and_swap<uint64_t>(runtime.virtual_membase()+filetime)==0x123456789ABCDEF0,"Invalid calendar leaves output unchanged");
  rex::cvar::SetFlagByName("ignore_thread_affinities","true");
  const auto start=std::chrono::steady_clock::now();
  for(unsigned i=0;i<100000;++i) thread.value->SetActiveCpu(i%6);
  std::cout << "100000 unpinned CPU updates: " << std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count() << " ms\n";
  std::mutex lock; std::set<uint32_t> ids; bool duplicate=false;
  std::vector<std::thread> workers;
  for (unsigned worker=0;worker<8;++worker) workers.emplace_back([&] {
    for(unsigned i=0;i<1000;++i) {
      Owner object{new Thread(runtime.kernel_state())};
      std::lock_guard guard(lock);
      if(!ids.insert(object.value->thread_id()).second) duplicate=true;
    }
  });
  for(auto& worker:workers) worker.join();
  require(!duplicate && ids.size()==8000,"Concurrent guest thread IDs are not unique");
  require(!XFAILED(runtime.LoadXexImage("game:\\default.xex")),"Load title for launch-path checks");
  auto xam=runtime.kernel_state()->GetKernelModule<rex::kernel::xam::XamModule>("xam.xex");
  require(bool(xam),"Actual XAM loader module available");
  const auto executable=runtime.kernel_state()->GetExecutableModule()->path();
  const auto directory=executable.substr(0,executable.find_last_of('\\')+1);
  const std::string relative="next-"+std::string(180,'r')+".xex";
  for(const auto& path : std::array<std::string,4>{"", "default.xex",relative,"game:\\folder\\next.xex"}) {
    const auto guest=allocation+1024;
    std::memcpy(runtime.virtual_membase()+guest,path.c_str(),path.size()+1);
    PPCContext context{};context.r3.u64=guest;context.r4.u64=0x1234;
    // Called outside an executing guest thread: TerminateTitle drains safely
    // and returns, letting the test inspect the real loader's retained path.
    __imp__XamLoaderLaunchTitle(context,runtime.virtual_membase());
    const auto expected=path.empty()?std::string("game:\\default.xex"):
        path.find(':')==std::string::npos?directory+path:path;
    if(xam->loader_data().launch_path!=expected)
      std::cerr<<"Expected launch path: "<<expected<<"; actual: "<<xam->loader_data().launch_path<<'\n';
    require(xam->loader_data().launch_path==expected,"Owned relative/absolute title launch path");
    require(xam->loader_data().launch_flags==0x1234,"Title launch flags preserved");
  }
  std::cout << checks << " packaged runtime stability checks passed (8000 concurrent IDs)\n";
  return 0;
} catch(const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; } }
