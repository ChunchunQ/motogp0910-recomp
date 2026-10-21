#pragma once
#include <rex/ui/imgui_dialog.h>
#include <rex/ui/presenter.h>
#include <atomic>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>
struct ImFont;
namespace rex::input {class InputSystem;}
namespace motogp::settings {
enum class Page {General,Graphics,Advanced,Audio,Controls};
enum class Kind {Toggle,Integer,Number,Choice,Binding};
struct Choice {std::string value,en,tr;};
struct Spec {
  std::string key,en,tr;Page page;Kind kind;
  double minimum=0,maximum=0;std::vector<Choice> choices;
};
const std::vector<Spec>& Catalog();
bool ValidBinding(std::string_view value);
// Updates the actual presenter config, not merely startup-only CVar storage.
bool PatchPaintConfig(rex::ui::Presenter::GuestOutputPaintConfig& config,
                      std::string_view key,std::string_view value);
class Model {
 public:
  Model(std::filesystem::path config,rex::ui::Presenter* presenter);
  ~Model();
  bool Available(const Spec& spec)const;
  std::string Value(std::string_view key)const;
  bool Set(std::string_view key,std::string value);
  bool Save();
  bool Revert();
  size_t Unsaved()const;
  const std::string& error()const{return error_;}
  const std::filesystem::path& path()const{return path_;}
 private:
  struct Disk;
  std::filesystem::path path_;std::unique_ptr<Disk> disk_;
  rex::ui::Presenter* presenter_;
  std::map<std::string,std::string,std::less<>> saved_;
  std::string error_;
};
struct View {
  Page page=Page::General;
  std::string capture;bool capture_wait=true;
  std::map<std::string,std::string> edits;
  bool saved=false;
};
bool DrawMenu(Model& model,View& view,bool turkish,ImFont* font,ImFont* heading);
// UI thread owns open/close and the dialog. Device polling and release masking
// run on the existing guest frame hook, never concurrently on the UI thread.
class InputGate {
 public:
  void Initialize(rex::input::InputSystem* input);
  void Open();void Close();void PollGuest();
  bool IsOpen()const{return open_.load(std::memory_order_acquire);}
  bool Blocking()const{return held_.load(std::memory_order_acquire);}
  void SetFocused(bool focused);
  uint16_t Buttons()const{return buttons_.load(std::memory_order_acquire);}
  bool NeutralReady()const{return IsOpen()&&focused_.load(std::memory_order_acquire)&&neutral_epoch_.load(std::memory_order_acquire)==Epoch();}
  uint64_t Epoch()const{return epoch_.load(std::memory_order_acquire);}
 private:
  rex::input::InputSystem* input_=nullptr;
  std::mutex transition_;
  bool restore_mouse_=false;
  std::atomic<bool> open_{false},held_{false},focused_{false};
  std::atomic<uint64_t> neutral_epoch_{UINT64_MAX};
  std::atomic<uint16_t> buttons_{0};std::atomic<uint64_t> epoch_{0};
};
std::unique_ptr<rex::ui::ImGuiDialog> CreateMenu(rex::ui::ImGuiDrawer* drawer,
    std::shared_ptr<Model> model,std::shared_ptr<InputGate> gate,
    ImFont* font,ImFont* heading,bool turkish,rex::ui::Window* window=nullptr,bool (*can_open)()=nullptr);
void SetGuestGate(InputGate* gate);
void PollGuestInput();
bool GuestInputBlocked();
// Compatibility access to InputStateMutex: serialize device polling and MnK
// binding edits. UI key handlers and binding edits run on the same UI thread.
std::mutex& InputValueMutex();
}
