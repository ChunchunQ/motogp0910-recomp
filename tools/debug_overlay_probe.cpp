#include "../src/debug_overlay.h"
#include "../src/frame_pacing.h"
#include <thread>
#include <chrono>
#include "../src/diagnostic_options.h"
#include <rex/perf/counter.h>
#include <rex/ui/keybinds.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>
#include <stdexcept>
#include <cmath>
namespace {int checks=0;void Check(bool value,const char* name){if(!value)throw std::runtime_error(name);++checks;}}
int main(int argc,char** argv){try{
 Check(argc==2,"Supply the packaged font");Check(!motogp::debug::Enabled(),"Overlay permission defaults off");
 rex::cvar::SetFlagByName("debug_overlay_enabled","true");Check(motogp::debug::Enabled(),"Actual startup flag permits the overlay");
 int legacy=0,toggles=0;rex::ui::RegisterBind("bind_debug_overlay","F3","Old SDK overlay",[&]{++legacy;});motogp::debug::RegisterToggle([&]{++toggles;});
 auto press=[&]{rex::ui::KeyEvent e(nullptr,rex::ui::VirtualKey::kF3,1,false,false,false,false,false);return rex::ui::ProcessKeyEvent(e);};
 rex::cvar::SetFlagByName("debug_overlay_enabled","false");Check(press()&&toggles==0&&legacy==0,"Real F3 dispatch cannot open either overlay when disabled");
 rex::cvar::SetFlagByName("debug_overlay_enabled","true");Check(press()&&toggles==1&&legacy==0,"Enabled F3 dispatch uses only the project overlay");
 motogp::debug::UnregisterToggle();Check(!press()&&toggles==1,"Shutdown removes the project callback completely");
 motogp::debug::Initialize("VulkanGraphicsSystem");Check(motogp::debug::GuestFrameTimeMs()==0,"Timing waits for actual frame-boundary observations");
 motogp::PaceGameFrame();std::this_thread::sleep_for(std::chrono::milliseconds(2));motogp::PaceGameFrame();Check(motogp::debug::GuestFrameTimeMs()>0&&std::isfinite(motogp::debug::GuestFrameTimeMs()),"Guest timing comes from the real pacing hook rather than the UI frame rate");
 auto* ctx=ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;
 static const ImWchar ranges[]={32,511,0};Check(io.Fonts->AddFontFromFileTTF(argv[1],15,nullptr,ranges)!=nullptr,"Packaged Turkish-capable font loads");
 unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);io.Fonts->SetTexID(1);io.DeltaTime=1.f/60;
 using C=rex::perf::CounterId;rex::perf::Init();rex::perf::SetCounter(C::kFrameTimeUs,16667);rex::perf::SetCounter(C::kDrawCalls,654);rex::perf::SetCounter(C::kAudioFrameLatencyUs,14000);rex::perf::ResetFrameCounters();
 for(bool tr:{false,true})for(auto size:{ImVec2(1040,732),ImVec2(1280,720),ImVec2(1920,1080),ImVec2(2560,1080)}){
  io.DisplaySize=size;for(int i=0;i<3;++i){ImGui::NewFrame();motogp::debug::Draw(nullptr,tr);ImGui::Render();}
  Check(ImGui::GetDrawData()->TotalVtxCount>0,"Production diagnostics emits actual ImGui geometry");
  for(const char* name:tr?std::initializer_list<const char*>{"Performans##pc_debug","Görüntü##pc_display","Ses ve önbellek##pc_audio"}:std::initializer_list<const char*>{"Performance##pc_debug","Display##pc_display","Audio and caches##pc_audio"}){
   auto* win=ImGui::FindWindowByName(name);Check(win&&win->Pos.x>=0&&win->Pos.y>=0&&win->Pos.x+win->Size.x<=size.x+1&&win->Pos.y+win->Size.y<=size.y+1,"Diagnostic panels fit the real output viewport");Check((win->Flags&ImGuiWindowFlags_NoInputs)!=0,"Diagnostics never steals gameplay input");
  }
  Check(!ctx->ColorStack.Size&&!ctx->StyleVarStack.Size,"Overlay leaves no leaked style state");
 }
 Check(rex::perf::GetSnapshotCounter(C::kDrawCalls)==654,"Reading diagnostics does not reset or mutate engine counters");
 rex::cvar::SetFlagByName("debug_overlay_enabled","false");Check(!motogp::debug::Enabled(),"Disabled permission remains enforced");
 motogp::debug::Shutdown();Check(motogp::debug::GuestFrameTimeMs()==0,"Shutdown clears timing and observer state");
 ImGui::DestroyContext();std::cout<<checks<<" real debug option, ImGui layout and counter checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<"Debug overlay probe failed after "<<checks<<": "<<e.what()<<'\n';return 1;}}
