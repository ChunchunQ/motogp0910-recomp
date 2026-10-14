#define NOMINMAX
#include <Windows.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <stdexcept>
#include <vector>
namespace {
unsigned checks = 0;
void Check(bool ok, const char* text) {
  if (!ok) throw std::runtime_error(text);
  ++checks;
}
uint16_t U16(std::span<const uint8_t> p, size_t offset) {
  if (offset + 2 > p.size()) throw std::runtime_error("Truncated icon");
  return uint16_t(p[offset]) | uint16_t(p[offset + 1]) << 8;
}
uint32_t U32(std::span<const uint8_t> p, size_t offset) {
  return uint32_t(U16(p, offset)) | uint32_t(U16(p, offset + 2)) << 16;
}
std::span<const uint8_t> Resource(HMODULE module, LPCWSTR type, unsigned id) {
  const auto resource = FindResourceW(module, MAKEINTRESOURCEW(id), type);
  Check(resource != nullptr, "Embedded icon resource missing");
  const auto bytes = LoadResource(module, resource);
  Check(bytes != nullptr, "Cannot load embedded icon resource");
  return {static_cast<const uint8_t*>(LockResource(bytes)), SizeofResource(module, resource)};
}
void Verify(const std::filesystem::path& executable, const std::filesystem::path& artwork) {
  std::ifstream input(artwork, std::ios::binary);
  Check(bool(input), "Cannot read expected ICO");
  const std::vector<uint8_t> source{std::istreambuf_iterator<char>(input), {}};
  Check(U16(source, 0) == 0 && U16(source, 2) == 1 && U16(source, 4) == 7,
        "Expected seven source icon sizes");
  const auto module = LoadLibraryExW(executable.c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE);
  Check(module != nullptr, "Cannot open executable resources");
  struct Module { HMODULE value; ~Module() { FreeLibrary(value); } } owner{module};
  const auto group = Resource(module, MAKEINTRESOURCEW(14), 101);
  Check(U16(group, 0) == 0 && U16(group, 2) == 1 && U16(group, 4) == 7,
        "Executable must embed seven icon sizes");
  for (unsigned i = 0; i < 7; ++i) {
    const auto s = 6 + i * 16, g = 6 + i * 14;
    const auto count = U32(source, s + 8), offset = U32(source, s + 12);
    Check(offset <= source.size() && count <= source.size() - offset, "Source icon bounds");
    const auto icon = Resource(module, MAKEINTRESOURCEW(3), U16(group, g + 12));
    Check(group[g] == source[s] && group[g + 1] == source[s + 1] && icon.size() == count,
          "Embedded icon dimensions and length match artwork");
    Check(std::equal(icon.begin(), icon.end(), source.begin() + offset),
          "Executable embeds the exact requested artwork for every size");
    const int size = source[s] ? source[s] : 256;
    const auto native = static_cast<HICON>(LoadImageW(module, MAKEINTRESOURCEW(101),
        IMAGE_ICON, size, size, 0));
    Check(native != nullptr, "Windows cannot decode icon size");
    ICONINFO info{};
    const bool ok = GetIconInfo(native, &info);
    BITMAP bitmap{};
    const bool bitmap_ok = ok && GetObjectW(info.hbmColor, sizeof(bitmap), &bitmap);
    DestroyIcon(native);
    if (info.hbmColor) DeleteObject(info.hbmColor);
    if (info.hbmMask) DeleteObject(info.hbmMask);
    Check(bitmap_ok && bitmap.bmWidth == size && bitmap.bmHeight == size,
          "Decoded Windows icon has the requested size");
  }
  std::cout << executable.filename().string() << ": exact ICO artwork and all seven sizes verified\n";
}
}
int main(int argc, char** argv) { try {
  if (argc != 5) throw std::runtime_error("Usage: windows_icon_probe GAME_EXE GAME_ICO LAUNCHER_EXE LAUNCHER_ICO");
  Verify(argv[1], argv[2]); Verify(argv[3], argv[4]);
  std::cout << checks << " Windows icon resource checks passed\n";
  return 0;
} catch (const std::exception& error) {
  std::cerr << "FAIL: " << error.what() << '\n'; return 1;
} }
