// item_unseal_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// item_unseal_side_effect_plan(). The data plane returns a
// single-step plan (BroadcastUnsealAck or BroadcastUnsealNack based
// on the legacy ItemUnsealing result); this header walks the plan and
// dispatches the single entry to a virtual ItemUnsealSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_SHOPITEM_UNSEAL_SYN from
// [Server]Map/ItemManager.cpp:5064-5085):
//   - ItemUnsealing succeeds: legacy mutates msg.Protocol to
//     MP_ITEM_SHOPITEM_UNSEAL_ACK (68) and sends it.
//   - ItemUnsealing fails: legacy mutates msg.Protocol to
//     MP_ITEM_SHOPITEM_UNSEAL_NACK (69) and sends it.
//   - msg.dwData is preserved across the protocol flip in both
//     branches (dwData = pmsg->dwData).
//
// Pattern mirrors item_dissolution_side_effect_runtime.hpp (D4.53)
// and the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/item_unseal_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ItemUnseal side-effect chain.
class ItemUnsealSideEffectSink {
public:
    virtual ~ItemUnsealSideEffectSink() = default;

    // Legacy: Protocol-flipped MSG with MP_ITEM_SHOPITEM_UNSEAL_ACK.
    // dw_data (the preserved pmsg->dwData) and target_pos (the
    // POSTYPE derived from dwData) let the caller rebuild the wire
    // message.
    virtual void broadcast_unseal_ack(std::uint32_t dw_data,
                                      std::uint16_t target_pos) = 0;

    // Legacy: Protocol-flipped MSG with MP_ITEM_SHOPITEM_UNSEAL_NACK.
    virtual void broadcast_unseal_nack(std::uint32_t dw_data,
                                       std::uint16_t target_pos) = 0;
};

struct ItemUnsealRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t acks_sent       = 0;
    std::size_t nacks_sent      = 0;
    bool ack_flag_consumed   = false;
    bool nack_flag_consumed  = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline ItemUnsealRuntimeOutcome apply_item_unseal_side_effects(
    const ItemUnsealSideEffectPlan& plan,
    ItemUnsealSideEffectSink& sink) {
    ItemUnsealRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ItemUnsealSideEffectKind::BroadcastUnsealAck:
            sink.broadcast_unseal_ack(effect.dw_data,
                                      effect.target_pos);
            ++out.acks_sent;
            ++out.effects_applied;
            break;
        case ItemUnsealSideEffectKind::BroadcastUnsealNack:
            sink.broadcast_unseal_nack(effect.dw_data,
                                       effect.target_pos);
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
