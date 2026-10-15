#include "quit_menu.h"
#include <rex/hook.h>
#include <rex/memory.h>
#include <rex/runtime.h>
#include <rex/input/input_system.h>
#include <imgui.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <mutex>

REX_EXTERN(__imp__sub_821FC650);
REX_EXTERN(__imp__sub_821FCCA8);
REX_EXTERN(__imp__sub_822AE9F8);
REX_EXTERN(__imp__sub_8229C178);

namespace motogp::quit {
namespace {
std::atomic<rex::input::InputSystem*> input{nullptr};
std::atomic<bool> pending{false},focused{false},input_armed{false};
std::atomic<uint64_t> focus_epoch{0};
std::atomic<uint16_t> buttons{0};
std::atomic<uint64_t> title_serial{0};
uint64_t title_consumed=0;
bool held_blocker=false,back_was_down=false;
std::atomic<uint32_t> label_address{0};
std::atomic<bool(*)()> other_menu{nullptr};
uint32_t Read(uint8_t* base,uint32_t address){return rex::memory::load_and_swap<uint32_t>(base+address);}
void Write(uint8_t* base,uint32_t address,uint32_t value){rex::memory::store_and_swap<uint32_t>(base+address,value);}
void SynchronizeBlocker() {
  // Called only on the guest frontend thread; device/assignment state isn't
  // polled by the ImGui thread. SDK release masks held dismissal buttons.
  auto* system=input.load(std::memory_order_acquire);
  if(!system)return;
  const bool wanted=pending.load(std::memory_order_acquire);
  if(wanted&&!held_blocker){system->AddUIInputBlocker();held_blocker=true;}
  else if(!wanted&&held_blocker){
    input_armed.store(false,std::memory_order_release);
    system->RemoveUIInputBlocker();held_blocker=false;
  }
}
void Poll() {
  // F4 may be editing MnK bindings on the UI thread. Its gate also keeps
  // held dismissal inputs from becoming a new quit request.
  if(auto check=other_menu.load(std::memory_order_acquire);check&&check()){buttons=0;input_armed=false;return;}
  uint16_t merged=0;
  auto* system=input.load(std::memory_order_acquire);
  if(system && focused.load(std::memory_order_acquire))for(uint32_t user=0;user<4;++user){
    rex::input::X_INPUT_STATE state{};
    if(system->GetStateForUI(user,&state)==0) {
      merged|=state.gamepad.buttons;
      if(state.gamepad.thumb_lx < -16000)merged|=rex::input::X_INPUT_GAMEPAD_DPAD_LEFT;
      if(state.gamepad.thumb_lx > 16000)merged|=rex::input::X_INPUT_GAMEPAD_DPAD_RIGHT;
    }
  }
  buttons.store(merged,std::memory_order_release);
  if(focused.load(std::memory_order_acquire)&&merged==0)input_armed.store(true,std::memory_order_release);
}
}
bool AppendItem(uint8_t* base,uint32_t menu,uint32_t label) {
  if(!menu||!label||Read(base,menu)!=kMainMenuVtable)return false;
  constexpr std::array<uint32_t,7> ids={2,0,1,3,4,5,0xFFFFFFFF};
  for(size_t i=0;i<ids.size();++i)if(Read(base,menu+0x20+4*i)!=ids[i])return false;
  const auto row=menu+kItemsOffset+kItemSize*kQuitIndex;
  if(Read(base,row+28)==label && Read(base,row+kItemSize+28)==0)return true;
  if(Read(base,row+28)!=0)return false;
  static_assert(kItemsOffset+8*kItemSize<0x244);
  // Preserve the title's terminator and the original six entries/ID array.
  std::memcpy(base+row+kItemSize,base+row,kItemSize);
  std::array<uint8_t,kItemSize> item{};
  std::memcpy(item.data(),base+row-kItemSize,kItemSize);
  rex::memory::store_and_swap<uint32_t>(item.data()+28,0);
  std::memcpy(base+row,item.data(),kItemSize);
  std::atomic_thread_fence(std::memory_order_release);
  Write(base,row+28,label);
  return true;
}
bool IsQuitSelection(uint8_t* base,uint32_t menu,uint32_t screen,uint32_t events) {
  return menu&&screen&&Read(base,menu)==kMainMenuVtable &&
      Read(base,screen+0x18)==kQuitIndex && (events&kAcceptEvents) &&
      base[menu+0x244]==0 && base[menu+0x245]==0 &&
      label_address && Read(base,menu+kItemsOffset+kQuitIndex*kItemSize+28)==label_address;
}
bool IsTitleMode(uint8_t* base) {
  // Original title renderer 822AE938 checks this property for mode 15 before
  // calling 8229C178 with r4=1. The property pointer is initialized by the game.
  const auto value=Read(base,0x8292710C);
  return value>=0x82000000 && value<0x84000000 && Read(base,value)==15;
}
bool ObserveTitleDraw(uint8_t* base,uint32_t startup,double alpha) {
  const bool eligible=startup==1&&IsTitleMode(base)&&alpha>=1.0;
  if(eligible)title_serial.fetch_add(1,std::memory_order_release);
  return eligible;
}
void PollFrontendInput(){Poll();SynchronizeBlocker();}
void Confirmation::Open(uint16_t held){previous_=held;armed_=false;exit_selected_=false;}
InputDecision Confirmation::Update(uint16_t current,bool suppress_direction) {
  using namespace rex::input;
  const auto rising=static_cast<uint16_t>(current&~previous_);previous_=current;
  if(!armed_){if(!current)armed_=true;return {};}
  if(!suppress_direction&&(rising&(X_INPUT_GAMEPAD_DPAD_LEFT|X_INPUT_GAMEPAD_DPAD_RIGHT|X_INPUT_GAMEPAD_DPAD_UP|X_INPUT_GAMEPAD_DPAD_DOWN)))exit_selected_=!exit_selected_;
  if(rising&X_INPUT_GAMEPAD_B)return {.cancel=true};
  if(rising&X_INPUT_GAMEPAD_A)return {.accept=exit_selected_,.cancel=!exit_selected_};
  return {};
}
void Initialize(rex::Runtime* runtime) {
  input=static_cast<rex::input::InputSystem*>(runtime->input_system());
  const auto address=runtime->memory()->SystemHeapAlloc(32);
  if(!address)throw std::runtime_error("Could not allocate the PC quit menu label");
  label_address=address;constexpr char label[]="QUIT GAME";
  // The title's 826EC558 decoder consumes UTF-8 bytes, not Xbox WCHAR data.
  std::memcpy(runtime->virtual_membase()+address,label,sizeof(label));
  pending=false;buttons=0;title_serial=0;title_consumed=0;held_blocker=false;back_was_down=false;
}
uint32_t LabelAddress(){return label_address;}
void SetFocused(bool value){
  if(focused.exchange(value,std::memory_order_acq_rel)!=value){
    input_armed.store(false,std::memory_order_release);
    focus_epoch.fetch_add(1,std::memory_order_release);
  }
}
void Stop(){pending=false;/* The runtime owns the stable guest label allocation. */input=nullptr;label_address=0;}
bool Pending(){return pending.load(std::memory_order_acquire);}
void SetOtherMenuCheck(bool (*check)()){other_menu.store(check,std::memory_order_release);}
void Request(){if(auto check=other_menu.load(std::memory_order_acquire);check&&check())return;if(focused.load(std::memory_order_acquire)&&input_armed.load(std::memory_order_acquire))pending.store(true,std::memory_order_release);}
namespace {
class Dialog final:public rex::ui::ImGuiDialog {
 public:
  Dialog(rex::ui::ImGuiDrawer* drawer,ImFont* font,bool tr,std::function<void()> exit)
      :ImGuiDialog(drawer),font_(font),turkish_(tr),exit_(std::move(exit)){}
 protected:
  void OnDraw(ImGuiIO& io) override {
    if(!Pending()){open_=false;return;}
    const auto epoch=focus_epoch.load(std::memory_order_acquire);
    if(!open_||epoch_!=epoch){
      model_.Open(buttons.load());keyboard_armed_=false;keyboard_direction_active_=false;epoch_=epoch;
      if(!open_){open_=true;ImGui::OpenPopup("##motogp_quit_confirmation");}
    }
    if(!focused.load())return;
    const auto current=buttons.load(std::memory_order_acquire);
    constexpr uint16_t directions=rex::input::X_INPUT_GAMEPAD_DPAD_LEFT|rex::input::X_INPUT_GAMEPAD_DPAD_RIGHT;
    // MnK can report Shift+Arrow after ImGui receives the physical key. Keep
    // keyboard ownership until BOTH channels release, including later frames.
    if(ImGui::IsKeyDown(ImGuiKey_LeftArrow)||ImGui::IsKeyDown(ImGuiKey_RightArrow))keyboard_direction_active_=true;
    else if(!(current&directions))keyboard_direction_active_=false;
    const auto decision=model_.Update(current,keyboard_direction_active_);bool confirm=decision.accept,cancel=decision.cancel;
    if(!ImGui::IsKeyDown(ImGuiKey_Enter)&&!ImGui::IsKeyDown(ImGuiKey_Space)&&!ImGui::IsKeyDown(ImGuiKey_Escape)&&!ImGui::IsKeyDown(ImGuiKey_Backspace))keyboard_armed_=true;
    const auto tr=[&](const char* en,const char* turkish){return turkish_?turkish:en;};
    if(font_)ImGui::PushFont(font_,17);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(26,24));ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,12);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(18,10));ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,6);
    ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(.055f,.064f,.08f,1));
    ImGui::PushStyleColor(ImGuiCol_PopupBg,ImVec4(.055f,.064f,.08f,1));
    ImGui::PushStyleColor(ImGuiCol_Border,ImVec4(.19f,.21f,.26f,1));
    ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(.94f,.95f,.97f,1));
    ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(.13f,.15f,.19f,1));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(.34f,.24f,.11f,1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(.5f,.3f,.10f,1));
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg,ImVec4(0,0,0,.6f));
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x*.5f,io.DisplaySize.y*.5f),ImGuiCond_Always,ImVec2(.5f,.5f));
    ImGui::SetNextWindowSize(ImVec2(std::min(490.f,io.DisplaySize.x-32),0),ImGuiCond_Always);
    if(ImGui::BeginPopupModal("##motogp_quit_confirmation",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_AlwaysAutoResize)){
      ImGui::TextColored(ImVec4(1,.65f,.12f,1),"%s",tr("QUIT GAME","OYUNDAN ÇIK"));
      ImGui::Spacing();ImGui::TextWrapped("%s",tr("Are you sure you want to quit?","Oyundan çıkmak istiyor musunuz?"));ImGui::Spacing();
      const float width=(ImGui::GetContentRegionAvail().x-12)*.5f;
      if(!model_.exit_selected())ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(.3f,.23f,.12f,1));
      if(ImGui::Button(tr("Cancel","Vazgeç"),ImVec2(width,0)))cancel=true;
      if(!model_.exit_selected())ImGui::PopStyleColor();ImGui::SameLine();
      if(model_.exit_selected())ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(.45f,.28f,.1f,1));
      if(ImGui::Button(tr("Quit game","Oyundan çık"),ImVec2(width,0)))confirm=true;
      if(model_.exit_selected())ImGui::PopStyleColor();
      if(keyboard_armed_){
        if(ImGui::IsKeyPressed(ImGuiKey_Escape,false)||ImGui::IsKeyPressed(ImGuiKey_Backspace,false))cancel=true;
        if(ImGui::IsKeyPressed(ImGuiKey_LeftArrow,false)||ImGui::IsKeyPressed(ImGuiKey_RightArrow,false)||ImGui::IsKeyPressed(ImGuiKey_Tab,false))model_.SelectExit(!model_.exit_selected());
        if(ImGui::IsKeyPressed(ImGuiKey_Enter,false)){if(model_.exit_selected())confirm=true;else cancel=true;}
      }
      if(cancel)ImGui::CloseCurrentPopup();
      ImGui::EndPopup();
    }
    ImGui::PopStyleColor(8);ImGui::PopStyleVar(4);if(font_)ImGui::PopFont();
    if(cancel){pending.store(false,std::memory_order_release);open_=false;}
    else if(confirm&&!exit_scheduled_){exit_scheduled_=true;exit_();}
  }
 private:
  ImFont* font_;bool turkish_,open_=false,keyboard_armed_=false,exit_scheduled_=false,keyboard_direction_active_=false;uint64_t epoch_=0;Confirmation model_;std::function<void()> exit_;
};
}
std::unique_ptr<rex::ui::ImGuiDialog> CreateConfirmationDialog(rex::ui::ImGuiDrawer* drawer,ImFont* font,bool turkish,std::function<void()> exit){return std::make_unique<Dialog>(drawer,font,turkish,std::move(exit));}
}
REX_HOOK_RAW(sub_821FC650) {
  const auto menu=ctx.r3.u32;
  __imp__sub_821FC650(ctx,base);
  if(motogp::quit::label_address && !motogp::quit::AppendItem(base,menu,motogp::quit::label_address))
    REXLOG_WARN("PC quit item skipped: unexpected native MainMenu layout");
}
REX_HOOK_RAW(sub_821FCCA8) {
  using namespace motogp::quit;
  PollFrontendInput();
  back_was_down=(buttons.load()&rex::input::X_INPUT_GAMEPAD_B)!=0;
  if(Pending()){ctx.r3.s64=-1;return;}
  if(IsQuitSelection(base,ctx.r3.u32,ctx.r4.u32,ctx.r5.u32)){
    Request();SynchronizeBlocker();ctx.r3.s64=-1;return;
  }
  __imp__sub_821FCCA8(ctx,base);
}
REX_HOOK_RAW(sub_8229C178) {
  const auto startup=ctx.r4.u32;const double alpha=ctx.f1.f64;
  __imp__sub_8229C178(ctx,base);
  motogp::quit::ObserveTitleDraw(base,startup,alpha);
}
// The live frontend callback, installed when boot movies finish. 8229C2C0
// only runs the preceding boot sequence and cannot see Press Start inputs.
REX_HOOK_RAW(sub_822AE9F8) {
  using namespace motogp::quit;
  PollFrontendInput();
  const bool back=(buttons.load()&rex::input::X_INPUT_GAMEPAD_B)!=0;
  const auto serial=title_serial.load(std::memory_order_acquire);
  if(!Pending() && serial!=title_consumed && IsTitleMode(base) && back&&!back_was_down){Request();SynchronizeBlocker();}
  title_consumed=serial;back_was_down=back;
  if(Pending())return;
  __imp__sub_822AE9F8(ctx,base);
}
