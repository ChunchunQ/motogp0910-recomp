#pragma once
#include <cstdint>
#include <filesystem>
namespace rex {class Runtime;}
namespace motogp::localization {
inline constexpr int kTurkishLanguage = 1000; // Launcher-only; never an Xbox language ID.
bool Turkish();
void ApplyStartupLanguage();
// Mounts a read-only text overlay; original game files and voice paths are unchanged.
void Install(rex::Runtime* runtime,const std::filesystem::path& resources);
}
