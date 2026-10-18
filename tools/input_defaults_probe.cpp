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
  const auto backbind = rex::cvar::GetFlagByName("keybind_b");
  const auto upgraded_default=[](unsigned index){const auto key=launcher::kBindings[index].key;const auto source=rex::cvar::GetFlagSource(key);return source==rex::cvar::Source::kDefault||(source==rex::cvar::Source::kConfig&&launcher::IsLegacyDefaultBinding(index,rex::cvar::GetFlagByName(key)));};
  const bool default_a=upgraded_default(0),default_b=upgraded_default(1);
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
      rex::cvar::GetFlagByName("keybind_a") != (default_a?"Space":keybind) ||
      rex::cvar::GetFlagByName("keybind_b")!=(default_b?"Backspace":backbind)) {
    std::cerr << "FAIL: " << scenario << '\n';
    return 1;
  }
  // The application must also leave runtime changes alone if invoked again.
  rex::cvar::SetFlagByName("mnk_mode", "false");
  rex::cvar::SetFlagByName("keybind_a", "Semicolon,Space");
  ApplyMotogpInputDefaults();
  if (rex::cvar::Query<bool>("mnk_mode")||rex::cvar::GetFlagByName("keybind_a")!="Semicolon,Space") {
    return 1;
  }
  std::cout << "PASS: " << scenario << "; backend=" << backend
            << "; keybind_a=" << rex::cvar::GetFlagByName("keybind_a") << '\n';
  return 0;
}
