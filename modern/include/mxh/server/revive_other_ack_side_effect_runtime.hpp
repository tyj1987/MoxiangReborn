// revive_other_ack_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// revive_other_ack_side_effect_plan(). The data plane returns either a
// NACK pair (NotDead / NotUsable / Fail), the success chain (with or
// without the initial DiscardShopItemFromTarget), or an empty plan;
// this header walks the plan and dispatches each entry to a virtual
// ReviveOtherAckSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_SHOPITEM_REVIVEOTHER_ACK from
// [Server]Map/ItemManager.cpp:5253-5352):
//   - NotDead: MSG_WORD REVIVEOTHER_NACK wData=NotDead(2) -> both,
//     then clear revive data/time.
//   - NotUsable: NACK wData=NotUse(3) -> target, NACK wData=Fail(1) ->
//     resurrector, then clear.
//   - BadItemInfo / Fail: NACK wData=Fail(1) -> both, then clear.
//   - Success / AlreadyUsed: DiscardShopItemFromTarget (success only)
//     -> ReviveShopItemOnResurrector -> SendUseAckToTarget ->
//     SendReviveAckToTarget -> SendReviveAckToResurrector, then clear.
//   - Legacy "Always" step: SetReviveData(0,0,0) + SetReviveTime(0)
//     runs for every non-empty branch; the runtime emits
//     clear_revive_data(target_id) when plan.clear_revive_data is set.
//
// Pattern mirrors shop_item_seal_side_effect_runtime.hpp (D4.72) and
// the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/revive_other_ack_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ReviveOtherAck side-effect chain.
class ReviveOtherAckSideEffectSink {
public:
    virtual ~ReviveOtherAckSideEffectSink() = default;

    // Legacy: SendMsg(REVIVEOTHER_NACK, wData=NotDead) -> target.
    virtual void send_not_dead_nack_to_target(
        std::uint32_t target_id, std::uint32_t resurrector_id,
        std::uint32_t nack_code) = 0;
    // Legacy: same NACK -> resurrector.
    virtual void send_not_dead_nack_to_resurrector(
        std::uint32_t target_id, std::uint32_t resurrector_id,
        std::uint32_t nack_code) = 0;
    // Legacy: SendMsg(REVIVEOTHER_NACK, wData=NotUse) -> target.
    virtual void send_not_usable_nack_to_target(
        std::uint32_t target_id, std::uint32_t resurrector_id,
        std::uint32_t nack_code) = 0;
    // Legacy: SendMsg(REVIVEOTHER_NACK, wData=Fail) -> resurrector.
    virtual void send_not_usable_nack_to_resurrector(
        std::uint32_t target_id, std::uint32_t resurrector_id,
        std::uint32_t nack_code) = 0;
    // Legacy: SendMsg(REVIVEOTHER_NACK, wData=Fail) -> target.
    virtual void send_failed_nack_to_target(
        std::uint32_t target_id, std::uint32_t resurrector_id,
        std::uint32_t nack_code) = 0;
    // Legacy: SendMsg(REVIVEOTHER_NACK, wData=Fail) -> resurrector.
    virtual void send_failed_nack_to_resurrector(
        std::uint32_t target_id, std::uint32_t resurrector_id,
        std::uint32_t nack_code) = 0;
    // Legacy: SendMsg(MSG_DWORD, REVIVEOTHER_ACK, dwData=resurrector_id)
    // -> target.
    virtual void send_revive_ack_to_target(
        std::uint32_t target_id, std::uint32_t resurrector_id) = 0;
    // Legacy: same ACK -> resurrector.
    virtual void send_revive_ack_to_resurrector(
        std::uint32_t target_id, std::uint32_t resurrector_id) = 0;
    // Legacy: SendMsg(SEND_SHOPITEM_BASEINFO, USE_ACK, ShopItemPos,
    // ShopItemIdx) -> target.
    virtual void send_use_ack_to_target(
        std::uint32_t target_id, std::uint16_t shop_item_idx,
        std::uint16_t shop_item_pos) = 0;
    // Legacy: pPlayer->ReviveShopItem(ItemIdx) on the resurrector.
    virtual void revive_shop_item_on_resurrector(
        std::uint32_t resurrector_id, std::uint16_t shop_item_idx) = 0;
    // Legacy: DiscardItem(...) on the target (success path only).
    virtual void discard_shop_item_from_target(
        std::uint32_t target_id, std::uint16_t shop_item_idx,
        std::uint16_t shop_item_pos) = 0;
    // Legacy "Always" step: SetReviveData(0,0,0) + SetReviveTime(0) on
    // the target. Fires for every non-empty branch.
    virtual void clear_revive_data(std::uint32_t target_id) = 0;
};

struct ReviveOtherAckRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t nacks_target    = 0;
    std::size_t nacks_resurrector = 0;
    std::size_t revive_acks     = 0;
    std::size_t use_acks        = 0;
    std::size_t revives         = 0;
    std::size_t discards        = 0;
    std::size_t clears          = 0;
    bool clear_flag_consumed    = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order,
// then reports the always-clear step.
inline ReviveOtherAckRuntimeOutcome apply_revive_other_ack_side_effects(
    const ReviveOtherAckSideEffectPlan& plan,
    ReviveOtherAckSideEffectSink& sink) {
    ReviveOtherAckRuntimeOutcome out;
    std::uint32_t last_target_id = 0;
    for (const auto& effect : plan.effects) {
        if (effect.target_id != 0) {
            last_target_id = effect.target_id;
        }
        switch (effect.kind) {
        case ReviveOtherAckSideEffectKind::SendNotDeadNackToTarget:
            sink.send_not_dead_nack_to_target(
                effect.target_id, effect.resurrector_id,
                effect.nack_code);
            ++out.nacks_target;
            ++out.effects_applied;
            break;
        case ReviveOtherAckSideEffectKind::SendNotDeadNackToResurrector:
            sink.send_not_dead_nack_to_resurrector(
                effect.target_id, effect.resurrector_id,
                effect.nack_code);
            ++out.nacks_resurrector;
            ++out.effects_applied;
            break;
        case ReviveOtherAckSideEffectKind::SendNotUsableNackToTarget:
            sink.send_not_usable_nack_to_target(
                effect.target_id, effect.resurrector_id,
                effect.nack_code);
            ++out.nacks_target;
            ++out.effects_applied;
            break;
        case ReviveOtherAckSideEffectKind::SendNotUsableNackToResurrector:
            sink.send_not_usable_nack_to_resurrector(
                effect.target_id, effect.resurrector_id,
                effect.nack_code);
            ++out.nacks_resurrector;
            ++out.effects_applied;
            break;
        case ReviveOtherAckSideEffectKind::SendFailedNackToTarget:
            sink.send_failed_nack_to_target(
                effect.target_id, effect.resurrector_id,
                effect.nack_code);
            ++out.nacks_target;
            ++out.effects_applied;
            break;
        case ReviveOtherAckSideEffectKind::SendFailedNackToResurrector:
            sink.send_failed_nack_to_resurrector(
                effect.target_id, effect.resurrector_id,
                effect.nack_code);
            ++out.nacks_resurrector;
            ++out.effects_applied;
            break;
        case ReviveOtherAckSideEffectKind::SendReviveAckToTarget:
            sink.send_revive_ack_to_target(
                effect.target_id, effect.resurrector_id);
            ++out.revive_acks;
            ++out.effects_applied;
            break;
        case ReviveOtherAckSideEffectKind::SendReviveAckToResurrector:
            sink.send_revive_ack_to_resurrector(
                effect.target_id, effect.resurrector_id);
            ++out.revive_acks;
            ++out.effects_applied;
            break;
        case ReviveOtherAckSideEffectKind::SendUseAckToTarget:
            sink.send_use_ack_to_target(
                effect.target_id, effect.shop_item_idx,
                effect.shop_item_pos);
            ++out.use_acks;
            ++out.effects_applied;
            break;
        case ReviveOtherAckSideEffectKind::ReviveShopItemOnResurrector:
            sink.revive_shop_item_on_resurrector(
                effect.resurrector_id, effect.shop_item_idx);
            ++out.revives;
            ++out.effects_applied;
            break;
        case ReviveOtherAckSideEffectKind::DiscardShopItemFromTarget:
            sink.discard_shop_item_from_target(
                effect.target_id, effect.shop_item_idx,
                effect.shop_item_pos);
            ++out.discards;
            ++out.effects_applied;
            break;
        case ReviveOtherAckSideEffectKind::ClearReviveDataOnTarget:
        case ReviveOtherAckSideEffectKind::ClearReviveTimeOnTarget:
            // The data plane carries these as the plan flag, not as
            // effects; handled below for forward compatibility.
            ++out.effects_applied;
            break;
        }
    }
    if (plan.clear_revive_data) {
        sink.clear_revive_data(last_target_id);
        ++out.clears;
    }
    out.clear_flag_consumed = plan.clear_revive_data;
    return out;
}

}  // namespace mxh::server
