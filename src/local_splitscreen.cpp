#include "local_splitscreen.h"
#include "splitscreen_assignment.h"
#include "input_synchronization.h"
#include <rex/hook.h>
#include <rex/input/input_system.h>
#include <rex/memory.h>
#include <rex/runtime.h>
#include <rex/system/xam/user_profile.h>
#include <atomic>
#include <cstring>
#include <memory>

REX_EXTERN(__imp__sub_8239B160);
REX_EXTERN(__imp__sub_8235ACB0);
REX_EXTERN(__imp__sub_821E4EE0);
REX_EXTERN(sub_82369C38);
REX_EXTERN(sub_8239AB38);
REX_EXTERN(sub_8239AA68);
REX_EXTERN(sub_8239A188);

namespace motogp::splitscreen {
namespace {
rex::Runtime* runtime = nullptr;
rex::input::InputSystem* input = nullptr;
Assignment* assignment = nullptr;  // Owned by InputSystem for its full lifetime.
std::atomic<bool> active{false};
uint64_t player_xuid = 0;
bool entered_mode = false;
constexpr uint32_t kUserManager = 0x834F9494;
constexpr uint32_t kMultiplayerVtable = 0x8203A0C4, kMainMenuVtable = 0x8200CF7C;
constexpr uint32_t kInvalidArgument = 0x80070057, kNoSuchUser = 0x80070525;
struct NativeUsers {
  uint32_t manager = 0, primary = 0, secondary = 0, owner = 0;
  uint32_t minimum = 0, signed_in = 0, online = 0;
} previous;

uint32_t Read(uint8_t* base, uint32_t address) {
  return rex::memory::load_and_swap<uint32_t>(base + address);
}
void Write(uint8_t* base, uint32_t address, uint32_t value) {
  rex::memory::store_and_swap<uint32_t>(base + address, value);
}
bool Valid(uint32_t address, size_t size, bool write = false) {
  if (!runtime || !address || !size || uint64_t(address) + size > 0x100000000ull) return false;
  const auto end = uint64_t(address) + size;
  for (uint64_t cursor = address; cursor < end;) {
    auto* heap = runtime->memory()->LookupHeap(static_cast<uint32_t>(cursor));
    rex::memory::HeapAllocationInfo info{};
    if (!heap || !heap->QueryRegionInfo(static_cast<uint32_t>(cursor), &info) ||
        !(info.state & rex::memory::kMemoryAllocationCommit) ||
        !(info.protect & rex::memory::kMemoryProtectRead) ||
        (write && !(info.protect & rex::memory::kMemoryProtectWrite))) return false;
    const auto next = uint64_t(info.base_address) + info.region_size;
    if (next <= cursor) return false;
    cursor = next;
  }
  return true;
}
void SelectUsers(PPCContext& ctx, uint8_t* base, uint32_t primary,
                 uint32_t secondary, uint32_t owner) {
  PPCContext call = ctx;
  call.r3.u64 = primary; sub_8239AB38(call, base);
  call = ctx; call.r3.u64 = secondary; sub_8239AA68(call, base);
  call = ctx; call.r3.u64 = owner; sub_8239A188(call, base);
}
void RefreshUsers(PPCContext& ctx, uint8_t* base, uint32_t manager) {
  // Reuse the game's connection/sign-in bookkeeping once at the transition.
  // Do not hold InputStateMutex while native XAM input hooks acquire it.
  base[manager + 44] = 1;
  PPCContext call = ctx; call.r3.u64 = manager; call.r4.u64 = 1;
  sub_82369C38(call, base);
}
void End(PPCContext& ctx, uint8_t* base) {
  if (!active.exchange(false, std::memory_order_acq_rel)) return;
  {
    const std::lock_guard lock(InputStateMutex());
    assignment->End();
    input->AddUIInputBlocker(); input->RemoveUIInputBlocker();
  }
  entered_mode = false;
  if (previous.manager && Valid(previous.manager, 200, true) &&
      Read(base, kUserManager) == previous.manager) {
    SelectUsers(ctx, base, previous.primary, previous.secondary, previous.owner);
    Write(base, previous.manager + 56, previous.minimum);
    Write(base, previous.manager + 60, previous.signed_in);
    Write(base, previous.manager + 64, previous.online);
    RefreshUsers(ctx, base, previous.manager);
  }
  previous = {};
  REXLOG_DEBUG("LOCAL_SPLITSCREEN: restored normal device assignment");
}
bool Begin(PPCContext& ctx, uint8_t* base) {
  if (!runtime || !input || !assignment) return false;
  if (active.load()) return true;
  const auto manager = Read(base, kUserManager);
  if (!Valid(manager, 200, true)) return false;
  bool prepared = false;
  {
    const std::lock_guard lock(InputStateMutex());
    rex::input::X_INPUT_STATE state{};
    input->GetStateForUI(0, &state);  // Refresh the SDK device snapshot on click.
    prepared = assignment->Begin();
    if (prepared) {
      previous = {manager, Read(base, manager + 8), Read(base, manager + 12),
                  Read(base, manager + 24), Read(base, manager + 56),
                  Read(base, manager + 60), Read(base, manager + 64)};
      // A distinct local-only identity. Never change the primary profile on disk.
      player_xuid = runtime->kernel_state()->user_profile()->xuid() ^ (uint64_t{1} << 32);
      active.store(true, std::memory_order_release);
      input->AddUIInputBlocker(); input->RemoveUIInputBlocker();
    }
  }
  // Refresh failures too: a pad removed just before clicking must not pass a
  // stale two-controller cache and enter the screen with one playable device.
  RefreshUsers(ctx, base, manager);
  if (!prepared) return false;
  // Native eligibility needs both the device and local-user caches this frame.
  SelectUsers(ctx, base, 0, 1, 0);
  RefreshUsers(ctx, base, manager);
  REXLOG_DEBUG("LOCAL_SPLITSCREEN: two independent local player slots prepared");
  return true;
}
}

void Initialize(rex::Runtime* value) {
  const std::lock_guard lock(InputStateMutex());
  runtime = value;
  input = static_cast<rex::input::InputSystem*>(value->input_system());
  auto policy = std::make_unique<Assignment>(); assignment = policy.get();
  input->SetDeviceAssignment(std::move(policy));
  previous = {}; entered_mode = false; active = false;
}
void Stop() {
  const std::lock_guard lock(InputStateMutex());
  active = false;
  if (assignment) assignment->End();
  // InputSystem retains the policy; never replace it while guest polling runs.
  runtime = nullptr; input = nullptr; assignment = nullptr;
  previous = {}; entered_mode = false;
}
bool Active() { return active.load(std::memory_order_acquire); }

bool HandleUserQuery(UserQuery query, PPCContext& ctx, uint8_t* base) {
  if (ctx.r3.u32 != 1 || !Active()) return false;
  if (query == UserQuery::SigninState) { ctx.r3.u64 = 1; return true; }
  if (query == UserQuery::Xuid) {
    if (!Valid(ctx.r5.u32, 8, true)) { ctx.r3.u64 = kInvalidArgument; return true; }
    const bool local = (ctx.r4.u32 & 1) != 0;
    rex::memory::store_and_swap<uint64_t>(base + ctx.r5.u32, local ? player_xuid : 0);
    ctx.r3.u64 = local ? 0 : kNoSuchUser;
    return true;
  }
  constexpr char name[] = "Player 2";
  if (query == UserQuery::Name) {
    const auto length = std::min(ctx.r5.u32, uint32_t{16});
    if (!Valid(ctx.r4.u32, length, true)) { ctx.r3.u64 = kInvalidArgument; return true; }
    const auto count = std::min<size_t>(length - 1, sizeof(name) - 1);
    std::memcpy(base + ctx.r4.u32, name, count); base[ctx.r4.u32 + count] = 0;
  } else if (query == UserQuery::SigninInfo) {
    const auto info = ctx.r5.u32;
    if (!Valid(info, 40, true)) { ctx.r3.u64 = kInvalidArgument; return true; }
    std::memset(base + info, 0, 40);
    rex::memory::store_and_swap<uint64_t>(base + info, player_xuid);
    Write(base, info + 12, 1);  // Signed in locally, no LIVE_ENABLED flag.
    // Do not set the Xbox LIVE guest flag: the title rejects guest accounts in
    // its local two-player eligibility check. This is a transient LOCAL user.
    std::memcpy(base + info + 24, name, sizeof(name));
  }
  // SDK profile reads/writes already reject user 1; don't alias user 0's saves.
  ctx.r3.u64 = 0; return true;
}
}

REX_HOOK_RAW(sub_8239B160) {
  // The only native caller is Multiplayer action 2 (Splitscreen). Preserve the
  // title's validation/warning instead of returning a fabricated success.
  const bool entry = ctx.lr == 0x821E5F38 && motogp::splitscreen::Begin(ctx, base);
  __imp__sub_8239B160(ctx, base);
  if (entry && !ctx.r3.u8) motogp::splitscreen::End(ctx, base);
}
REX_HOOK_RAW(sub_8235ACB0) {
  if (motogp::splitscreen::Active() && ctx.r3.u32) {
    const auto vtable = motogp::splitscreen::Read(base, ctx.r3.u32);
    if (vtable == motogp::splitscreen::kMultiplayerVtable ||
        vtable == motogp::splitscreen::kMainMenuVtable)
      motogp::splitscreen::End(ctx, base);
  }
  __imp__sub_8235ACB0(ctx, base);
}
REX_HOOK_RAW(sub_821E4EE0) {
  if (motogp::splitscreen::Active()) {
    const auto mode = ctx.r3.u32;
    if (mode == 9 || mode == 10) motogp::splitscreen::entered_mode = true;
    else if (motogp::splitscreen::entered_mode) motogp::splitscreen::End(ctx, base);
  }
  __imp__sub_821E4EE0(ctx, base);
}
