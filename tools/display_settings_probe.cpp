// Exercise the shipped runtime's SDL window, the app's startup settings, and
// frame pacing without a GPU. Physical refresh-rate changes need a real monitor.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <rex/cvar.h>
#include <rex/ui/windowed_app_context_sdl.h>
#include "../src/display_settings.h"
#include "../src/frame_pacing.h"
#include <iostream>

namespace {
int checks=0;
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);++checks;}
}
int main(){
  try {
    using Schedule=motogp::FrameSchedule;
    using namespace std::chrono;
    const auto start=Schedule::Clock::time_point{}+seconds(1);
    Schedule schedule;
    Check(schedule.Next(start,0)==start,"Unlimited adds no wait");
    Check(schedule.Next(start,60)==start,"First frame is immediate");
    const auto period=duration_cast<Schedule::Clock::duration>(duration<double>(1./60));
    Check(schedule.Next(start+milliseconds(2),60)==start+period,"60 FPS deadline");
    Check(schedule.Next(start+period+milliseconds(2),60)==start+period*2,"Frame work does not add another full period");
    const auto stalled=start+seconds(2);
    Check(schedule.Next(stalled,60)==stalled,"Stall resets deadline without catch-up burst");
    Check(schedule.Next(stalled+milliseconds(1),60)==stalled+period,"Frame after stall remains limited");
    Check(schedule.Next(stalled+milliseconds(2),144)==stalled+milliseconds(2),"Rate change resets phase");
    Check(schedule.Next(stalled+milliseconds(3),0)==stalled+milliseconds(3),"Disabling cap clears deadline");
    Check(schedule.Next(stalled+milliseconds(4),144)==stalled+milliseconds(4),"Re-enabling cap starts cleanly");
    Check(schedule.Next(stalled+milliseconds(5),-1)==stalled+milliseconds(5),"Invalid rate cannot divide by zero");
    Check(schedule.Next(stalled+milliseconds(6),1001)==stalled+milliseconds(6),"Out-of-range rate adds no wait");
    Check(rex::cvar::SetFlagByName("fps_limit","200"),"Production frame-limit cvar accepts rate");
    auto before=Schedule::Clock::now();for(int i=0;i<5;++i)motogp::PaceGameFrame();
    Check(Schedule::Clock::now()-before>=milliseconds(19),"Real frame pacer sleeps between frames");
    Check(rex::cvar::SetFlagByName("fps_limit","0"),"Frame limiter can be disabled");
    Check(!rex::cvar::SetFlagByName("fps_limit","1001"),"Production frame-limit range enforced");
    rex::ui::SDLWindowedAppContext context;Check(context.Initialize(),"Actual runtime SDL video initialized");
    rex::cvar::SetFlagByName("window_width","800");rex::cvar::SetFlagByName("window_height","500");
    auto window=rex::ui::Window::Create(context,"Display settings test",800,500);
    Check(window && window->Open(),"Actual SDK host window opened");
    using Configure=int32_t(*)(uint32_t,void*,int32_t,int32_t,int32_t,int32_t);
    const auto configure=reinterpret_cast<Configure>(GetProcAddress(GetModuleHandleW(L"rexruntime.dll"),"Motogp_ConfigureHostDisplay"));
    Check(configure!=nullptr,"Paired runtime exports host display API");
    auto hwnd=static_cast<HWND>(window->GetNativeWindowHandle());
    Check(configure(2,hwnd,1,0,0,0)==-1,"Unknown API version rejected");
    Check(configure(1,hwnd,3,0,0,0)==-1,"Invalid display mode rejected");
    Check(configure(1,hwnd,2,0,0,-1)==-1,"Invalid refresh rate rejected");
    Check(configure(1,nullptr,0,0,0,0)==-2,"Null window rejected");
    Check(configure(1,reinterpret_cast<void*>(1),0,0,0,0)==-2,"Foreign window rejected");
    Check(configure(1,hwnd,2,8191,8191,997)==-3,"Unsupported fullscreen mode is rejected");
    Check(configure(1,hwnd,1,0,0,0)==0,"Borderless mode applied through runtime SDL");
    RECT rectangle{};GetWindowRect(hwnd,&rectangle);
    Check(rectangle.right-rectangle.left==GetSystemMetrics(SM_CXSCREEN) && rectangle.bottom-rectangle.top==GetSystemMetrics(SM_CYSCREEN),"Borderless covers virtual desktop");
    Check(configure(1,hwnd,2,0,0,0)==0,"Actual fullscreen at desktop mode applied");
    Check(configure(1,hwnd,0,0,0,0)==0,"Fullscreen returns to windowed mode");
    Check((GetWindowLongPtrW(hwnd,GWL_STYLE)&WS_CAPTION)==WS_CAPTION,"Windowed caption restored");
    rex::cvar::SetFlagFromCommandLine("display_mode","windowed");
    rex::cvar::SetFlagFromCommandLine("fullscreen","true");
    motogp::ApplyDisplayStartupFlags();
    Check(!rex::cvar::Query<bool>("fullscreen"),"Explicit display mode wins at equal CLI priority");
    motogp::ApplyHostDisplay(window.get());
    Check((GetWindowLongPtrW(hwnd,GWL_STYLE)&WS_CAPTION)==WS_CAPTION,"Cached startup mode survives runtime source update");
    rex::cvar::SetFlagByName("display_mode","borderless");
    motogp::ApplyDisplayStartupFlags();motogp::ApplyHostDisplay(window.get());
    Check(rex::cvar::Query<bool>("fullscreen"),"Borderless enables SDK fullscreen state");
    rex::cvar::SetFlagByName("display_mode","auto");rex::cvar::SetFlagByName("fullscreen","false");
    motogp::ApplyDisplayStartupFlags();
    Check(!rex::cvar::Query<bool>("fullscreen"),"Legacy fullscreen=false remains windowed");
    configure(1,hwnd,0,0,0,0);window.reset();
    std::cout<<checks<<" frame-pacing and real host-window checks passed\n";
    return 0;
  }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<'\n';return 1;}
}
