#pragma once
#include <rex/ui/imgui_dialog.h>
#include <memory>
#include <functional>
namespace rex::ui {class Window;}
namespace motogp::debug {
bool Enabled();
void Initialize(const char* graphics_type_name);
void Shutdown();
float GuestFrameTimeMs();
void RegisterToggle(std::function<void()> callback);
void UnregisterToggle();
void Draw(rex::ui::Window* window,bool turkish);
std::unique_ptr<rex::ui::ImGuiDialog> Create(rex::ui::ImGuiDrawer*,rex::ui::Window*,bool turkish);
}
