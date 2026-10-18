#pragma once
#include <filesystem>

namespace motogp::portable {
// Keep configuration, assets and logs beside the public launcher even when
// the game, private launcher and runtime DLLs live in the bin directory.
inline std::filesystem::path Root(const std::filesystem::path& executable_directory) {
  return executable_directory.filename()=="bin" ? executable_directory.parent_path() : executable_directory;
}
inline std::filesystem::path Binaries(const std::filesystem::path& root) {
  return std::filesystem::is_regular_file(root/"bin/motogp0910.exe") ? root/"bin" : root;
}
}
