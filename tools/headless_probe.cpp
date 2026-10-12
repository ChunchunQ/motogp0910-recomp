// Integration probe for the supplied TU build. This does not launch the game.
#include "motogp0910_init.h"
#include <rex/kernel/init.h>
#include <rex/memory.h>
#include <rex/runtime.h>
#include <rex/system/function_dispatcher.h>
#include <rex/system/user_module.h>
#include <rex/system/xex_module.h>

#include <cmath>
#include <array>
#include <filesystem>
#include <iostream>
#include <stdexcept>

// Exercise the callback from the reported startup failure using the real
// generated code. The CRT's TLS-disabled state keeps detach testing independent
// of guest thread/heap initialization; the original slot is restored afterward.
int check_thread_callback(rex::Runtime& runtime) {
  auto* callback = runtime.function_dispatcher()->GetFunction(0x8270DF08);
  if (!callback) {
    throw std::runtime_error("Startup callback 0x8270DF08 is not registered");
  }
  auto* indirect_callback = rex::runtime::ResolveIndirectFunction(0x8270DF08);
  if (indirect_callback != callback) {
    throw std::runtime_error("Startup callback indirect resolution mismatch");
  }
  auto* memory = runtime.memory();
  auto* tls_slot = memory->TranslateVirtual(0x82941AF0);
  const auto original_slot = rex::memory::load_and_swap<uint32_t>(tls_slot);
  const uint32_t allocation = memory->SystemHeapAlloc(0x1000);
  if (!allocation) throw std::runtime_error("Could not allocate callback test stack");
  rex::memory::store_and_swap<uint32_t>(tls_slot, 0xFFFFFFFF);
  int count = 0;
  try {
    for (uint32_t argument : std::array<uint32_t, 5>{0, 1, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF}) {
      PPCContext context{};
      const auto stack = allocation + 0x800;
      context.r1.u64 = stack;
      context.r3.u64 = argument;
      context.r30.u64 = 0x123456789ABCDEF0;
      context.r31.u64 = 0x0FEDCBA987654321;
      context.lr = 0xBCBCBCBC;
      // Match the game's indirect dispatch, rather than calling a substitute.
      context.ctr.u64 = 0x8270DF08;
      context.last_indirect_target = context.ctr.u32;
      indirect_callback(context, runtime.virtual_membase());
      const bool cleanup_path = argument == 0;
      if (context.r1.u64 != stack || context.lr != 0xBCBCBCBC ||
          context.r3.u32 != argument || context.r30.u64 != 0x123456789ABCDEF0 ||
          context.r31.u64 != 0x0FEDCBA987654321 ||
          (!cleanup_path && (context.cr6.eq || context.cr6.lt != (argument >= 0x80000000))) ||
          (cleanup_path && (!context.cr6.eq || context.r11.u32 != 0xFFFFFFFF))) {
        throw std::runtime_error("Startup callback return/cleanup state mismatch");
      }
      ++count;
    }
  } catch (...) {
    rex::memory::store_and_swap<uint32_t>(tls_slot, original_slot);
    memory->SystemHeapFree(allocation);
    throw;
  }
  rex::memory::store_and_swap<uint32_t>(tls_slot, original_slot);
  memory->SystemHeapFree(allocation);
  return count;
}

int main(int argc, char** argv) {
  try {
    if (argc != 2) {
      std::cerr << "Usage: motogp_headless_probe GAME_ROOT_WITH_DEFAULT_XEX_AND_XEXP\n";
      return 2;
    }
    rex::Runtime runtime(std::filesystem::canonical(argv[1]));
    auto status = runtime.Setup(PPCImageConfig, rex::RuntimeConfig{
        .kernel_init = rex::kernel::InitializeKernel, .tool_mode = true});
    if (XFAILED(status)) throw std::runtime_error("Runtime setup failed");
    if (XFAILED(runtime.LoadXexImage("game:\\default.xex"))) {
      throw std::runtime_error("Patched XEX load failed");
    }
    auto module = runtime.kernel_state()->GetExecutableModule();
    auto* execution = module->xex_module()->opt_execution_info();
    if (!execution || execution->title_id != 0x434307F0) {
      throw std::runtime_error("Wrong title loaded");
    }
    auto version = execution->version();
    if (version.major != 0 || version.minor != 0 || version.build != 1 || version.qfe != 1) {
      throw std::runtime_error("Expected patched XEX version 0.0.1.1");
    }
    auto* routine = runtime.function_dispatcher()->GetFunction(0x82709EE0);
    if (!routine || routine != sub_82709EE0) {
      throw std::runtime_error("Generated function mapping was not registered");
    }
    int count = 0;
    for (double input : {-0.9, -0.5, 0.0, 0.5, 0.9}) {
      for (int operation : {0, 1}) {
        PPCContext context{};
        context.fpscr.loadFromHost();
        context.f1.f64 = input;
        context.r4.u64 = operation;
        routine(context, runtime.virtual_membase());
        double expected = operation ? std::acos(input) : std::asin(input);
        if (!std::isfinite(context.f1.f64) || std::abs(context.f1.f64 - expected) > 1e-12) {
          std::cerr << "Mismatch input=" << input << " operation=" << operation
                    << " actual=" << context.f1.f64 << " expected=" << expected << '\n';
          return 1;
        }
        ++count;
      }
    }
    const auto callback_count = check_thread_callback(runtime);
    std::cout << "Patched XEX 0.0.1.1 loaded; native function mapping verified; "
              << count << " asin/acos and " << callback_count
              << " startup callback checks passed. Gameplay was not exercised.\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
