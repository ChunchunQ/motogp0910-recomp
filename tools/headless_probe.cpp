// Integration checks for the supplied TU build. --launch-cpu additionally runs
// a bounded guest-entry diagnostic without presentation or GPU.
#include "motogp0910_init.h"
#include "startup_initializers.h"
#include <rex/audio/sdl/sdl_audio_system.h>
#include <rex/input/input_system.h>
#include <rex/kernel/init.h>
#include <rex/memory.h>
#include <rex/runtime.h>
#include <rex/system/function_dispatcher.h>
#include <rex/system/user_module.h>
#include <rex/system/xex_module.h>
#include <rex/system/xthread.h>

#include <cmath>
#include <array>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <thread>

int check_initializer_table(rex::Runtime& runtime) {
  int count = 0;
  for (uint32_t slot = kInitializerTableStart; slot < kInitializerTableEnd; slot += 4) {
    const auto address = rex::memory::load_and_swap<uint32_t>(runtime.memory()->TranslateVirtual(slot));
    if (!runtime.function_dispatcher()->GetFunction(address)) {
      std::cerr << "Unregistered CRT initializer 0x" << std::hex << address
                << " at table slot 0x" << slot << std::dec << '\n';
      throw std::runtime_error("CRT initializer table has unregistered entries");
    }
    ++count;
  }
  return count;
}

int check_registry_initializers(rex::Runtime& runtime) {
  auto* memory = runtime.memory();
  auto* head = memory->TranslateVirtual(kInitializerRegistryHead);
  const auto original_head = rex::memory::load_and_swap<uint32_t>(head);
  std::array<std::array<uint8_t, 20>, kStartupInitializers.size()> saved{};
  for (size_t i = 0; i < saved.size(); ++i) {
    std::memcpy(saved[i].data(), memory->TranslateVirtual(kStartupInitializers[i].node), 20);
  }
  auto restore = [&] {
    for (size_t i = 0; i < saved.size(); ++i) {
      std::memcpy(memory->TranslateVirtual(kStartupInitializers[i].node), saved[i].data(), 20);
    }
    rex::memory::store_and_swap<uint32_t>(head, original_head);
  };
  int count = 0;
  try {
    auto previous = original_head;
    for (const auto& test : kStartupInitializers) {
      PPCContext context{};
      context.r1.u64 = 0x12345678;
      context.lr = 0xBCBCBCBC;
      context.last_indirect_target = test.address;
      auto* function = rex::runtime::ResolveIndirectFunction(test.address);
      function(context, runtime.virtual_membase());
      auto* node = memory->TranslateVirtual<uint8_t*>(test.node);
      if (context.r1.u64 != 0x12345678 || context.lr != 0xBCBCBCBC ||
          context.r3.u32 != test.node ||
          rex::memory::load_and_swap<uint32_t>(head) != test.node ||
          rex::memory::load_and_swap<uint32_t>(node) != previous ||
          rex::memory::load_and_swap<uint32_t>(node + 4) != 0 ||
          rex::memory::load_and_swap<uint32_t>(node + 8) != test.name ||
          rex::memory::load_and_swap<uint32_t>(node + 12) != test.payload ||
          rex::memory::load_and_swap<uint32_t>(node + 16) != test.size) {
        throw std::runtime_error("CRT registry initializer state mismatch");
      }
      previous = test.node;
      ++count;
    }
  } catch (...) {
    restore();
    throw;
  }
  restore();
  return count;
}

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

// The resource comparator follows two linked lists and compares their maximum
// packed 16-bit keys. Exercise both returns and list traversal using guest memory.
int check_resource_comparator(rex::Runtime& runtime) {
  auto* memory = runtime.memory();
  const auto allocation = memory->SystemHeapAlloc(0x1000);
  if (!allocation) throw std::runtime_error("Could not allocate comparator fixture");
  int count = 0;
  try {
    auto write = [&](uint32_t offset, uint32_t value) {
      rex::memory::store_and_swap<uint32_t>(memory->TranslateVirtual(allocation + offset), value);
    };
    auto list = [&](uint32_t offset, std::array<uint16_t, 3> values, size_t size) {
      write(offset, allocation + offset + 16);
      write(offset + 16 + 32, size ? allocation + offset + 64 : 0);
      for (size_t i = 0; i < size; ++i) {
        const auto node = offset + 64 + static_cast<uint32_t>(i) * 32;
        write(node, allocation + node + 16);
        write(node + 4, i + 1 < size ? allocation + node + 32 : 0);
        write(node + 16, uint32_t(values[i]) << 13);
      }
    };
    struct Case { std::array<uint16_t, 3> a, b; size_t na, nb; int result; };
    for (const auto& test : std::array<Case, 6>{{
        {{0,0,0}, {0,0,0}, 0,0,0}, {{0,0,0}, {2,0,0}, 0,1,1},
        {{7,0,0}, {3,0,0}, 1,1,-1}, {{3,0,0}, {7,0,0}, 1,1,1},
        {{1,9,2}, {9,0,0}, 3,2,0}, {{1,9,2}, {8,7,0}, 3,2,-1}}}) {
      list(0, test.a, test.na);
      list(256, test.b, test.nb);
      PPCContext context{};
      context.r1.u64 = 0x12345678;
      context.lr = 0xBCBCBCBC;
      context.r3.u64 = allocation;
      context.r4.u64 = allocation + 256;
      context.last_indirect_target = 0x82500660;
      rex::runtime::ResolveIndirectFunction(0x82500660)(context, runtime.virtual_membase());
      if (context.r3.s32 != test.result || context.r1.u64 != 0x12345678 || context.lr != 0xBCBCBCBC) {
        throw std::runtime_error("Resource comparator result mismatch");
      }
      ++count;
    }
  } catch (...) {
    memory->SystemHeapFree(allocation);
    throw;
  }
  memory->SystemHeapFree(allocation);
  return count;
}

int check_interface_getter(rex::Runtime& runtime) {
  auto* memory = runtime.memory();
  const auto allocation = memory->SystemHeapAlloc(0x1000);
  if (!allocation) throw std::runtime_error("Could not allocate interface getter fixture");
  auto store = [&](uint32_t offset, uint32_t value) {
    rex::memory::store_and_swap<uint32_t>(memory->TranslateVirtual(allocation + offset), value);
  };
  try {
    store(28, 0x12345678);
    store(128, 0xDEADBEEF);
    for (bool has_output : {false, true}) {
      PPCContext context{};
      context.r1.u64 = 0x12345678;
      context.lr = 0xBCBCBCBC;
      context.r3.u64 = allocation;
      context.r4.u64 = has_output ? allocation + 128 : 0;
      context.last_indirect_target = 0x826543E8;
      rex::runtime::ResolveIndirectFunction(0x826543E8)(context, runtime.virtual_membase());
      const auto output = rex::memory::load_and_swap<uint32_t>(memory->TranslateVirtual(allocation + 128));
      if (context.r3.u32 != (has_output ? 0u : 37u) ||
          output != (has_output ? 0x12345678u : 0xDEADBEEFu) ||
          context.r1.u64 != 0x12345678 || context.lr != 0xBCBCBCBC) {
        throw std::runtime_error("Interface getter null/output path mismatch");
      }
    }
  } catch (...) {
    memory->SystemHeapFree(allocation);
    throw;
  }
  memory->SystemHeapFree(allocation);
  return 2;
}

int main(int argc, char** argv) {
  try {
    if (argc != 2 && !(argc == 3 && std::string(argv[2]) == "--launch-cpu")) {
      std::cerr << "Usage: motogp_headless_probe GAME_ROOT_WITH_DEFAULT_XEX_AND_XEXP [--launch-cpu]\n";
      return 2;
    }
    rex::Runtime runtime(std::filesystem::canonical(argv[1]));
    rex::RuntimeConfig config{.kernel_init = rex::kernel::InitializeKernel, .tool_mode = true};
    if (argc == 3) {
      // Guest startup registers an XAudio render client. Use the application's
      // real audio backend (SDL_AUDIODRIVER=dummy for this headless diagnostic).
      config.audio_factory = REX_AUDIO_BACKEND(rex::audio::sdl::SDLAudioSystem);
      config.input_factory = REX_INPUT_BACKEND(rex::input::CreateDefaultInputSystem);
    }
    auto status = runtime.Setup(PPCImageConfig, std::move(config));
    if (XFAILED(status)) throw std::runtime_error("Runtime setup failed");
    if (argc == 3 && !runtime.audio_system()) {
      throw std::runtime_error("CPU launch diagnostic requires a working SDL audio backend");
    }
    if (argc == 3 && !runtime.input_system()) {
      throw std::runtime_error("CPU launch diagnostic requires the SDK input backend");
    }
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
    const auto initializer_table_count = check_initializer_table(runtime);
    const auto registry_count = check_registry_initializers(runtime);
    const auto comparator_count = check_resource_comparator(runtime);
    const auto getter_count = check_interface_getter(runtime);
    std::cout << "Patched XEX 0.0.1.1 loaded; native function mapping verified; "
              << count << " asin/acos and " << callback_count
              << " startup callback checks passed; " << initializer_table_count
              << " CRT initializer targets registered; " << registry_count
              << " registry initializers executed and checked; " << comparator_count
              << " resource comparator checks passed; " << getter_count
              << " interface getter checks passed. Gameplay was not exercised.\n";
    if (argc == 3) {
      std::cout << "CPU-only diagnostic: launching the real guest entry point without GPU; SDL audio backend active.\n" << std::flush;
      auto thread = runtime.LaunchModule();
      if (!thread) throw std::runtime_error("Guest launch failed");
      for (int i = 0; i < 60; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (!thread->is_running()) {
          std::cout << "Guest main thread exited during CPU-only diagnostic. Gameplay was not validated.\n" << std::flush;
          std::_Exit(4);
        }
      }
      std::cout << "CPU-only observation limit reached. Graphics/audio/gameplay were not validated.\n" << std::flush;
      // Isolated diagnostic process: reap guest threads without an unsafe live
      // runtime teardown. Exit 4 deliberately does not count as gameplay success.
      std::_Exit(4);
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
