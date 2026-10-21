#include "quit_menu.h"
#include "input_synchronization.h"
#include "localization.h"
#include <rex/hook.h>
#include "diagnostic_options.h"
#include <rex/memory.h>
#include <rex/runtime.h>
#include <rex/system/function_dispatcher.h>
#include <rex/graphics/xenos.h>
#include <rex/input/input_system.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <mutex>
#include <map>
#include <set>

REX_EXTERN(__imp__sub_821FC650);
REX_EXTERN(__imp__sub_821FC9B0);
REX_EXTERN(__imp__sub_82288CD0);
REX_EXTERN(__imp__sub_8227FEE8);
REX_EXTERN(__imp__sub_82389D10);
REX_EXTERN(__imp__sub_82379D38);
REX_EXTERN(__imp__sub_8236E940);
REX_EXTERN(__imp__sub_821FCCA8);
REX_EXTERN(__imp__sub_822AE9F8);
REX_EXTERN(__imp__sub_8229C2C0);
REX_EXTERN(__imp__sub_8229C178);


namespace motogp::quit {
namespace {
std::atomic<rex::input::InputSystem*> input{nullptr};
std::atomic<bool> intro_keyboard{false};
int32_t intro_pending_index=-1;uint8_t intro_old_request=0;bool intro_held=false;
constexpr uint32_t kIntroIndex=0x82942CC0,kIntroCount=0x82942CC4,kIntroRequest=0x82942CB2;
std::atomic<bool> pending{false},focused{false},input_armed{false};
std::atomic<uint64_t> focus_epoch{0};
std::atomic<uint16_t> buttons{0};
std::atomic<uint64_t> title_serial{0};
std::atomic<uint32_t> observed_title_mode{0},title_trace_mask{0},back_trace_count{0};
uint64_t title_consumed=0;
bool held_blocker=false,back_was_down=false;
std::atomic<uint32_t> label_address{0};
std::atomic<bool(*)()> other_menu{nullptr};
rex::Runtime* quit_runtime=nullptr;
std::function<void()> exit_handler;
std::atomic<bool> native_open{false};
uint32_t native_storage=0,native_callback=0,native_record=0;
uint64_t native_focus_epoch=0;
bool exit_scheduled=false;
constexpr uint32_t kNativePopup=0x8336E274;
constexpr uint32_t kNativeCurrent=0x8336F4C0,kNativeClosing=0x8336F4E0;
uint32_t Read(uint8_t* base,uint32_t address){return rex::memory::load_and_swap<uint32_t>(base+address);}
void Write(uint8_t* base,uint32_t address,uint32_t value){rex::memory::store_and_swap<uint32_t>(base+address,value);}
void SynchronizeBlocker() {
  // Called only on the guest frontend thread. SDK release masks held
  // dismissal buttons before the underlying menu receives input again.
  const std::lock_guard lock(motogp::InputStateMutex());
  auto* system=input.load(std::memory_order_acquire);
  if(!system)return;
  // The real game dialog needs guest input. Block only while waiting for the
  // opening press/focus transition to return to neutral, not while it is active.
  const bool wanted=pending.load(std::memory_order_acquire)&&
      (!native_open.load(std::memory_order_acquire)||!input_armed.load(std::memory_order_acquire));
  if(wanted&&!held_blocker){system->AddUIInputBlocker();held_blocker=true;}
  else if(!wanted&&held_blocker){
    system->RemoveUIInputBlocker();held_blocker=false;
  }
}
void Poll() {
  // F1 may be editing MnK bindings on the UI thread. Its gate also keeps
  // held dismissal inputs from becoming a new quit request.
  if(auto check=other_menu.load(std::memory_order_acquire);check&&check()){buttons=0;input_armed=false;return;}
  const std::lock_guard lock(motogp::InputStateMutex());
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
#include "quit_accent.inc"
void RequestIntroSkip(){intro_keyboard.store(true,std::memory_order_release);}
bool IntroSkipAllowed(uint8_t* base){
  const auto index=Read(base,kIntroIndex),count=Read(base,kIntroCount);
  // The same native updater also owns final Press Start. Only a loaded,
  // non-final startup entry may request the original fade/cleanup transition.
  const auto entries=Read(base,0x8288B188);
  return REXCVAR_GET(allow_intro_skip)&&Read(base,0x828DA060)==11&&count>1&&count<=32&&index<count-1&&
      Read(base,0x82942CBC)>10&&entries>=0x10000&&entries<=0xFFFFFBFF;
}
bool BeginIntroSkip(uint8_t* base){
  if(intro_pending_index>=0)return false;
  if(!IntroSkipAllowed(base)||base[kIntroRequest])return false;
  intro_pending_index=static_cast<int32_t>(Read(base,kIntroIndex));intro_old_request=base[kIntroRequest];base[kIntroRequest]=1;
  return true;
}
void FinishIntroSkip(uint8_t* base){
  if(intro_pending_index>=0&&(Read(base,kIntroIndex)!=static_cast<uint32_t>(intro_pending_index)||Read(base,0x828DA060)!=11)){
    base[kIntroRequest]=intro_old_request;intro_pending_index=-1;
  }
}
void PollIntroInput(uint8_t* base){
  FinishIntroSkip(base);
  const bool key=intro_keyboard.exchange(false,std::memory_order_acq_rel);
  const bool held=(buttons.load()&(rex::input::X_INPUT_GAMEPAD_A|rex::input::X_INPUT_GAMEPAD_START))!=0;
  const bool fresh=held&&!intro_held;intro_held=held;
  if(!focused.load()||Pending()||!input_armed.load())return;
  if((key||fresh)&&BeginIntroSkip(base)){
    // Use the SDK's held-release mask so the accepting button cannot enter the
    // next menu or skip another entry until released. Do not rewrite mappings.
    const std::lock_guard lock(motogp::InputStateMutex());
    if(auto* system=input.load()){system->AddUIInputBlocker();system->RemoveUIInputBlocker();}
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
bool ObserveTitleDraw(uint8_t* base,uint32_t startup,double alpha,uint64_t caller) {
  const auto value=Read(base,0x8292710C);
  const auto mode=value>=0x82000000&&value<0x84000000?Read(base,value):0;
  // There are exactly TWO native call sites. Boot's real PRESS START passes
  // r4=0 (not an in-race overlay); the secondary frontend path passes r4=1.
  // Match the caller as well as the mode so arbitrary/in-game calls can't arm.
  const uint32_t trace_bit=caller==0x8229CA24?1:caller==0x822AE94C?2:0;
  if(trace_bit&&!(title_trace_mask.fetch_or(trace_bit)&trace_bit))REXLOG_DEBUG("PC_QUIT_TITLE_DRAW mode={} caller=0x{:08X} flag={} alpha={} focused={} boot_mode={}",mode,caller,startup,alpha,focused.load(),Read(base,0x828DA060));
  const auto boot_mode=Read(base,0x828DA060);
  const bool boot=caller==0x8229CA24&&startup==0&&boot_mode==11;
  const bool frontend=caller==0x822AE94C&&startup==1&&mode==15&&alpha>=1.0;
  if(boot||frontend){observed_title_mode.store(boot?11:mode,std::memory_order_relaxed);title_serial.fetch_add(1,std::memory_order_release);}
  return boot||frontend;
}
void PollFrontendInput(){Poll();SynchronizeBlocker();}
void Initialize(rex::Runtime* runtime) {
  intro_keyboard=false;intro_pending_index=-1;intro_old_request=0;intro_held=false;
  input=static_cast<rex::input::InputSystem*>(runtime->input_system());
  ResetAccent(runtime->memory());
  const auto address=runtime->memory()->SystemHeapAlloc(128);
  if(!address)throw std::runtime_error("Could not allocate the PC quit menu label");
  label_address=address;const char* label=motogp::localization::Turkish()?"OYUNDAN ÇIK":"QUIT GAME";
  // The title's 826EC558 decoder consumes UTF-8 bytes, not Xbox WCHAR data.
  std::memcpy(runtime->virtual_membase()+address,label,std::strlen(label)+1);
  quit_runtime=runtime;native_storage=address+32;native_callback=0;native_record=0;
  native_open=false;exit_scheduled=false;native_focus_epoch=focus_epoch.load();
  pending=false;buttons=0;title_serial=0;observed_title_mode=0;title_trace_mask=0;back_trace_count=0;title_consumed=0;held_blocker=false;back_was_down=false;
}
uint32_t LabelAddress(){return label_address;}
void SetFocused(bool value){
  if(focused.exchange(value,std::memory_order_acq_rel)!=value){
    input_armed.store(false,std::memory_order_release);
    focus_epoch.fetch_add(1,std::memory_order_release);
  }
}
void Stop(){ResetAccent(nullptr);pending=false;native_open=false;exit_handler={};quit_runtime=nullptr;
  /* The runtime owns guest labels, dialog storage and the callback thunk. */
  input=nullptr;label_address=0;native_storage=0;native_callback=0;native_record=0;}
bool Pending(){return pending.load(std::memory_order_acquire);}
void SetOtherMenuCheck(bool (*check)()){other_menu.store(check,std::memory_order_release);}
void Request(){if(auto check=other_menu.load(std::memory_order_acquire);check&&check())return;if(focused.load(std::memory_order_acquire)&&input_armed.load(std::memory_order_acquire))pending.store(true,std::memory_order_release);}
namespace {
void CancelNative() {
  pending=false;native_open=false;native_record=0;input_armed=false;
  // Consume the native dismissal input before the underlying menu resumes.
  const std::lock_guard lock(motogp::InputStateMutex());
  if(auto* system=input.load()){system->AddUIInputBlocker();system->RemoveUIInputBlocker();}
}
void NativeResult(PPCContext& ctx,uint8_t* base) {
  // The real screen manager passes its copied payload in r3, result in r4.
  // Validate our cookie so stale callbacks cannot close the application.
  if(!pending.load()||!native_open.load()||!ctx.r3.u32||
      Read(base,ctx.r3.u32+16)!=native_storage+64)return;
  if(ctx.r4.s32==0&&focused.load()&&input_armed.load()&&exit_handler){
    if(!exit_scheduled){exit_scheduled=true;exit_handler();}
  }else CancelNative();
}
}
void SetExitHandler(std::function<void()> exit){exit_handler=std::move(exit);}
void TickConfirmation(PPCContext& ctx,uint8_t* base) {
  if(!pending.load()||!quit_runtime||exit_scheduled)return;
  const auto epoch=focus_epoch.load(std::memory_order_acquire);
  if(native_open.load()){
    if(Read(base,kNativeCurrent)!=native_record&&Read(base,kNativeClosing)!=native_record){CancelNative();return;}
    if(native_focus_epoch!=epoch){
      // Native payload +28/+48 hold the choice and its animated position.
      Write(base,native_record+8+28,1);
      rex::memory::store_and_swap<float>(base+native_record+8+48,1.f);
      native_focus_epoch=epoch;
    }
    SynchronizeBlocker();return;
  }
  if(!focused.load()||!input_armed.load()||buttons.load()!=0)return;
  // Do not replace another native dialog, including its closing animation.
  if(!Read(base,kNativePopup)||Read(base,kNativeCurrent)||Read(base,kNativeClosing))return;
  auto* dispatcher=quit_runtime->function_dispatcher();
  if(!native_callback){
    native_callback=dispatcher->AllocateThunk(NativeResult,0x821FC650);
    if(!native_callback){CancelNative();REXLOG_WARN("PC_QUIT: native callback allocation failed");return;}
  }
  PPCContext call=ctx;
  const auto message=[&](uint32_t handle){
    PPCContext lookup=ctx;lookup.r3.u64=handle;
    dispatcher->GetFunction(0x826EDB38)(lookup,base);return lookup.r3.u32;
  };
  const auto question=message(0x8288DB18),yes=message(0x8288C870),no=message(0x8288C874);
  if(!question||!yes||!no)return; // Message bank is not ready yet.
  std::memset(base+native_storage,0,80);
  Write(base,native_storage,question);
  // Explicit native Yes/No strings avoid the default Accept/Cancel labels.
  Write(base,native_storage+32,yes);Write(base,native_storage+36,no);
  // Keep Back cancellation. Native setup initially selects the last row; set
  // the requested first choice after setup, before any input/draw can run.
  Write(base,native_storage+32+12,0x0D);
  call=ctx;call.r3.u64=native_storage;call.r4.u64=label_address.load();
  call.r5.u64=native_storage+32;call.r6.u64=native_callback;
  call.r7.u64=4;call.r8.s64=-1;call.r9.u64=native_storage+64;
  dispatcher->GetFunction(0x82356E40)(call,base);
  native_record=Read(base,kNativeCurrent);
  if(!native_record||Read(base,native_record+4)!=native_callback){
    CancelNative();REXLOG_WARN("PC_QUIT: native confirmation could not be opened");return;
  }
  Write(base,native_record+8+28,0);
  rex::memory::store_and_swap<float>(base+native_record+8+48,0.f);
  native_focus_epoch=epoch;native_open=true;SynchronizeBlocker();
}
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
  if(Pending()){TickConfirmation(ctx,base);ctx.r3.s64=-1;return;}
  if(IsQuitSelection(base,ctx.r3.u32,ctx.r4.u32,ctx.r5.u32)){
    Request();SynchronizeBlocker();TickConfirmation(ctx,base);ctx.r3.s64=-1;return;
  }
  __imp__sub_821FCCA8(ctx,base);
}
REX_HOOK_RAW(sub_8229C178) {
  const auto startup=ctx.r4.u32;const double alpha=ctx.f1.f64;const auto caller=ctx.lr;
  __imp__sub_8229C178(ctx,base);
  motogp::quit::ObserveTitleDraw(base,startup,alpha,caller);
}
namespace motogp::quit {
bool PollTitleInput(uint8_t* base,uint32_t expected_mode) {
  using namespace motogp::quit;
  PollFrontendInput();
  const bool back=(buttons.load()&rex::input::X_INPUT_GAMEPAD_B)!=0;
  const auto serial=title_serial.load(std::memory_order_acquire);
  const auto value=Read(base,0x8292710C);
  const bool mode_matches=expected_mode==11?Read(base,0x828DA060)==11:
      value>=0x82000000&&value<0x84000000&&Read(base,value)==expected_mode;
  if(back&&!back_was_down&&serial&&observed_title_mode.load()==expected_mode&&back_trace_count.fetch_add(1)<8)REXLOG_DEBUG("PC_QUIT_TITLE_BACK mode={} fresh_draw={} focused={} armed={} pending={}",expected_mode,serial!=title_consumed,focused.load(),input_armed.load(),Pending());
  if(!Pending()&&serial!=title_consumed&&observed_title_mode.load()==expected_mode&&mode_matches&&back&&!back_was_down){Request();SynchronizeBlocker();}
  // Several native updates may occur between draws. A neutral poll must not
  // consume the visible title, otherwise a later Back edge in the same frame
  // is lost before the next draw can rearm it.
  if((back&&!back_was_down)||!mode_matches)title_consumed=serial;
  back_was_down=back;
  return Pending();
}
}
REX_HOOK_RAW(sub_8229C2C0) {
  if(motogp::quit::PollTitleInput(base,11)){motogp::quit::TickConfirmation(ctx,base);return;}
  motogp::quit::PollIntroInput(base);
  __imp__sub_8229C2C0(ctx,base);
  motogp::quit::FinishIntroSkip(base);
}
REX_HOOK_RAW(sub_822AE9F8) {
  if(motogp::quit::PollTitleInput(base,15)){motogp::quit::TickConfirmation(ctx,base);return;}
  __imp__sub_822AE9F8(ctx,base);
}

REX_HOOK_RAW(sub_821FC9B0) {
  using namespace motogp::quit;
  const auto previous=drawing_main_menu;drawing_main_menu=ctx.r3.u32;
  struct Restore {uint32_t previous;~Restore(){drawing_main_menu=previous;}} restore{previous};
  __imp__sub_821FC9B0(ctx,base);
}
REX_HOOK_RAW(sub_8227FEE8) {
  using namespace motogp::quit;
  const bool previous=drawing_quit_label;
  drawing_quit_label=IsQuitLabelDraw(base,drawing_main_menu,ctx.r3.u32,ctx.r4.u32,ctx.lr);
  struct Restore {bool previous;~Restore(){drawing_quit_label=previous;}} restore{previous};
  __imp__sub_8227FEE8(ctx,base);
}
REX_HOOK_RAW(sub_82288CD0) {
  using namespace motogp::quit;
  const bool previous=drawing_white_accent;
  drawing_white_accent=IsWhiteAccentDraw(base,drawing_main_menu,ctx.r31.u32,ctx.r18.u32,ctx.lr)||
      (drawing_quit_label&&IsWhiteCapPass(ctx.lr));
  struct Restore {bool previous;~Restore(){drawing_white_accent=previous;}} restore{previous};
  __imp__sub_82288CD0(ctx,base);
}
REX_HOOK_RAW(sub_82389D10) {
  using namespace motogp::quit;
  if(drawing_white_accent)if(const auto material=WhiteMaterial(base,ctx.r4.u32))ctx.r4.u64=material;
  __imp__sub_82389D10(ctx,base);
}
REX_HOOK_RAW(sub_82379D38) {
  using namespace motogp::quit;
  const bool previous=binding_white_accent;
  binding_white_accent=drawing_white_accent||IsWhiteMaterial(ctx.r4.u32);
  struct Restore {bool previous;~Restore(){binding_white_accent=previous;}} restore{previous};
  __imp__sub_82379D38(ctx,base);
}
REX_HOOK_RAW(sub_8236E940) {
  using namespace motogp::quit;
  if(binding_white_accent&&ctx.r3.u32==15&&ctx.lr==0x82379EA4)
    if(const auto descriptor=WhiteAccentDescriptor(base,ctx.r4.u32))ctx.r4.u64=descriptor;
  __imp__sub_8236E940(ctx,base);
}
