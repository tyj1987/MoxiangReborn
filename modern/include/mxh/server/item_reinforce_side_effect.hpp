// 1:1 side-effect-dispatcher port of
// CItemManager::MP_ITEM_REINFORCE_SYN from legacy
// [Server]Map/ItemManager.cpp:4776-4812.
//
// The legacy handler reinforces (socket-gem insertion) an item.
// The flow is:
//   1. FindUser(pmsg->dwObjectID) -> pPlayer (return if null).
//   2. rt = ReinforceItem(pPlayer, wTargetItemIdx, TargetPos,
//      JewelWhich, wJewelUnit)
//   3. rt == EI_TRUE (0): silent success.
//   4. rt == 99: send MP_ITEM_REINFORCE_FAILED_ACK (echo pmsg with
//      Protocol flipped).
//   5. otherwise: send MP_ITEM_REINFORCE_NACK with ECode = rt.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_REINFORCE_*
inline constexpr std::uint8_t LEGACY_MP_ITEM_REINFORCE_FAILED_ACK = 95u;
inline constexpr std::uint8_t LEGACY_MP_ITEM_REINFORCE_NACK      = 96u;

// 1:1 with legacy rt == 99 sentinel for failed reinforcement.
inline constexpr int LEGACY_REINFORCE_FAILED_RT = 99;

enum class ItemReinforceOutcome : std::uint8_t {
    SilentSuccess = 0,  // legacy rt == EI_TRUE (0)
    FailedAck     = 1,  // legacy rt == 99
    FailureNack   = 2,  // legacy rt != 0, != 99
    NoPlayer      = 3,  // legacy FindUser returned null
};

inline ItemReinforceOutcome classify_item_reinforce_outcome(
    int reinforce_rt) noexcept {
    if (reinforce_rt == 0) {
        return ItemReinforceOutcome::SilentSuccess;
    }
    if (reinforce_rt == LEGACY_REINFORCE_FAILED_RT) {
        return ItemReinforceOutcome::FailedAck;
    }
    return ItemReinforceOutcome::FailureNack;
}

enum class ItemReinforceSideEffectKind : std::uint8_t {
    SilentSuccess            = 0,
    BroadcastReinforceFailed = 1,  // legacy SendAckMsg(REINFORCE_FAILED_ACK)
    BroadcastReinforceNack   = 2,  // legacy SendErrorMsg(REINFORCE_NACK, rt)
};

struct ItemReinforceSideEffect final {
    ItemReinforceSideEffectKind kind =
        ItemReinforceSideEffectKind::SilentSuccess;
    std::uint16_t target_item_idx = 0;  // legacy pmsg->wTargetItemIdx
    std::uint16_t target_pos = 0;      // legacy pmsg->TargetPos
    int jewel_which = 0;                // legacy pmsg->JewelWhich
    std::uint16_t jewel_unit = 0;       // legacy pmsg->wJewelUnit
    int error_code = 0;
    int original_rt = 0;
};

struct ItemReinforceSideEffectPlan final {
    std::vector<ItemReinforceSideEffect> effects;
    bool send_failed_ack = false;
    bool send_nack = false;
    int error_code = 0;
};

inline ItemReinforceSideEffectPlan item_reinforce_side_effect_plan(
    int reinforce_rt,
    std::uint16_t target_item_idx,
    std::uint16_t target_pos,
    int jewel_which,
    std::uint16_t jewel_unit) {
    ItemReinforceSideEffectPlan plan;
    plan.effects.reserve(1u);
    ItemReinforceSideEffect eff{};
    eff.target_item_idx = target_item_idx;
    eff.target_pos = target_pos;
    eff.jewel_which = jewel_which;
    eff.jewel_unit = jewel_unit;
    eff.original_rt = reinforce_rt;
    if (reinforce_rt == 0) {
        eff.kind = ItemReinforceSideEffectKind::SilentSuccess;
    } else if (reinforce_rt == LEGACY_REINFORCE_FAILED_RT) {
        plan.send_failed_ack = true;
        eff.kind = ItemReinforceSideEffectKind::BroadcastReinforceFailed;
    } else {
        plan.send_nack = true;
        eff.kind = ItemReinforceSideEffectKind::BroadcastReinforceNack;
        eff.error_code = reinforce_rt;
        plan.error_code = reinforce_rt;
    }
    plan.effects.push_back(eff);
    return plan;
}

}  // namespace mxh::server
