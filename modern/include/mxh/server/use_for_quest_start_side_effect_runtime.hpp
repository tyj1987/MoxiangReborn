// use_for_quest_start_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// use_for_quest_start_side_effect_plan(). The data plane returns a
// single-step plan (BroadcastUseAck or BroadcastUseNack based on the
// legacy UseItem return code); this header walks the plan and
// dispatches the single entry to a virtual
// UseForQuestStartSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_USE_FOR_QUESTSTART_SYN from
// [Server]Map/ItemManager.cpp:4955-4979):
//   - UseItem returns 0: legacy echoes the original pmsg as
//     MSG_ITEM_USE_ACK (memcpy + Protocol flip to MP_ITEM_USE_ACK=71),
//     preserving TargetPos and wItemIdx.
//   - UseItem returns non-zero: legacy sends MSG_ITEM_ERROR with
//     Protocol = MP_ITEM_USE_NACK (72), ECode = eItemUseErr_Quest (= 7).
//
// Pattern mirrors item_sell_side_effect_runtime.hpp (D4.47) and the
// rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/use_for_quest_start_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the UseForQuestStart side-effect chain.
class UseForQuestStartSideEffectSink {
public:
    virtual ~UseForQuestStartSideEffectSink() = default;

    // Legacy: SendAckMsg(MP_ITEM_USE_ACK) -- echoes pmsg with
    // Protocol flipped to the ACK code.
    virtual void broadcast_use_ack(std::uint16_t target_pos,
                                   std::uint16_t item_idx,
                                   int original_rt) = 0;

    // Legacy: SendErrorMsg(MP_ITEM_USE_NACK, ECode=eItemUseErr_Quest)
    // -- sends the fixed quest error code to the originating player.
    virtual void broadcast_use_nack(std::uint16_t target_pos,
                                    std::uint16_t item_idx,
                                    int original_rt,
                                    int error_code) = 0;
};

struct UseForQuestStartRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t acks_sent       = 0;
    std::size_t nacks_sent      = 0;
    bool ack_flag_consumed   = false;
    bool nack_flag_consumed  = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline UseForQuestStartRuntimeOutcome
apply_use_for_quest_start_side_effects(
    const UseForQuestStartSideEffectPlan& plan,
    UseForQuestStartSideEffectSink& sink) {
    UseForQuestStartRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case UseForQuestStartSideEffectKind::BroadcastUseAck:
            sink.broadcast_use_ack(
                effect.target_pos, effect.item_idx, effect.original_rt);
            ++out.acks_sent;
            ++out.effects_applied;
            break;
        case UseForQuestStartSideEffectKind::BroadcastUseNack:
            sink.broadcast_use_nack(
                effect.target_pos, effect.item_idx,
                effect.original_rt, effect.error_code);
            ++out.nacks_sent;
            ++out.effects_applied;
            break;
        }
    }
    out.ack_flag_consumed = plan.send_ack;
    out.nack_flag_consumed = plan.send_nack;
    return out;
}

}  // namespace mxh::server
