#include "nobk_panel.h"
#include "achievement_ui.h"
#include "in_game_settings.h"
#include "overlay_theme.h"
#include "quit_menu.h"
#include "../third_party/nlohmann/json.hpp"

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/input/input_system.h>
#include <rex/ui/keybinds.h>
#include <rex/ui/window.h>
#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <optional>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <ctime>
#include <deque>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#include <winhttp.h>
#endif

REXCVAR_DEFINE_STRING(bind_nobk_panel,"F1","Keybinds","Open the NOBK Online panel (friends, server, records)");

namespace motogp::nobk {
namespace {
using namespace motogp::overlay;
using json = nlohmann::json;
using Clock = std::chrono::steady_clock;

// Native track index (view - 1) % 21. Slot 17 is reserved and empty in every package.
constexpr const char* kTracks[21]={"Losail","Motegi","Jerez","Le Mans","Mugello","Catalunya","Assen",
    "Laguna Seca","Sachsenring","Donington Park","Brno","Indianapolis","Misano","Estoril","Phillip Island",
    "Sepang","Valencia","","Silverstone","Shanghai","Istanbul Park"};

std::string Encode(std::string_view text) {
  constexpr char hex[]="0123456789ABCDEF";
  std::string out;
  for(unsigned char ch:text) {
    if((ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')||(ch>='0'&&ch<='9')||ch=='-'||ch=='_'||ch=='.'||ch=='~')out+=char(ch);
    else {out+='%';out+=hex[ch>>4];out+=hex[ch&15];}
  }
  return out;
}
bool SameName(std::string_view a,std::string_view b) {
  if(a.size()!=b.size())return false;
  for(size_t i=0;i<a.size();++i){auto x=a[i],y=b[i];if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return false;}
  return true;
}
std::string Clean(const json& object,const char* key,size_t limit=40) {
  const auto it=object.find(key);
  if(it==object.end()||!it->is_string())return {};
  auto value=it->get<std::string>();
  if(value.size()>limit)value.resize(limit);
  std::erase_if(value,[](unsigned char ch){return ch<0x20||ch==0x7F;});
  return value;
}
int64_t Int(const json& object,const char* key) {
  const auto it=object.find(key);
  return it!=object.end()&&it->is_number_integer()?it->get<int64_t>():0;
}
bool OpenUrl(const std::string& url) {
#ifdef _WIN32
  const std::wstring wide(url.begin(),url.end());
  return reinterpret_cast<intptr_t>(ShellExecuteW(nullptr,L"open",wide.c_str(),nullptr,nullptr,SW_SHOWNORMAL))>32;
#else
  (void)url;return false;
#endif
}

// Small synchronous HTTPS GET to www.nobkonline.com on the panel's worker.
// Public read-only endpoints; redirects are never followed. Stop() closes the
// session handle, which aborts a request in progress.
class Http {
 public:
  ~Http(){Close();}
  void Close() {
#ifdef _WIN32
    std::lock_guard lock(mutex_);
    if(connection_)WinHttpCloseHandle(std::exchange(connection_,nullptr));
    if(session_)WinHttpCloseHandle(std::exchange(session_,nullptr));
#endif
    closed_=true;
  }
  // -> HTTP status (0 = network failure); body only for 200.
  int Get(const std::string& path,std::string& body,size_t limit=512*1024) {
    body.clear();
#ifdef _WIN32
    HINTERNET connection=nullptr;
    {
      std::lock_guard lock(mutex_);
      if(closed_)return 0;
      if(!session_) {
        session_=WinHttpOpen(L"MotoGP0910-NOBK/1",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
        if(!session_)session_=WinHttpOpen(L"MotoGP0910-NOBK/1",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
        if(!session_)return 0;
        WinHttpSetTimeouts(session_,3000,4000,4000,5000);
      }
      if(!connection_)connection_=WinHttpConnect(session_,L"www.nobkonline.com",INTERNET_DEFAULT_HTTPS_PORT,0);
      connection=connection_;
    }
    if(!connection)return 0;
    const std::wstring wide(path.begin(),path.end());
    HINTERNET request=WinHttpOpenRequest(connection,L"GET",wide.c_str(),nullptr,WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE);
    if(!request)return 0;
    struct Closer{HINTERNET h;~Closer(){WinHttpCloseHandle(h);}} closer{request};
    DWORD never=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    WinHttpSetOption(request,WINHTTP_OPTION_REDIRECT_POLICY,&never,sizeof(never));
    if(!WinHttpSendRequest(request,WINHTTP_NO_ADDITIONAL_HEADERS,0,WINHTTP_NO_REQUEST_DATA,0,0,0)||
       !WinHttpReceiveResponse(request,nullptr))return 0;
    DWORD status=0,size=sizeof(status);
    if(!WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,&status,&size,WINHTTP_NO_HEADER_INDEX))return 0;
    if(status!=200)return int(status);
    char chunk[8192];
    for(;;) {
      DWORD got=0;
      if(!WinHttpReadData(request,chunk,sizeof(chunk),&got))return 0;
      if(!got)break;
      if(body.size()+got>limit)return 0;
      body.append(chunk,got);
    }
    return 200;
#else
    (void)path;(void)limit;return 0;
#endif
  }
 private:
  std::mutex mutex_;
  bool closed_=false;
#ifdef _WIN32
  HINTERNET session_=nullptr,connection_=nullptr;
#endif
};

enum class Where { Offline, Site, Menus, Room, Racing };
struct Friend { std::string name,country,room; Where where=Where::Offline; };
struct Room { std::string name,host; int members=0,capacity=0; bool racing=false,friends=false,mine=false; };
struct Recent { std::string name,board,time; int rank=0; int64_t at=0; bool mine=false,friend_=false; };
struct Event { std::string text; int64_t at=0; int kind=0; };  // kind 0 info, 1 good, 2 bad
struct Snapshot {
  bool available=false,loaded=false,failed=false;
  std::string me;
  std::vector<Friend> friends;
  std::vector<Room> rooms;
  std::vector<Recent> recent;
  std::deque<Event> events;
  int online=0,open_rooms=0,racing=0,riders=0,records=0;
  int my_records=-1,my_boards=0,my_podiums=0;
  bool server_up=false;
  uint64_t serial=0;
};

std::string BoardName(const json& row,bool tr) {
  auto track=Clean(row,"track",32);
  const auto view=Int(row,"view");
  if(view>=1) {const auto i=(view-1)%21;if(kTracks[i][0])track=kTracks[i];}
  if(track.empty())track=std::string(tr?"Tablo #":"Board #")+std::to_string(view);
  const auto cls=Clean(row,"class",16);
  return cls.empty()?track:track+" · "+cls;
}
std::string Ago(int64_t when,bool tr) {
  const auto now=int64_t(std::time(nullptr));
  const auto s=std::max<int64_t>(0,now-when);
  if(s<60)return tr?"az önce":"just now";
  if(s<3600)return std::to_string(s/60)+(tr?" dk":" min");
  if(s<86400)return std::to_string(s/3600)+(tr?" sa":" h");
  return std::to_string(s/86400)+(tr?" gün":" d");
}

class State {
 public:
  State(std::shared_ptr<settings::InputGate> input_gate):gate(std::move(input_gate)) {
    // Independent servers reuse names and IDs; only the production service
    // maps to nobkonline.com accounts.
    const auto origin=rex::cvar::Query<std::string>("online_server");
    snapshot_.available=rex::cvar::Query<std::string>("network_mode")=="online"&&
        (origin=="https://motogp.nobkonline.com"||origin=="https://motogp.nobkonline.com/");
    snapshot_.me=rex::cvar::Query<std::string>("lan_player_name");
    if(snapshot_.available&&!snapshot_.me.empty())worker_=std::thread([this]{Run();});
  }
  ~State(){Stop();}
  void Toggle() {
    if(gate->IsOpen()){gate->Close();return;}
    if(quit::Pending()||settings::GuestInputBlocked())return;
    gate->Open();
    if(gate->IsOpen()){std::lock_guard lock(mutex_);wake_=true;changed_.notify_one();}
  }
  void Close(){gate->Close();}
  Snapshot Read(){std::lock_guard lock(mutex_);return snapshot_;}
  bool PopToast(Event& out) {
    std::lock_guard lock(mutex_);
    if(toasts_.empty())return false;
    out=toasts_.front();toasts_.pop_front();return true;
  }
  void Stop() {
    {std::lock_guard lock(mutex_);stopping_=true;}
    changed_.notify_one();
    http_.Close();
    if(worker_.joinable())worker_.join();
    gate->Close();
  }
  void PollPad(rex::input::InputSystem* input) {
    if(!input||!snapshot_.available)return;
    const auto now=Clock::now();
    if(now<next_pad_)return;
    next_pad_=now+std::chrono::milliseconds(50);
    bool held=false;
    {
      const std::lock_guard lock(settings::InputValueMutex());
      for(uint32_t user=0;user<4&&!held;++user) {
        rex::input::X_INPUT_STATE state{};
        if(input->GetStateForUI(user,&state)!=0)continue;
        const auto b=state.gamepad.buttons;
        held=(b&rex::input::X_INPUT_GAMEPAD_BACK)&&(b&rex::input::X_INPUT_GAMEPAD_START);
      }
    }
    if(!held){combo_since_.reset();combo_used_=false;return;}
    if(!combo_since_)combo_since_=now;
    if(!combo_used_&&now-*combo_since_>=std::chrono::milliseconds(450)&&!gate->IsOpen()){combo_used_=true;Toggle();}
  }
  const std::shared_ptr<settings::InputGate> gate;

 private:
  void Push(std::string text,int kind,bool toast=true) {
    // mutex_ held by caller
    Event e{std::move(text),int64_t(std::time(nullptr)),kind};
    snapshot_.events.push_front(e);
    while(snapshot_.events.size()>30)snapshot_.events.pop_back();
    if(toast&&toasts_.size()<6)toasts_.push_back(std::move(e));
  }
  bool Fetch(const std::string& path,json& out) {
    std::string body;
    const auto status=http_.Get(path,body);
    if(status!=200)return false;
    out=json::parse(body,nullptr,false);
    return !out.is_discarded()&&out.is_object();
  }
  void Run() {
    Clock::time_point next_friends{},next_me{},next_recent{};
    bool first=true;
    for(;;) {
      {
        std::unique_lock lock(mutex_);
        if(!first) {
          const auto wait=gate->IsOpen()?std::chrono::seconds(5):std::chrono::seconds(20);
          changed_.wait_for(lock,wait,[&]{return stopping_||wake_;});
        }
        if(stopping_)return;
        wake_=false;
      }
      const bool open=gate->IsOpen();
      const auto now=Clock::now();
      json live,status,friends,me,recent;
      const bool got_live=Fetch("/api/motogp/live",live);
      const bool got_status=open?Fetch("/api/motogp/status",status):false;
      const bool got_friends=now>=next_friends&&Fetch("/api/account/friends_of?name="+Encode(snapshot_.me),friends);
      if(got_friends)next_friends=now+std::chrono::seconds(open?20:60);
      const bool got_me=now>=next_me&&Fetch("/api/motogp/player?name="+Encode(snapshot_.me),me);
      if(got_me)next_me=now+std::chrono::seconds(open?15:45);
      const bool got_recent=open&&now>=next_recent&&Fetch("/api/motogp/recent?limit=12",recent);
      if(got_recent)next_recent=now+std::chrono::seconds(15);
      std::lock_guard lock(mutex_);
      if(stopping_)return;
      Apply(live,got_live,status,got_status,friends,got_friends,me,got_me,recent,got_recent,first);
      first=false;
    }
  }
  void Apply(const json& live,bool got_live,const json& status,bool got_status,const json& friends,bool got_friends,
      const json& me,bool got_me,const json& recent,bool got_recent,bool first) {
    auto& s=snapshot_;
    s.failed=!got_live;
    if(got_friends&&friends.contains("friends")&&friends["friends"].is_array()) {
      std::vector<Friend> list;
      for(const auto& f:friends["friends"]) {
        if(!f.is_object())continue;
        Friend item{Clean(f,"username"),Clean(f,"country",2),{},f.value("online",false)?Where::Site:Where::Offline};
        if(!item.name.empty()&&list.size()<100)list.push_back(std::move(item));
      }
      // keep the MotoGP presence of the previous poll until live data merges below
      for(auto& f:list)for(const auto& old:s.friends)if(SameName(f.name,old.name)&&old.where>=Where::Menus){f.where=old.where;f.room=old.room;}
      s.friends=std::move(list);
    }
    if(got_live) {
      std::map<std::string,std::pair<Where,std::string>> where;   // lower-case name -> presence
      auto lower=[](std::string v){for(auto& c:v)if(c>='A'&&c<='Z')c+=32;return v;};
      if(live.contains("online")&&live["online"].is_array())
        for(const auto& o:live["online"])if(o.is_object())where[lower(Clean(o,"name"))]={Where::Menus,{}};
      std::vector<Room> rooms;
      if(live.contains("rooms")&&live["rooms"].is_array())for(const auto& r:live["rooms"]) {
        if(!r.is_object())continue;
        Room room{Clean(r,"name",31),Clean(r,"host_name"),0,int(Int(r,"capacity")),r.value("in_race",false),false,false};
        if(r.contains("members")&&r["members"].is_array())for(const auto& m:r["members"]) {
          if(!m.is_object())continue;
          ++room.members;
          const auto name=Clean(m,"name");
          where[lower(name)]={room.racing?Where::Racing:Where::Room,room.name};
          if(SameName(name,s.me))room.mine=true;
          for(const auto& f:s.friends)if(SameName(f.name,name))room.friends=true;
        }
        if(rooms.size()<40)rooms.push_back(std::move(room));
      }
      std::stable_sort(rooms.begin(),rooms.end(),[](const Room& a,const Room& b){
        if(a.mine!=b.mine)return a.mine;if(a.friends!=b.friends)return a.friends;return a.members>b.members;});
      for(auto& f:s.friends) {
        const auto before=f.where;const auto before_room=f.room;
        const auto it=where.find(lower(f.name));
        if(it!=where.end()){f.where=it->second.first;f.room=it->second.second;}
        else if(f.where>=Where::Menus){f.where=Where::Offline;f.room.clear();}
        if(first||!s.loaded)continue;
        if(before<Where::Menus&&f.where>=Where::Menus)
          Push(f.name+" · MotoGP",0);
        bool hosts=false;for(const auto& r:rooms)if(SameName(r.host,f.name)&&r.name==f.room)hosts=true;
        if(hosts&&f.where==Where::Room&&(before<Where::Room||before_room!=f.room))
          Push(f.name+" ▸ "+f.room,1);
      }
      std::stable_sort(s.friends.begin(),s.friends.end(),[](const Friend& a,const Friend& b){
        if(a.where!=b.where)return a.where>b.where;return a.name<b.name;});
      s.rooms=std::move(rooms);
      s.server_up=!live.value("stale",false);
      s.online=int(where.size());
    }
    if(got_status) {
      s.online=int(Int(status,"online"));s.open_rooms=int(Int(status,"rooms"));s.racing=int(Int(status,"in_race"));
      s.riders=int(Int(status,"racers"));s.records=int(Int(status,"records"));s.server_up=status.value("server_up",false);
    } else if(got_live) {
      s.open_rooms=int(s.rooms.size());s.racing=int(std::count_if(s.rooms.begin(),s.rooms.end(),[](const Room& r){return r.racing;}));
    }
    if(got_me&&me.contains("records")&&me["records"].is_array()) {
      std::map<int64_t,std::string> held;
      int podiums=0,boards=0;
      for(const auto& r:me["records"]) {
        if(!r.is_object())continue;
        ++boards;
        const auto rank=Int(r,"rank");
        if(rank>=1&&rank<=3)++podiums;
        if(rank==1)held[Int(r,"view")]=BoardName(r,tr_);
      }
      if(s.my_records>=0&&!first) {
        for(const auto& [view,name]:held)if(!held_.count(view))Push((tr_?"Yeni rekor: ":"New record: ")+name,1);
        for(const auto& [view,name]:held_)if(!held.count(view))Push((tr_?"Rekorun geçildi: ":"Your record was beaten: ")+name,2);
      }
      held_=std::move(held);
      s.my_records=int(held_.size());s.my_boards=boards;s.my_podiums=podiums;
    } else if(got_me) { s.my_records=0; }
    if(got_recent&&recent.contains("records")&&recent["records"].is_array()) {
      std::vector<Recent> list;
      for(const auto& r:recent["records"]) {
        if(!r.is_object())continue;
        Recent item{Clean(r,"name"),BoardName(r,tr_),{},int(Int(r,"rank")),Int(r,"updated"),false,false};
        if(r.contains("display")&&r["display"].is_object())item.time=Clean(r["display"],"text",16);
        item.mine=SameName(item.name,s.me);
        for(const auto& f:s.friends)if(SameName(f.name,item.name))item.friend_=true;
        list.push_back(std::move(item));
      }
      s.recent=std::move(list);
    }
    if(got_live)s.loaded=true;
    ++s.serial;
  }

 public:
  bool tr_=false;
 private:
  std::mutex mutex_;
  std::condition_variable changed_;
  bool stopping_=false,wake_=false;
  Snapshot snapshot_;
  std::deque<Event> toasts_;
  std::map<int64_t,std::string> held_;
  Http http_;
  std::thread worker_;
  Clock::time_point next_pad_{};
  std::optional<Clock::time_point> combo_since_;
  bool combo_used_=false;
};

std::atomic<std::shared_ptr<State>> current;
std::atomic<rex::input::InputSystem*> input_system{nullptr};

ImU32 Rgb(int r,int g,int b,int a=255){return IM_COL32(r,g,b,a);}
const char* WhereText(const Friend& f,bool tr) {
  switch(f.where) {
    case Where::Racing:return tr?"Yarışta":"Racing";
    case Where::Room:return tr?"Odada":"In a room";
    case Where::Menus:return tr?"MotoGP'de":"In MotoGP";
    case Where::Site:return tr?"FIFA 09'da":"In FIFA 09";  // friends_of: online in the FIFA 09 lobby
    default:return tr?"Çevrimdışı":"Offline";
  }
}
ImU32 WhereColor(Where w) {
  switch(w) {
    case Where::Racing:return Rgb(255,106,19);
    case Where::Room:case Where::Menus:return Rgb(34,197,94);
    case Where::Site:return Rgb(96,165,250);
    default:return Rgb(90,96,110);
  }
}

class Dialog final : public rex::ui::ImGuiDialog, public rex::ui::WindowInputListener {
 public:
  Dialog(rex::ui::ImGuiDrawer* drawer,rex::ui::Window* window,rex::system::AchievementManager* achievements,
      std::shared_ptr<State> state,ImFont* font,ImFont* heading,bool tr)
      :ImGuiDialog(drawer),window_(window),achievements_(achievements),state_(std::move(state)),font_(font),heading_(heading),tr_(tr) {
    state_->tr_=tr;
    if(window_)window_->AddInputListener(this,61);
  }
  ~Dialog() override {if(window_)window_->RemoveInputListener(this);state_->Close();}
  void OnKeyDown(rex::ui::KeyEvent& e) override {
    const auto bind=rex::ui::ParseVirtualKey(rex::cvar::Query<std::string>("bind_nobk_panel"));
    if(bind!=rex::ui::VirtualKey::kNone&&e.virtual_key()==bind&&!e.is_alt_pressed()&&!e.is_ctrl_pressed()) {
      if(!e.prev_state())state_->Toggle();
      e.set_handled(true);return;
    }
    if(state_->gate->IsOpen()&&!(e.is_alt_pressed()&&e.virtual_key()==rex::ui::VirtualKey::kF4))e.set_handled(true);
  }
  void OnKeyChar(rex::ui::KeyEvent& e) override {if(state_->gate->IsOpen())e.set_handled(true);}
  void OnMouseDown(rex::ui::MouseEvent& e) override {if(state_->gate->IsOpen())e.set_handled(true);}

 protected:
  void OnDraw(ImGuiIO& io) override {
    DrawToast(io);
    if(!state_->gate->IsOpen()){armed_=false;previous_=0;return;}
    const auto snapshot=state_->Read();
    if(epoch_!=state_->gate->Epoch()){epoch_=state_->gate->Epoch();armed_=false;previous_=0;selected_=0;ImGui::SetNextWindowFocus();}
    const auto buttons=state_->gate->Buttons();
    if(!armed_&&state_->gate->NeutralReady()&&!buttons)armed_=true;
    const uint16_t fresh=armed_?buttons&~previous_:0;
    previous_=buttons;
    using namespace rex::input;
    if(armed_) {
      if((fresh&X_INPUT_GAMEPAD_B)||ImGui::IsKeyPressed(ImGuiKey_Escape,false)){state_->Close();return;}
      if((fresh&X_INPUT_GAMEPAD_LEFT_SHOULDER)||ImGui::IsKeyPressed(ImGuiKey_Q,false)){tab_=(tab_+3)%4;selected_=0;}
      if((fresh&X_INPUT_GAMEPAD_RIGHT_SHOULDER)||ImGui::IsKeyPressed(ImGuiKey_E,false)){tab_=(tab_+1)%4;selected_=0;}
      if((fresh&X_INPUT_GAMEPAD_DPAD_DOWN)||ImGui::IsKeyPressed(ImGuiKey_DownArrow))++selected_;
      if((fresh&X_INPUT_GAMEPAD_DPAD_UP)||ImGui::IsKeyPressed(ImGuiKey_UpArrow))--selected_;
      activate_=(fresh&X_INPUT_GAMEPAD_A)||ImGui::IsKeyPressed(ImGuiKey_Enter,false);
      web_=(fresh&X_INPUT_GAMEPAD_Y)||ImGui::IsKeyPressed(ImGuiKey_W,false);
    } else activate_=web_=false;
    Draw(io,snapshot);
  }

 private:
  void DrawToast(ImGuiIO& io) {
    const double now=ImGui::GetTime();
    if(!toast_||now-toast_at_>6.0) {
      Event next;
      if(state_->PopToast(next)){toast_=next;toast_at_=now;}
      else toast_.reset();
    }
    if(!toast_||state_->gate->IsOpen())return;
    const float t=float(now-toast_at_);
    const float alpha=std::clamp(std::min(t/.25f,(6.f-t)/.4f),0.f,1.f);
    Theme theme(font_);
    const ImVec2 size(360,0);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x-24,24),ImGuiCond_Always,ImVec2(1,0));
    ImGui::SetNextWindowSize(size);
    ImGui::SetNextWindowBgAlpha(.94f*alpha);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha,alpha);
    if(ImGui::Begin("##nobk_toast",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoInputs|ImGuiWindowFlags_NoFocusOnAppearing|
        ImGuiWindowFlags_NoNav|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_AlwaysAutoResize)) {
      Brand(14.f);
      const ImVec4 color=toast_->kind==2?ImVec4(1,.45f,.4f,1):toast_->kind==1?orange:white;
      ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+312);
      ImGui::TextColored(color,"%s",toast_->text.c_str());
      ImGui::PopTextWrapPos();
      auto bind=rex::cvar::Query<std::string>("bind_nobk_panel");if(bind.empty())bind="F1";
      ImGui::TextDisabled("%s",(bind+(tr_?" · panel":" · panel")).c_str());
    }
    ImGui::End();
    ImGui::PopStyleVar();
  }
  void Brand(float size) {
    // NOBK·ONLINE wordmark like the site's top bar: caps, orange dot.
    auto* draw=ImGui::GetWindowDrawList();
    const auto pos=ImGui::GetCursorScreenPos();
    ImFont* f=heading_?heading_:ImGui::GetFont();
    const char* a="NOBK";const char* b="ONLINE";
    const auto wa=f->CalcTextSizeA(size,FLT_MAX,0,a).x;
    draw->AddText(f,size,pos,ImGui::GetColorU32(white),a);
    draw->AddCircleFilled(ImVec2(pos.x+wa+size*.32f,pos.y+size*.55f),size*.13f,ImGui::GetColorU32(orange));
    draw->AddText(f,size,ImVec2(pos.x+wa+size*.64f,pos.y),ImGui::GetColorU32(white),b);
    const auto wb=f->CalcTextSizeA(size,FLT_MAX,0,b).x;
    ImGui::Dummy(ImVec2(wa+size*.64f+wb,size*1.1f));
  }
  void Draw(ImGuiIO& io,const Snapshot& s) {
    Theme theme(font_);
    const ImVec2 size(std::min(920.f,io.DisplaySize.x*.92f),std::min(620.f,io.DisplaySize.y*.86f));
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x*.5f,io.DisplaySize.y*.5f),ImGuiCond_Always,ImVec2(.5f,.5f));
    ImGui::SetNextWindowSize(size,ImGuiCond_Always);
    if(ImGui::Begin("##nobk_panel",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoMove)) {
      Brand(26.f);
      ImGui::SameLine();
      const float right=ImGui::GetWindowContentRegionMax().x;
      const std::string who=s.me+"  ·  MotoGP 09/10";
      ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(),right-ImGui::CalcTextSize(who.c_str()).x));
      ImGui::SetCursorPosY(ImGui::GetCursorPosY()+6);
      ImGui::TextColored(muted,"%s",who.c_str());
      // tabs
      const char* tabs[4]={Tr(tr_,"FRIENDS","ARKADAŞLAR"),Tr(tr_,"SERVER","SUNUCU"),Tr(tr_,"RECORDS","REKORLAR"),Tr(tr_,"ACHIEVEMENTS","BAŞARIMLAR")};
      ImGui::Spacing();
      const float tab_w=(ImGui::GetContentRegionAvail().x-ImGui::GetStyle().ItemSpacing.x*3)/4;
      for(int i=0;i<4;++i) {
        if(i)ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button,i==tab_?AccentTint(.55f):card);
        ImGui::PushStyleColor(ImGuiCol_Text,i==tab_?white:muted);
        if(ImGui::Button(tabs[i],ImVec2(tab_w,38))){tab_=i;selected_=0;}
        ImGui::PopStyleColor(2);
      }
      ImGui::Spacing();
      const float footer=ImGui::GetTextLineHeightWithSpacing()+8;
      ImGui::BeginChild("##nobk_body",ImVec2(0,-footer),ImGuiChildFlags_None);
      if(!s.available)ImGui::TextWrapped("%s",Tr(tr_,"The NOBK panel works in Online mode on the NOBK Online server.","NOBK paneli NOBK Online sunucusunda Online modda çalışır."));
      else if(!s.loaded)ImGui::TextDisabled("%s",s.failed?Tr(tr_,"Cannot reach nobkonline.com right now.","nobkonline.com'a şu an ulaşılamıyor."):Tr(tr_,"Loading…","Yükleniyor…"));
      else if(tab_==0)Friends(s);
      else if(tab_==1)Server(s);
      else if(tab_==2)Records(s);
      else Achievements();
      ImGui::EndChild();
      auto bind=rex::cvar::Query<std::string>("bind_nobk_panel");if(bind.empty())bind="F1";
      ImGui::TextDisabled("%s",(std::string(Tr(tr_,"LB/RB · Q/E tabs   A · Enter select   Y · W website   B · Esc · ","LB/RB · Q/E sekme   A · Enter seç   Y · W site   B · Esc · "))+bind+Tr(tr_," close"," kapat")).c_str());
      if(web_)OpenUrl(tab_==0?"https://nobkonline.com/hesabim/":tab_==2?"https://nobkonline.com/motogp/records/":"https://nobkonline.com/motogp/");
    }
    ImGui::End();
  }
  bool Row(int index,float height) {
    const bool focus=index==selected_;
    const auto pos=ImGui::GetCursorScreenPos();
    const float width=ImGui::GetContentRegionAvail().x;
    ImGui::PushID(index);
    const bool clicked=ImGui::InvisibleButton("##row",ImVec2(width,height));
    const bool hovered=ImGui::IsItemHovered();
    ImGui::PopID();
    if(hovered&&ImGui::GetIO().MouseDelta.x+ImGui::GetIO().MouseDelta.y!=0)selected_=index;
    auto* draw=ImGui::GetWindowDrawList();
    draw->AddRectFilled(pos,ImVec2(pos.x+width,pos.y+height),ImGui::GetColorU32(focus?AccentTint(.18f):card),8.f);
    if(focus){draw->AddRectFilled(pos,ImVec2(pos.x+4,pos.y+height),ImGui::GetColorU32(orange),2.f);ImGui::SetScrollHereY(.5f);}
    ImGui::SetCursorScreenPos(pos);
    return clicked||(focus&&activate_);
  }
  void Initial(ImVec2 pos,float size,const std::string& name,ImU32 ring) {
    auto* draw=ImGui::GetWindowDrawList();
    const ImVec2 c(pos.x+size*.5f,pos.y+size*.5f);
    draw->AddCircleFilled(c,size*.5f,Rgb(255,106,19));
    draw->AddCircle(c,size*.5f+2,ring,0,2.f);
    char letter[2]={name.empty()?'?':char(std::toupper(static_cast<unsigned char>(name[0]))),0};
    ImFont* f=heading_?heading_:ImGui::GetFont();
    const auto ts=f->CalcTextSizeA(size*.55f,FLT_MAX,0,letter);
    draw->AddText(f,size*.55f,ImVec2(c.x-ts.x*.5f,c.y-ts.y*.5f),Rgb(255,255,255),letter);
  }
  void Friends(const Snapshot& s) {
    if(s.friends.empty()) {
      ImGui::TextWrapped("%s",Tr(tr_,"No friends yet. Add friends on nobkonline.com (your profile or My account); they show up here with what they are doing in MotoGP.",
          "Henüz arkadaşın yok. nobkonline.com'da (profil ya da Hesabım) arkadaş ekle; burada MotoGP'de ne yaptıklarıyla görünürler."));
      return;
    }
    const int in_game=int(std::count_if(s.friends.begin(),s.friends.end(),[](const Friend& f){return f.where>=Where::Menus;}));
    ImGui::TextColored(muted,tr_?"%d arkadaş · %d tanesi MotoGP'de":"%d friends · %d in MotoGP",int(s.friends.size()),in_game);
    selected_=std::clamp(selected_,0,int(s.friends.size())-1);
    for(int i=0;i<int(s.friends.size());++i) {
      const auto& f=s.friends[i];
      const float h=54;
      const auto pos=ImGui::GetCursorScreenPos();
      if(Row(i,h))OpenUrl("https://nobkonline.com/profile/?u="+Encode(f.name));
      Initial(ImVec2(pos.x+14,pos.y+9),36,f.name,WhereColor(f.where));
      auto* draw=ImGui::GetWindowDrawList();
      draw->AddText(ImVec2(pos.x+62,pos.y+8),ImGui::GetColorU32(white),f.name.c_str());
      std::string line=WhereText(f,tr_);
      if(!f.room.empty())line+=" · "+f.room;
      draw->AddText(ImVec2(pos.x+62,pos.y+29),WhereColor(f.where),line.c_str());
      if(!f.country.empty()) {
        const auto w=ImGui::CalcTextSize(f.country.c_str()).x;
        draw->AddText(ImVec2(pos.x+ImGui::GetContentRegionAvail().x-w-16,pos.y+18),ImGui::GetColorU32(muted),f.country.c_str());
      }
      ImGui::Dummy(ImVec2(0,h));
    }
  }
  void Stat(const char* label,int value,float width) {
    ImGui::BeginGroup();
    const auto pos=ImGui::GetCursorScreenPos();
    auto* draw=ImGui::GetWindowDrawList();
    draw->AddRectFilled(pos,ImVec2(pos.x+width,pos.y+74),ImGui::GetColorU32(card),10.f);
    draw->AddRectFilled(pos,ImVec2(pos.x+3,pos.y+74),ImGui::GetColorU32(orange),2.f);
    ImFont* f=heading_?heading_:ImGui::GetFont();
    draw->AddText(f,30.f,ImVec2(pos.x+14,pos.y+8),ImGui::GetColorU32(white),std::to_string(value).c_str());
    draw->AddText(ImVec2(pos.x+14,pos.y+46),ImGui::GetColorU32(muted),label);
    ImGui::Dummy(ImVec2(width,74));
    ImGui::EndGroup();
  }
  void Server(const Snapshot& s) {
    const float gap=ImGui::GetStyle().ItemSpacing.x;
    const float w=(ImGui::GetContentRegionAvail().x-gap*4)/5;
    Stat(Tr(tr_,"Online","Çevrimiçi"),s.online,w);ImGui::SameLine();
    Stat(Tr(tr_,"Open rooms","Açık oda"),s.open_rooms,w);ImGui::SameLine();
    Stat(Tr(tr_,"Racing","Yarışta"),s.racing,w);ImGui::SameLine();
    Stat(Tr(tr_,"Riders","Pilot"),s.riders,w);ImGui::SameLine();
    Stat(Tr(tr_,"Records","Rekor"),s.records,w);
    ImGui::Spacing();
    ImGui::TextColored(s.server_up?ImVec4(.13f,.77f,.37f,1):ImVec4(1,.45f,.4f,1),"%s",
        s.server_up?Tr(tr_,"● Server online","● Sunucu açık"):Tr(tr_,"● Server not answering","● Sunucu cevap vermiyor"));
    ImGui::Spacing();
    if(s.rooms.empty()) {
      ImGui::TextWrapped("%s",Tr(tr_,"No room is open right now. Host one from Player Match; it shows up here and on nobkonline.com/motogp.",
          "Şu an açık oda yok. Player Match'ten bir oda kur; burada ve nobkonline.com/motogp'de görünür."));
      return;
    }
    selected_=std::clamp(selected_,0,int(s.rooms.size())-1);
    for(int i=0;i<int(s.rooms.size());++i) {
      const auto& r=s.rooms[i];
      const auto pos=ImGui::GetCursorScreenPos();
      Row(i,50);
      auto* draw=ImGui::GetWindowDrawList();
      draw->AddText(ImVec2(pos.x+16,pos.y+6),ImGui::GetColorU32(r.mine||r.friends?orange:white),r.name.c_str());
      std::string line=std::string(Tr(tr_,"Host: ","Kurucu: "))+r.host+(r.friends?Tr(tr_,"  · friends inside","  · arkadaşların var"):"");
      draw->AddText(ImVec2(pos.x+16,pos.y+27),ImGui::GetColorU32(muted),line.c_str());
      const std::string count=std::to_string(r.members)+"/"+std::to_string(r.capacity)+"   "+(r.racing?Tr(tr_,"RACING","YARIŞTA"):Tr(tr_,"lobby","lobide"));
      const auto cw=ImGui::CalcTextSize(count.c_str()).x;
      draw->AddText(ImVec2(pos.x+ImGui::GetContentRegionAvail().x-cw-16,pos.y+16),ImGui::GetColorU32(r.racing?orange:muted),count.c_str());
      ImGui::Dummy(ImVec2(0,50));
    }
  }
  void Records(const Snapshot& s) {
    if(s.my_records>=0)ImGui::TextColored(orange,tr_?"Elindeki rekor: %d   ·   Podyum: %d   ·   Tablo: %d":"Records held: %d   ·   Podiums: %d   ·   Boards: %d",
        s.my_records,s.my_podiums,s.my_boards);
    if(!s.events.empty()) {
      ImGui::Spacing();ImGui::TextColored(muted,"%s",Tr(tr_,"THIS SESSION","BU OTURUM"));
      for(const auto& e:s.events)
        ImGui::TextColored(e.kind==2?ImVec4(1,.45f,.4f,1):e.kind==1?orange:white,"%s  ·  %s",e.text.c_str(),Ago(e.at,tr_).c_str());
    }
    ImGui::Spacing();ImGui::TextColored(muted,"%s",Tr(tr_,"LATEST RECORDS","SON REKORLAR"));
    if(s.recent.empty()){ImGui::TextDisabled("%s",Tr(tr_,"Loading…","Yükleniyor…"));return;}
    selected_=std::clamp(selected_,0,int(s.recent.size())-1);
    for(int i=0;i<int(s.recent.size());++i) {
      const auto& r=s.recent[i];
      const auto pos=ImGui::GetCursorScreenPos();
      Row(i,50);
      auto* draw=ImGui::GetWindowDrawList();
      const std::string who=r.name+(r.rank==1?Tr(tr_,"  took the record","  rekoru aldı"):(std::string("  P")+std::to_string(r.rank)));
      draw->AddText(ImVec2(pos.x+16,pos.y+6),ImGui::GetColorU32(r.mine||r.friend_?orange:white),who.c_str());
      draw->AddText(ImVec2(pos.x+16,pos.y+27),ImGui::GetColorU32(muted),(r.board+"  ·  "+Ago(r.at,tr_)).c_str());
      ImFont* f=heading_?heading_:ImGui::GetFont();
      const auto tw=f->CalcTextSizeA(24.f,FLT_MAX,0,r.time.c_str()).x;
      draw->AddText(f,24.f,ImVec2(pos.x+ImGui::GetContentRegionAvail().x-tw-16,pos.y+12),ImGui::GetColorU32(r.rank==1?orange:white),r.time.c_str());
      ImGui::Dummy(ImVec2(0,50));
    }
  }
  void Achievements() {
    if(!achievements_){ImGui::TextDisabled("%s",Tr(tr_,"Achievements are not available.","Başarımlar kullanılamıyor."));return;}
    const auto p=motogp::achievements::Progress(*achievements_);
    ImFont* f=heading_?heading_:ImGui::GetFont();
    ImGui::PushFont(f,30.f);
    ImGui::Text("%zu / %zu",p.earned,p.total);
    ImGui::PopFont();
    ImGui::TextColored(muted,tr_?"Gamerscore %llu / %llu":"Gamerscore %llu / %llu",
        static_cast<unsigned long long>(p.score),static_cast<unsigned long long>(p.available_score));
    ImGui::ProgressBar(p.total?float(p.earned)/float(p.total):0.f,ImVec2(-1,14),"");
    ImGui::Spacing();
    auto key=rex::cvar::Query<std::string>("bind_achievements");if(key.empty())key="F7";
    ImGui::TextWrapped(tr_?"Bütün başarımlar ve ilerleme için oyun içinde %s tuşuna bas.":"Press %s in game for every achievement and its progress.",key.c_str());
  }

  rex::ui::Window* window_;
  rex::system::AchievementManager* achievements_;
  std::shared_ptr<State> state_;
  ImFont* font_;ImFont* heading_;
  bool tr_;
  bool armed_=false,activate_=false,web_=false;
  uint16_t previous_=0;
  uint64_t epoch_=0;
  int tab_=0,selected_=0;
  std::optional<Event> toast_;
  double toast_at_=0;
};
}  // namespace

void Initialize(rex::Runtime* runtime,std::shared_ptr<settings::InputGate> gate) {
  input_system.store(runtime?static_cast<rex::input::InputSystem*>(runtime->input_system()):nullptr,std::memory_order_release);
  current.store(std::make_shared<State>(std::move(gate)),std::memory_order_release);
}
void Shutdown() {
  input_system.store(nullptr,std::memory_order_release);
  if(auto state=current.exchange({},std::memory_order_acq_rel))state->Stop();
}
bool Active() {
  const auto state=current.load(std::memory_order_acquire);
  return state&&state->gate->Blocking();
}
void PollPad() {
  if(auto state=current.load(std::memory_order_acquire))state->PollPad(input_system.load(std::memory_order_acquire));
}
std::unique_ptr<rex::ui::ImGuiDialog> CreateMenu(rex::ui::ImGuiDrawer* drawer,rex::ui::Window* window,
    rex::system::AchievementManager* achievements,ImFont* font,ImFont* heading,bool turkish) {
  auto state=current.load(std::memory_order_acquire);
  return state?std::make_unique<Dialog>(drawer,window,achievements,std::move(state),font,heading,turkish):nullptr;
}
}  // namespace motogp::nobk
