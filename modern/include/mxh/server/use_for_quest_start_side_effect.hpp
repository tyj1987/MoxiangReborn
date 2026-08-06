// 1:1 side-effect-dispatcher port of
// CItemManager::MP_ITEM_USE_FOR_QUESTSTART_SYN from legacy
// [Server]Map/ItemManager.cpp:4955-4979.
//
// The legacy handler calls UseItem(player, TargetPos, ItemIdx) and
// routes to one of two branches:
//   1. rt == eItemUseSuccess (rt == 0): echo pmsg as MSG_ITEM_USE_ACK
//      (memcpy + Protocol flip to MP_ITEM_USE_ACK).
//   2. rt != 0: send MSG_ITEM_ERROR with Protocol = MP_ITEM_USE_NACK,
//      ECode = eItemUseErr_Quest (legacy value 7).
//
// The packet format is MSG_ITEM_USE_SYN which carries TargetPos and
// wItemIdx from the client; the server preserves both fields across
// the ACK.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_USE_ACK / _NACK.
inline constexpr std::uint8_t LEGACY_MP_ITEM_USE_ACK       = 71u;
inline constexpr std::uint8_t LEGACY_MP_ITEM_USE_NACK      = 72u;

// 1:1 with legacy [CC]Header/CommonGameDefine.h eItemUseErr_Quest
// (7th value of eItemUse_Err, 0-indexed: 0=Success, 7=Quest).
inline constexpr int LEGACY_EITEMUSE_QUEST = 7;

enum class UseForQuestStartOutcome : std::uint8_t {
    Success = 0,  // legacy rt == 0
    Failure = 1,  // legacy rt != 0 (eItemUseErr_Quest)
};

inline UseForQuestStartOutcome classify_use_for_quest_start_outcome(
    int use_rt) noexcept {
    if (use_rt == 0) {
        return UseForQuestStartOutcome::Success;
    }
    return UseForQuestStartOutcome::Failure;
}

enum class UseForQuestStartSideEffectKind : std::uint8_t {
    BroadcastUseAck  = 0,  // legacy SendAckMsg(MP_ITEM_USE_ACK)
    BroadcastUseNack = 1,  // legacy SendErrorMsg(MP_ITEM_USE_NACK, eItemUseErr_Quest)
};

struct UseForQuestStartSideEffect final {
    UseForQuestStartSideEffectKind kind =
        UseForQuestStartSideEffectKind::BroadcastUseAck;
    std::uint16_t target_pos = 0;     // legacy pmsg->TargetPos
    std::uint16_t item_idx = 0;       // legacy pmsg->wItemIdx
    int error_code = 0;               // legacy ECode (= eItemUseErr_Quest on NACK)
    int original_rt = 0;
};

struct UseForQuestStartSideEffectPlan final {
    std::vector<UseForQuestStartSideEffect> effects;
    bool send_ack = false;
    bool send_nack = false;
    int error_code = 0;
};

inline UseForQuestStartSideEffectPlan use_for_quest_start_side_effect_plan(
    int use_rt,
    std::uint16_t target_pos,
    std::uint16_t item_idx) {
    UseForQuestStartSideEffectPlan plan;
    const UseForQuestStartOutcome outcome =
        classify_use_for_quest_start_outcome(use_rt);
    plan.effects.reserve(1u);
    UseForQuestStartSideEffect eff{};
    eff.target_pos = target_pos;
    eff.item_idx = item_idx;
    eff.original_rt = use_rt;
    if (outcome == UseForQuestStartOutcome::Success) {
        plan.send_ack = true;
        eff.kind = UseForQuestStartSideEffectKind::BroadcastUseAck;
    } else {
        plan.send_nack = true;
        eff.kind = UseForQuestStartSideEffectKind::BroadcastUseNack;
        eff.error_code = LEGACY_EITEMUSE_QUEST;
        plan.error_code = LEGACY_EITEMUSE_QUEST;
    }
    plan.effects.push_back(eff);
    return plan;
}

}  // namespace mxh::server
