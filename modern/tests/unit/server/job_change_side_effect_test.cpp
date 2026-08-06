// Tests for MP_ITEM_SHOPITEM_JOBCHANGE_SYN side-effect dispatcher.

#include <mxh/server/job_change_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

JobChangeValidationInput success_input() {
    JobChangeValidationInput in{};
    in.stage_is_hwa_or_geuk = true;
    in.slot_exists = true;
    in.item_exists = true;
    in.item_icon_is_change_job = true;
    in.item_db_idx_matches = true;
    in.discard_returned_true = true;
    return in;
}

TEST(JobChangeOutcome, AllGatesPassIsSuccess) {
    EXPECT_EQ(classify_job_change_outcome(success_input()),
              JobChangeOutcome::Success);
}

TEST(JobChangeOutcome, StageNotHwaOrGeukIsBadStage) {
    auto in = success_input();
    in.stage_is_hwa_or_geuk = false;
    EXPECT_EQ(classify_job_change_outcome(in),
              JobChangeOutcome::BadStage);
}

TEST(JobChangeOutcome, SlotMissingIsBadItem) {
    auto in = success_input();
    in.slot_exists = false;
    EXPECT_EQ(classify_job_change_outcome(in),
              JobChangeOutcome::BadItem);
}

TEST(JobChangeOutcome, ItemMissingIsBadItem) {
    auto in = success_input();
    in.item_exists = false;
    EXPECT_EQ(classify_job_change_outcome(in),
              JobChangeOutcome::BadItem);
}

TEST(JobChangeOutcome, WrongIconIsBadItem) {
    auto in = success_input();
    in.item_icon_is_change_job = false;
    EXPECT_EQ(classify_job_change_outcome(in),
              JobChangeOutcome::BadItem);
}

TEST(JobChangeOutcome, WrongDbIdxIsBadItem) {
    auto in = success_input();
    in.item_db_idx_matches = false;
    EXPECT_EQ(classify_job_change_outcome(in),
              JobChangeOutcome::BadItem);
}

TEST(JobChangeOutcome, DiscardFailedIsDiscardFailed) {
    auto in = success_input();
    in.discard_returned_true = false;
    EXPECT_EQ(classify_job_change_outcome(in),
              JobChangeOutcome::DiscardFailed);
}

TEST(JobChangeOutcome, BadStageTakesPrecedence) {
    auto in = success_input();
    in.stage_is_hwa_or_geuk = false;
    in.slot_exists = false;
    in.discard_returned_true = false;
    EXPECT_EQ(classify_job_change_outcome(in),
              JobChangeOutcome::BadStage);
}

TEST(JobChangeNackCode, MapsOutcomesToLegacyCodes) {
    EXPECT_EQ(job_change_nack_code(JobChangeOutcome::BadStage), 1u);
    EXPECT_EQ(job_change_nack_code(JobChangeOutcome::BadItem), 2u);
    EXPECT_EQ(job_change_nack_code(JobChangeOutcome::DiscardFailed), 3u);
    EXPECT_EQ(job_change_nack_code(JobChangeOutcome::Success), 0u);
}

TEST(JobChangePlan, SuccessEmitsFullSequence) {
    auto in = success_input();
    auto plan = job_change_side_effect_plan(in, 100, LEGACY_ESTAGE_HWA, 5);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.discard_item);
    EXPECT_TRUE(plan.change_stage_ability);
    EXPECT_TRUE(plan.set_stage);
    EXPECT_TRUE(plan.log_item_money);
    EXPECT_EQ(plan.effects.size(), 5u);
    EXPECT_EQ(plan.effects[0].kind,
              JobChangeSideEffectKind::DiscardChangeJobItem);
    EXPECT_EQ(plan.effects[1].kind,
              JobChangeSideEffectKind::ChangeCharacterStageAbility);
    EXPECT_EQ(plan.effects[1].current_stage, LEGACY_ESTAGE_HWA);
    EXPECT_EQ(plan.effects[1].ability_group, 5u);
    EXPECT_EQ(plan.effects[2].kind,
              JobChangeSideEffectKind::SetStage);
    EXPECT_EQ(plan.effects[2].current_stage, LEGACY_ESTAGE_HWA);
    EXPECT_EQ(plan.effects[2].new_stage, LEGACY_ESTAGE_GEUK);
    EXPECT_EQ(plan.effects[3].kind,
              JobChangeSideEffectKind::SendAckToPlayer);
    EXPECT_EQ(plan.effects[4].kind,
              JobChangeSideEffectKind::LogItemMoney);
    EXPECT_EQ(plan.effects[4].new_stage, LEGACY_ESTAGE_GEUK);
}

TEST(JobChangePlan, SuccessFromGeukStagesToHwa) {
    auto in = success_input();
    auto plan = job_change_side_effect_plan(in, 100, LEGACY_ESTAGE_GEUK, 5);
    EXPECT_EQ(plan.effects[2].current_stage, LEGACY_ESTAGE_GEUK);
    EXPECT_EQ(plan.effects[2].new_stage, LEGACY_ESTAGE_HWA);
}

TEST(JobChangePlan, BadStageSendsNackWithCode1) {
    auto in = success_input();
    in.stage_is_hwa_or_geuk = false;
    auto plan = job_change_side_effect_plan(in, 100, 99, 5);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              JobChangeSideEffectKind::SendNackToPlayer);
    EXPECT_EQ(plan.effects[0].nack_code, 1u);
}

TEST(JobChangePlan, BadItemSendsNackWithCode2) {
    auto in = success_input();
    in.item_icon_is_change_job = false;
    auto plan = job_change_side_effect_plan(in, 100, 1, 5);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              JobChangeSideEffectKind::SendNackToPlayer);
    EXPECT_EQ(plan.effects[0].nack_code, 2u);
}

TEST(JobChangePlan, DiscardFailedSendsNackWithCode3) {
    auto in = success_input();
    in.discard_returned_true = false;
    auto plan = job_change_side_effect_plan(in, 100, 1, 5);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              JobChangeSideEffectKind::SendNackToPlayer);
    EXPECT_EQ(plan.effects[0].nack_code, 3u);
}

TEST(JobChangePlan, PlanIsIdempotent) {
    auto in = success_input();
    auto a = job_change_side_effect_plan(in, 100, LEGACY_ESTAGE_HWA, 5);
    auto b = job_change_side_effect_plan(in, 100, LEGACY_ESTAGE_HWA, 5);
    EXPECT_EQ(a.send_ack, b.send_ack);
    EXPECT_EQ(a.set_stage, b.set_stage);
    EXPECT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].player_id, b.effects[i].player_id);
    }
}
