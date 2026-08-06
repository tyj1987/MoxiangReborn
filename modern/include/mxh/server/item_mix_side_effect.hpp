// 1:1 side-effect-dispatcher port of CItemManager::MP_ITEM_MIX_SYN
// from legacy [Server]Map/ItemManager.cpp:4360-4424.
//
// The legacy handler calls MixItem and routes to one of five branches
// based on the return code (rt):
//   1. rt == EI_TRUE (0): success -> echo pmsg as MSG_ITEM_MIX_ACK
//      with Protocol=MP_ITEM_MIX_SUCCESS_ACK.
//   2. rt == 1000: "big fail" -> echo pmsg as MSG_ITEM_MIX_ACK with
//      Protocol=MP_ITEM_MIX_BIGFAILED_ACK.
//   3. rt == 1001: "fail" -> echo pmsg as MSG_ITEM_MIX_ACK with
//      Protocol=MP_ITEM_MIX_FAILED_ACK.
//   4. rt == 20, 21, 22, 23: send MSG_DWORD2 with
//      Protocol=MP_ITEM_MIX_MSG, dwData1=rt, dwData2=BasicItemPos.
//   5. rt == 2 or other: ASSERT + send MSG_ITEM_ERROR with
//      Protocol=MP_ITEM_ERROR_NACK, ECode=rt.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_MIX_*_ACK / _NACK.
inline constexpr std::uint8_t LEGACY_MP_ITEM_MIX_SUCCESS_ACK   = 60u;
inline constexpr std::uint8_t LEGACY_MP_ITEM_MIX_BIGFAILED_ACK = 61u;
inline constexpr std::uint8_t LEGACY_MP_ITEM_MIX_FAILED_ACK    = 62u;
inline constexpr std::uint8_t LEGACY_MP_ITEM_MIX_MSG           = 63u;
inline constexpr std::uint8_t LEGACY_MP_ITEM_ERROR_NACK_MIX    = 99u;

// Legacy mix-item rt sentinel values.
inline constexpr int LEGACY_MIX_RT_BIGFAIL = 1000;
inline constexpr int LEGACY_MIX_RT_FAIL    = 1001;
inline constexpr int LEGACY_MIX_RT_MSG_MIN = 20;
inline constexpr int LEGACY_MIX_RT_MSG_MAX = 23;
inline constexpr int LEGACY_MIX_RT_ASSERT  = 2;

enum class ItemMixOutcome : std::uint8_t {
    Success   = 0,  // legacy rt == 0
    BigFail   = 1,  // legacy rt == 1000
    Fail      = 2,  // legacy rt == 1001
    Msg       = 3,  // legacy rt in {20, 21, 22, 23}
    ErrorNack = 4,  // legacy rt == 2 or other
};

inline ItemMixOutcome classify_item_mix_outcome(int mix_rt) noexcept {
    if (mix_rt == 0) {
        return ItemMixOutcome::Success;
    }
    if (mix_rt == LEGACY_MIX_RT_BIGFAIL) {
        return ItemMixOutcome::BigFail;
    }
    if (mix_rt == LEGACY_MIX_RT_FAIL) {
        return ItemMixOutcome::Fail;
    }
    if (mix_rt >= LEGACY_MIX_RT_MSG_MIN &&
        mix_rt <= LEGACY_MIX_RT_MSG_MAX) {
        return ItemMixOutcome::Msg;
    }
    return ItemMixOutcome::ErrorNack;
}

enum class ItemMixSideEffectKind : std::uint8_t {
    BroadcastSuccessAck = 0,    // MP_ITEM_MIX_SUCCESS_ACK
    BroadcastBigFailAck = 1,    // MP_ITEM_MIX_BIGFAILED_ACK
    BroadcastFailAck    = 2,    // MP_ITEM_MIX_FAILED_ACK
    BroadcastMixMsg     = 3,    // MP_ITEM_MIX_MSG (MSG_DWORD2)
    BroadcastErrorNack  = 4,    // MP_ITEM_ERROR_NACK + ASSERT
};

struct ItemMixSideEffect final {
    ItemMixSideEffectKind kind =
        ItemMixSideEffectKind::BroadcastSuccessAck;
    std::uint16_t basic_item_idx = 0;     // legacy pmsg->wBasicItemIdx
    std::uint16_t basic_item_pos = 0;     // legacy pmsg->BasicItemPos
    std::uint16_t result_index = 0;       // legacy pmsg->ResultIndex
    std::uint16_t material_num = 0;       // legacy pmsg->wMaterialNum
    std::uint16_t shop_item_idx = 0;      // legacy pmsg->ShopItemIdx
    std::uint16_t shop_item_pos = 0;      // legacy pmsg->ShopItemPos
    int original_rt = 0;
    int ecode = 0;
};

struct ItemMixSideEffectPlan final {
    std::vector<ItemMixSideEffect> effects;
    bool send_ack = false;
    bool send_msg = false;
    bool send_error_nack = false;
};

inline ItemMixSideEffectPlan item_mix_side_effect_plan(
    int mix_rt,
    std::uint16_t basic_item_idx,
    std::uint16_t basic_item_pos,
    std::uint16_t result_index,
    std::uint16_t material_num,
    std::uint16_t shop_item_idx,
    std::uint16_t shop_item_pos) {
    ItemMixSideEffectPlan plan;
    const ItemMixOutcome outcome = classify_item_mix_outcome(mix_rt);
    plan.effects.reserve(1u);

    ItemMixSideEffect eff{};
    eff.basic_item_idx = basic_item_idx;
    eff.basic_item_pos = basic_item_pos;
    eff.result_index = result_index;
    eff.material_num = material_num;
    eff.shop_item_idx = shop_item_idx;
    eff.shop_item_pos = shop_item_pos;
    eff.original_rt = mix_rt;
    eff.ecode = mix_rt;

    switch (outcome) {
    case ItemMixOutcome::Success:
        plan.send_ack = true;
        eff.kind = ItemMixSideEffectKind::BroadcastSuccessAck;
        break;
    case ItemMixOutcome::BigFail:
        plan.send_ack = true;
        eff.kind = ItemMixSideEffectKind::BroadcastBigFailAck;
        break;
    case ItemMixOutcome::Fail:
        plan.send_ack = true;
        eff.kind = ItemMixSideEffectKind::BroadcastFailAck;
        break;
    case ItemMixOutcome::Msg:
        plan.send_msg = true;
        eff.kind = ItemMixSideEffectKind::BroadcastMixMsg;
        eff.ecode = mix_rt;
        break;
    case ItemMixOutcome::ErrorNack:
        plan.send_error_nack = true;
        eff.kind = ItemMixSideEffectKind::BroadcastErrorNack;
        eff.ecode = mix_rt;
        break;
    }
    plan.effects.push_back(eff);
    return plan;
}

}  // namespace mxh::server
