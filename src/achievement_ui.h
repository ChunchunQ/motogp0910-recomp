#pragma once
#include <rex/system/achievement_manager.h>
#include <rex/ui/imgui_dialog.h>
#include <rex/ui/overlay/achievement_notification.h>
#include <atomic>
#include <deque>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
struct ImFont;
namespace rex { class Runtime; namespace ui { class ImmediateDrawer; class AchievementIconCache; class Window; class WindowedAppContext; } }
namespace motogp::achievements {
struct Preferences {
  std::atomic<bool> sound{true};
  std::atomic<int> volume{25};
  bool turkish = false;
  std::filesystem::path path;
  void Load(const std::filesystem::path& user_root, const std::filesystem::path& executable_root);
  bool Save() const;
};
struct MenuState { int filter = 0; char search[160]{}; };
struct Summary { size_t earned=0, total=0; uint64_t score=0, available_score=0; };
Summary Progress(const rex::system::AchievementManager& manager);
bool Matches(const rex::system::AchievementInfo& info, bool unlocked, const MenuState& state);
// These render functions are also exercised with the actual runtime ImGui in the Windows probe.
bool DrawMenu(rex::system::AchievementManager& manager, MenuState& state, Preferences& preferences,
              ImFont* font, rex::ui::AchievementIconCache* icons=nullptr, ImFont* heading=nullptr);
void DrawToast(const rex::system::AchievementEvent& event, float alpha, bool turkish,
               ImFont* font, rex::ui::AchievementIconCache* icons=nullptr);
struct VisibleToast { rex::system::AchievementEvent event; float alpha=0; bool play_sound=false; };
class ToastQueue {
 public:
  void Push(const rex::system::AchievementEvent& event);
  std::optional<VisibleToast> Next(double now, bool focused);
 private:
  std::mutex mutex_;
  std::deque<rex::system::AchievementEvent> pending_, seen_;
  std::optional<rex::system::AchievementEvent> visible_;
  double started_=0;
};
float CueGain(const Preferences& preferences);
std::vector<int16_t> CueSamples(float gain);
class Sound {
 public:
  explicit Sound(std::shared_ptr<Preferences> preferences);
  ~Sound();
  void Play();
 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
void RequestClose(rex::ui::WindowedAppContext& context, rex::ui::Window* window, std::weak_ptr<void> lifetime);
using CloseRequest = std::function<void(std::weak_ptr<void>)>;
std::unique_ptr<rex::ui::ImGuiDialog> CreateMenu(rex::ui::ImGuiDrawer* drawer,
    rex::ui::ImmediateDrawer* immediate, rex::Runtime* runtime,
    rex::system::AchievementManager& manager, ImFont* font, ImFont* heading,
    std::shared_ptr<Preferences> preferences, CloseRequest close);
std::unique_ptr<rex::ui::AchievementNotificationDialog> CreateNotifications(
    rex::ui::ImGuiDrawer* drawer, rex::ui::ImmediateDrawer* immediate, rex::Runtime* runtime,
    ImFont* font, std::shared_ptr<Preferences> preferences);
}
