//
// CItemManager::MP_ITEM_SHOPITEM_CHARCHANGE_SYN from legacy
// [Server]Map/ItemManager.cpp:5577-5674.
//
// The legacy handler runs 6 gates with NACK codes 1..6:
//   1. ShopItemStats->Avatar[i] all zero for i in [0, eAvatar_Effect)
//      (no avatar effect active) - code 6 if any set.
//   2. pItem exists and is eIncantation_CharChange / _ShapeChange -
//      code 1.
//   3. Info.Height in [0.9, 1.1] and Info.Width in [0.9, 1.1] -
//      code 2.
//   4. Info.Gender in [0, 2] - code 3.
//   5. Info.HairType in [0, 4] and Info.FaceType in [0, 4] - code 4.
//   6. DiscardItem returns EI_TRUE - code 5.
//
// On success:
//   - pPlayer->SetCharChangeInfo(&pmsg->Info).
//   - Send USE_ACK (SEND_SHOPITEM_BASEINFO {USE_ACK, ShopItemPos,
//     ShopItemIdx}).
//   - Build SEND_CHARACTERCHANGE_INFO {CHARCHANGE, player id, Info}
//     and QuickSend (broadcast); if ShapeChange, override Info's
//     Gender/Height/Width with the saved CTInfo values.
//   - CharacterChangeInfoToDB (varies by item kind).
//   - Send CHARCHANGE_ACK (MSGBASE).
//   - LogItemMoney (eLog_ShopItemUse).

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/CommonGameDefine.h eIncantation_CharChange / _ShapeChange.
inline constexpr std::uint32_t LEGACY_EINCANTATION_CHARCHANGE    = 37u;
inline constexpr std::uint32_t LEGACY_EINCANTATION_SHAPECHANGE   = 38u;

// 1:1 with legacy [CC]Header/CommonGameDefine.h eAvatar_Effect count.
inline constexpr std::uint32_t LEGACY_EAVATAR_EFFECT_COUNT = 8u;

// Bounds used by the legacy gates.
inline constexpr float LEGACY_CHARSHAPE_MIN_LOW  = 0.9f;
inline constexpr float LEGACY_CHARSHAPE_MAX_HIGH = 1.1f;
inline constexpr std::uint32_t LEGACY_CHARGENDER_MAX = 2u;
inline constexpr std::uint32_t LEGACY_CHARHAIRFACE_MAX = 4u;

enum class CharChangeOutcome : std::uint8_t {
    Success       = 0,
    AvatarEffect  = 6,  // legacy NACK code = 6
    BadItem       = 1,
    BadShape      = 2,  // height/width out of [0.9, 1.1]
    BadGender     = 3,
    BadHairFace   = 4,
    DiscardFailed = 5,
};

enum class CharChangeIcon : std::uint8_t {
    CharChange  = 0,  // eIncantation_CharChange
    ShapeChange = 1,  // eIncantation_ShapeChange
};

struct CharChangeValidationInput final {
    bool avatar_effect_clear = true;       // gate 1
    bool item_exists = false;              // gate 2
    bool item_icon_is_char_or_shape = false;
    bool height_in_range = false;          // gate 3
    bool width_in_range = false;
    bool gender_in_range = false;          // gate 4
    bool hair_face_in_range = false;       // gate 5
    bool discard_returned_true = false;    // gate 6
};

inline CharChangeOutcome classify_char_change_outcome(
    const CharChangeValidationInput& in) noexcept {
    if (!in.avatar_effect_clear) {
        return CharChangeOutcome::AvatarEffect;
    }
    if (!in.item_exists || !in.item_icon_is_char_or_shape) {
        return CharChangeOutcome::BadItem;
    }
    if (!in.height_in_range || !in.width_in_range) {
        return CharChangeOutcome::BadShape;
    }
    if (!in.gender_in_range) {
        return CharChangeOutcome::BadGender;
    }
    if (!in.hair_face_in_range) {
        return CharChangeOutcome::BadHairFace;
    }
    if (!in.discard_returned_true) {
        return CharChangeOutcome::DiscardFailed;
    }
    return CharChangeOutcome::Success;
}

inline std::uint8_t char_change_nack_code(CharChangeOutcome o) noexcept {
    switch (o) {
        case CharChangeOutcome::BadItem:       return 1u;
        case CharChangeOutcome::BadShape:      return 2u;
        case CharChangeOutcome::BadGender:     return 3u;
        case CharChangeOutcome::BadHairFace:   return 4u;
        case CharChangeOutcome::DiscardFailed: return 5u;
        case CharChangeOutcome::AvatarEffect:  return 6u;
        default:                              return 0u;
    }
}

enum class CharChangeSideEffectKind : std::uint8_t {
    SendNackToPlayer         = 0,
    SendUseAckToPlayer       = 1,  // SEND_SHOPITEM_BASEINFO USE_ACK
    DiscardCharChangeItem    = 2,
    SetCharChangeInfo        = 3,
    BroadcastCharChange      = 4,  // SEND_CHARACTERCHANGE_INFO CHARCHANGE
    CharacterChangeInfoToDB  = 5,  // DB
    SendCharChangeAck        = 6,  // MSGBASE CHARCHANGE_ACK
    LogItemMoney             = 7,
};

struct CharChangeValidationFields final {
    float height = 1.0f;
    float width  = 1.0f;
    std::uint8_t gender = 0;
    std::uint8_t hair_type = 0;
    std::uint8_t face_type = 0;
};

struct CharChangeSideEffect final {
    CharChangeSideEffectKind kind =
        CharChangeSideEffectKind::SendNackToPlayer;
    std::uint32_t player_id = 0;
    std::uint32_t nack_code = 0;
    std::uint16_t shop_item_idx = 0;
    std::uint16_t shop_item_pos = 0;
    CharChangeIcon icon_kind = CharChangeIcon::CharChange;
    CharChangeValidationFields info{};
    std::uint8_t saved_gender = 0;
    float saved_height = 1.0f;
    float saved_width  = 1.0f;
    std::uint8_t db_gender = 0;
    std::uint8_t db_hair = 0;
    std::uint8_t db_face = 0;
    float db_height = 1.0f;
    float db_width  = 1.0f;
};

struct CharChangeSideEffectPlan final {
    std::vector<CharChangeSideEffect> effects;
    bool send_nack = false;
    bool send_use_ack = false;
    bool send_char_change_ack = false;
    bool broadcast = false;
    bool db_call = false;
    bool discard_item = false;
    bool set_char_change_info = false;
    bool log_item_money = false;
    std::uint8_t nack_code = 0;
};

inline CharChangeSideEffectPlan char_change_side_effect_plan(
    const CharChangeValidationInput& in,
    std::uint32_t player_id,
    std::uint16_t shop_item_idx,
    std::uint16_t shop_item_pos,
    CharChangeIcon icon_kind,
    const CharChangeValidationFields& info,
    std::uint8_t saved_gender,
    float saved_height,
    float saved_width) {
    CharChangeSideEffectPlan plan;
    const CharChangeOutcome outcome = classify_char_change_outcome(in);

    if (outcome != CharChangeOutcome::Success) {
        plan.send_nack = true;
        plan.nack_code = char_change_nack_code(outcome);
        plan.effects.reserve(1u);
        CharChangeSideEffect nack{};
        nack.kind = CharChangeSideEffectKind::SendNackToPlayer;
        nack.player_id = player_id;
        nack.nack_code = plan.nack_code;
        plan.effects.push_back(nack);
        return plan;
    }

    plan.send_use_ack = true;
    plan.send_char_change_ack = true;
    plan.broadcast = true;
    plan.db_call = true;
    plan.discard_item = true;
    plan.set_char_change_info = true;
    plan.log_item_money = true;

    CharChangeSideEffect setinfo{};
    setinfo.kind = CharChangeSideEffectKind::SetCharChangeInfo;
    setinfo.player_id = player_id;
    setinfo.info = info;

    CharChangeSideEffect discard{};
    discard.kind = CharChangeSideEffectKind::DiscardCharChangeItem;
    discard.player_id = player_id;
    discard.shop_item_idx = shop_item_idx;
    discard.shop_item_pos = shop_item_pos;

    CharChangeSideEffect use_ack{};
    use_ack.kind = CharChangeSideEffectKind::SendUseAckToPlayer;
    use_ack.player_id = player_id;
    use_ack.shop_item_idx = shop_item_idx;
    use_ack.shop_item_pos = shop_item_pos;

    CharChangeSideEffect bc{};
    bc.kind = CharChangeSideEffectKind::BroadcastCharChange;
    bc.player_id = player_id;
    bc.icon_kind = icon_kind;
    bc.info = info;
    bc.saved_gender = saved_gender;
    bc.saved_height = saved_height;
    bc.saved_width = saved_width;

    CharChangeSideEffect db{};
    db.kind = CharChangeSideEffectKind::CharacterChangeInfoToDB;
    db.player_id = player_id;
    db.icon_kind = icon_kind;
    db.info = info;
    db.saved_gender = saved_gender;
    db.saved_height = saved_height;
    db.saved_width = saved_width;

    CharChangeSideEffect ack{};
    ack.kind = CharChangeSideEffectKind::SendCharChangeAck;
    ack.player_id = player_id;

    CharChangeSideEffect log{};
    log.kind = CharChangeSideEffectKind::LogItemMoney;
    log.player_id = player_id;
    log.shop_item_idx = shop_item_idx;
    log.shop_item_pos = shop_item_pos;

    plan.effects.reserve(7u);
    plan.effects.push_back(discard);
    plan.effects.push_back(setinfo);
    plan.effects.push_back(use_ack);
    plan.effects.push_back(bc);
    plan.effects.push_back(db);
    plan.effects.push_back(ack);
    plan.effects.push_back(log);
    return plan;
}

}  // namespace mxh::server
