// shout_pass_through_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// shout_pass_through_side_effect_plan(). The data plane returns an
// empty plan (player not found) or a single ForwardToPlayer entry;
// this header walks the plan and dispatches each entry to a virtual
// ShoutPassThroughSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_SHOPITEM_SHOUT_ACK/_NACK from
// [Server]Map/ItemManager.cpp:6071-6091):
//   - SHOUT_ACK: FindUser(Receive.CharacterIdx); null -> return;
//     rewrite Protocol = MP_ITEM_SHOPITEM_SHOUT_SENDACK(94) then
//     SendMsg to the player.
//   - SHOUT_NACK: FindUser(dwData); null -> return; SendMsg as-is
//     (no protocol rewrite).
//
// Pattern mirrors shop_item_change_map_side_effect_runtime.hpp
// (D4.59) and the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/shout_pass_through_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ShoutPassThrough side-effect chain.
class ShoutPassThroughSideEffectSink {
public:
    virtual ~ShoutPassThroughSideEffectSink() = default;

    // Legacy: pPlayer->SendMsg(...) -- forwards the agent shout
    // response to the player; rewrite_to_sendack tells the caller to
    // flip the protocol to SHOUT_SENDACK(94) for the ACK variant.
    virtual void forward_to_player(
        std::uint32_t player_id, ShoutPassThroughVariant variant,
        bool rewrite_to_sendack) = 0;
};

struct ShoutPassThroughRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t forwards_sent   = 0;
    bool forward_flag_consumed    = false;
    bool rewrite_flag_consumed    = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline ShoutPassThroughRuntimeOutcome apply_shout_pass_through_side_effects(
    const ShoutPassThroughSideEffectPlan& plan,
    ShoutPassThroughSideEffectSink& sink) {
    ShoutPassThroughRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ShoutPassThroughSideEffectKind::ForwardToPlayer:
            sink.forward_to_player(effect.player_id, effect.variant,
                                   effect.rewrite_to_sendack);
            ++out.forwards_sent;
            ++out.effects_applied;
            break;
        }
    }
    out.forward_flag_consumed = plan.forward;
    out.rewrite_flag_consumed = plan.rewrite_protocol;
    return out;
}

}  // namespace mxh::server
