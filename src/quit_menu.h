#pragma once
#include <rex/ui/imgui_dialog.h>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
struct ImFont;
namespace rex {class Runtime;namespace input {class InputSystem;} }
namespace motogp::quit {
constexpr uint32_t kMainMenuVtable=0x8200CF7C, kItemsOffset=0x3C, kItemSize=40, kQuitIndex=6;
constexpr uint32_t kAcceptEvents=0x400|0x2000;
// Pure native layout helpers also used by the generated-code regression probe.
bool IsWhiteAccentDraw(uint8_t* base,uint32_t menu,uint32_t list,uint32_t index,uint64_t caller);
uint32_t WhiteAccentDescriptor(uint8_t* base,uint32_t original);
bool AppendItem(uint8_t* base,uint32_t menu,uint32_t label);
bool IsQuitSelection(uint8_t* base,uint32_t menu,uint32_t screen,uint32_t events);
bool IsTitleMode(uint8_t* base);
bool ObserveTitleDraw(uint8_t* base,uint32_t startup,double alpha,uint64_t caller=0);
void PollFrontendInput();
bool PollTitleInput(uint8_t* base,uint32_t expected_mode);
struct InputDecision {bool accept=false,cancel=false;};
class Confirmation {
 public:
  void Open(uint16_t held);
  InputDecision Update(uint16_t buttons,bool suppress_direction=false);
  bool exit_selected()const{return exit_selected_;}
  void SelectExit(bool value){exit_selected_=value;}
 private:
  uint16_t previous_=0;bool armed_=false,exit_selected_=false;
};
void Initialize(rex::Runtime* runtime);
uint32_t LabelAddress();
void SetFocused(bool focused);
void SetOtherMenuCheck(bool (*check)());
void Stop();
bool Pending();
void Request();
std::unique_ptr<rex::ui::ImGuiDialog> CreateConfirmationDialog(rex::ui::ImGuiDrawer* drawer,
    ImFont* font,bool turkish,std::function<void()> exit);
}
