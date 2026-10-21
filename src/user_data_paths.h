#pragma once
#include <filesystem>

namespace motogp::user_data {
// Only for the default user root. Explicit save locations bypass migration.
std::filesystem::path DefaultRoot(const std::filesystem::path& documents);
}
