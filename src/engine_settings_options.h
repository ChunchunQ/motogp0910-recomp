#pragma once
#include <rex/cvar.h>
// Include in one translation unit per executable: these macros own storage.
REXCVAR_DEFINE_STRING(engine_speed_blur,"game","MotoGP engine","Speed blur: game default or off").allowed({"game","off"});
REXCVAR_DEFINE_STRING(engine_depth_of_field,"game","MotoGP engine","Depth of field: game default or off").allowed({"game","off"});
REXCVAR_DEFINE_STRING(engine_dynamic_shadows,"game","MotoGP engine","Dynamic shadows: game default or off").allowed({"game","off"});
REXCVAR_DEFINE_STRING(engine_vignette,"game","MotoGP engine","Speed vignette strength relative to the game value").allowed({"game","off","25","50","75","100"});
REXCVAR_DEFINE_STRING(engine_bloom,"game","MotoGP engine","Bloom strength relative to the game value").allowed({"game","off","25","50","75","100"});
REXCVAR_DEFINE_STRING(engine_shadow_filter,"game","MotoGP engine","Native shadow filtering").allowed({"game","off","on"});
REXCVAR_DEFINE_STRING(engine_soft_shadow_distance,"game","MotoGP engine","Soft shadow distance: game default or 45/90/180").allowed({"game","45","90","180"});
REXCVAR_DEFINE_STRING(engine_particle_density,"game","MotoGP engine","Native particle density percentage").allowed({"game","off","25","50","75","100"});
REXCVAR_DEFINE_STRING(engine_draw_distance,"game","MotoGP engine","Instance far clip scale relative to the game value").allowed({"game","50","75","100","125","150"});
REXCVAR_DEFINE_INT32(engine_shadow_distance,0,"MotoGP engine","Shadow render distance; 0 keeps the game default").range(0,180);
