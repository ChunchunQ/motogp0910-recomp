// Read-only inspection using the pinned ReXGlue SDK loaders.
#include <rex/filesystem/devices/stfs_container_device.h>
#include <rex/filesystem/file.h>
#include <rex/kernel/init.h>
#include <rex/runtime.h>
#include <rex/system/user_module.h>
#include <rex/system/xex_module.h>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

void write_new(const fs::path& path, const uint8_t* bytes, size_t size) {
  if (fs::exists(path)) throw std::runtime_error("Refusing to overwrite: " + path.string());
  std::ofstream out(path, std::ios::binary);
  out.write(reinterpret_cast<const char*>(bytes), size);
  if (!out) throw std::runtime_error("Write failed: " + path.string());
}

void extract(rex::filesystem::Entry* entry, const fs::path& destination) {
  for (const auto& child : entry->children()) {
    fs::path name(child->name());
    if (name.empty() || name.has_parent_path() || name == "." || name == "..") {
      throw std::runtime_error("Invalid package entry name");
    }
    auto path = destination / name;
    if (child->attributes() & rex::filesystem::kFileAttributeDirectory) {
      if (!fs::create_directory(path)) throw std::runtime_error("Directory already exists");
      extract(child.get(), path);
      continue;
    }
    rex::filesystem::File* file = nullptr;
    if (XFAILED(child->Open(rex::filesystem::FileAccess::kGenericRead, &file))) {
      throw std::runtime_error("Open failed: " + child->name());
    }
    std::vector<uint8_t> bytes(child->size());
    size_t read = 0;
    auto status = bytes.empty() ? rex::X_STATUS{0} : file->ReadSync(bytes, 0, &read);
    file->Destroy();
    if (XFAILED(status) || read != bytes.size()) throw std::runtime_error("Short package read");
    write_new(path, bytes.data(), bytes.size());
    std::cout << path.string() << " " << bytes.size() << " bytes\n";
  }
}

int main(int argc, char** argv) {
  try {
    if (argc != 4) {
      std::cerr << "Usage: motogp_inspect xex-dump|stfs-extract INPUT NEW_OUTPUT_DIRECTORY\n";
      return 2;
    }
    const std::string mode(argv[1]);
    auto input = fs::canonical(argv[2]);
    const fs::path output(argv[3]);
    if (fs::exists(output)) throw std::runtime_error("Output directory must not exist");
    if (mode == "stfs-extract") {
      rex::filesystem::StfsContainerDevice device("update:", input);
      if (!device.Initialize()) throw std::runtime_error("STFS initialization failed");
      auto* root = device.ResolvePath("");
      if (!root) throw std::runtime_error("STFS root is missing");
      fs::create_directories(output);
      extract(root, output);
    } else if (mode == "xex-dump") {
      rex::Runtime runtime(input.parent_path());
      auto status = runtime.Setup(rex::RuntimeConfig{
          .kernel_init = rex::kernel::InitializeKernel, .tool_mode = true});
      if (XFAILED(status)) throw std::runtime_error("Runtime initialization failed");
      status = runtime.LoadXexImage("game:\\" + input.filename().string());
      if (XFAILED(status)) throw std::runtime_error("XEX load failed");
      auto module = runtime.kernel_state()->GetExecutableModule();
      auto* xex = module->xex_module();
      fs::create_directories(output);
      std::cout << "Base 0x" << std::hex << xex->base_address()
                << " entry 0x" << xex->entry_point() << std::dec << '\n';
      for (const auto& section : xex->binary_sections()) {
        if (!section.host_data || !section.virtual_size) continue;
        std::string name(section.name);
        if (fs::path(name).has_parent_path()) throw std::runtime_error("Invalid section name");
        write_new(output / (name + ".bin"), section.host_data, section.virtual_size);
        std::cout << name << " 0x" << std::hex << section.virtual_address << " size 0x"
                  << section.virtual_size << " exec=" << section.executable << std::dec << '\n';
      }
    } else {
      throw std::runtime_error("Unknown inspection mode");
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
