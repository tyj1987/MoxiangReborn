// 1:1 side-effect-dispatcher port of
// CItemManager::MP_ITEM_UPGRADE_SYN from legacy
// [Server]Map/ItemManager.cpp:4751-4775.
//
// The legacy handler upgrades an item. The flow is:
//   1. FindUser(pmsg->dwObjectID) -> pPlayer (return if null).
//   2. rt = UpgradeItem(pPlayer, wItemIdx, ItemPos, wMaterialItemIdx,
//      MaterialItemPos)
//   3. rt == EI_TRUE: send MP_ITEM_UPGRADE_SUCCESS_ACK (echo of
//      the original pmsg with Protocol flipped).
//   4. rt != EI_TRUE: send MP_ITEM_ERROR_NACK with ECode =
//      eItemUseErr_Upgrade (= 10 in legacy eItemUse_Err enum).

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_UPGRADE_SUCCESS_ACK.
inline constexpr std::uint8_t LEGACY_MP_ITEM_UPGRADE_SUCCESS_ACK = 94u;
// 1:1 with legacy MP_ITEM_ERROR_NACK shared across item handlers.
inline constexpr std::uint8_t LEGACY_MP_ITEM_ERROR_NACK         = 99u;

// 1:1 with legacy eItemUseErr_Upgrade = 10 in eItemUse_Err.
inline constexpr int LEGACY_EITEMUSE_UPGRADE = 10;

enum class ItemUpgradeOutcome : std::uint8_t {
    Success = 0,  // legacy rt == EI_TRUE (0)
    Failure = 1,  // legacy rt != EI_TRUE
    NoPlayer = 2, // legacy FindUser returned null
};

inline ItemUpgradeOutcome classify_item_upgrade_outcome(
    int upgrade_rt) noexcept {
    if (upgrade_rt == 0) {
        return ItemUpgradeOutcome::Success;
    }
    return ItemUpgradeOutcome::Failure;
}

enum class ItemUpgradeSideEffectKind : std::uint8_t {
    BroadcastUpgradeSuccessAck = 0,  // legacy SendAckMsg
    BroadcastUpgradeErrorNack  = 1,  // legacy SendErrorMsg
};

struct ItemUpgradeSideEffect final {
    ItemUpgradeSideEffectKind kind =
        ItemUpgradeSideEffectKind::BroadcastUpgradeSuccessAck;
    std::uint16_t item_idx = 0;          // legacy pmsg->wItemIdx
    std::uint16_t item_pos = 0;          // legacy pmsg->ItemPos
    std::uint16_t material_item_idx = 0; // legacy pmsg->wMaterialItemIdx
    std::uint16_t material_item_pos = 0; // legacy pmsg->MaterialItemPos
    int error_code = 0;
    int original_rt = 0;
};

struct ItemUpgradeSideEffectPlan final {
    std::vector<ItemUpgradeSideEffect> effects;
    bool send_ack = false;
    bool send_nack = false;
    int error_code = 0;
};

inline ItemUpgradeSideEffectPlan item_upgrade_side_effect_plan(
    int upgrade_rt,
    std::uint16_t item_idx,
    std::uint16_t item_pos,
    std::uint16_t material_item_idx,
    std::uint16_t material_item_pos) {
    ItemUpgradeSideEffectPlan plan;
    plan.effects.reserve(1u);
    ItemUpgradeSideEffect eff{};
    eff.item_idx = item_idx;
    eff.item_pos = item_pos;
    eff.material_item_idx = material_item_idx;
    eff.material_item_pos = material_item_pos;
    eff.original_rt = upgrade_rt;
    if (upgrade_rt == 0) {
        plan.send_ack = true;
        eff.kind = ItemUpgradeSideEffectKind::BroadcastUpgradeSuccessAck;
    } else {
        plan.send_nack = true;
        eff.kind = ItemUpgradeSideEffectKind::BroadcastUpgradeErrorNack;
        eff.error_code = LEGACY_EITEMUSE_UPGRADE;
        plan.error_code = LEGACY_EITEMUSE_UPGRADE;
    }
    plan.effects.push_back(eff);
    return plan;
}

}  // namespace mxh::server
