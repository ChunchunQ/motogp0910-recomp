#pragma once
#include "guest_bridge.h"
namespace motogp::lan {
void Start(rex::system::KernelState*);
void Stop();
// Value snapshot for diagnostics; never exposes guest buffers or kernel objects.
std::optional<Room> CurrentRoom();
}
