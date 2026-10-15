#pragma once
#include "adaptive_trigger_config.h"
#include <string>
namespace motogp::adaptive {
bool LiveAvailable();
Profile ReadProfile();
bool WriteProfile(const Profile& profile);
// Start after the SDK input backend is initialized; Stop before its teardown.
void Start(bool focused);
void Stop();
void SetFocused(bool focused);
std::string Status(bool turkish);
}
