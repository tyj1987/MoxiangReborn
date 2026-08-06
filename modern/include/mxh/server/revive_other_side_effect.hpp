// 1:1 side-effect-dispatcher port of CItemManager's
// MP_ITEM_SHOPITEM_REVIVEOTHER_SYN handler from legacy
// [Server]Map/ItemManager.cpp:5190-5252.
//
// The legacy handler runs 3 gates in order and routes to one of 4
// branches:
//   1. Target not dead (or not found): NACK with dwData =
//      eShopItemErr_Revive_NotDead (= 2).
//   2. Siege-war observer team + LimitLevel incantation: NACK with
//      dwData = eShopItemErr_Revive_NotReady (= 7).
//   3. IsUseAbleShopItem fails: NACK with dwData =
//      eShopItemErr_Revive_NotUse (= 3).
//   4. IsUseAbleShopItem success: forward REVIVEOTHER_SYN to the
//      target + SetReviveData + SetReviveTime(60000).

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/CommonGameDefine.h eShopItemErr_Revive_*.
inline constexpr std::uint32_t LEGACY_ESHOPITEM_REVIVE_NOTDEAD  = 2u;
inline constexpr std::uint32_t LEGACY_ESHOPITEM_REVIVE_NOTUSE  = 3u;
inline constexpr std::uint32_t LEGACY_ESHOPITEM_REVIVE_NOTREADY = 7u;

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_SHOPITEM_REVIVEOTHER_*
// and the legacy 60-second revive-time constant.
inline constexpr std::uint8_t LEGACY_MP_ITEM_REVIVEOTHER_NACK = 70u;
inline constexpr std::uint32_t LEGACY_REVIVETIME_60SEC_MS = 60000u;

enum class ReviveOtherOutcome : std::uint8_t {
    Success        = 0,  // legacy: target dead + IsUseAbleShopItem ok
    NotDead        = 1,  // legacy: target not dead or not found
    NotReady       = 2,  // legacy: siege war + observer + LimitLevel
    NotUsable      = 3,  // legacy: IsUseAbleShopItem returned false
};

struct ReviveOtherValidationInput final {
    bool target_found = false;
    bool target_is_dead = false;
    bool siege_war_active = false;
    bool observer_team = false;
    bool incantation_limit_level = false;
    bool is_useable_shop_item = false;
};

inline ReviveOtherOutcome classify_revive_other_outcome(
    const ReviveOtherValidationInput& in) noexcept {
    if (!in.target_found || !in.target_is_dead) {
        return ReviveOtherOutcome::NotDead;
    }
    if (in.siege_war_active && in.observer_team &&
        in.incantation_limit_level) {
        return ReviveOtherOutcome::NotReady;
    }
    if (!in.is_useable_shop_item) {
        return ReviveOtherOutcome::NotUsable;
    }
    return ReviveOtherOutcome::Success;
}

enum class ReviveOtherSideEffectKind : std::uint8_t {
    ForwardReviveOtherSyn = 0,  // legacy pTargetPlayer->SendMsg(REVIVEOTHER_SYN)
    SetReviveData         = 1,  // legacy pPlayer->SetReviveData
    SetReviveTime         = 2,  // legacy pPlayer->SetReviveTime(60000)
    BroadcastReviveNack   = 3,  // legacy SendMsg(MP_ITEM_SHOPITEM_REVIVEOTHER_NACK)
};

struct ReviveOtherSideEffect final {
    ReviveOtherSideEffectKind kind =
        ReviveOtherSideEffectKind::ForwardReviveOtherSyn;
    std::uint32_t target_id = 0;       // legacy msg.TargetID
    std::uint32_t target_data1 = 0;    // legacy pmsg->dwData1 (target player id)
    std::uint16_t item_idx = 0;        // legacy pmsg->dwData2 (cast to WORD)
    std::uint16_t item_pos = 0;        // legacy pmsg->dwData3 (cast to POSTYPE)
    std::uint32_t revive_time_ms = 0;  // legacy SetReviveTime value
    std::uint32_t nack_code = 0;       // legacy msg.dwData
};

struct ReviveOtherSideEffectPlan final {
    std::vector<ReviveOtherSideEffect> effects;
    bool forward_syn = false;
    bool send_nack = false;
    std::uint32_t nack_code = 0;
};

inline ReviveOtherSideEffectPlan revive_other_side_effect_plan(
    const ReviveOtherValidationInput& in,
    std::uint32_t target_data1,
    std::uint16_t item_idx,
    std::uint16_t item_pos) {
    ReviveOtherSideEffectPlan plan;
    const ReviveOtherOutcome outcome = classify_revive_other_outcome(in);
    if (outcome == ReviveOtherOutcome::Success) {
        plan.forward_syn = true;
        plan.effects.reserve(3u);

        ReviveOtherSideEffect forward{};
        forward.kind = ReviveOtherSideEffectKind::ForwardReviveOtherSyn;
        forward.target_data1 = target_data1;
        forward.item_idx = item_idx;
        forward.item_pos = item_pos;
        plan.effects.push_back(forward);

        ReviveOtherSideEffect data{};
        data.kind = ReviveOtherSideEffectKind::SetReviveData;
        data.target_data1 = target_data1;
        data.item_idx = item_idx;
        data.item_pos = item_pos;
        plan.effects.push_back(data);

        ReviveOtherSideEffect time{};
        time.kind = ReviveOtherSideEffectKind::SetReviveTime;
        time.revive_time_ms = LEGACY_REVIVETIME_60SEC_MS;
        plan.effects.push_back(time);
        return plan;
    }
    plan.send_nack = true;
    plan.effects.reserve(1u);
    ReviveOtherSideEffect nack{};
    nack.kind = ReviveOtherSideEffectKind::BroadcastReviveNack;
    nack.target_data1 = target_data1;
    nack.item_idx = item_idx;
    nack.item_pos = item_pos;
    switch (outcome) {
    case ReviveOtherOutcome::NotDead:
        nack.nack_code = LEGACY_ESHOPITEM_REVIVE_NOTDEAD;
        break;
    case ReviveOtherOutcome::NotReady:
        nack.nack_code = LEGACY_ESHOPITEM_REVIVE_NOTREADY;
        break;
    case ReviveOtherOutcome::NotUsable:
        nack.nack_code = LEGACY_ESHOPITEM_REVIVE_NOTUSE;
        break;
    default:
        break;
    }
    plan.nack_code = nack.nack_code;
    plan.effects.push_back(nack);
    return plan;
}

}  // namespace mxh::server
