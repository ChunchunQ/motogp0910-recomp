#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
struct PPCContext;
namespace rex {class Runtime;namespace input {class InputSystem;} }
namespace motogp::quit {
constexpr uint32_t kMainMenuVtable=0x8200CF7C, kItemsOffset=0x3C, kItemSize=40, kQuitIndex=6;
constexpr uint32_t kAcceptEvents=0x400|0x2000;
// Pure native layout helpers also used by the generated-code regression probe.
bool IsWhiteAccentDraw(uint8_t* base,uint32_t menu,uint32_t list,uint32_t index,uint64_t caller);
bool IsQuitLabelDraw(uint8_t* base,uint32_t menu,uint32_t array,uint32_t index,uint64_t caller);
bool IsWhiteCapPass(uint64_t caller);
uint32_t WhiteAccentDescriptor(uint8_t* base,uint32_t original);
bool AppendItem(uint8_t* base,uint32_t menu,uint32_t label);
bool IsQuitSelection(uint8_t* base,uint32_t menu,uint32_t screen,uint32_t events);
bool IsTitleMode(uint8_t* base);
bool ObserveTitleDraw(uint8_t* base,uint32_t startup,double alpha,uint64_t caller=0);
void PollFrontendInput();
bool PollTitleInput(uint8_t* base,uint32_t expected_mode);
void RequestIntroSkip();
void PollIntroInput(uint8_t* base);
bool IntroSkipAllowed(uint8_t* base);
bool BeginIntroSkip(uint8_t* base);
void FinishIntroSkip(uint8_t* base);
void Initialize(rex::Runtime* runtime);
uint32_t LabelAddress();
void SetFocused(bool focused);
void SetOtherMenuCheck(bool (*check)());
void Stop();
bool Pending();
void Request();
void SetExitHandler(std::function<void()> exit);
// Called on the guest frontend thread; uses the title's real Yes/No screen.
void TickConfirmation(PPCContext& ctx,uint8_t* base);
}
