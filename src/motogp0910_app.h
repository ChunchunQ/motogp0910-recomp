#include "adaptive_triggers.h"
#include "ultrawide.h"
// motogp0910 - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <rex/rex_app.h>
#include <rex/filesystem.h>
#include <rex/filesystem/vfs.h>
#include <rex/filesystem/devices/stfs_container_device.h>
#include <rex/filesystem/file.h>
#include <rex/system.h>
#include <rex/system/user_module.h>
#include <rex/system/xex_module.h>

#include <fstream>
#include <typeinfo>
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
#include "options_menu.h"
#include "localization.h"
#include "in_game_settings.h"
#include "frame_pacing.h"
#include "debug_overlay.h"
#include "portable_paths.h"
#include "config_file.h"
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
    const bool ready=rex::ReXApp::SetupEnvironment();
    sdk_config_.Clear();
    if (!ready) {
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
    motogp::localization::ApplyStartupLanguage();
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
    motogp::localization::Install(runtime(),motogp::portable::Root(rex::filesystem::GetExecutableFolder())/"resources/localization/tr/overlay");
    // The title opens its image volume by the Xbox device name during startup.
    if(auto* root=runtime()->file_system()->ResolvePath("game:\\"))
      runtime()->file_system()->RegisterSymbolicLink("\\Device\\Image",root->absolute_path());
    try {
      motogp::lan::Start(runtime()->kernel_state());
    } catch(const std::exception& error) {
      motogp::lan::Stop();
      REXLOG_ERROR("LAN_STARTUP_FAILED: {}. Continuing in offline mode.",error.what());
      rex::ShowSimpleMessageBox(rex::SimpleMessageBoxType::Error,
          std::string("LAN could not start: ")+error.what()+
          "\n\nThe game will continue in OFFLINE mode. Close it and correct the LAN settings in the launcher before trying multiplayer again.");
    }
    auto* graphics = runtime()->graphics_system();
    motogp::debug::Initialize(graphics?typeid(*graphics).name():nullptr);
    motogp::ApplyHostDisplay(window());
    motogp::wide::Start(window());
    motogp::livery::Initialize();
    achievement_preferences_->Load(user_data_root(), motogp::portable::Root(rex::filesystem::GetExecutableFolder()));
    if(motogp::localization::Turkish())achievement_preferences_->turkish=true;
    motogp::quit::Initialize(runtime());
    motogp::options::Initialize(runtime());
    settings_gate_->Initialize(static_cast<rex::input::InputSystem*>(runtime()->input_system()));
    settings_gate_->SetFocused(window()->HasFocus());
    settings_model_=std::make_shared<motogp::settings::Model>(settings_path_,runtime()->graphics_system()?runtime()->graphics_system()->presenter():nullptr);
    settings_dialog_=motogp::settings::CreateMenu(imgui_drawer(),settings_model_,settings_gate_,
        achievement_font_,achievement_heading_font_,achievement_preferences_->turkish,window(),[]{return !motogp::quit::Pending();});
    motogp::settings::SetGuestGate(settings_gate_.get());
    motogp::SetFrameObserver(motogp::settings::PollGuestInput);
    motogp::quit::SetOtherMenuCheck(motogp::settings::GuestInputBlocked);
    motogp::prompts::LoadPS3Artwork(runtime()->memory(),
        motogp::portable::Root(rex::filesystem::GetExecutableFolder())/"resources/controller/ps3-prompts.bin");
    motogp::prompts::LoadPS3ControlArtwork(runtime()->memory(),motogp::portable::Root(rex::filesystem::GetExecutableFolder())/"resources/controller/ps3-controls.bin");
    motogp::prompts::StartInputObserver(window());
    motogp::adaptive::Start(window()->HasFocus());
  }
  void OnKeyDown(rex::ui::KeyEvent& event) override {
    if(!event.prev_state()&&!event.is_ctrl_pressed()&&!event.is_alt_pressed()&&!event.is_super_pressed()&&
       (event.virtual_key()==rex::ui::VirtualKey::kSpace||event.virtual_key()==rex::ui::VirtualKey::kReturn))motogp::quit::RequestIntroSkip();
    rex::ui::ProcessKeyEvent(event);
  }
  void OnWindowFocusChanged(bool focused) override {motogp::adaptive::SetFocused(focused);motogp::quit::SetFocused(focused);settings_gate_->SetFocused(focused);}
  bool OnWindowCloseRequested() override {
    // The pinned SDK hard-exits in OnClosing before OnShutdown can run.
    // Release controller firmware effects while SDL and the runtime are alive.
    motogp::adaptive::Stop();
    return true;
  }
  void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {
    // The SDK has just registered F3; replace it before the game is launched.
    motogp::debug::RegisterToggle([this]{
      if(debug_dialog_)debug_dialog_.reset();else debug_dialog_=motogp::debug::Create(imgui_drawer(),window(),achievement_preferences_->turkish);
    });
    achievement_preferences_->Load(user_data_root(),motogp::portable::Root(rex::filesystem::GetExecutableFolder()));
    if(motogp::localization::Turkish())achievement_preferences_->turkish=true;
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
    const auto path = motogp::portable::Root(rex::filesystem::GetExecutableFolder())/"resources/ui/OpenSans-Semibold.ttf";
    if(std::filesystem::is_regular_file(path)) {
      const auto utf8_path=rex::path_to_utf8(path);
      achievement_font_ = atlas->AddFontFromFileTTF(utf8_path.c_str(),17.f,nullptr,ranges);
      achievement_heading_font_ = atlas->AddFontFromFileTTF(utf8_path.c_str(),30.f,nullptr,ranges);
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
    motogp::debug::UnregisterToggle();
    motogp::debug::Shutdown();
    debug_dialog_.reset();
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
    const auto executable_directory = rex::filesystem::GetExecutableFolder();
    const auto root=motogp::portable::Root(executable_directory);
    settings_path_=motogp::config::Ensure(root);
    const auto settings=motogp::config::Load(settings_path_);
    paths.config_path=sdk_config_.Prepare(settings);
    // Relative portable paths share the public installation root, including
    // direct game starts from Explorer or a different working directory.
    if(root!=executable_directory)std::filesystem::current_path(root);
    const auto configured=[&](const char* key,std::filesystem::path& path){
      if(rex::cvar::GetFlagSource(key)>rex::cvar::Source::kConfig)return false;
      const auto* node=motogp::config::Find(settings,key);if(!node)return false;
      const auto value=node->value<std::string>();if(!value||value->empty())return false;
      auto selected=rex::to_path(*value);path=selected.is_absolute()?selected:root/selected;return true;
    };
    configured("game_data_root",paths.game_data_root);
    const bool user_root_changed=configured("user_data_root",paths.user_data_root);
    if(!configured("cache_root",paths.cache_root)&&user_root_changed&&rex::cvar::GetFlagSource("cache_root")<=rex::cvar::Source::kConfig)paths.cache_root=paths.user_data_root/"cache";
    configured("update_data_root",paths.update_data_root);configured("metadata_root",paths.metadata_root);
    if (!paths.game_data_root.empty()) {
      return;
    }
    // Standard portable layout: executable/DLLs above the complete "game"
    // directory. Prefer it when both layouts exist; explicit roots still win.
    // Keep the original flat layout as a fallback for existing installations.
    for (const auto& candidate : {root / "game", root}) {
      if (std::filesystem::is_regular_file(candidate / "default.xex")) {
        paths.game_data_root = candidate;
        return;
      }
    }
  }
  void OnConfigureLogging(rex::LogConfig& config) override {
    config.log_dir=motogp::portable::Root(rex::filesystem::GetExecutableFolder())/"logs";
  }
  void OnPostInitLogging() override {
    // Resolve the default after TOML loading too, including old explicit
    // defaults. An ASCII relative path works with both native filesystem APIs
    // and SDL UTF-8 even when the installation directory contains Turkish text.
    if(rex::filesystem::GetExecutableFolder().filename()=="bin"&&rex::cvar::Query<std::string>("hid_mappings_file")=="gamecontrollerdb.txt")
      rex::cvar::SetFlagByName("hid_mappings_file","bin/gamecontrollerdb.txt");
  }
 private:
  motogp::config::SdkBridge sdk_config_;
  std::unique_ptr<rex::ui::ImGuiDialog> debug_dialog_;
  std::filesystem::path settings_path_;
  std::shared_ptr<motogp::settings::InputGate> settings_gate_=std::make_shared<motogp::settings::InputGate>();
  std::shared_ptr<motogp::settings::Model> settings_model_;
  std::unique_ptr<rex::ui::ImGuiDialog> settings_dialog_;
  std::unique_ptr<rex::ui::ImGuiDialog> quit_dialog_;
  ImFont* achievement_font_ = nullptr;
  ImFont* achievement_heading_font_ = nullptr;
  std::shared_ptr<motogp::achievements::Preferences> achievement_preferences_ = std::make_shared<motogp::achievements::Preferences>();
};
