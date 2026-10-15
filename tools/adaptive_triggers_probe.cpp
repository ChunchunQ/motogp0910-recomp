// Real shared runtime SDL transport and virtual pads. No physical resistance
// claim: callbacks validate which device receives the exact production report.
#include "../src/adaptive_triggers.h"
#include "../src/shared_sdl.h"
#include "../src/launcher/core.h"
#include <rex/cvar.h>
#include <mutex>
#include <thread>
#include <iostream>
#include <stdexcept>
#include <cstring>
using namespace motogp;
namespace {
int checks=0;
void Check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);++checks;}
struct Capture {
  std::mutex mutex;std::vector<adaptive::Report> reports;bool fail=false;
  static bool SDLCALL Send(void* data,const void* bytes,int size){
    auto& self=*static_cast<Capture*>(data);std::lock_guard lock(self.mutex);
    if(size!=47)return false;adaptive::Report r{};std::memcpy(r.data(),bytes,47);self.reports.push_back(r);return !self.fail;
  }
  size_t Count(){std::lock_guard lock(mutex);return reports.size();}
  adaptive::Report Last(){std::lock_guard lock(mutex);return reports.empty()?adaptive::Report{}:reports.back();}
};
void Wait(Capture& c,size_t previous,const adaptive::Report& expected){
  const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
  while(std::chrono::steady_clock::now()<deadline){if(c.Count()>previous&&c.Last()==expected){++checks;return;}std::this_thread::sleep_for(std::chrono::milliseconds(10));}
  throw std::runtime_error("Timed out waiting for production SDL effect report");
}
}
int main(){try{
  Check(shared_sdl::HasGamepadEffects(),"Matching runtime has additive effect export");
  adaptive::Profile p;p.enabled=true;const auto expected=adaptive::Encode(p);
  Check(expected[0]==0x0c&&expected[10]==2&&expected[11]==85&&expected[12]==170&&expected[13]==159&&expected[21]==1&&expected[22]==57&&expected[23]==128,"Reference legacy mode bytes scale UI positions and forces to full byte range");
  for(size_t i=1;i<expected.size();++i)if(i<10||i>=32)Check(expected[i]==0,"No rumble, audio, LED or unrelated output flags");
  const auto off=adaptive::Encode(adaptive::Profile{});Check(off[0]==12&&std::all_of(off.begin()+1,off.end(),[](uint8_t v){return v==0;}),"Disabled profile explicitly releases BOTH triggers");
  Check(adaptive::Encode(p,false)==off,"Focus loss encodes complete release");
  auto invalid=p;invalid.right.start=9;invalid.right.end=0;
  Check(!invalid.Valid()&&adaptive::Encode(invalid)[10]==0&&adaptive::Encode(invalid)[21]==1,"Invalid right range fails closed independently of valid left trigger");
  invalid=p;invalid.left.strength=9;Check(!invalid.Valid()&&adaptive::Encode(invalid)[21]==0,"Invalid strength cannot reach hardware");
  invalid=p;invalid.right.strength=0;Check(adaptive::Encode(invalid)[10]==0,"Strength zero releases the side");
  for(int start=0;start<=9;++start)for(int end=0;end<=9;++end){auto q=p;q.right.start=start;q.right.end=end;
    Check(q.Valid()==(end>start),"Weapon end must exceed start");if(q.Valid())Check(adaptive::Encode(q)[11]<adaptive::Encode(q)[12],"Encoded interval retains ordering");}
  launcher::Settings settings;settings.adaptive=p;toml::table table;settings.Store(table);
  Check(launcher::Settings::Load(table).adaptive==p,"All nine launcher keys round-trip without SDK dependence");
  Check(adaptive::WriteProfile(p)&&adaptive::ReadProfile()==p,"Profile updates actual registered runtime flags");
  Check(!adaptive::WriteProfile(invalid=adaptive::Profile{true,{"weapon",9,1,5},{"feedback",2,0,4}})&&adaptive::ReadProfile()==p,"Invalid live profile is rejected without partial flag changes");
  rex::cvar::SetFlagByName("input_backend","sdl");
  Check(shared_sdl::Init(SDL_INIT_GAMEPAD),"Initialize actual runtime SDL");
  Capture sony,xbox,ps4,bitdo;
  auto attach=[](Capture& capture,Uint16 vendor,Uint16 product,const char* name){
    SDL_VirtualJoystickDesc d{};SDL_INIT_INTERFACE(&d);d.type=SDL_JOYSTICK_TYPE_GAMEPAD;d.naxes=6;d.nbuttons=15;d.axis_mask=63;d.button_mask=0x7fff;d.vendor_id=vendor;d.product_id=product;d.name=name;d.userdata=&capture;d.SendEffect=Capture::Send;
    return shared_sdl::AttachVirtualJoystick(&d);
  };
  const auto ps=attach(sony,0x054c,0x0ce6,"DualSense adaptive probe");
  const auto xb=attach(xbox,0x045e,0x02ea,"Xbox adaptive exclusion probe");
  const auto ds4=attach(ps4,0x054c,0x09cc,"DualShock 4 exclusion probe");
  const auto bd=attach(bitdo,0x2dc8,0x3106,"8BitDo Ultimate exclusion probe");
  Check(ps&&xb&&ds4&&bd&&shared_sdl::GetGamepadTypeForID(ps)==SDL_GAMEPAD_TYPE_PS5,"Virtual identities use real SDL classification");
  auto pump=[] {shared_sdl::UpdateJoysticks();SDL_Event e;while(shared_sdl::PollEvent(&e)){};};pump();
  adaptive::Start(true);struct Cleanup{~Cleanup(){adaptive::Stop();}} cleanup;
  Wait(sony,0,expected);
  std::this_thread::sleep_for(std::chrono::milliseconds(350));
  Check(sony.Count()==1&&!xbox.Count()&&!ps4.Count()&&!bitdo.Count(),"Unchanged effects are not spammed; Xbox, DS4 and 8BitDo never receive trigger reports");
  p.right={"feedback",4,6,8};size_t n=sony.Count();Check(adaptive::WriteProfile(p),"Change live right profile");Wait(sony,n,adaptive::Encode(p));
  p.right.mode="off";n=sony.Count();adaptive::WriteProfile(p);Wait(sony,n,adaptive::Encode(p));Check(sony.Last()[10]==0&&sony.Last()[21]==1,"One side off explicitly resets it while retaining other side");
  n=sony.Count();adaptive::SetFocused(false);Wait(sony,n,off);
  n=sony.Count();adaptive::SetFocused(true);Wait(sony,n,adaptive::Encode(p));
  n=sony.Count();p.enabled=false;adaptive::WriteProfile(p);Wait(sony,n,off);
  p.enabled=true;n=sony.Count();adaptive::WriteProfile(p);Wait(sony,n,adaptive::Encode(p));
  n=sony.Count();rex::cvar::SetFlagByName("input_backend","xinput");Wait(sony,n,off);
  n=sony.Count();rex::cvar::SetFlagByName("input_backend","sdl");Wait(sony,n,adaptive::Encode(p));
  n=sony.Count();adaptive::Stop();Check(sony.Count()>n&&sony.Last()==off,"Shutdown releases held firmware effects before closing handles");
  n=sony.Count();adaptive::Start(true);Wait(sony,n,adaptive::Encode(p));
  shared_sdl::DetachVirtualJoystick(ps);pump();std::this_thread::sleep_for(std::chrono::milliseconds(160));
  n=sony.Count();const auto ps2=attach(sony,0x054c,0x0df2,"DualSense Edge reconnect probe");pump();Wait(sony,n,adaptive::Encode(p));Check(shared_sdl::GetGamepadTypeForID(ps2)==SDL_GAMEPAD_TYPE_PS5,"DualSense Edge reconnection reapplies current profile");
  adaptive::Stop();
  {std::lock_guard lock(sony.mutex);sony.fail=true;}
  n=sony.Count();adaptive::Start(true);Wait(sony,n,adaptive::Encode(p));std::this_thread::sleep_for(std::chrono::milliseconds(350));
  Check(sony.Count()==n+1&&adaptive::Status(false).find("rejected")!=std::string::npos,"Rejected device produces one attempt and visible failure, without periodic log spam");
  adaptive::Stop();
  for(auto id:{ps2,xb,ds4,bd})shared_sdl::DetachVirtualJoystick(id);
  Check(!xbox.Count()&&!ps4.Count()&&!bitdo.Count(),"Unsupported controllers remain untouched over every lifecycle transition");
  std::cout<<"Passed "<<checks<<" adaptive trigger software/transport checks; physical DualSense testing pending\n";return 0;
}catch(const std::exception& e){adaptive::Stop();std::cerr<<"FAIL after "<<checks<<" checks: "<<e.what()<<'\n';return 1;}}
