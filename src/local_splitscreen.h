#pragma once
#include <cstdint>

struct PPCContext;
namespace rex { class Runtime; }

namespace motogp::splitscreen {
// Install before guest input polling starts. Eligibility is evaluated only by
// the Multiplayer -> Splitscreen action; normal input keeps the SDK policy.
void Initialize(rex::Runtime* runtime);
void Stop();
bool Active();
enum class UserQuery { SigninState, Xuid, SigninInfo, Name };
// Handles only the temporary second LOCAL user during a splitscreen session.
// It never turns that player into a community Online account.
bool HandleUserQuery(UserQuery query, PPCContext& ctx, uint8_t* base);
}
