#pragma once
#include <filesystem>
namespace rex {class Runtime;}
namespace motogp::dlc {
void Initialize(rex::Runtime* runtime,const std::filesystem::path& application);
void Stop();
}
