// use_change_item_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// use_change_item_side_effect_plan(). The data plane returns an empty
// plan (no player), a BroadcastUseNack entry (rt == 0), or a
// SilentSuccess entry (rt != 0); this header walks the plan and
// dispatches each entry to a virtual UseChangeItemSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_USE_CHANGEITEM_SYN from
// [Server]Map/ItemManager.cpp:5002-5019) -- the INVERSE of
// MP_ITEM_USE_SYN:
//   - FindUser returns null: handler returns (empty plan).
//   - UseChangeItem returns 0 (legacy "not use"): legacy sends
//     MSG_ITEM_ERROR with Protocol = MP_ITEM_USE_NACK (74), ECode = rt
//     (which is 0).
//   - UseChangeItem returns non-zero (legacy success, item changed):
//     NO network response -- silent success (the legacy client
//     updates its own UI based on the inventory diff).
//
// Pattern mirrors use_for_quest_start_side_effect_runtime.hpp (D4.58)
// and the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/use_change_item_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the UseChangeItem side-effect chain.
class UseChangeItemSideEffectSink {
public:
    virtual ~UseChangeItemSideEffectSink() = default;

    // Legacy: SendErrorMsg(MP_ITEM_USE_NACK, ECode=rt) -- sends the
    // "not used" return code (rt == 0) to the originating player.
    virtual void broadcast_use_nack(std::uint16_t target_pos,
                                    std::uint16_t item_idx,
                                    int original_rt,
                                    int error_code) = 0;

    // Legacy: rt != 0 -> no network I/O. The runtime reports the
    // silent success so callers skip the broadcast step.
    virtual void silent_success() = 0;
};

struct UseChangeItemRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t nacks_sent      = 0;
    std::size_t silent_successes = 0;
    bool nack_flag_consumed    = false;
    bool silent_flag_consumed  = false;
};

// Runtime: walks the plan and dispatches each entry.
inline UseChangeItemRuntimeOutcome apply_use_change_item_side_effects(
    const UseChangeItemSideEffectPlan& plan,
    UseChangeItemSideEffectSink& sink) {
    UseChangeItemRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case UseChangeItemSideEffectKind::BroadcastUseNack:
            sink.broadcast_use_nack(
                effect.target_pos, effect.item_idx,
                effect.original_rt, effect.error_code);
            ++out.nacks_sent;
            ++out.effects_applied;
            break;
        case UseChangeItemSideEffectKind::SilentSuccess:
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
