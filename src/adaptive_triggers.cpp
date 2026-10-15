#include "adaptive_triggers.h"
#include "shared_sdl.h"
#include <rex/cvar.h>
#include <rex/logging.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

REXCVAR_DEFINE_BOOL(dualsense_adaptive_triggers,false,"Input","Experimental DualSense adaptive trigger resistance; requires native SDL access");
REXCVAR_DEFINE_STRING(dualsense_rt_mode,"weapon","Input","Right trigger resistance profile").allowed({"off","feedback","weapon"});
REXCVAR_DEFINE_INT32(dualsense_rt_start,3,"Input","Right trigger start position (0-9)").range(0,9);
REXCVAR_DEFINE_INT32(dualsense_rt_end,6,"Input","Right trigger end position (0-9), greater than start in Weapon mode").range(0,9);
REXCVAR_DEFINE_INT32(dualsense_rt_strength,5,"Input","Right trigger strength (0-8; zero releases resistance)").range(0,8);
REXCVAR_DEFINE_STRING(dualsense_lt_mode,"feedback","Input","Left trigger resistance profile").allowed({"off","feedback","weapon"});
REXCVAR_DEFINE_INT32(dualsense_lt_start,2,"Input","Left trigger start position (0-9)").range(0,9);
REXCVAR_DEFINE_INT32(dualsense_lt_end,0,"Input","Left trigger end position (0-9), greater than start in Weapon mode").range(0,9);
REXCVAR_DEFINE_INT32(dualsense_lt_strength,4,"Input","Left trigger strength (0-8; zero releases resistance)").range(0,8);
namespace motogp::adaptive {
bool LiveAvailable(){return rex::cvar::GetFlagByName("input_backend")=="sdl"&&shared_sdl::HasGamepadEffects();}
namespace {
std::atomic<bool> focused{false};
std::mutex profile_mutex;
// Status describes an accepted SDL report, never a claim about physical feel.
enum class State {Disabled,Backend,Waiting,Sent,Failed,Unfocused,Unavailable};
std::atomic<State> state{State::Disabled};
std::jthread worker;
std::mutex wake_mutex;
std::condition_variable_any wake;
struct Pad {SDL_Gamepad* handle=nullptr;Report last{};bool sent=false,owned_effect=false,failed=false;};
void Run(std::stop_token stop) {
  std::map<SDL_JoystickID,Pad> pads;
  const auto release=[](Pad& pad) {
    if(pad.owned_effect) {
      const auto off=Encode(Profile{});
      if(!shared_sdl::SendGamepadEffect(pad.handle,off.data(),static_cast<int>(off.size())))
        REXLOG_WARN("DUALSENSE: release report failed: {}",shared_sdl::GetError());
    }
    shared_sdl::CloseGamepad(pad.handle);
  };
  try {
    while(!stop.stop_requested()) {
      const auto profile=ReadProfile();const bool sdl=rex::cvar::GetFlagByName("input_backend")=="sdl";
      if(!profile.enabled||!sdl) {
        for(auto& [id,pad]:pads)release(pad);pads.clear();
        state.store(profile.enabled?State::Backend:State::Disabled);
      } else {
        int count=0;auto* found=shared_sdl::GetGamepads(&count);std::vector<SDL_JoystickID> ids;
        if(found){ids.assign(found,found+count);shared_sdl::Free(found);}
        for(auto it=pads.begin();it!=pads.end();) {
          if(std::find(ids.begin(),ids.end(),it->first)==ids.end()) {
            shared_sdl::CloseGamepad(it->second.handle);it=pads.erase(it);
          }else ++it;
        }
        for(auto id:ids)if(!pads.contains(id)&&shared_sdl::GetGamepadTypeForID(id)==SDL_GAMEPAD_TYPE_PS5) {
          if(auto* handle=shared_sdl::OpenGamepad(id))pads.emplace(id,Pad{.handle=handle});
        }
        bool failed=false;const bool active=focused.load();const auto report=Encode(profile,active);
        for(auto& [id,pad]:pads) {
          // No writes for unchanged profiles, even on a failed device. A new
          // profile, focus transition or reconnection permits a fresh attempt.
          if(!pad.sent||pad.last!=report) {
            pad.last=report;pad.sent=true;
            const bool ok=shared_sdl::SendGamepadEffect(pad.handle,report.data(),static_cast<int>(report.size()));
            pad.failed=!ok;
            if(ok){pad.owned_effect=true;REXLOG_INFO("DUALSENSE: trigger report accepted for device {}; focused={}",id,active);}
            else REXLOG_WARN("DUALSENSE: trigger report rejected for device {}: {}",id,shared_sdl::GetError());
          }
          failed|=pad.failed;
        }
        state.store(pads.empty()?State::Waiting:failed?State::Failed:!active?State::Unfocused:State::Sent);
      }
      std::unique_lock lock(wake_mutex);wake.wait_for(lock,stop,std::chrono::milliseconds(100),[]{return false;});
    }
  } catch(const std::exception& e) {state.store(State::Failed);REXLOG_ERROR("DUALSENSE: effect service stopped: {}",e.what());}
  for(auto& [id,pad]:pads)release(pad);
}
}
Profile ReadProfile() {
  const std::lock_guard lock(profile_mutex);
  Profile p;p.enabled=rex::cvar::Query<bool>("dualsense_adaptive_triggers");
  p.right={rex::cvar::GetFlagByName("dualsense_rt_mode"),rex::cvar::Query<int32_t>("dualsense_rt_start"),rex::cvar::Query<int32_t>("dualsense_rt_end"),rex::cvar::Query<int32_t>("dualsense_rt_strength")};
  p.left={rex::cvar::GetFlagByName("dualsense_lt_mode"),rex::cvar::Query<int32_t>("dualsense_lt_start"),rex::cvar::Query<int32_t>("dualsense_lt_end"),rex::cvar::Query<int32_t>("dualsense_lt_strength")};return p;
}
bool WriteProfile(const Profile& p) {
  if(!p.Valid())return false;
  const std::lock_guard lock(profile_mutex);
  bool ok=rex::cvar::SetFlagByName("dualsense_adaptive_triggers",p.enabled?"true":"false");
  for(int side=0;side<2;++side){const auto& t=side?p.left:p.right;const std::string key=side?"dualsense_lt_":"dualsense_rt_";
    ok=rex::cvar::SetFlagByName(key+"mode",t.mode)&&ok;
    ok=rex::cvar::SetFlagByName(key+"start",std::to_string(t.start))&&ok;
    ok=rex::cvar::SetFlagByName(key+"end",std::to_string(t.end))&&ok;
    ok=rex::cvar::SetFlagByName(key+"strength",std::to_string(t.strength))&&ok;
  }
  wake.notify_all();return ok;
}
void Start(bool has_focus) {
  Stop();focused.store(has_focus);
  if(!shared_sdl::HasGamepadEffects()){state.store(State::Unavailable);REXLOG_WARN("DUALSENSE: matching shared SDL effect API is missing");return;}
  worker=std::jthread(Run);
}
void Stop(){if(worker.joinable()){worker.request_stop();wake.notify_all();worker.join();}state.store(State::Disabled);}
void SetFocused(bool value){focused.store(value);wake.notify_all();}
std::string Status(bool tr) {
  switch(state.load()) {
    case State::Backend:return tr?"Adaptif tetikler için SDL kontrolcü arka ucunu seç.":"Select the SDL controller backend for adaptive triggers.";
    case State::Waiting:return tr?"Yerel DualSense bekleniyor. 8BitDo / DS3 / DS4 desteklemez.":"Waiting for a native DualSense. 8BitDo / DS3 / DS4 do not support this.";
    case State::Sent:return tr?"Efekt SDL tarafından kabul edildi; fiziksel direnç testi gerekli.":"SDL accepted the effect; physical resistance testing is still required.";
    case State::Failed:return tr?"Kol efekti kabul etmedi. USB / yerel SDL bağlantısını kontrol et.":"Controller rejected the effect. Check USB / native SDL access.";
    case State::Unfocused:return tr?"Oyun odakta değil: tetik direnci bırakıldı.":"Game unfocused: trigger resistance released.";
    case State::Unavailable:return tr?"Bu buildin eşleşen runtime dosyası gerekli.":"The matching runtime from this build is required.";
    default:return tr?"DualSense adaptif tetikler kapalı.":"DualSense adaptive triggers are off.";
  }
}
}
