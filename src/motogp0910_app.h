#include "adaptive_triggers.h"
#include "ultrawide.h"
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

#include "input_defaults.h"
#include "audio_settings.h"
#include "video_settings.h"
#include "button_prompts.h"
#include "ps3_prompt_textures.h"
#include "livery_readback.h"
#include "display_settings.h"
#include "window_icons.h"
#include "lan/runtime.h"
#include "crash_report.h"
#include "achievement_ui.h"
#include "quit_menu.h"
#include "in_game_settings.h"
#include "frame_pacing.h"
#include <rex/input/input_system.h>
#include <imgui.h>
#include <rex/ui/keybinds.h>

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
    motogp::crash::Install();
    motogp::ApplyDisplayStartupFlags();
    ApplyMotogpInputDefaults();
    ApplyMotogpAudioSettings();
    ApplyMotogpVideoSettings();
    WatchMotogpAudioSettings();
    if (config.gpu_plugin.empty()) {
      config.gpu_plugin = "xenos";
    }
  }
  // void OnLoadXexImage(std::string& xex_image) override {}
  // void OnPostLoadXexImage() override {}
  void OnPostSetup() override {
    motogp::ApplyWindowIcon(window()->GetNativeWindowHandle());
    try {
      motogp::lan::Start(runtime()->kernel_state());
    } catch(const std::exception& error) {
      motogp::lan::Stop();
      REXLOG_ERROR("LAN_STARTUP_FAILED: {}. Continuing in offline mode.",error.what());
      rex::ShowSimpleMessageBox(rex::SimpleMessageBoxType::Error,
          std::string("LAN could not start: ")+error.what()+
          "\n\nThe game will continue in OFFLINE mode. Close it and correct the LAN settings in the launcher before trying multiplayer again.");
    }
    motogp::ApplyHostDisplay(window());
    motogp::wide::Start(window());
    motogp::livery::Initialize();
    achievement_preferences_->Load(user_data_root(), rex::filesystem::GetExecutableFolder());
    motogp::quit::Initialize(runtime());
    settings_gate_->Initialize(static_cast<rex::input::InputSystem*>(runtime()->input_system()));
    settings_gate_->SetFocused(window()->HasFocus());
    settings_model_=std::make_shared<motogp::settings::Model>(settings_path_,runtime()->graphics_system()?runtime()->graphics_system()->presenter():nullptr);
    settings_dialog_=motogp::settings::CreateMenu(imgui_drawer(),settings_model_,settings_gate_,
        achievement_font_,achievement_heading_font_,achievement_preferences_->turkish,window(),[]{return !motogp::quit::Pending();});
    motogp::settings::SetGuestGate(settings_gate_.get());
    motogp::SetFrameObserver(motogp::settings::PollGuestInput);
    motogp::quit::SetOtherMenuCheck(motogp::settings::GuestInputBlocked);
    motogp::prompts::LoadPS3Artwork(runtime()->memory(),
        rex::filesystem::GetExecutableFolder()/"resources/controller/ps3-prompts.bin");
    motogp::prompts::LoadPS3ControlArtwork(runtime()->memory(),rex::filesystem::GetExecutableFolder()/"resources/controller/ps3-controls.bin");
    motogp::prompts::StartInputObserver(window());
    motogp::adaptive::Start(window()->HasFocus());
  }
  void OnWindowFocusChanged(bool focused) override {motogp::adaptive::SetFocused(focused);motogp::quit::SetFocused(focused);settings_gate_->SetFocused(focused);}
  void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {
    achievement_preferences_->Load(user_data_root(),rex::filesystem::GetExecutableFolder());
    motogp::quit::SetFocused(window()->HasFocus());
    quit_dialog_=motogp::quit::CreateConfirmationDialog(drawer,achievement_font_,achievement_preferences_->turkish,[this] {
      app_context().CallInUIThreadDeferred([this] {
        achievements().SaveUnlockState();
        motogp::lan::Stop();
        window()->RequestClose();
      });
    });
  }
  void OnConfigureFonts(ImFontAtlas* atlas) override {
    static const ImWchar ranges[] = {0x20, 0x1ff, 0};
    const auto path = rex::filesystem::GetExecutableFolder()/"resources/ui/OpenSans-Semibold.ttf";
    if(std::filesystem::is_regular_file(path)) {
      achievement_font_ = atlas->AddFontFromFileTTF(path.string().c_str(),17.f,nullptr,ranges);
      achievement_heading_font_ = atlas->AddFontFromFileTTF(path.string().c_str(),30.f,nullptr,ranges);
    }
    else REXLOG_WARN("Achievement font is missing; extract the complete build including resources/ui");
  }
  std::unique_ptr<rex::ui::ImGuiDialog> CreateAchievementsOverlay() override {
    if(!imgui_drawer() || !runtime()) return nullptr;
    return motogp::achievements::CreateMenu(imgui_drawer(), immediate_drawer(), runtime(),
        achievements(), achievement_font_, achievement_heading_font_, achievement_preferences_, [this](std::weak_ptr<void> lifetime) {
      // ReXApp owns the dialog in a unique_ptr. Never call ImGuiDialog::Close(),
      // which self-deletes. Dispatch the current F7 binding after this draw.
      motogp::achievements::RequestClose(app_context(),window(),lifetime);
    });
  }
  std::unique_ptr<rex::ui::AchievementNotificationDialog> CreateAchievementNotificationDialog() override {
    if(!imgui_drawer() || !runtime()) return nullptr;
    return motogp::achievements::CreateNotifications(imgui_drawer(), immediate_drawer(), runtime(),
        achievement_font_, achievement_preferences_);
  }
  void OnShutdown() override {
    motogp::adaptive::Stop();
    motogp::SetFrameObserver(nullptr);
    motogp::settings::SetGuestGate(nullptr);
    motogp::quit::SetOtherMenuCheck(nullptr);
    settings_dialog_.reset();
    settings_model_.reset();
    quit_dialog_.reset();
    motogp::quit::Stop();
    motogp::lan::Stop();
    motogp::wide::Stop();
    motogp::prompts::StopInputObserver();
    motogp::prompts::StopPS3Artwork();
  }
  void OnConfigurePaths(rex::PathConfig& paths) override {
    settings_path_=paths.config_path;
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
 private:
  std::filesystem::path settings_path_;
  std::shared_ptr<motogp::settings::InputGate> settings_gate_=std::make_shared<motogp::settings::InputGate>();
  std::shared_ptr<motogp::settings::Model> settings_model_;
  std::unique_ptr<rex::ui::ImGuiDialog> settings_dialog_;
  std::unique_ptr<rex::ui::ImGuiDialog> quit_dialog_;
  ImFont* achievement_font_ = nullptr;
  ImFont* achievement_heading_font_ = nullptr;
  std::shared_ptr<motogp::achievements::Preferences> achievement_preferences_ = std::make_shared<motogp::achievements::Preferences>();
};
