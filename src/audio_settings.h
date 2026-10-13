#pragma once

#include <rex/audio/downmix.h>
#include <rex/cvar.h>
#include <charconv>

REXCVAR_DEFINE_INT32(audio_master_volume, 100, "Audio",
    "Master output volume in percent; applies to both stereo and surround")
    .range(0, 100);

inline void ApplyMotogpAudioSettings() {
  rex::audio::SetOutputGain(REXCVAR_GET(audio_master_volume) / 100.0f);
}

inline void WatchMotogpAudioSettings() {
  // Use the supplied value instead of querying the registry in its callback.
  rex::cvar::RegisterChangeCallback("audio_master_volume",
      [](std::string_view, std::string_view value) {
        int percent = 0;
        const auto result = std::from_chars(value.data(), value.data()+value.size(), percent);
        if(result.ec == std::errc{} && result.ptr == value.data()+value.size() && percent>=0 && percent<=100)
          rex::audio::SetOutputGain(percent / 100.0f);
      });
}
