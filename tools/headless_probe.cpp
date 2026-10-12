// Integration probe for the supplied TU build. This does not launch the game.
#include "motogp0910_init.h"
#include <rex/kernel/init.h>
#include <rex/runtime.h>
#include <rex/system/function_dispatcher.h>
#include <rex/system/user_module.h>
#include <rex/system/xex_module.h>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

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
    std::cout << "Patched XEX 0.0.1.1 loaded; native function mapping verified; "
              << count << " asin/acos checks passed. Gameplay was not exercised.\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
