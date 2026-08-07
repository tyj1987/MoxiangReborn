// item_upgrade_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// item_upgrade_side_effect_plan(). The data plane returns a
// single-step plan (BroadcastUpgradeSuccessAck or
// BroadcastUpgradeErrorNack based on the legacy UpgradeItem return
// code); this header walks the plan and dispatches the single entry
// to a virtual ItemUpgradeSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::MP_ITEM_UPGRADE_SYN
// from [Server]Map/ItemManager.cpp:4751-4775):
//   - UpgradeItem returns EI_TRUE (0): legacy echoes the original
//     pmsg as MP_ITEM_UPGRADE_SUCCESS_ACK (94) with Protocol flipped.
//   - UpgradeItem returns non-zero: legacy sends MSG_ITEM_ERROR with
//     Protocol = MP_ITEM_ERROR_NACK (99), ECode = eItemUseErr_Upgrade
//     (= 10).
//   - (The data plane keys solely off rt; the caller resolves the
//     player before building the plan.)
//
// Pattern mirrors item_combine_side_effect_runtime.hpp (D4.50) and
// the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/item_upgrade_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ItemUpgrade side-effect chain.
class ItemUpgradeSideEffectSink {
public:
    virtual ~ItemUpgradeSideEffectSink() = default;

    // Legacy: SendAckMsg(MP_ITEM_UPGRADE_SUCCESS_ACK) -- echoes pmsg
    // with the protocol byte flipped to the success ACK.
    virtual void broadcast_upgrade_success_ack(
        std::uint16_t item_idx, std::uint16_t item_pos,
        std::uint16_t material_item_idx, std::uint16_t material_item_pos,
        int original_rt) = 0;

    // Legacy: SendErrorMsg(MP_ITEM_ERROR_NACK, ECode=eItemUseErr_Upgrade)
    // -- sends the fixed upgrade error code.
    virtual void broadcast_upgrade_error_nack(
        std::uint16_t item_idx, std::uint16_t item_pos,
        std::uint16_t material_item_idx, std::uint16_t material_item_pos,
        int original_rt, int error_code) = 0;
};

struct ItemUpgradeRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t acks_sent       = 0;
    std::size_t nacks_sent      = 0;
    bool ack_flag_consumed   = false;
    bool nack_flag_consumed  = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline ItemUpgradeRuntimeOutcome apply_item_upgrade_side_effects(
    const ItemUpgradeSideEffectPlan& plan,
    ItemUpgradeSideEffectSink& sink) {
    ItemUpgradeRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ItemUpgradeSideEffectKind::BroadcastUpgradeSuccessAck:
            sink.broadcast_upgrade_success_ack(
                effect.item_idx, effect.item_pos,
                effect.material_item_idx, effect.material_item_pos,
                effect.original_rt);
            ++out.acks_sent;
            ++out.effects_applied;
            break;
        case ItemUpgradeSideEffectKind::BroadcastUpgradeErrorNack:
            sink.broadcast_upgrade_error_nack(
                effect.item_idx, effect.item_pos,
                effect.material_item_idx, effect.material_item_pos,
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
