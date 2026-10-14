#pragma once
#include <rex/cvar.h>
// Include in one translation unit per executable: these macros own storage.
REXCVAR_DEFINE_STRING(engine_speed_blur,"game","MotoGP engine","Speed blur: game default or off").allowed({"game","off"});
REXCVAR_DEFINE_STRING(engine_depth_of_field,"game","MotoGP engine","Depth of field: game default or off").allowed({"game","off"});
REXCVAR_DEFINE_STRING(engine_dynamic_shadows,"game","MotoGP engine","Dynamic shadows: game default or off").allowed({"game","off"});
REXCVAR_DEFINE_INT32(engine_shadow_distance,0,"MotoGP engine","Shadow render distance; 0 keeps the game default").range(0,180);
