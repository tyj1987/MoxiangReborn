//
// CItemManager::MP_ITEM_SHOPITEM_REVIVEOTHER_NACK from legacy
// [Server]Map/ItemManager.cpp:5353-5372.
//
// The legacy handler is a 2-player cleanup pass-through:
//   1. FindUser(pmsg->dwObjectID) -> pPlayer; if null, return.
//   2. FindUser(pmsg->dwData1) -> pTargetPlayer; if null, return.
//   3. Build MSG_DWORD {MP_ITEM, REVIVEOTHER_NACK, dwData =
//      pmsg->dwData2} and SendMsg to pTargetPlayer.
//   4. pTargetPlayer->SetReviveData(0, 0, 0).
//   5. pTargetPlayer->SetReviveTime(0).

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

enum class ReviveOtherNackOutcome : std::uint8_t {
    Forwarded = 0,
    NoPlayer  = 1,
};

struct ReviveOtherNackValidationInput final {
    bool player_found = false;
    bool target_found = false;
};

inline ReviveOtherNackOutcome classify_revive_other_nack_outcome(
    const ReviveOtherNackValidationInput& in) noexcept {
    if (!in.player_found || !in.target_found) {
        return ReviveOtherNackOutcome::NoPlayer;
    }
    return ReviveOtherNackOutcome::Forwarded;
}

enum class ReviveOtherNackSideEffectKind : std::uint8_t {
    ForwardNackToTarget       = 0,
    ClearReviveDataOnTarget   = 1,
    ClearReviveTimeOnTarget   = 2,
};

struct ReviveOtherNackSideEffect final {
    ReviveOtherNackSideEffectKind kind =
        ReviveOtherNackSideEffectKind::ForwardNackToTarget;
    std::uint32_t target_id = 0;
    std::uint32_t nack_code = 0;
};

struct ReviveOtherNackSideEffectPlan final {
    std::vector<ReviveOtherNackSideEffect> effects;
    bool forward_nack = false;
    bool clear_revive_data = false;
    bool clear_revive_time = false;
};

inline ReviveOtherNackSideEffectPlan revive_other_nack_side_effect_plan(
    const ReviveOtherNackValidationInput& in,
    std::uint32_t target_id,
    std::uint32_t nack_code) {
    ReviveOtherNackSideEffectPlan plan;
    const ReviveOtherNackOutcome outcome =
        classify_revive_other_nack_outcome(in);
    if (outcome == ReviveOtherNackOutcome::NoPlayer) {
        return plan;
    }
    plan.forward_nack = true;
    plan.clear_revive_data = true;
    plan.clear_revive_time = true;
    plan.effects.reserve(3u);
    ReviveOtherNackSideEffect nack{};
    nack.kind = ReviveOtherNackSideEffectKind::ForwardNackToTarget;
    nack.target_id = target_id;
    nack.nack_code = nack_code;
    plan.effects.push_back(nack);
    ReviveOtherNackSideEffect clr_data{};
    clr_data.kind = ReviveOtherNackSideEffectKind::ClearReviveDataOnTarget;
    clr_data.target_id = target_id;
    plan.effects.push_back(clr_data);
    ReviveOtherNackSideEffect clr_time{};
    clr_time.kind = ReviveOtherNackSideEffectKind::ClearReviveTimeOnTarget;
    clr_time.target_id = target_id;
    plan.effects.push_back(clr_time);
    return plan;
}

}  // namespace mxh::server
