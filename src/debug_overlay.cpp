#include "debug_overlay.h"
#include "overlay_theme.h"
#include "frame_pacing.h"
#include <atomic>
#ifndef _WIN32
#include <dlfcn.h>
#endif
#include "diagnostic_options.h"
#include <rex/perf/counter.h>
#include <rex/ui/window.h>
#include <rex/ui/keybinds.h>
#include <imgui.h>
#include <array>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <string>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif
namespace motogp::debug {
bool Enabled(){return rex::cvar::Query<bool>("debug_overlay_enabled");}
void RegisterToggle(std::function<void()> callback){
  auto key=rex::cvar::GetFlagByName("bind_debug_overlay");if(key.empty())key="F3";
  rex::ui::UnregisterBind("bind_debug_overlay");
  rex::ui::RegisterBind("bind_motogp_debug_overlay",key,"Toggle enabled MotoGP diagnostics",[callback=std::move(callback)]{if(Enabled())callback();});
}
void UnregisterToggle(){rex::ui::UnregisterBind("bind_motogp_debug_overlay");}
namespace {
using C=rex::perf::CounterId;
using GpuCounters=uint32_t(*)(uint32_t,int64_t*,uint32_t);
GpuCounters gpu_counters=nullptr;
std::string renderer="n/a";
std::atomic<float> guest_ms{0};
std::atomic<uint64_t> timing_generation{0};
void FrameTick(){
 thread_local uint64_t observed_generation=0;thread_local std::chrono::steady_clock::time_point previous{};
 const auto generation=timing_generation.load(std::memory_order_relaxed);
 if(observed_generation!=generation){previous={};observed_generation=generation;}
 const auto now=std::chrono::steady_clock::now();
 if(previous.time_since_epoch().count()){const float ms=std::chrono::duration<float,std::milli>(now-previous).count();if(ms>0)guest_ms.store(ms,std::memory_order_relaxed);}
 previous=now;
}
using motogp::overlay::orange;
struct Panel {Panel(const char* title,ImVec2 position,ImVec2 pivot){ImGui::SetNextWindowPos(position,ImGuiCond_Always,pivot);ImGui::SetNextWindowBgAlpha(.82f);ImGui::PushStyleColor(ImGuiCol_TitleBg,motogp::overlay::AccentTint(.08f));ImGui::PushStyleColor(ImGuiCol_TitleBgActive,motogp::overlay::AccentTint(.18f));ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(.05f,.06f,.08f,1));ImGui::PushStyleColor(ImGuiCol_Border,motogp::overlay::AccentTint(.55f,.8f));ImGui::PushStyleColor(ImGuiCol_PlotLines,orange);ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(.95f,.96f,.97f,1));ImGui::Begin(title,nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoNav|ImGuiWindowFlags_NoInputs|ImGuiWindowFlags_NoCollapse);}~Panel(){ImGui::End();ImGui::PopStyleColor(6);}};
void RuntimeCounter(const char* label,C id){ImGui::Text("%s: %lld",label,static_cast<long long>(rex::perf::GetCounter(id)));}
void GpuCounter(const char* label,C id,const std::array<int64_t,static_cast<size_t>(C::kCount)>& counters,bool available){if(available)ImGui::Text("%s: %lld",label,static_cast<long long>(counters[static_cast<size_t>(id)]));else ImGui::TextDisabled("%s: n/a",label);}
void Flag(const char* label,const char* key){auto value=rex::cvar::GetFlagByName(key);ImGui::Text("%s: %s",label,value.empty()?"n/a":value.c_str());}
double ResidentMiB(){
#ifdef _WIN32
 PROCESS_MEMORY_COUNTERS p{};if(K32GetProcessMemoryInfo(GetCurrentProcess(),&p,sizeof(p)))return p.WorkingSetSize/1048576.;
#else
 std::ifstream f("/proc/self/status");std::string line;while(std::getline(f,line))if(line.starts_with("VmRSS:"))return std::stod(line.substr(6))/1024.;
#endif
 return -1;
}
}
void Initialize(const char* graphics_type_name){
 guest_ms=0;timing_generation.fetch_add(1,std::memory_order_relaxed);
 const std::string name=graphics_type_name?graphics_type_name:"";
 renderer=name.find("D3D12")!=std::string::npos?"Direct3D 12":name.find("Vulkan")!=std::string::npos?"Vulkan":"unidentified backend";
#ifdef _WIN32
 const auto module=GetModuleHandleW(L"rexgpu-xenos.dll");gpu_counters=module?reinterpret_cast<GpuCounters>(GetProcAddress(module,"Motogp_ReadGpuCounters")):nullptr;
#else
 const auto module=dlopen("librexgpu-xenos.so",RTLD_NOW|RTLD_NOLOAD);gpu_counters=module?reinterpret_cast<GpuCounters>(dlsym(module,"Motogp_ReadGpuCounters")):nullptr;if(module)dlclose(module);
#endif
 if(Enabled())motogp::SetFrameTimingObserver(FrameTick);
}
float GuestFrameTimeMs(){return guest_ms.load(std::memory_order_relaxed);}
void Shutdown(){motogp::SetFrameTimingObserver(nullptr);gpu_counters=nullptr;guest_ms=0;}
void Draw(rex::ui::Window* window,bool tr){
 const auto& io=ImGui::GetIO();const float margin=12.f;
 struct History {std::array<float,120> values{};int index=0,count=0;double last=-1,rss=-1;};static History h;
 const double now=ImGui::GetTime();const float ms=guest_ms.load(std::memory_order_relaxed);
 std::array<int64_t,static_cast<size_t>(C::kCount)> gpu{};const bool gpu_available=gpu_counters&&gpu_counters(1,gpu.data(),static_cast<uint32_t>(gpu.size()))==1;
 if(now<h.last)h={};
 if(now-h.last>=.1){h.last=now;h.values[h.index]=ms;h.index=(h.index+1)%120;h.count=std::min(120,h.count+1);h.rss=ResidentMiB();}
 const float font=std::clamp(io.DisplaySize.y/720.f, .8f,1.35f);
 {Panel panel(tr?"Performans##pc_debug":"Performance##pc_debug",{margin,margin},{0,0});ImGui::SetWindowFontScale(font);
  ImGui::TextColored(orange,"MotoGP 09/10 | F3");
  if(ms>0)ImGui::Text("Guest presents: %.1f FPS | %.2f ms",1000.f/ms,ms);else ImGui::TextDisabled("Guest timing: waiting for presents");
  ImGui::Text("UI: %.1f FPS",io.Framerate);
  ImGui::PlotLines("##frame_times",h.values.data(),h.count,h.count==120?h.index:0,"Guest present samples (ms)",0.f,std::max(50.f,ms*1.25f),ImVec2(250*font,48*font));
  GpuCounter("Draw calls",C::kDrawCalls,gpu,gpu_available);GpuCounter("Vertices",C::kVerticesProcessed,gpu,gpu_available);GpuCounter("Command stalls",C::kCommandBufferStalls,gpu,gpu_available);
  ImGui::Separator();RuntimeCounter("Dispatches",C::kFunctionsDispatched);RuntimeCounter("Interrupts",C::kInterruptDispatches);RuntimeCounter("Active threads",C::kActiveThreads);RuntimeCounter("APC events",C::kApcQueueDepth);RuntimeCounter("Lock contention (total)",C::kCriticalRegionContentions);
  if(h.rss>=0)ImGui::Text("Process resident memory: %.1f MiB",h.rss);
 }
 {Panel panel(tr?"Görüntü##pc_display":"Display##pc_display",{io.DisplaySize.x-margin,margin},{1,0});ImGui::SetWindowFontScale(font);
  if(window)ImGui::Text("Window: %u x %u",window->GetActualPhysicalWidth(),window->GetActualPhysicalHeight());
  ImGui::Text("Renderer: %s",renderer.c_str());Flag("Internal scale X","draw_resolution_scale_x");Flag("Internal scale Y","draw_resolution_scale_y");Flag("Guest width","video_mode_width");Flag("Guest height","video_mode_height");Flag("Display mode","display_mode");Flag("FPS limit","fps_limit");Flag("VSync","vsync");Flag("Native MSAA","native_2x_msaa");Flag("Upscaling","present_effect");Flag("FSR quality","present_fsr_quality_mode");Flag("Aspect","aspect_ratio");Flag("Input backend","input_backend");Flag("Prompt style","button_prompt_style");Flag("Network mode (configured)","network_mode");
 }
 {Panel panel(tr?"Ses ve önbellek##pc_audio":"Audio and caches##pc_audio",{margin,io.DisplaySize.y-margin},{0,1});ImGui::SetWindowFontScale(font);
  RuntimeCounter("XMA decoded frames",C::kXmaFramesDecoded);const auto latency=rex::perf::GetCounter(C::kAudioFrameLatencyUs);ImGui::Text("Audio latency: %.2f ms",latency/1000.);RuntimeCounter("Audio queue",C::kBufferQueueDepth);
  GpuCounter("Texture hits",C::kTextureCacheHits,gpu,gpu_available);GpuCounter("Texture misses",C::kTextureCacheMisses,gpu,gpu_available);GpuCounter("Pipeline hits",C::kPipelineCacheHits,gpu,gpu_available);GpuCounter("Pipeline misses",C::kPipelineCacheMisses,gpu,gpu_available);
  ImGui::TextDisabled("GPU: last frame | runtime: live totals / gauges");
 }
}
namespace {class Dialog final:public rex::ui::ImGuiDialog {rex::ui::Window* window_;bool tr_;public:Dialog(rex::ui::ImGuiDrawer* d,rex::ui::Window* w,bool t):ImGuiDialog(d),window_(w),tr_(t){}void OnDraw(ImGuiIO&)override{if(Enabled())motogp::debug::Draw(window_,tr_);}};}
std::unique_ptr<rex::ui::ImGuiDialog> Create(rex::ui::ImGuiDrawer* d,rex::ui::Window* w,bool t){return std::make_unique<Dialog>(d,w,t);}
}
