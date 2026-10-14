#include "achievement_ui.h"
#include <rex/cvar.h>
#include <rex/logging.h>
#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <thread>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#endif
namespace motogp::achievements {
float CueGain(const Preferences& preferences) {
  if (!preferences.sound.load() || rex::cvar::Query<bool>("audio_mute")) return 0;
  return std::clamp(preferences.volume.load(),0,100)*0.01f *
         std::clamp(rex::cvar::Query<int32_t>("audio_master_volume"),0,100)*0.01f;
}
std::vector<int16_t> CueSamples(float gain) {
  constexpr int rate=48000, count=21600; // 450 ms, original two-note soft bell.
  std::vector<int16_t> samples(count);
  for (int i=0;i<count;++i) {
    const double t=double(i)/rate;
    double value=0;
    for (int note=0;note<2;++note) {
      const double u=t-note*0.11;
      if(u<0) continue;
      const double hz=note ? 880.0 : 659.255;
      const double envelope=std::min(u/0.018,1.0)*std::exp(-u*11.5)*std::min((0.45-t)/0.05,1.0);
      value+=0.15*envelope*(std::sin(6.283185307179586*hz*u)+0.12*std::sin(12.566370614359172*hz*u));
    }
    samples[i]=static_cast<int16_t>(std::clamp(value*std::clamp(gain,0.f,1.f),-1.0,1.0)*32767);
  }
  return samples;
}
struct Sound::Impl {
  explicit Impl(std::shared_ptr<Preferences> p): preferences(std::move(p)), worker([this]{Run();}) {}
  ~Impl() { {std::lock_guard lock(mutex); stopping=true;} wake.notify_one(); worker.join(); }
  std::shared_ptr<Preferences> preferences;
  std::mutex mutex;
  std::condition_variable wake;
  bool stopping=false, requested=false, warned=false;
  std::thread worker;
  void Run() {
    for (;;) {
      {std::unique_lock lock(mutex); wake.wait(lock,[&]{return stopping||requested;});
       if(stopping) return; requested=false;}
      const float gain=CueGain(*preferences);
      if(gain<=0) continue;
#ifdef _WIN32
      HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
      if(!event) continue;
      WAVEFORMATEX format{}; format.wFormatTag=WAVE_FORMAT_PCM; format.nChannels=1;
      format.nSamplesPerSec=48000; format.wBitsPerSample=16; format.nBlockAlign=2; format.nAvgBytesPerSec=96000;
      HWAVEOUT output=nullptr;
      auto result=waveOutOpen(&output,WAVE_MAPPER,&format,reinterpret_cast<DWORD_PTR>(event),0,CALLBACK_EVENT);
      if(result!=MMSYSERR_NOERROR) {
        CloseHandle(event);
        if(!warned) { REXLOG_WARN("Achievement sound unavailable (WinMM {}); visual notifications remain enabled",result); warned=true; }
        continue;
      }
      auto samples=CueSamples(gain);
      WAVEHDR header{}; header.lpData=reinterpret_cast<LPSTR>(samples.data()); header.dwBufferLength=static_cast<DWORD>(samples.size()*2);
      const bool prepared=waveOutPrepareHeader(output,&header,sizeof(header))==MMSYSERR_NOERROR;
      const bool written=prepared && waveOutWrite(output,&header,sizeof(header))==MMSYSERR_NOERROR;
      if(written) {
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
        for(;;) {
          WaitForSingleObject(event,20);
          bool stop; {std::lock_guard lock(mutex); stop=stopping;}
          if((header.dwFlags&WHDR_DONE)||stop||CueGain(*preferences)<=0||std::chrono::steady_clock::now()>=deadline) break;
        }
      }
      waveOutReset(output);
      if(prepared) waveOutUnprepareHeader(output,&header,sizeof(header));
      waveOutClose(output); CloseHandle(event);
#endif
    }
  }
};
Sound::Sound(std::shared_ptr<Preferences> p): impl_(std::make_unique<Impl>(std::move(p))) {}
Sound::~Sound()=default;
void Sound::Play() { {std::lock_guard lock(impl_->mutex); impl_->requested=true;} impl_->wake.notify_one(); }
}
