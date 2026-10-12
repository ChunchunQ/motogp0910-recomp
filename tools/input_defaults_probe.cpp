// Exercise the real SDK configuration precedence without presentation/GPU.
#include "../src/input_defaults.h"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: input_defaults_probe default|config-false|env-false|cli-false|cli-true [SDK flags]\n";
    return 2;
  }
  const std::string scenario = argv[1];
  rex::cvar::Init(argc - 1, argv + 1);
  rex::cvar::ApplyEnvironment();
  rex::cvar::LoadConfig("input-probe.toml");
  const auto backend = rex::cvar::GetFlagByName("input_backend");
  const auto keybind = rex::cvar::GetFlagByName("keybind_a");
  ApplyMotogpInputDefaults();

  using Source = rex::cvar::Source;
  Source expected_source;
  bool expected;
  if (scenario == "default") {
    expected = true;
    expected_source = Source::kRuntime;
  } else if (scenario == "config-false") {
    expected = false;
    expected_source = Source::kConfig;
  } else if (scenario == "env-false") {
    expected = false;
    expected_source = Source::kEnvironment;
  } else if (scenario == "cli-false" || scenario == "cli-true") {
    expected = scenario == "cli-true";
    expected_source = Source::kCommandLine;
  } else {
    return 2;
  }
  if (!rex::cvar::GetFlagInfo("mnk_mode") || backend.empty() || keybind.empty() ||
      rex::cvar::Query<bool>("mnk_mode") != expected ||
      rex::cvar::GetFlagSource("mnk_mode") != expected_source ||
      rex::cvar::GetFlagByName("input_backend") != backend ||
      rex::cvar::GetFlagByName("keybind_a") != keybind) {
    std::cerr << "FAIL: " << scenario << '\n';
    return 1;
  }
  // The application must also leave runtime changes alone if invoked again.
  rex::cvar::SetFlagByName("mnk_mode", "false");
  ApplyMotogpInputDefaults();
  if (rex::cvar::Query<bool>("mnk_mode")) {
    return 1;
  }
  std::cout << "PASS: " << scenario << "; backend=" << backend
            << "; keybind_a=" << keybind << '\n';
  return 0;
}
