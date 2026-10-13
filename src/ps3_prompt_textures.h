#pragma once
#include <rex/memory.h>
#include <filesystem>

namespace motogp::prompts {
void LoadPS3Artwork(rex::memory::Memory* memory,const std::filesystem::path& file);
void StopPS3Artwork();
// Template is an original game UI texture wrapper. The returned wrapper uses
// the game's material and a native Xenos BC3 fetch descriptor.
uint32_t PS3Texture(char glyph,uint32_t original,uint8_t* base);
}
