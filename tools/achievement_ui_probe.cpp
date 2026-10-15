// Windows executable: actual pinned runtime ImGui and real achievement storage.
#include "../src/achievement_ui.h"
#include "../src/audio_settings.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <rex/ui/imgui_drawer.h>
#include <rex/ui/window.h>
#include <rex/ui/windowed_app_context.h>
#include <rex/ui/keybinds.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace motogp::achievements;
using rex::system::AchievementInfo;
using rex::system::AchievementEvent;
namespace {
class TestContext final:public rex::ui::WindowedAppContext {
 protected: void NotifyUILoopOfPendingFunctions() override {} void PlatformQuitFromUIThread() override {}
};
int checks=0;
void Check(bool ok,const char* message) {if(!ok) throw std::runtime_error(message);++checks;}
void Dump(const std::filesystem::path& stem) {
  unsigned char* pixels;int w,h;ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
  std::ofstream texture(stem.string()+".rgba",std::ios::binary);texture.write(reinterpret_cast<char*>(pixels),w*h*4);
  auto* data=ImGui::GetDrawData();std::ofstream out(stem.string()+".json");
  out<<"{\"size\":["<<data->DisplaySize.x<<','<<data->DisplaySize.y<<"],\"texture\":["<<w<<','<<h<<"],\"lists\":[";
  for(int l=0;l<data->CmdListsCount;++l) {
    if(l)out<<',';const auto* list=data->CmdLists[l];out<<"{\"v\":[";
    for(int i=0;i<list->VtxBuffer.Size;++i) {if(i)out<<',';auto& v=list->VtxBuffer[i];out<<'['<<v.pos.x<<','<<v.pos.y<<','<<v.uv.x<<','<<v.uv.y<<','<<v.col<<']';}
    out<<"],\"i\":[";for(int i=0;i<list->IdxBuffer.Size;++i) {if(i)out<<',';out<<list->IdxBuffer[i];}
    out<<"],\"cmd\":[";
    for(int i=0;i<list->CmdBuffer.Size;++i) {if(i)out<<',';auto& c=list->CmdBuffer[i];out<<'['<<c.ClipRect.x<<','<<c.ClipRect.y<<','<<c.ClipRect.z<<','<<c.ClipRect.w<<','<<c.IdxOffset<<','<<c.ElemCount<<','<<c.VtxOffset<<']';}
    out<<"]}";
  }out<<"]}";
}
}
int main(int argc,char** argv) {try {
  Check(argc==3,"Usage: achievement_ui_probe font.ttf output-directory");
  const std::filesystem::path out=argv[2];std::filesystem::create_directories(out);
  Preferences preferences;preferences.path=out/"fixture-preferences.toml";preferences.sound=false;preferences.volume=38;
  Check(preferences.Save(),"Preferences save atomically");Preferences loaded;loaded.Load(out,out);
  // The production filename is separate from the test fixture; load explicitly from an isolated root.
  std::filesystem::copy_file(preferences.path,out/"achievement-ui.toml",std::filesystem::copy_options::overwrite_existing);
  loaded.Load(out,out);Check(!loaded.sound && loaded.volume==38,"Sound preferences survive reload");
  {std::ofstream t(out/"achievement-ui.toml");t<<"sound_enabled = true\nsound_volume = 1000\n";}
  loaded.Load(out,out);Check(loaded.volume==100,"Invalid volume clamps safely");
  {std::ofstream t(out/"motogp0910-launcher.toml");t<<"language = 'tr'\n";}
  loaded.Load(out,out);Check(loaded.turkish,"Launcher language reused");
  { // Two independent UI processes edit different fields from a stale baseline.
    const auto file=out/"shared-preferences.toml";
    {std::ofstream f(file);f<<"future_setting='preserve'\nsound_volume=25\n";}
    const auto old=ReadOptions(file);auto launcher=old,overlay=old;launcher.popup=false;overlay.volume=13;
    Check(WriteOptions(file,old,launcher)&&WriteOptions(file,old,overlay),"Launcher and overlay merge independent preference changes");
    auto merged=ReadOptions(file);Check(!merged.popup&&merged.sound&&merged.volume==13,"Stale writer cannot replace other UI's preference changes");
    Check(toml::parse_file(file.string())["future_setting"].value_or(std::string{})=="preserve","Unknown preference fields survive");
    Preferences live;live.Load(out,out);auto changed=ReadOptions(live.path),next=changed;next.popup=false;next.sound=false;next.volume=9;
    Check(WriteOptions(live.path,changed,next),"External launcher change saved");live.Refresh();
    Check(!live.popup&&!live.sound&&live.volume==9,"Running overlay reloads launcher notification and sound changes");
    {std::ofstream f(file);f<<"[broken";}Check(!WriteOptions(file,old,launcher),"Corrupt preferences are never silently overwritten");
  }
  rex::system::AchievementManager manager;manager.SetUnlockSavePath(out/"fixture-unlocks.toml");
  std::vector<AchievementInfo> catalog;
  for(int i=0;i<18;++i) {AchievementInfo info;info.id=i+1;info.gamerscore=10+i*5;info.label=i%2?"Clean racing":"First victory";info.description="Keep pushing. Complete the challenge and make your mark on the circuit.";info.unachieved_description="Complete the race challenge to unlock this achievement.";catalog.push_back(info);}
  catalog[2].label="A very long achievement title that wraps clearly without touching the score or spilling outside the card";
  catalog[3].label="Şampiyonluğa doğru: güçlü bir başlangıç";
  manager.RegisterAchievements(catalog);manager.UnlockAchievement(1);manager.UnlockAchievement(3);
  const auto before=Progress(manager);Check(before.earned==2 && before.total==18 && before.score==30,"Actual manager progress and gamerscore");
  MenuState state;state.filter=1;Check(Matches(catalog[0],true,state)&&!Matches(catalog[1],false,state),"Unlocked filter");
  state.filter=2;Check(!Matches(catalog[0],true,state)&&Matches(catalog[1],false,state),"Locked filter");
  state.filter=0;std::memcpy(state.search,"VICTORY",8);Check(Matches(catalog[0],true,state)&&!Matches(catalog[1],false,state),"Case-insensitive search");
  ToastQueue queue;AchievementEvent first{catalog[0],100},second{catalog[1],200};
  std::vector<std::thread> producers;for(int i=0;i<8;++i) producers.emplace_back([&]{for(int j=0;j<100;++j)queue.Push(first);});for(auto& thread:producers) thread.join();
  Check(!queue.Next(0,false),"Background notifications wait without sound");auto current=queue.Next(10,true);Check(current && current->play_sound && current->event.achievement.id==1,"First visible notification sounds exactly once");
  current=queue.Next(10.2,true);Check(current && !current->play_sound && current->alpha>0,"Next frame does not repeat sound");queue.Push(first);queue.Push(second);
  current=queue.Next(14.6,true);Check(current && current->play_sound && current->event.achievement.id==2,"Duplicates suppressed and queue advances");
  Check(!queue.Next(19.2,true),"Toasts expire without stale duplicates");
  ToastQueue bounded;for(int i=0;i<1000;++i) {auto e=first;e.achievement.id=i;bounded.Push(e);}for(int i=0;i<64;++i) Check(bounded.Next(i*5,true).has_value(),"Bounded queue retains first 64 events");Check(!bounded.Next(320,true),"Burst queue is bounded");
  preferences.sound=true;preferences.volume=25;rex::cvar::SetFlagByName("audio_master_volume","100");rex::cvar::SetFlagByName("audio_mute","false");Check(std::abs(CueGain(preferences)-.25f)<.001f,"Soft default cue gain");
  auto samples=CueSamples(CueGain(preferences));int peak=0;for(auto v:samples) peak=std::max(peak,std::abs(int(v)));Check(samples.size()==21600 && samples.front()==0 && std::abs(int(samples.back()))<2 && peak<2100,"450ms cue has soft level and no edge clicks");
  std::ofstream wav(out/"achievement-unlock-preview.wav",std::ios::binary);const int size=static_cast<int>(samples.size()*2),riff=size+36;const int fmt_size=16,rate=48000,byte_rate=96000;const int16_t pcm=1,channels=1,align=2,bits=16;
  wav.write("RIFF",4);wav.write(reinterpret_cast<const char*>(&riff),4);wav.write("WAVEfmt ",8);wav.write(reinterpret_cast<const char*>(&fmt_size),4);wav.write(reinterpret_cast<const char*>(&pcm),2);wav.write(reinterpret_cast<const char*>(&channels),2);wav.write(reinterpret_cast<const char*>(&rate),4);wav.write(reinterpret_cast<const char*>(&byte_rate),4);wav.write(reinterpret_cast<const char*>(&align),2);wav.write(reinterpret_cast<const char*>(&bits),2);wav.write("data",4);wav.write(reinterpret_cast<const char*>(&size),4);wav.write(reinterpret_cast<const char*>(samples.data()),size);wav.close();
  rex::cvar::SetFlagByName("audio_master_volume","40");Check(std::abs(CueGain(preferences)-.10f)<.001f,"Cue follows game master volume");
  rex::cvar::SetFlagByName("audio_mute","true");Check(CueGain(preferences)==0,"Game mute silences cue");rex::cvar::SetFlagByName("audio_mute","false");preferences.sound=false;Check(CueGain(preferences)==0,"User can disable cue");preferences.sound=true;
  {Sound sound(std::make_shared<Preferences>());sound.Play();}Check(true,"Windows sound worker shuts down with playback pending");
  auto* context=ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;
  static const ImWchar ranges[]={0x20,0x1ff,0};auto* font=io.Fonts->AddFontFromFileTTF(argv[1],17,nullptr,ranges);auto* heading=io.Fonts->AddFontFromFileTTF(argv[1],30,nullptr,ranges);Check(font!=nullptr && heading!=nullptr,"Packaged Open Sans fonts load");
  unsigned char* pixels;int tw,th;io.Fonts->GetTexDataAsRGBA32(&pixels,&tw,&th);io.Fonts->SetTexID(1);io.DeltaTime=1.f/60;
  preferences.path.clear();
  for(bool tr:{false,true})for(ImVec2 size:{ImVec2(1280,720),ImVec2(1920,1080),ImVec2(3840,2160),ImVec2(2560,1080),ImVec2(640,480)})for(int filter=0;filter<3;++filter) {
    preferences.turkish=tr;state={};state.filter=filter;io.DisplaySize=size;
    for(int frame=0;frame<2;++frame) {ImGui::NewFrame();Check(!DrawMenu(manager,state,preferences,font,nullptr,heading),"Render does not request spontaneous close");DrawToast({catalog[2],100},1,tr,font);ImGui::Render();}
    auto* window=ImGui::FindWindowByName("##motogp_achievements");auto* toast=ImGui::FindWindowByName("##motogp_achievement_toast");
    Check(window && window->Pos.x>=0 && window->Pos.y>=0 && window->Pos.x+window->Size.x<=size.x+1 && window->Pos.y+window->Size.y<=size.y+1,"Responsive menu remains within viewport");
    Check(toast && toast->Pos.x>=0 && toast->Pos.y>=0 && toast->Pos.x+toast->Size.x<=size.x && toast->Pos.y+toast->Size.y<=size.y,"Responsive notification remains within viewport");
    Check(toast->ContentSize.y<=toast->InnerRect.GetHeight()+1,"Long titles and gamerscore fit inside toast");
    Check(context->ColorStack.Size==0 && context->StyleVarStack.Size==0 && context->FontStack.Size==0,"No style or font stack leaks");
    if(size.x==1280 && filter==0) Dump(out/(tr?"achievements-turkish":"achievements-english"));
  }
  rex::system::AchievementManager empty;state={};io.DisplaySize=ImVec2(1280,720);ImGui::NewFrame();DrawMenu(empty,state,preferences,font);ImGui::Render();Check(ImGui::GetDrawData()->TotalVtxCount>0,"Empty catalog displays usable UI");
  Check(Progress(manager).earned==before.earned && Progress(manager).score==before.score,"Viewing UI leaves achievement progress untouched");
  rex::system::AchievementManager restored;restored.SetUnlockSavePath(out/"fixture-unlocks.toml");restored.RegisterAchievements(catalog);restored.LoadUnlockState();Check(Progress(restored).earned==2,"Existing unlock persistence still loads");
  ImGui::DestroyContext(context);
  {
    TestContext ctx;auto window=rex::ui::Window::Create(ctx,"Achievement lifecycle probe");
    rex::ui::ImGuiDrawer drawer(window.get(),100);
    auto& dialog_io=drawer.GetIO();dialog_io.DisplaySize=ImVec2(1280,720);dialog_io.DeltaTime=1.f/60;
    dialog_io.Fonts->GetTexDataAsRGBA32(&pixels,&tw,&th);dialog_io.Fonts->SetTexID(1);
    auto prefs=std::make_shared<Preferences>();std::unique_ptr<rex::ui::ImGuiDialog> menu;
    rex::ui::RegisterBind("bind_achievements","F7","Achievement probe",[&] {
      if(menu) menu.reset(); else menu=CreateMenu(&drawer,nullptr,nullptr,manager,nullptr,nullptr,prefs,[&](std::weak_ptr<void> token){RequestClose(ctx,window.get(),token);});
    });
    auto toggle=[&]{rex::ui::KeyEvent key(window.get(),rex::ui::ParseVirtualKey(rex::cvar::Query<std::string>("bind_achievements")),1,false,false,false,false,false);Check(rex::ui::ProcessKeyEvent(key),"Actual SDK achievement key dispatch");};
    auto frame=[&]{drawer.GetIO();ImGui::NewFrame();if(menu)menu->Draw();ImGui::Render();};
    toggle();Check(menu && drawer.HasDialogs(),"F7 creates and attaches custom menu");frame();frame();
    dialog_io.AddKeyEvent(ImGuiKey_Escape,true);frame();Check(menu!=nullptr,"Esc close remains alive until draw ends");
    ctx.ExecutePendingFunctionsFromUIThread();Check(!menu && !drawer.HasDialogs(),"Deferred Esc safely resets SDK-owned dialog");
    dialog_io.AddKeyEvent(ImGuiKey_Escape,false);frame();
    toggle();frame();frame();dialog_io.AddKeyEvent(ImGuiKey_Escape,true);frame();toggle();
    ctx.ExecutePendingFunctionsFromUIThread();Check(!menu,"Expired deferred close cannot reopen manually closed menu");
    rex::cvar::SetFlagByName("bind_achievements","F8");toggle();Check(menu!=nullptr,"Remapped overlay binding opens normally");toggle();Check(!menu,"Remapped binding closes normally");
    rex::ui::UnregisterBind("bind_achievements");
  }
  std::cout<<checks<<" achievement UI, storage, notification and sound checks passed\n";return 0;
} catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
