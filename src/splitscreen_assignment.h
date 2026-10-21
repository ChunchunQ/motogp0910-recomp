#pragma once
#include <rex/input/device_assignment.h>
#include <algorithm>
#include <array>

namespace motogp::splitscreen {
// InputSystem callers and session changes share InputStateMutex. No new timer
// or per-frame eligibility scan: this receives the SDK's existing hotplug list.
class Assignment final : public rex::input::DeviceAssignment {
 public:
  using DeviceId = rex::input::DeviceId;
  using DeviceInfo = rex::input::DeviceInfo;
  static constexpr auto kKeyboard = static_cast<DeviceId>(0x4D4E4B00);

  void OnDevicesChanged(const std::vector<DeviceInfo>& devices) override {
    devices_ = devices;
    normal_.OnDevicesChanged(devices);
  }
  void DevicesForUser(uint32_t user, std::vector<DeviceId>& out) const override {
    if (!active_) { normal_.DevicesForUser(user, out); return; }
    out.clear();
    if (user >= players_.size()) return;
    const auto& player = players_[user];
    const auto it = std::find_if(devices_.begin(), devices_.end(), [&](const auto& d) {
      // Physical ordinals remain stable across other players disconnecting.
      // A replacement may reclaim the vacant ordinal, never a live player's.
      return player.keyboard ? UsableKeyboard(d) :
          !d.synthetic && d.ordinal == player.ordinal;
    });
    if (it != devices_.end()) out.push_back(it->id);
  }
  bool Begin() {
    if (active_) return true;
    std::vector<DeviceInfo> pads;
    bool keyboard = false;
    for (const auto& d : devices_) {
      if (!d.synthetic) pads.push_back(d);
      keyboard |= UsableKeyboard(d);
    }
    std::sort(pads.begin(), pads.end(), [](const auto& a, const auto& b) {
      return a.ordinal < b.ordinal;
    });
    if (pads.size() >= 2) {
      players_ = {Player{false, pads[0].ordinal}, Player{false, pads[1].ordinal}};
    } else if (keyboard && pads.size() == 1) {
      // The controller always owns player 1; keyboard drives player 2.
      players_ = {Player{false, pads[0].ordinal}, Player{true, 0}};
    } else return false;  // The NOP stand-in never counts as a second device.
    active_ = true;
    return true;
  }
  void End() { active_ = false; players_ = {}; }
  bool active() const { return active_; }

 private:
  struct Player { bool keyboard = false; uint32_t ordinal = 0; };
  static bool UsableKeyboard(const DeviceInfo& d) {
    // MnK passthrough is an Xbox USB keyboard, not the virtual racing pad.
    return d.synthetic && d.id == kKeyboard && d.subtype == 0x01;
  }
  rex::input::SlotAssignment normal_;
  std::vector<DeviceInfo> devices_;
  std::array<Player, 2> players_{};
  bool active_ = false;
};
}
