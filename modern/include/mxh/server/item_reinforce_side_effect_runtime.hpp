// item_reinforce_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// item_reinforce_side_effect_plan(). The data plane returns a
// single-step plan (SilentSuccess / BroadcastReinforceFailed /
// BroadcastReinforceNack based on the legacy ReinforceItem return
// code); this header walks the plan and dispatches the single entry
// to a virtual ItemReinforceSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::MP_ITEM_REINFORCE_SYN
// from [Server]Map/ItemManager.cpp:4776-4812):
//   - ReinforceItem returns EI_TRUE (0): silent success (no ACK).
//   - ReinforceItem returns 99: echo pmsg as
//     MP_ITEM_REINFORCE_FAILED_ACK (95) with Protocol flipped.
//   - ReinforceItem returns anything else: send MP_ITEM_REINFORCE_NACK
//     (96) with ECode = rt.
//   - (The data plane keys solely off rt; the caller resolves the
//     player before building the plan.)
//
// Pattern mirrors item_mix_side_effect_runtime.hpp (D4.52) and the
// rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/item_reinforce_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ItemReinforce side-effect chain.
class ItemReinforceSideEffectSink {
public:
    virtual ~ItemReinforceSideEffectSink() = default;

    // Legacy: rt == 0 -> no network I/O. The runtime reports the
    // silent success so callers skip the broadcast step.
    virtual void silent_success() = 0;

    // Legacy: SendAckMsg(MP_ITEM_REINFORCE_FAILED_ACK) -- echo pmsg
    // with the protocol byte flipped to the failed-ACK.
    virtual void broadcast_reinforce_failed(
        std::uint16_t target_item_idx, std::uint16_t target_pos,
        int jewel_which, std::uint16_t jewel_unit,
        int original_rt) = 0;

    // Legacy: SendErrorMsg(MP_ITEM_REINFORCE_NACK, ECode=rt).
    virtual void broadcast_reinforce_nack(
        std::uint16_t target_item_idx, std::uint16_t target_pos,
        int jewel_which, std::uint16_t jewel_unit,
        int original_rt, int error_code) = 0;
};

struct ItemReinforceRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t silent_successes = 0;
    std::size_t failed_acks     = 0;
    std::size_t nacks_sent      = 0;
    bool failed_ack_flag_consumed = false;
    bool nack_flag_consumed     = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline ItemReinforceRuntimeOutcome apply_item_reinforce_side_effects(
    const ItemReinforceSideEffectPlan& plan,
    ItemReinforceSideEffectSink& sink) {
    ItemReinforceRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ItemReinforceSideEffectKind::SilentSuccess:
            sink.silent_success();
            ++out.silent_successes;
            ++out.effects_applied;
            break;
        case ItemReinforceSideEffectKind::BroadcastReinforceFailed:
            sink.broadcast_reinforce_failed(
                effect.target_item_idx, effect.target_pos,
                effect.jewel_which, effect.jewel_unit,
                effect.original_rt);
            ++out.failed_acks;
            ++out.effects_applied;
            break;
        case ItemReinforceSideEffectKind::BroadcastReinforceNack:
            sink.broadcast_reinforce_nack(
                effect.target_item_idx, effect.target_pos,
                effect.jewel_which, effect.jewel_unit,
                effect.original_rt, effect.error_code);
            ++out.nacks_sent;
            ++out.effects_applied;
            break;
        }
    }
    out.failed_ack_flag_consumed = plan.send_failed_ack;
    out.nack_flag_consumed = plan.send_nack;
    return out;
}

}  // namespace mxh::server
