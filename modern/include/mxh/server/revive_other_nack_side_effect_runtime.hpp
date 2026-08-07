// revive_other_nack_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// revive_other_nack_side_effect_plan(). The data plane returns an
// empty plan (player or target missing) or a 3-step cleanup chain;
// this header walks the plan and dispatches each entry to a virtual
// ReviveOtherNackSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_SHOPITEM_REVIVEOTHER_NACK from
// [Server]Map/ItemManager.cpp:5353-5372):
//   - FindUser(dwObjectID) null -> return; FindUser(dwData1) null ->
//     return.
//   - Forwarded chain in legacy order: MSG_DWORD {REVIVEOTHER_NACK,
//     dwData = dwData2} to target -> SetReviveData(0,0,0) ->
//     SetReviveTime(0).
//
// Pattern mirrors revive_other_ack_side_effect_runtime.hpp (D4.78)
// and the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/revive_other_nack_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ReviveOtherNack side-effect chain.
class ReviveOtherNackSideEffectSink {
public:
    virtual ~ReviveOtherNackSideEffectSink() = default;

    // Legacy: SendMsg(MSG_DWORD {MP_ITEM, REVIVEOTHER_NACK,
    // dwData = nack_code}) to the target player.
    virtual void forward_nack_to_target(std::uint32_t target_id,
                                        std::uint32_t nack_code) = 0;

    // Legacy: pTargetPlayer->SetReviveData(0, 0, 0).
    virtual void clear_revive_data_on_target(std::uint32_t target_id) = 0;

    // Legacy: pTargetPlayer->SetReviveTime(0).
    virtual void clear_revive_time_on_target(std::uint32_t target_id) = 0;
};

struct ReviveOtherNackRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t nacks_forwarded = 0;
    std::size_t revive_data_clears = 0;
    std::size_t revive_time_clears = 0;
    bool forward_flag_consumed   = false;
    bool data_flag_consumed      = false;
    bool time_flag_consumed      = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order.
inline ReviveOtherNackRuntimeOutcome apply_revive_other_nack_side_effects(
    const ReviveOtherNackSideEffectPlan& plan,
    ReviveOtherNackSideEffectSink& sink) {
    ReviveOtherNackRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ReviveOtherNackSideEffectKind::ForwardNackToTarget:
            sink.forward_nack_to_target(effect.target_id,
                                        effect.nack_code);
            ++out.nacks_forwarded;
            ++out.effects_applied;
            break;
        case ReviveOtherNackSideEffectKind::ClearReviveDataOnTarget:
            sink.clear_revive_data_on_target(effect.target_id);
            ++out.revive_data_clears;
            ++out.effects_applied;
            break;
        case ReviveOtherNackSideEffectKind::ClearReviveTimeOnTarget:
            sink.clear_revive_time_on_target(effect.target_id);
            ++out.revive_time_clears;
            ++out.effects_applied;
            break;
        }
    }
    out.forward_flag_consumed = plan.forward_nack;
    out.data_flag_consumed = plan.clear_revive_data;
    out.time_flag_consumed = plan.clear_revive_time;
    return out;
}

}  // namespace mxh::server
