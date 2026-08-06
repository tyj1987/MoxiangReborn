// 1:1 side-effect-dispatcher port of
// CItemManager::MP_ITEM_USE_CHANGEITEM_SYN from legacy
// [Server]Map/ItemManager.cpp:5002-5019.
//
// The legacy handler calls CHANGEITEMMGR->UseChangeItem and routes to
// one of two branches (the inverse of MP_ITEM_USE_SYN):
//   1. rt == 0 (legacy: not use): send MSG_ITEM_ERROR with
//      Protocol = MP_ITEM_USE_NACK, ECode = rt (which is 0).
//   2. rt != 0 (legacy: success, item changed): no network
//      response - silent success.
//
// This handler is used by event-driven change items (e.g., seasonal
// item transformations). The ACK is not sent on success because the
// legacy client updates its own UI based on inventory diff.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_USE_NACK.
inline constexpr std::uint8_t LEGACY_MP_ITEM_USE_NACK_CHANGE = 74u;

enum class UseChangeItemOutcome : std::uint8_t {
    Success = 0,  // legacy rt != 0 (item transformed)
    NotUsed = 1,  // legacy rt == 0 (could not transform)
    NoPlayer = 2, // legacy FindUser returned null
};

inline UseChangeItemOutcome classify_use_change_item_outcome(
    int use_rt, bool player_found) noexcept {
    if (!player_found) {
        return UseChangeItemOutcome::NoPlayer;
    }
    if (use_rt == 0) {
        return UseChangeItemOutcome::NotUsed;
    }
    return UseChangeItemOutcome::Success;
}

enum class UseChangeItemSideEffectKind : std::uint8_t {
    BroadcastUseNack = 0,  // legacy SendErrorMsg(MP_ITEM_USE_NACK, ECode=rt)
    SilentSuccess    = 1,  // legacy: no network I/O
};

struct UseChangeItemSideEffect final {
    UseChangeItemSideEffectKind kind =
        UseChangeItemSideEffectKind::SilentSuccess;
    std::uint16_t target_pos = 0;     // legacy pmsg->TargetPos
    std::uint16_t item_idx = 0;       // legacy pmsg->wItemIdx
    int error_code = 0;               // legacy ECode (NACK payload)
    int original_rt = 0;
};

struct UseChangeItemSideEffectPlan final {
    std::vector<UseChangeItemSideEffect> effects;
    bool send_nack = false;
    bool silent_success = false;
    int error_code = 0;
};

inline UseChangeItemSideEffectPlan use_change_item_side_effect_plan(
    int use_rt,
    bool player_found,
    std::uint16_t target_pos,
    std::uint16_t item_idx) {
    UseChangeItemSideEffectPlan plan;
    const UseChangeItemOutcome outcome =
        classify_use_change_item_outcome(use_rt, player_found);
    if (outcome == UseChangeItemOutcome::NoPlayer) {
        return plan;
    }
    plan.effects.reserve(1u);
    UseChangeItemSideEffect eff{};
    eff.target_pos = target_pos;
    eff.item_idx = item_idx;
    eff.original_rt = use_rt;
    if (outcome == UseChangeItemOutcome::NotUsed) {
        plan.send_nack = true;
        eff.kind = UseChangeItemSideEffectKind::BroadcastUseNack;
        eff.error_code = use_rt;
        plan.error_code = use_rt;
    } else {
        plan.silent_success = true;
        eff.kind = UseChangeItemSideEffectKind::SilentSuccess;
    }
    plan.effects.push_back(eff);
    return plan;
}

}  // namespace mxh::server
