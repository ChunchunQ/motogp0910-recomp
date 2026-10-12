// motogp0910 - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <rex/rex_app.h>
#include <rex/filesystem.h>
#include <rex/filesystem/devices/stfs_container_device.h>
#include <rex/filesystem/file.h>
#include <rex/system.h>
#include <rex/system/user_module.h>
#include <rex/system/xex_module.h>

#include <fstream>
#include <vector>

class Motogp0910App : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<Motogp0910App>(new Motogp0910App(ctx, "motogp0910",
        PPCImageConfig));
  }

  // Override virtual hooks for customization:
  // void OnPostInitLogging() override {}
  bool SetupEnvironment() override {
    if (!rex::ReXApp::SetupEnvironment()) {
      return false;
    }
    const auto root = game_data_root();
    if (root.empty() || std::filesystem::is_regular_file(root / "default.xexp")) {
      return true;
    }
    const auto update = root / "TU_11K61VG_0000004000000.0000000000081";
    if (!std::filesystem::is_regular_file(update)) {
      rex::ShowSimpleMessageBox(rex::SimpleMessageBoxType::Error,
          "Copy your MotoGP 09/10 title-update file "
          "TU_11K61VG_0000004000000.0000000000081 next to default.xex, then start again.");
      return false;
    }
    rex::filesystem::StfsContainerDevice device("update:", update);
    if (!device.Initialize()) {
      rex::ShowSimpleMessageBox(rex::SimpleMessageBoxType::Error,
          "Could not read the MotoGP title-update package.");
      return false;
    }
    auto* entry = device.ResolvePath("default.xexp");
    rex::filesystem::File* file = nullptr;
    if (!entry || entry->size() != 1032192 ||
        XFAILED(entry->Open(rex::filesystem::FileAccess::kGenericRead, &file))) {
      rex::ShowSimpleMessageBox(rex::SimpleMessageBoxType::Error,
          "The title-update package does not contain the expected MotoGP patch.");
      return false;
    }
    std::vector<uint8_t> bytes(entry->size());
    size_t read = 0;
    auto status = file->ReadSync(bytes, 0, &read);
    file->Destroy();
    if (XFAILED(status) || read != bytes.size()) {
      return false;
    }
    const auto patch = root / "default.xexp";
    // Preserve existing files, including a patch created by another launch.
    if (std::filesystem::exists(patch)) {
      return std::filesystem::is_regular_file(patch);
    }
    std::ofstream output(patch, std::ios::binary);
    output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    output.close();
    if (!output) {
      rex::ShowSimpleMessageBox(rex::SimpleMessageBoxType::Error,
          "Could not save default.xexp. Use a writable copy of your extracted game folder.");
      return false;
    }
    return true;
  }
  bool ConstructRuntime(const rex::PathConfig& paths) override {
    if (!rex::ReXApp::ConstructRuntime(paths)) {
      return false;
    }
    const auto module = runtime()->kernel_state()->GetExecutableModule();
    const auto* info = module->xex_module()->opt_execution_info();
    if (info && info->title_id == 0x434307F0) {
      const auto version = info->version();
      if (version.major == 0 && version.minor == 0 && version.build == 1 && version.qfe == 1) {
        return true;
      }
    }
    rex::ShowSimpleMessageBox(rex::SimpleMessageBoxType::Error,
        "This build needs MotoGP 09/10 version 0.0.1.1. Keep the supplied "
        "default.xexp next to your original default.xex.");
    return false;
  }
  void OnPreSetup(rex::RuntimeConfig& config) override {
    if (config.gpu_plugin.empty()) {
      config.gpu_plugin = "xenos";
    }
  }
  // void OnLoadXexImage(std::string& xex_image) override {}
  // void OnPostLoadXexImage() override {}
  // void OnPostSetup() override {}
  // void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {}
  // std::unique_ptr<rex::ui::ImGuiDialog> CreateAchievementsOverlay() override;
  // std::unique_ptr<rex::ui::AchievementNotificationDialog>
  // CreateAchievementNotificationDialog() override;
  // void OnShutdown() override {}
  void OnConfigurePaths(rex::PathConfig& paths) override {
    if (!paths.game_data_root.empty()) {
      return;
    }
    const auto executable_directory = rex::filesystem::GetExecutableFolder();
    // Standard portable layout: executable/DLLs above the complete "game"
    // directory. Prefer it when both layouts exist; explicit roots still win.
    // Keep the original flat layout as a fallback for existing installations.
    for (const auto& candidate : {executable_directory / "game", executable_directory}) {
      if (std::filesystem::is_regular_file(candidate / "default.xex")) {
        paths.game_data_root = candidate;
        return;
      }
    }
  }
};
