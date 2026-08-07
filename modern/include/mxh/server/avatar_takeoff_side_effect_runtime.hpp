// avatar_takeoff_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// avatar_takeoff_side_effect_plan(). The data plane returns an empty
// plan (no player), a BroadcastAvatarUseNack entry (not usable /
// take-off blocked), or a SilentSuccess entry; this header walks the
// plan and dispatches each entry to a virtual
// AvatarTakeoffSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_SHOPITEM_AVATAR_TAKEOFF from
// [Server]Map/ItemManager.cpp:5373-5393):
//   - FindUser returns null: handler returns (empty plan).
//   - IsUseAbleShopItem returns false: handler sends
//     MP_ITEM_SHOPITEM_AVATAR_USE_NACK (84) and returns.
//   - TakeOffAvatarItem returns false: handler sends the same NACK.
//   - Both pass: handler is SILENT (no ACK sent); the client updates
//     its UI based on the inventory diff.
//
// Pattern mirrors use_change_item_side_effect_runtime.hpp (D4.60) and
// the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/avatar_takeoff_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the AvatarTakeoff side-effect chain.
class AvatarTakeoffSideEffectSink {
public:
    virtual ~AvatarTakeoffSideEffectSink() = default;

    // Legacy: SendMsg(MP_ITEM_SHOPITEM_AVATAR_USE_NACK) -- not usable
    // or take-off blocked.
    virtual void broadcast_avatar_use_nack(std::uint16_t item_idx,
                                           std::uint16_t item_pos) = 0;

    // Legacy: take-off success -> NO network I/O. The runtime reports
    // the silent success so callers skip the broadcast step.
    virtual void silent_success() = 0;
};

struct AvatarTakeoffRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t nacks_sent      = 0;
    std::size_t silent_successes = 0;
    bool nack_flag_consumed    = false;
    bool silent_flag_consumed  = false;
};

// Runtime: walks the plan and dispatches each entry.
inline AvatarTakeoffRuntimeOutcome apply_avatar_takeoff_side_effects(
    const AvatarTakeoffSideEffectPlan& plan,
    AvatarTakeoffSideEffectSink& sink) {
    AvatarTakeoffRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case AvatarTakeoffSideEffectKind::BroadcastAvatarUseNack:
            sink.broadcast_avatar_use_nack(effect.item_idx,
                                           effect.item_pos);
            ++out.nacks_sent;
            ++out.effects_applied;
            break;
        case AvatarTakeoffSideEffectKind::SilentSuccess:
            sink.silent_success();
            ++out.silent_successes;
            ++out.effects_applied;
            break;
        }
    }
    out.nack_flag_consumed = plan.send_nack;
    out.silent_flag_consumed = plan.silent_success;
    return out;
}

}  // namespace mxh::server
