#include "achievement_ui.h"
#include "overlay_theme.h"
#include <imgui.h>
#include <rex/ui/immediate_drawer.h>
#include <rex/ui/overlay/achievement_icon_cache.h>
#include <rex/logging.h>
#include <rex/cvar.h>
#include <rex/ui/keybinds.h>
#include <rex/ui/windowed_app_context.h>
#include <toml++/toml.hpp>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <string_view>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace motogp::achievements {
namespace {
using namespace motogp::overlay;
void Trophy(ImVec2 pos,float size,ImU32 color) {
  auto* draw=ImGui::GetWindowDrawList();
  const auto p=[&](float x,float y){return ImVec2(pos.x+x*size,pos.y+y*size);};
  draw->AddRectFilled(p(.25f,.15f),p(.75f,.57f),color,size*.10f,ImDrawFlags_RoundCornersBottom);
  draw->AddRect(p(.10f,.22f),p(.9f,.44f),color,size*.10f,0,size*.065f);
  draw->AddRectFilled(p(.45f,.55f),p(.55f,.77f),color);
  draw->AddRectFilled(p(.28f,.78f),p(.72f,.85f),color,size*.03f);
}
void Icon(const rex::system::AchievementInfo& info,bool unlocked,float size,rex::ui::AchievementIconCache* icons) {
  auto* texture=icons?icons->GetIcon(info):nullptr;
  if(texture) ImGui::ImageWithBg(reinterpret_cast<ImTextureID>(texture),ImVec2(size,size),ImVec2(0,0),ImVec2(1,1),ImVec4(0,0,0,0), unlocked?white:muted);
  else {Trophy(ImGui::GetCursorScreenPos(),size,ImGui::GetColorU32(unlocked?orange:muted)); ImGui::Dummy(ImVec2(size,size));}
}
std::string Fold(std::string value) { for(auto& c:value) if(c>='A'&&c<='Z') c+=32; return value; }
}
void Preferences::Load(const std::filesystem::path& user_root,const std::filesystem::path& executable_root) {
  path=user_root/"achievement-ui.toml";
  try { if(std::filesystem::is_regular_file(path)) {auto t=toml::parse_file(path.string()); sound=t["sound_enabled"].value_or(true); volume=std::clamp(t["sound_volume"].value_or(25),0,100);} }
  catch(const std::exception& e) {REXLOG_WARN("Could not load achievement preferences: {}",e.what());}
  try {const auto file=executable_root/"motogp0910-launcher.toml"; if(std::filesystem::is_regular_file(file)) turkish=toml::parse_file(file.string())["language"].value_or(std::string("en"))=="tr";}
  catch(const std::exception&) {turkish=false;}
}
bool Preferences::Save() const {
  if(path.empty()) return true;
  try {
    std::filesystem::create_directories(path.parent_path());
    auto temp=path;temp+=".tmp";
    std::ofstream file(temp,std::ios::trunc); file<<"sound_enabled = "<<(sound.load()?"true":"false")<<"\nsound_volume = "<<volume.load()<<'\n'; file.close();
    if(!file) return false;
#ifdef _WIN32
    if(!MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) return false;
#else
    std::filesystem::rename(temp,path);
#endif
    return true;
  } catch(const std::exception& e) {REXLOG_WARN("Could not save achievement preferences: {}",e.what()); return false;}
}
Summary Progress(const rex::system::AchievementManager& manager) {
  Summary out;auto list=manager.ListAchievements();out.total=list.size();
  for(const auto& info:list) {out.available_score+=info.gamerscore; if(manager.IsUnlocked(info.id)) {++out.earned;out.score+=info.gamerscore;}}
  return out;
}
bool Matches(const rex::system::AchievementInfo& info,bool unlocked,const MenuState& state) {
  if((state.filter==1&&!unlocked)||(state.filter==2&&unlocked)) return false;
  const auto search=Fold(state.search);
  return search.empty() || Fold(info.label).find(search)!=std::string::npos || Fold(unlocked?info.description:info.unachieved_description.empty()?info.description:info.unachieved_description).find(search)!=std::string::npos;
}
bool DrawMenu(rex::system::AchievementManager& manager,MenuState& state,Preferences& preferences,ImFont* font,rex::ui::AchievementIconCache* icons,ImFont* heading) {
  Theme theme(font); const bool tr=preferences.turkish; const auto& io=ImGui::GetIO();
  ImVec2 size(std::min(900.f,std::max(280.f,io.DisplaySize.x-40)),std::min(680.f,std::max(260.f,io.DisplaySize.y-40)));
  ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x*.5f,io.DisplaySize.y*.5f),ImGuiCond_Always,ImVec2(.5f,.5f));
  ImGui::SetNextWindowSize(size,ImGuiCond_Always);
  bool close=false;
  if(ImGui::Begin("##motogp_achievements",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoMove)) {
    const auto summary=Progress(manager);
    ImGui::TextColored(orange,"MotoGP 09/10");
    if(heading) ImGui::PushFont(heading,30.f);
    ImGui::TextUnformatted(Tr(tr,"Achievements","Başarımlar"));
    if(heading) ImGui::PopFont();
    ImGui::TextDisabled("%s",Tr(tr,"Your milestones. Every race counts.","Her yarış, yeni bir hedef."));
    ImGui::Spacing();
    ImGui::Text("%zu / %zu %s",summary.earned,summary.total,Tr(tr,"unlocked","kazanıldı"));
    ImGui::SameLine(); ImGui::TextColored(orange,"  %llu / %llu G",static_cast<unsigned long long>(summary.score),static_cast<unsigned long long>(summary.available_score));
    ImGui::ProgressBar(summary.total?float(summary.earned)/summary.total:0,ImVec2(-1,5),"");
    const char* filters[]={Tr(tr,"All","Tümü"),Tr(tr,"Unlocked","Kazanılan"),Tr(tr,"Locked","Kilitli")};
    for(int i=0;i<3;++i) {if(i) ImGui::SameLine(); const bool selected=state.filter==i; if(selected) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(.32f,.23f,.11f,1));
      if(ImGui::Button(filters[i])) state.filter=i;
      if(selected) ImGui::PopStyleColor();
    }
    ImGui::SetNextItemWidth(-1); ImGui::InputTextWithHint("##achievement_search",Tr(tr,"Search achievements...","Başarımlarda ara..."),state.search,sizeof(state.search));
    ImGui::BeginChild("##achievement_list",ImVec2(0,-92),ImGuiChildFlags_None);
    size_t shown=0;
    for(const auto& info:manager.ListAchievements()) {
      const bool earned=manager.IsUnlocked(info.id); if(!Matches(info,earned,state)) continue; ++shown;
      ImGui::PushID(static_cast<int>(info.id));
      const auto& description=earned?info.description:info.unachieved_description.empty()?info.description:info.unachieved_description;
      const float available=ImGui::GetContentRegionAvail().x;
      const float text_width=std::max(80.f,available-130.f);
      const float title_height=ImGui::CalcTextSize(info.label.c_str(),nullptr,false,text_width).y;
      const float description_height=ImGui::CalcTextSize(description.c_str(),nullptr,false,text_width).y;
      const float row_height=std::max(96.f,title_height+description_height+46.f);
      ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,8.f); ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(14,12));
      ImGui::BeginChild("##row",ImVec2(0,row_height),ImGuiChildFlags_Borders|ImGuiChildFlags_AlwaysUseWindowPadding,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
      Icon(info,earned,48,icons);ImGui::SameLine();
      ImGui::BeginGroup(); ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+text_width);
      ImGui::TextUnformatted(info.label.c_str()); ImGui::TextColored(muted,"%s",description.c_str());
      ImGui::TextColored(earned?orange:muted,"%s  ·  %u G",Tr(tr,earned?"UNLOCKED":"LOCKED",earned?"KAZANILDI":"KİLİTLİ"),info.gamerscore);
      ImGui::PopTextWrapPos();ImGui::EndGroup(); ImGui::EndChild(); ImGui::PopStyleVar(2); ImGui::PopID();
    }
    if(!shown) {ImGui::Spacing(); ImGui::TextDisabled("%s",Tr(tr,summary.total?"No achievements match this filter.":"Achievement catalog is not available yet.",summary.total?"Bu filtreye uygun başarım bulunamadı.":"Başarım listesi henüz hazır değil."));}
    ImGui::EndChild();
    ImGui::Separator();
    bool sound=preferences.sound.load(); if(ImGui::Checkbox(Tr(tr,"Unlock sound","Başarım sesi"),&sound)) {preferences.sound=sound; preferences.Save();}
    ImGui::SameLine(); int volume=preferences.volume.load(); ImGui::SetNextItemWidth(std::max(90.f,std::min(170.f,size.x*.22f)));
    if(ImGui::SliderInt("##achievement_volume",&volume,0,100,"%d%%")) preferences.volume=volume;
    if(ImGui::IsItemDeactivatedAfterEdit()) preferences.Save();
    ImGui::TextDisabled("%s",Tr(tr,"F7 / Esc to return to the game","Oyuna dönmek için F7 / Esc"));
    ImGui::SameLine(std::max(0.f,size.x-136.f)); close=ImGui::Button(Tr(tr,"Close","Kapat"),ImVec2(88,0));
    if(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)&&ImGui::IsKeyPressed(ImGuiKey_Escape,false)) close=true;
  }
  ImGui::End(); return close;
}
void DrawToast(const rex::system::AchievementEvent& event,float alpha,bool tr,ImFont* font,rex::ui::AchievementIconCache* icons) {
  Theme theme(font); const auto& io=ImGui::GetIO(); const float width=std::min(410.f,std::max(240.f,io.DisplaySize.x-32.f));
  const float label_width=width-116;
  const float height=std::max(114.f,104.f+ImGui::CalcTextSize(event.achievement.label.c_str(),nullptr,false,label_width).y);
  ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x-20,io.DisplaySize.y-20),ImGuiCond_Always,ImVec2(1,1));
  ImGui::SetNextWindowSize(ImVec2(width,height),ImGuiCond_Always); ImGui::PushStyleVar(ImGuiStyleVar_Alpha,alpha);
  if(ImGui::Begin("##motogp_achievement_toast",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoNav|ImGuiWindowFlags_NoInputs|ImGuiWindowFlags_NoSavedSettings)) {
    const auto pos=ImGui::GetWindowPos();auto* draw=ImGui::GetWindowDrawList();draw->AddRectFilled(ImVec2(pos.x,pos.y+14),ImVec2(pos.x+4,pos.y+height-14),ImGui::GetColorU32(orange),2);
    Icon(event.achievement,true,48,icons);ImGui::SameLine(); ImGui::BeginGroup();
    ImGui::TextColored(orange,"%s",Tr(tr,"ACHIEVEMENT UNLOCKED","BAŞARIM KAZANILDI"));
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+label_width); ImGui::TextUnformatted(event.achievement.label.c_str()); ImGui::PopTextWrapPos();
    ImGui::TextDisabled("+%u G",event.achievement.gamerscore);ImGui::EndGroup();
  }
  ImGui::End();ImGui::PopStyleVar();
}
void ToastQueue::Push(const rex::system::AchievementEvent& event) {
  std::lock_guard lock(mutex_);
  const auto same=[&](const auto& old){return old.achievement.id==event.achievement.id&&old.unlocked_at_filetime==event.unlocked_at_filetime;};
  if(std::any_of(seen_.begin(),seen_.end(),same)||pending_.size()>=64) return;
  seen_.push_back(event);if(seen_.size()>128) seen_.pop_front();pending_.push_back(event);
}
std::optional<VisibleToast> ToastQueue::Next(double now,bool focused) {
  std::lock_guard lock(mutex_);bool fresh=false;
  if(visible_&&now-started_>=4.5) visible_.reset();
  if(!focused) return std::nullopt;
  if(!visible_&&!pending_.empty()) {visible_=std::move(pending_.front());pending_.pop_front();started_=now;fresh=true;}
  if(!visible_) return std::nullopt;
  const double age=std::max(0.0,now-started_);
  const float alpha=static_cast<float>(std::clamp(std::min(age/.25,(4.5-age)/.5),0.0,1.0));
  return VisibleToast{*visible_,alpha,fresh};
}
namespace {
class Menu final:public rex::ui::ImGuiDialog {
 public:
  Menu(rex::ui::ImGuiDrawer* d,rex::ui::ImmediateDrawer* immediate,rex::Runtime* runtime,rex::system::AchievementManager& manager,ImFont* font,ImFont* heading,std::shared_ptr<Preferences> p,CloseRequest close)
      :ImGuiDialog(d),manager_(manager),font_(font),heading_(heading),prefs_(std::move(p)),close_(std::move(close)),icons_(immediate,runtime) {}
 protected:
  void OnDraw(ImGuiIO&) override {if(DrawMenu(manager_,state_,*prefs_,font_,&icons_,heading_)&&!closing_) {closing_=true;close_(lifetime_);}}
 private:
  rex::system::AchievementManager& manager_; MenuState state_; ImFont* font_; ImFont* heading_; std::shared_ptr<Preferences> prefs_; CloseRequest close_;
  rex::ui::AchievementIconCache icons_; std::shared_ptr<void> lifetime_=std::make_shared<int>(0);bool closing_=false;
};
class Notifications final:public rex::ui::AchievementNotificationDialog {
 public:
  Notifications(rex::ui::ImGuiDrawer* d,rex::ui::ImmediateDrawer* immediate,rex::Runtime* runtime,ImFont* font,std::shared_ptr<Preferences> p)
    :AchievementNotificationDialog(d),font_(font),prefs_(std::move(p)),icons_(immediate,runtime),sound_(prefs_) {}
  void Push(const rex::system::AchievementEvent& event) override {queue_.Push(event);}
 protected:
  void OnDraw(ImGuiIO& io) override {
    const double now=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    if(const auto toast=queue_.Next(now,!io.AppFocusLost)) {if(toast->play_sound) sound_.Play();DrawToast(toast->event,toast->alpha,prefs_->turkish,font_,&icons_);}
  }
 private:
  ToastQueue queue_;ImFont* font_;std::shared_ptr<Preferences> prefs_;rex::ui::AchievementIconCache icons_;Sound sound_;
};
}
void RequestClose(rex::ui::WindowedAppContext& context,rex::ui::Window* window,std::weak_ptr<void> lifetime) {
  context.CallInUIThreadDeferred([window,lifetime] {
    if(lifetime.expired()) return;
    const auto key=rex::ui::ParseVirtualKey(rex::cvar::Query<std::string>("bind_achievements"));
    rex::ui::KeyEvent event(window,key,1,false,false,false,false,false);
    rex::ui::ProcessKeyEvent(event);
  });
}
std::unique_ptr<rex::ui::ImGuiDialog> CreateMenu(rex::ui::ImGuiDrawer* drawer,rex::ui::ImmediateDrawer* immediate,rex::Runtime* runtime,rex::system::AchievementManager& manager,ImFont* font,ImFont* heading,std::shared_ptr<Preferences> p,CloseRequest close) {
  return std::make_unique<Menu>(drawer,immediate,runtime,manager,font,heading,std::move(p),std::move(close));
}
std::unique_ptr<rex::ui::AchievementNotificationDialog> CreateNotifications(rex::ui::ImGuiDrawer* drawer,rex::ui::ImmediateDrawer* immediate,rex::Runtime* runtime,ImFont* font,std::shared_ptr<Preferences> p) {
  return std::make_unique<Notifications>(drawer,immediate,runtime,font,std::move(p));
}
}
