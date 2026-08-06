//
// CItemManager::MP_ITEM_SHOPITEM_AVATAR_USE_SYN from legacy
// [Server]Map/ItemManager.cpp:5410-5484.
//
// The legacy handler equips an avatar item with 5 gates:
//   1. pPlayer->GetState() must be eObjectState_None or _Immortal,
//      otherwise _Avataruse_failed.
//   2. IsUseAbleShopItem(pPlayer, wData1, wData2) must be true.
//   3. GetItemInfoAbsIn(pPlayer, wData2) must return non-null.
//   4. CheckWeaponToShopItem(pPlayer, wData1) must be true.
//   5. Branch by pItem = GetUsingItemInfo(wData1):
//      a. pItem == null: call IsUseAbleShopAvatarItem (async DB
//         query, returns nothing immediate).
//      b. pItem != null && DBIdx matches: PutOnAvatarItem, ACK on
//         success, NACK on failure.
//      c. pItem != null && DBIdx mismatch: NACK.
//
// _Avataruse_failed label: send USE_NACK (rewrites MSG_WORD2 protocol
// field in place, then sends).

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_SHOPITEM_AVATAR_USE_*.
inline constexpr std::uint8_t LEGACY_MP_ITEM_SHOPITEM_AVATAR_USE_ACK  = 89u;
inline constexpr std::uint8_t LEGACY_MP_ITEM_SHOPITEM_AVATAR_USE_NACK = 90u;

// 1:1 with legacy [CC]Header/CommonGameDefine.h eObjectState_None/_Immortal.
inline constexpr std::uint8_t LEGACY_EOBJECTSTATE_NONE     = 0u;
inline constexpr std::uint8_t LEGACY_EOBJECTSTATE_IMMORTAL = 4u;

enum class AvatarUseOutcome : std::uint8_t {
    Success           = 0,
    GateFailed        = 1,  // legacy _Avataruse_failed (state/use/item/weapon)
    UsingListMismatch = 2,  // legacy: pItem in using list, DBIdx != pItembase->dwDBIdx
    PutOnFailed       = 3,  // legacy: PutOnAvatarItem returned false
    AsyncDbQuery      = 4,  // legacy: not in using list -> IsUseAbleShopAvatarItem
};

struct AvatarUseValidationInput final {
    bool state_is_none_or_immortal = false;  // gate 1
    bool item_is_useable = false;            // gate 2 (IsUseAbleShopItem)
    bool item_base_exists = false;           // gate 3 (GetItemInfoAbsIn)
    bool weapon_to_shop_item_ok = false;     // gate 4 (CheckWeaponToShopItem)
    bool item_in_using_list = false;         // gate 5a (pItem != null)
    bool using_list_db_idx_matches = false;  // gate 5b (dwDBIdx == pItembase->dwDBIdx)
    bool put_on_avatar_item_ok = false;      // gate 5c (PutOnAvatarItem)
};

inline AvatarUseOutcome classify_avatar_use_outcome(
    const AvatarUseValidationInput& in) noexcept {
    if (!in.state_is_none_or_immortal ||
        !in.item_is_useable ||
        !in.item_base_exists ||
        !in.weapon_to_shop_item_ok) {
        return AvatarUseOutcome::GateFailed;
    }
    if (!in.item_in_using_list) {
        return AvatarUseOutcome::AsyncDbQuery;
    }
    if (!in.using_list_db_idx_matches) {
        return AvatarUseOutcome::UsingListMismatch;
    }
    if (!in.put_on_avatar_item_ok) {
        return AvatarUseOutcome::PutOnFailed;
    }
    return AvatarUseOutcome::Success;
}

enum class AvatarUseSideEffectKind : std::uint8_t {
    SendAckToPlayer       = 0,  // legacy MP_ITEM_SHOPITEM_AVATAR_USE_ACK
    SendNackToPlayer      = 1,  // legacy MP_ITEM_SHOPITEM_AVATAR_USE_NACK
    PutOnAvatarItem       = 2,  // legacy ShopItemManager->PutOnAvatarItem
    QueryDbForAvatarItem  = 3,  // legacy IsUseAbleShopAvatarItem (async)
};

struct AvatarUseSideEffect final {
    AvatarUseSideEffectKind kind =
        AvatarUseSideEffectKind::SendAckToPlayer;
    std::uint32_t player_id = 0;       // pPlayer->GetID()
    std::uint16_t item_idx = 0;        // wData1 (ShopItemIdx)
    std::uint16_t item_pos = 0;        // wData2 (ShopItemPos)
    std::uint32_t item_db_idx = 0;     // pItembase->dwDBIdx (for DB query)
    std::uint32_t item_icon_idx = 0;   // pItembase->wIconIdx (for DB query)
    std::uint32_t item_position = 0;   // pItembase->Position (for DB query)
};

struct AvatarUseSideEffectPlan final {
    std::vector<AvatarUseSideEffect> effects;
    bool send_ack = false;
    bool send_nack = false;
    bool put_on_avatar_item = false;
    bool query_db = false;
};

inline AvatarUseSideEffectPlan avatar_use_side_effect_plan(
    const AvatarUseValidationInput& in,
    std::uint32_t player_id,
    std::uint16_t item_idx,
    std::uint16_t item_pos,
    std::uint32_t item_db_idx,
    std::uint32_t item_icon_idx,
    std::uint32_t item_position) {
    AvatarUseSideEffectPlan plan;
    const AvatarUseOutcome outcome = classify_avatar_use_outcome(in);

    if (outcome == AvatarUseOutcome::Success) {
        plan.send_ack = true;
        plan.put_on_avatar_item = true;
        plan.effects.reserve(2u);
        AvatarUseSideEffect put{};
        put.kind = AvatarUseSideEffectKind::PutOnAvatarItem;
        put.player_id = player_id;
        put.item_idx = item_idx;
        put.item_pos = item_pos;
        plan.effects.push_back(put);
        AvatarUseSideEffect ack{};
        ack.kind = AvatarUseSideEffectKind::SendAckToPlayer;
        ack.player_id = player_id;
        ack.item_idx = item_idx;
        ack.item_pos = item_pos;
        plan.effects.push_back(ack);
        return plan;
    }
    if (outcome == AvatarUseOutcome::AsyncDbQuery) {
        plan.query_db = true;
        plan.effects.reserve(1u);
        AvatarUseSideEffect q{};
        q.kind = AvatarUseSideEffectKind::QueryDbForAvatarItem;
        q.player_id = player_id;
        q.item_db_idx = item_db_idx;
        q.item_icon_idx = item_icon_idx;
        q.item_position = item_position;
        plan.effects.push_back(q);
        return plan;
    }
    plan.send_nack = true;
    plan.effects.reserve(1u);
    AvatarUseSideEffect nack{};
    nack.kind = AvatarUseSideEffectKind::SendNackToPlayer;
    nack.player_id = player_id;
    nack.item_idx = item_idx;
    nack.item_pos = item_pos;
    plan.effects.push_back(nack);
    return plan;
}

}  // namespace mxh::server
