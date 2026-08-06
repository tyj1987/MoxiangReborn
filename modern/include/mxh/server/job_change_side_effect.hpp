//
// CItemManager::MP_ITEM_SHOPITEM_JOBCHANGE_SYN from legacy
// [Server]Map/ItemManager.cpp:5776-5832.
//
// The legacy handler runs 4 gates in order:
//   1. stage must be eStage_Hwa or eStage_Geuk (else NACK code=1).
//   2. slot + pItem must exist (else NACK code=2).
//   3. pItem->wIconIdx == eIncantation_ChangeJob and
//      pItem->dwDBIdx == pmsg->dwData2 (else NACK code=2).
//   4. DiscardItem returns EI_TRUE (else NACK code=3).
//
// On success:
//   - ChangeCharacterStageAbility(pPlayer, current stage,
//     AbilityGroup).
//   - Compute changestage = (Hwa -> Geuk) | (Geuk -> Hwa).
//   - pPlayer->SetStage(changestage).
//   - Send JOBCHANGE_ACK.
//   - LogItemMoney.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/CommonGameDefine.h eStage_Hwa / eStage_Geuk.
inline constexpr std::uint8_t LEGACY_ESTAGE_HWA = 1u;
inline constexpr std::uint8_t LEGACY_ESTAGE_GEUK = 2u;

// 1:1 with legacy [CC]Header/CommonGameDefine.h eIncantation_ChangeJob.
inline constexpr std::uint32_t LEGACY_EINCANTATION_CHANGEJOB = 36u;

enum class JobChangeOutcome : std::uint8_t {
    Success        = 0,
    BadStage       = 1,  // legacy NACK code = 1
    BadItem        = 2,  // legacy NACK code = 2 (slot/item missing/wrong)
    DiscardFailed  = 3,  // legacy NACK code = 3
};

struct JobChangeValidationInput final {
    bool stage_is_hwa_or_geuk = false;
    bool slot_exists = false;
    bool item_exists = false;
    bool item_icon_is_change_job = false;
    bool item_db_idx_matches = false;
    bool discard_returned_true = false;
};

inline JobChangeOutcome classify_job_change_outcome(
    const JobChangeValidationInput& in) noexcept {
    if (!in.stage_is_hwa_or_geuk) {
        return JobChangeOutcome::BadStage;
    }
    if (!in.slot_exists || !in.item_exists ||
        !in.item_icon_is_change_job ||
        !in.item_db_idx_matches) {
        return JobChangeOutcome::BadItem;
    }
    if (!in.discard_returned_true) {
        return JobChangeOutcome::DiscardFailed;
    }
    return JobChangeOutcome::Success;
}

inline std::uint8_t job_change_nack_code(JobChangeOutcome o) noexcept {
    switch (o) {
        case JobChangeOutcome::BadStage:      return 1u;
        case JobChangeOutcome::BadItem:       return 2u;
        case JobChangeOutcome::DiscardFailed: return 3u;
        default:                              return 0u;
    }
}

enum class JobChangeSideEffectKind : std::uint8_t {
    SendAckToPlayer             = 0,
    SendNackToPlayer            = 1,
    DiscardChangeJobItem        = 2,
    ChangeCharacterStageAbility = 3,  // DB call
    SetStage                    = 4,
    LogItemMoney                = 5,
};

struct JobChangeSideEffect final {
    JobChangeSideEffectKind kind =
        JobChangeSideEffectKind::SendAckToPlayer;
    std::uint32_t player_id = 0;
    std::uint8_t  current_stage = 0;
    std::uint8_t  new_stage = 0;
    std::uint32_t ability_group = 0;
    std::uint32_t nack_code = 0;
};

struct JobChangeSideEffectPlan final {
    std::vector<JobChangeSideEffect> effects;
    bool send_ack = false;
    bool send_nack = false;
    bool discard_item = false;
    bool change_stage_ability = false;
    bool set_stage = false;
    bool log_item_money = false;
};

inline JobChangeSideEffectPlan job_change_side_effect_plan(
    const JobChangeValidationInput& in,
    std::uint32_t player_id,
    std::uint8_t current_stage,
    std::uint32_t ability_group) {
    JobChangeSideEffectPlan plan;
    const JobChangeOutcome outcome = classify_job_change_outcome(in);

    if (outcome == JobChangeOutcome::Success) {
        plan.send_ack = true;
        plan.discard_item = true;
        plan.change_stage_ability = true;
        plan.set_stage = true;
        plan.log_item_money = true;
        const std::uint8_t new_stage =
            (current_stage == LEGACY_ESTAGE_HWA) ? LEGACY_ESTAGE_GEUK :
            (current_stage == LEGACY_ESTAGE_GEUK) ? LEGACY_ESTAGE_HWA :
            current_stage;
        plan.effects.reserve(5u);
        JobChangeSideEffect discard{};
        discard.kind = JobChangeSideEffectKind::DiscardChangeJobItem;
        discard.player_id = player_id;
        plan.effects.push_back(discard);
        JobChangeSideEffect ability{};
        ability.kind = JobChangeSideEffectKind::ChangeCharacterStageAbility;
        ability.player_id = player_id;
        ability.current_stage = current_stage;
        ability.ability_group = ability_group;
        plan.effects.push_back(ability);
        JobChangeSideEffect setstage{};
        setstage.kind = JobChangeSideEffectKind::SetStage;
        setstage.player_id = player_id;
        setstage.current_stage = current_stage;
        setstage.new_stage = new_stage;
        plan.effects.push_back(setstage);
        JobChangeSideEffect ack{};
        ack.kind = JobChangeSideEffectKind::SendAckToPlayer;
        ack.player_id = player_id;
        plan.effects.push_back(ack);
        JobChangeSideEffect log{};
        log.kind = JobChangeSideEffectKind::LogItemMoney;
        log.player_id = player_id;
        log.current_stage = current_stage;
        log.new_stage = new_stage;
        plan.effects.push_back(log);
        return plan;
    }

    plan.send_nack = true;
    plan.effects.reserve(1u);
    JobChangeSideEffect nack{};
    nack.kind = JobChangeSideEffectKind::SendNackToPlayer;
    nack.player_id = player_id;
    nack.nack_code = job_change_nack_code(outcome);
    plan.effects.push_back(nack);
    return plan;
}

}  // namespace mxh::server
