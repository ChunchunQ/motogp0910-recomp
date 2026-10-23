#pragma once
// NOBK Online panel (F1, or Back + Start on a controller): friends, server,
// records and achievements while playing Online. Same idea as FIFA 09's
// NOBK.Friends overlay, drawn by this game (no SDK change). Read-only public
// site/API data only: no launcher token or password is sent.
#include <memory>
#include <rex/ui/imgui_dialog.h>

struct ImFont;
namespace rex { class Runtime; }
namespace rex::system { class AchievementManager; }
namespace rex::ui { class Window; }
namespace motogp::settings { class InputGate; }

namespace motogp::nobk {
void Initialize(rex::Runtime* runtime, std::shared_ptr<settings::InputGate> gate);
void Shutdown();
bool Active();
// Called once per game frame (guest thread): Back + Start held opens the panel.
void PollPad();
std::unique_ptr<rex::ui::ImGuiDialog> CreateMenu(rex::ui::ImGuiDrawer* drawer, rex::ui::Window* window,
    rex::system::AchievementManager* achievements, ImFont* font, ImFont* heading, bool turkish);
}
