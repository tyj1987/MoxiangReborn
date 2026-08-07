// item_move_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// item_move_side_effect_plan(). The data plane returns a single-step
// plan (BroadcastMoveAck / BroadcastMoveNack as effect entries, or an
// empty plan + silent flag for the legacy rt==99 branch); this header
// walks the plan and dispatches the entry to a virtual
// ItemMoveSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::MP_ITEM_MOVE_SYN from
// [Server]Map/ItemManager.cpp:4042-4067):
//   - MoveItem returns 0 (EI_TRUE): legacy echoes the original pmsg as
//     MSG_ITEM_MOVE_ACK (memcpy + Protocol flip). The runtime's
//     BroadcastMoveAck carries (from_item_idx, from_pos, to_item_idx,
//     to_pos, original_rt) so the orchestrator can rebuild the wire
//     bytes via SendAckMsg().
//   - MoveItem returns non-zero and != 99: legacy sends MSG_ITEM_ERROR
//     with Protocol = MP_ITEM_ERROR_NACK, ECode = eItemUseErr_Move
//     (= 2), and the original rt as the SendErrorMsg auxiliary code.
//     The runtime's BroadcastMoveNack carries ecode = 2 + original_rt.
//   - MoveItem returns 99: legacy suppresses both ACK and NACK (the
//     handler returns with no message). The runtime reports the
//     silent-skip via the sink so the caller can log / release
//     resources, but emits no wire message.
//
// Pattern mirrors item_sell_side_effect_runtime.hpp (D4.47) and the
// rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/item_move_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ItemMove side-effect chain.
class ItemMoveSideEffectSink {
public:
    virtual ~ItemMoveSideEffectSink() = default;

    // Legacy: SendAckMsg(MP_ITEM_MOVE_ACK) -- echoes pmsg with
    // Protocol flipped to the ACK code.
    virtual void broadcast_move_ack(std::uint16_t from_item_idx,
                                    std::uint16_t from_pos,
                                    std::uint16_t to_item_idx,
                                    std::uint16_t to_pos,
                                    int original_rt) = 0;

    // Legacy: SendErrorMsg(MP_ITEM_ERROR_NACK, ECode=eItemUseErr_Move,
    // aux=rt) -- sends the fixed move error code plus the original rt
    // as the auxiliary code.
    virtual void broadcast_move_nack(std::uint16_t from_item_idx,
                                     std::uint16_t from_pos,
                                     std::uint16_t to_item_idx,
                                     std::uint16_t to_pos,
                                     int original_rt,
                                     int ecode) = 0;

    // Legacy: rt == 99 -> no message. The data plane emits an empty
    // plan for this branch (locked by SilentRtNinetyNineProducesEmpty
    // Plan); the runtime still reports the skip via this callback so
    // callers can trace the legacy silent branch.
    virtual void silent_skip() = 0;
};

struct ItemMoveRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t acks_sent       = 0;
    std::size_t nacks_sent      = 0;
    std::size_t silent_skips    = 0;
    bool ack_flag_consumed   = false;
    bool nack_flag_consumed  = false;
    bool silent_flag_consumed = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline ItemMoveRuntimeOutcome apply_item_move_side_effects(
    const ItemMoveSideEffectPlan& plan,
    ItemMoveSideEffectSink& sink) {
    ItemMoveRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ItemMoveSideEffectKind::BroadcastMoveAck:
            sink.broadcast_move_ack(
                effect.from_item_idx, effect.from_pos,
                effect.to_item_idx, effect.to_pos, effect.original_rt);
            ++out.acks_sent;
            ++out.effects_applied;
            break;
        case ItemMoveSideEffectKind::BroadcastMoveNack:
            sink.broadcast_move_nack(
                effect.from_item_idx, effect.from_pos,
                effect.to_item_idx, effect.to_pos,
                effect.original_rt, LEGACY_EITEMUSE_MOVE);
            ++out.nacks_sent;
            ++out.effects_applied;
            break;
        case ItemMoveSideEffectKind::SilentSkip:
            sink.silent_skip();
            ++out.silent_skips;
            ++out.effects_applied;
            break;
        }
    }
    // Legacy rt==99 branch: the data plane emits no effect entries;
    // the silent flag alone carries the branch (1:1 with
    // SilentRtNinetyNineProducesEmptyPlan).
    if (plan.silent && out.silent_skips == 0u) {
        sink.silent_skip();
        ++out.silent_skips;
    }
    out.ack_flag_consumed = plan.send_ack;
    out.nack_flag_consumed = plan.send_nack;
    out.silent_flag_consumed = plan.silent;
    return out;
}

}  // namespace mxh::server
