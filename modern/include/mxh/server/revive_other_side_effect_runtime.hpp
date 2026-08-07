// revive_other_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// revive_other_side_effect_plan(). The data plane returns a single
// NACK (3 gate categories with codes 2/7/3) or a 3-step success
// chain; this header walks the plan and dispatches each entry to a
// virtual ReviveOtherSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_SHOPITEM_REVIVEOTHER_SYN from
// [Server]Map/ItemManager.cpp:5190-5252):
//   - Gate 1: target not dead / not found -> REVIVEOTHER_NACK(70)
//     dwData = NotDead(2).
//   - Gate 2: siege-war observer + LimitLevel incantation -> NACK
//     dwData = NotReady(7).
//   - Gate 3: !IsUseAbleShopItem -> NACK dwData = NotUse(3).
//   - Success chain in legacy order: ForwardReviveOtherSyn to target
//     -> SetReviveData -> SetReviveTime(60000).
//
// Pattern mirrors revive_other_nack_side_effect_runtime.hpp (D4.87)
// and the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/revive_other_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the ReviveOther side-effect chain.
class ReviveOtherSideEffectSink {
public:
    virtual ~ReviveOtherSideEffectSink() = default;

    // Legacy: pTargetPlayer->SendMsg(REVIVEOTHER_SYN) with the
    // original dwData1/2/3 preserved.
    virtual void forward_revive_other_syn(
        std::uint32_t target_data1, std::uint16_t item_idx,
        std::uint16_t item_pos) = 0;

    // Legacy: pPlayer->SetReviveData(...).
    virtual void set_revive_data(std::uint32_t target_data1,
                                 std::uint16_t item_idx,
                                 std::uint16_t item_pos) = 0;

    // Legacy: pPlayer->SetReviveTime(60000).
    virtual void set_revive_time(std::uint32_t revive_time_ms) = 0;

    // Legacy: SendMsg(MP_ITEM_SHOPITEM_REVIVEOTHER_NACK, dwData =
    // nack_code).
    virtual void broadcast_revive_nack(
        std::uint32_t target_data1, std::uint16_t item_idx,
        std::uint16_t item_pos, std::uint32_t nack_code) = 0;
};

struct ReviveOtherRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t forwards        = 0;
    std::size_t revive_data_sets = 0;
    std::size_t revive_time_sets = 0;
    std::size_t nacks_sent      = 0;
    bool forward_flag_consumed = false;
    bool nack_flag_consumed    = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order.
inline ReviveOtherRuntimeOutcome apply_revive_other_side_effects(
    const ReviveOtherSideEffectPlan& plan,
    ReviveOtherSideEffectSink& sink) {
    ReviveOtherRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case ReviveOtherSideEffectKind::ForwardReviveOtherSyn:
            sink.forward_revive_other_syn(
                effect.target_data1, effect.item_idx, effect.item_pos);
            ++out.forwards;
            ++out.effects_applied;
            break;
        case ReviveOtherSideEffectKind::SetReviveData:
            sink.set_revive_data(
                effect.target_data1, effect.item_idx, effect.item_pos);
            ++out.revive_data_sets;
            ++out.effects_applied;
            break;
        case ReviveOtherSideEffectKind::SetReviveTime:
            sink.set_revive_time(effect.revive_time_ms);
            ++out.revive_time_sets;
            ++out.effects_applied;
            break;
        case ReviveOtherSideEffectKind::BroadcastReviveNack:
            sink.broadcast_revive_nack(
                effect.target_data1, effect.item_idx, effect.item_pos,
                effect.nack_code);
            ++out.nacks_sent;
            ++out.effects_applied;
            break;
        }
    }
    out.forward_flag_consumed = plan.forward_syn;
    out.nack_flag_consumed = plan.send_nack;
    return out;
}

}  // namespace mxh::server
