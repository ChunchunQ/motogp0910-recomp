#pragma once
#include <filesystem>

namespace motogp::dlc {
// The caller supplies the public launcher's directory, not its private bin.
inline std::filesystem::path ContentRoot(const std::filesystem::path& application){return application/"dlc";}
inline std::filesystem::path TitleRoot(const std::filesystem::path& root){return root/"434307F0";}
inline std::filesystem::path Packages(const std::filesystem::path& root){return TitleRoot(root)/"00000002";}
inline std::filesystem::path Headers(const std::filesystem::path& root){return TitleRoot(root)/"Headers"/"00000002";}
}
