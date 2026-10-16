#pragma once
#include <windowsx.h>
namespace launcher {
struct ScrollDrag {bool active=false;int origin_y=0,origin_offset=0;};
struct ScrollRail {RECT track{},thumb{};int maximum=0,travel=0;};
}
