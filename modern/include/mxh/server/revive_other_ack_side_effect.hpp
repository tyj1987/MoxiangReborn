//
// CItemManager::MP_ITEM_SHOPITEM_REVIVEOTHER_ACK from legacy
// [Server]Map/ItemManager.cpp:5253-5352.
//
// The legacy handler processes the revival response from a dead
// teammate (who got the REVIVEOTHER_SYN forwarded by D4.56). The
// flow:
//   1. FindUser(pmsg->dwObjectID) -> pPlayer (resurrector).
//   2. FindUser(pmsg->dwData1) -> pTargetPlayer (the one whose
//      REVIVEOTHER_SYN was answered).
//   3. If pPlayer->GetState() != eObjectState_Die:
//      a. MSG_WORD {MP_ITEM, REVIVEOTHER_NACK, wData=NotDead} -> both.
//      b. pTargetPlayer->SetReviveData(0,0,0).
//      c. pTargetPlayer->SetReviveTime(0).
//      d. return.
//   4. Get pData = pTargetPlayer->GetReviveData() (saved by D4.56).
//   5. If IsUseAbleShopItem(pTargetPlayer, pData->ItemIdx,
//      pData->ItemPos) == FALSE:
//      a. MSG_WORD {MP_ITEM, REVIVEOTHER_NACK, wData=NotUse} -> target.
//      b. MSG_WORD {MP_ITEM, REVIVEOTHER_NACK, wData=Fail} -> resurrector.
//      c. SetReviveData(0,0,0); SetReviveTime(0); return.
//   6. pItemInfo = GetItemInfo(pData->ItemIdx).
//   7. If !pItemInfo or ItemKind != eSHOP_ITEM_INCANTATION or
//      LimitLevel == 0: jump to Revive_Failed (both NACK Fail).
//   8. If pPlayer->IsAbleReviveOther():
//      a. pShopItem = GetUsingItemInfo(pData->ItemIdx).
//      b. If !pShopItem:
//         i. If pItemInfo->SellPrice > 0: goto Revive_Failed.
//         ii. If DiscardItem(...) != EI_TRUE: goto Revive_Failed.
//      c. pPlayer->ReviveShopItem(pData->ItemIdx).
//      d. SEND_SHOPITEM_BASEINFO {MP_ITEM, USE_ACK, ShopItemPos,
//         ShopItemIdx} -> target.
//      e. MSG_DWORD {MP_ITEM, REVIVEOTHER_ACK, dwData=pPlayer->GetID()}
//         -> both.
//   9. else (Revive_Failed): MSG_WORD {MP_ITEM, REVIVEOTHER_NACK,
//      wData=Fail} -> both.
//  10. Always: SetReviveData(0,0,0); SetReviveTime(0).
//
// We split outcomes into 5 categories: NotDead, NotUsable, BadItemInfo,
// AlreadyUsed (time-limited item still in using list; legacy skips
// DiscardItem path), and the catch-all Fail (covers NoItemInfo,
// WrongKind, SellPriced, DiscardFailed, NotAbleToRevive).

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/CommonGameDefine.h eShopItemErr_Revive_*
inline constexpr std::uint32_t LEGACY_ESHOPITEM_REVIVE_FAIL    = 1u;
inline constexpr std::uint32_t LEGACY_ESHOPITEM_REVIVE_NOTDEAD = 2u;
inline constexpr std::uint32_t LEGACY_ESHOPITEM_REVIVE_NOTUSE  = 3u;

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_SHOPITEM_REVIVEOTHER_ACK.
inline constexpr std::uint8_t LEGACY_MP_ITEM_REVIVEOTHER_ACK = 71u;

// 1:1 with legacy [CC]Header/CommonGameDefine.h eObjectState_Die.
inline constexpr std::uint8_t LEGACY_EOBJECTSTATE_DIE = 2u;

// 1:1 with legacy [CC]Header/CommonGameDefine.h eSHOP_ITEM_INCANTATION.
inline constexpr std::uint8_t LEGACY_ESHOPITEM_INCANTATION = 5u;

// 1:1 with legacy EI_TRUE / eItemTable_Inventory return code (DiscardItem).
inline constexpr std::uint8_t LEGACY_EI_TRUE = 0u;

enum class ReviveOtherAckOutcome : std::uint8_t {
    Success      = 0,
    NotDead      = 1,
    NotUsable    = 2,
    BadItemInfo  = 3,
    Fail         = 4,
    AlreadyUsed  = 5,
};

struct ReviveOtherAckValidationInput final {
    bool resurrector_state_is_die = false;
    bool item_is_useable = false;
    bool item_info_exists = false;
    bool item_kind_is_incantation = false;
    bool item_limit_level_nonzero = false;
    bool item_in_using_list = false;
    bool item_sell_price_zero = false;
    bool discard_returned_true = false;
    bool resurrector_is_able = false;
};

inline ReviveOtherAckOutcome classify_revive_other_ack_outcome(
    const ReviveOtherAckValidationInput& in) noexcept {
    if (!in.resurrector_state_is_die) {
        return ReviveOtherAckOutcome::NotDead;
    }
    if (!in.item_is_useable) {
        return ReviveOtherAckOutcome::NotUsable;
    }
    if (!in.item_info_exists || !in.item_kind_is_incantation ||
        !in.item_limit_level_nonzero) {
        return ReviveOtherAckOutcome::BadItemInfo;
    }
    if (!in.resurrector_is_able) {
        return ReviveOtherAckOutcome::Fail;
    }
    if (in.item_in_using_list) {
        return ReviveOtherAckOutcome::AlreadyUsed;
    }
    if (!in.item_sell_price_zero) {
        return ReviveOtherAckOutcome::Fail;
    }
    if (!in.discard_returned_true) {
        return ReviveOtherAckOutcome::Fail;
    }
    return ReviveOtherAckOutcome::Success;
}

enum class ReviveOtherAckSideEffectKind : std::uint8_t {
    SendNotDeadNackToTarget        = 0,
    SendNotDeadNackToResurrector   = 1,
    SendNotUsableNackToTarget      = 2,
    SendNotUsableNackToResurrector = 3,
    SendFailedNackToTarget         = 4,
    SendFailedNackToResurrector    = 5,
    SendReviveAckToTarget          = 6,
    SendReviveAckToResurrector     = 7,
    SendUseAckToTarget             = 8,
    ReviveShopItemOnResurrector    = 9,
    DiscardShopItemFromTarget      = 10,
    ClearReviveDataOnTarget        = 11,
    ClearReviveTimeOnTarget        = 12,
};

struct ReviveOtherAckSideEffect final {
    ReviveOtherAckSideEffectKind kind =
        ReviveOtherAckSideEffectKind::SendReviveAckToTarget;
    std::uint32_t target_id = 0;
    std::uint32_t resurrector_id = 0;
    std::uint32_t nack_code = 0;
    std::uint16_t shop_item_idx = 0;
    std::uint16_t shop_item_pos = 0;
};

struct ReviveOtherAckSideEffectPlan final {
    std::vector<ReviveOtherAckSideEffect> effects;
    bool clear_revive_data = false;
    bool revive_shop_item  = false;
    bool discard_shop_item = false;
    bool send_use_ack_to_target = false;
    bool send_revive_ack = false;
    bool send_failed_nack = false;
    bool send_not_dead_nack = false;
    bool send_not_usable_nack = false;
};

inline void add_nack_pair(ReviveOtherAckSideEffectPlan& plan,
                          ReviveOtherAckSideEffectKind target_kind,
                          ReviveOtherAckSideEffectKind resurrector_kind,
                          std::uint32_t target_id,
                          std::uint32_t resurrector_id,
                          std::uint32_t nack_code) {
    plan.effects.reserve(plan.effects.size() + 2u);
    ReviveOtherAckSideEffect to_target{};
    to_target.kind = target_kind;
    to_target.target_id = target_id;
    to_target.resurrector_id = resurrector_id;
    to_target.nack_code = nack_code;
    plan.effects.push_back(to_target);
    ReviveOtherAckSideEffect to_resurrector{};
    to_resurrector.kind = resurrector_kind;
    to_resurrector.target_id = target_id;
    to_resurrector.resurrector_id = resurrector_id;
    to_resurrector.nack_code = nack_code;
    plan.effects.push_back(to_resurrector);
}

inline ReviveOtherAckSideEffectPlan revive_other_ack_side_effect_plan(
    const ReviveOtherAckValidationInput& in,
    std::uint32_t target_id,
    std::uint32_t resurrector_id,
    std::uint16_t shop_item_idx,
    std::uint16_t shop_item_pos) {
    ReviveOtherAckSideEffectPlan plan;
    plan.clear_revive_data = true;

    const ReviveOtherAckOutcome outcome =
        classify_revive_other_ack_outcome(in);

    if (outcome == ReviveOtherAckOutcome::NotDead) {
        plan.send_not_dead_nack = true;
        add_nack_pair(plan,
                      ReviveOtherAckSideEffectKind::SendNotDeadNackToTarget,
                      ReviveOtherAckSideEffectKind::SendNotDeadNackToResurrector,
                      target_id, resurrector_id,
                      LEGACY_ESHOPITEM_REVIVE_NOTDEAD);
        return plan;
    }
    if (outcome == ReviveOtherAckOutcome::NotUsable) {
        plan.send_not_usable_nack = true;
        plan.effects.reserve(plan.effects.size() + 2u);
        ReviveOtherAckSideEffect to_target{};
        to_target.kind =
            ReviveOtherAckSideEffectKind::SendNotUsableNackToTarget;
        to_target.target_id = target_id;
        to_target.resurrector_id = resurrector_id;
        to_target.nack_code = LEGACY_ESHOPITEM_REVIVE_NOTUSE;
        plan.effects.push_back(to_target);
        ReviveOtherAckSideEffect to_res{};
        to_res.kind =
            ReviveOtherAckSideEffectKind::SendNotUsableNackToResurrector;
        to_res.target_id = target_id;
        to_res.resurrector_id = resurrector_id;
        to_res.nack_code = LEGACY_ESHOPITEM_REVIVE_FAIL;
        plan.effects.push_back(to_res);
        return plan;
    }
    if (outcome == ReviveOtherAckOutcome::BadItemInfo ||
        outcome == ReviveOtherAckOutcome::Fail) {
        plan.send_failed_nack = true;
        add_nack_pair(plan,
                      ReviveOtherAckSideEffectKind::SendFailedNackToTarget,
                      ReviveOtherAckSideEffectKind::SendFailedNackToResurrector,
                      target_id, resurrector_id,
                      LEGACY_ESHOPITEM_REVIVE_FAIL);
        return plan;
    }

    plan.send_revive_ack = true;
    plan.revive_shop_item = true;
    if (outcome == ReviveOtherAckOutcome::Success) {
        plan.discard_shop_item = true;
    }
    plan.send_use_ack_to_target = true;

    plan.effects.reserve(plan.effects.size() + 4u);
    {
        ReviveOtherAckSideEffect discard_or_skip{};
        discard_or_skip.kind =
            ReviveOtherAckSideEffectKind::DiscardShopItemFromTarget;
        discard_or_skip.target_id = target_id;
        discard_or_skip.shop_item_idx = shop_item_idx;
        discard_or_skip.shop_item_pos = shop_item_pos;
        if (outcome == ReviveOtherAckOutcome::Success) {
            plan.effects.push_back(discard_or_skip);
        }
    }
    {
        ReviveOtherAckSideEffect revive{};
        revive.kind =
            ReviveOtherAckSideEffectKind::ReviveShopItemOnResurrector;
        revive.resurrector_id = resurrector_id;
        revive.shop_item_idx = shop_item_idx;
        plan.effects.push_back(revive);
    }
    {
        ReviveOtherAckSideEffect use_ack{};
        use_ack.kind = ReviveOtherAckSideEffectKind::SendUseAckToTarget;
        use_ack.target_id = target_id;
        use_ack.shop_item_idx = shop_item_idx;
        use_ack.shop_item_pos = shop_item_pos;
        plan.effects.push_back(use_ack);
    }
    {
        ReviveOtherAckSideEffect ack_to_target{};
        ack_to_target.kind =
            ReviveOtherAckSideEffectKind::SendReviveAckToTarget;
        ack_to_target.target_id = target_id;
        ack_to_target.resurrector_id = resurrector_id;
        plan.effects.push_back(ack_to_target);
    }
    {
        ReviveOtherAckSideEffect ack_to_res{};
        ack_to_res.kind =
            ReviveOtherAckSideEffectKind::SendReviveAckToResurrector;
        ack_to_res.target_id = target_id;
        ack_to_res.resurrector_id = resurrector_id;
        plan.effects.push_back(ack_to_res);
    }
    return plan;
}

}  // namespace mxh::server
