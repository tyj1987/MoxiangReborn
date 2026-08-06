// 1:1 side-effect-dispatcher port of CItemManager::MP_ITEM_DIVIDE_SYN
// from legacy [Server]Map/ItemManager.cpp:4092-4114.
//
// The legacy handler calls DivideItem and routes to one of two
// branches:
//   1. EI_TRUE (rt == 0): success -> no message (legacy has empty
//      body; the DivideItem -> ObtainItemEx path emits its own
//      ITEMOBTAINARRAYINFO ACK through Alloc's DBResult chain).
//   2. rt != 0: failure -> send MSG_ITEM_ERROR with Protocol =
//      MP_ITEM_ERROR_NACK, ECode = eItemUseErr_Divide (= 4), and
//      the SendErrorMsg auxiliary code = rt.
//
// The "silent success" semantic is preserved here: the success plan
// carries no effects, only a `silent_success` flag the orchestrator
// can use to skip the broadcast step.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/CommonGameDefine.h eItemUse_Err.
inline constexpr int LEGACY_EITEMUSE_DIVIDE = 4;

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_ERROR_NACK.
inline constexpr std::uint8_t LEGACY_MP_ITEM_ERROR_NACK_DIVIDE = 99u;

enum class ItemDivideOutcome : std::uint8_t {
    Success = 0,  // legacy rt == 0
    Failure = 1,  // legacy rt != 0
};

inline ItemDivideOutcome classify_item_divide_outcome(
    int divide_rt) noexcept {
    if (divide_rt == 0) {
        return ItemDivideOutcome::Success;
    }
    return ItemDivideOutcome::Failure;
}

enum class ItemDivideSideEffectKind : std::uint8_t {
    BroadcastErrorNack = 0,  // legacy SendErrorMsg(MP_ITEM_ERROR_NACK, eItemUseErr_Divide, rt)
};

struct ItemDivideSideEffect final {
    ItemDivideSideEffectKind kind =
        ItemDivideSideEffectKind::BroadcastErrorNack;
    std::uint16_t from_pos = 0;   // legacy pmsg->FromPos
    std::uint16_t to_pos = 0;     // legacy pmsg->ToPos
    std::uint16_t item_idx = 0;   // legacy pmsg->wItemIdx
    std::uint16_t from_dur = 0;   // legacy pmsg->FromDur
    std::uint16_t to_dur = 0;     // legacy pmsg->ToDur
    int original_rt = 0;
};

struct ItemDivideSideEffectPlan final {
    std::vector<ItemDivideSideEffect> effects;
    bool send_nack = false;
    bool silent_success = false;
};

inline ItemDivideSideEffectPlan item_divide_side_effect_plan(
    int divide_rt,
    std::uint16_t from_pos,
    std::uint16_t to_pos,
    std::uint16_t item_idx,
    std::uint16_t from_dur,
    std::uint16_t to_dur) {
    ItemDivideSideEffectPlan plan;
    const ItemDivideOutcome outcome =
        classify_item_divide_outcome(divide_rt);
    if (outcome == ItemDivideOutcome::Success) {
        // Legacy: success has empty body. The ObtainItemEx path emits
        // its own ITEMOBTAINARRAYINFO ACK; the orchestrator just
        // marks the plan as silent-success so it doesn't double up.
        plan.silent_success = true;
        return plan;
    }
    plan.send_nack = true;
    plan.effects.reserve(1u);
    ItemDivideSideEffect nack{};
    nack.kind = ItemDivideSideEffectKind::BroadcastErrorNack;
    nack.from_pos = from_pos;
    nack.to_pos = to_pos;
    nack.item_idx = item_idx;
    nack.from_dur = from_dur;
    nack.to_dur = to_dur;
    nack.original_rt = divide_rt;
    plan.effects.push_back(nack);
    return plan;
}

}  // namespace mxh::server
