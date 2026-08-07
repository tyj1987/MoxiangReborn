// job_change_side_effect_runtime_test.cpp
//
// Verifies apply_job_change_side_effects() (the runtime orchestrator
// for the CItemManager::MP_ITEM_SHOPITEM_JOBCHANGE_SYN side-effect
// chain) walks the data-plane plan and dispatches each entry: the
// 5-step success chain in legacy order / the 3-way gate NACK.

#include <mxh/server/job_change_side_effect.hpp>
#include <mxh/server/job_change_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::JobChangeSideEffectKind;
using mxh::server::JobChangeSideEffectSink;
using mxh::server::JobChangeValidationInput;
using mxh::server::LEGACY_ESTAGE_GEUK;
using mxh::server::LEGACY_ESTAGE_HWA;
using mxh::server::apply_job_change_side_effects;
using mxh::server::job_change_side_effect_plan;

class RecordingSink final : public JobChangeSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_player_id = 0;
    std::uint32_t last_nack_code = 0;
    std::uint8_t last_current_stage = 0;
    std::uint8_t last_new_stage = 0;
    std::uint32_t last_ability_group = 0;
    std::size_t ack_count = 0;
    std::size_t nack_count = 0;

    void send_ack_to_player(std::uint32_t player_id) override {
        calls.push_back("ack");
        last_player_id = player_id;
        ++ack_count;
    }
    void send_nack_to_player(std::uint32_t player_id,
                             std::uint32_t nack_code) override {
        calls.push_back("nack");
        last_player_id = player_id;
        last_nack_code = nack_code;
        ++nack_count;
    }
    void discard_change_job_item(std::uint32_t player_id) override {
        calls.push_back("discard");
        last_player_id = player_id;
    }
    void change_character_stage_ability(
        std::uint32_t player_id, std::uint8_t current_stage,
        std::uint32_t ability_group) override {
        calls.push_back("ability");
        last_player_id = player_id;
        last_current_stage = current_stage;
        last_ability_group = ability_group;
    }
    void set_stage(std::uint32_t player_id,
                   std::uint8_t current_stage,
                   std::uint8_t new_stage) override {
        calls.push_back("setstage");
        last_player_id = player_id;
        last_current_stage = current_stage;
        last_new_stage = new_stage;
    }
    void log_item_money(std::uint32_t player_id,
                        std::uint8_t current_stage,
                        std::uint8_t new_stage) override {
        calls.push_back("log");
        last_player_id = player_id;
        last_current_stage = current_stage;
        last_new_stage = new_stage;
    }
};

JobChangeValidationInput PassingGates() {
    JobChangeValidationInput in;
    in.stage_is_hwa_or_geuk = true;
    in.slot_exists = true;
    in.item_exists = true;
    in.item_icon_is_change_job = true;
    in.item_db_idx_matches = true;
    in.discard_returned_true = true;
    return in;
}

}  // namespace

TEST(ApplyJobChangeSideEffects, SuccessEmitsFiveStepChainInOrder) {
    auto in = PassingGates();
    auto plan = job_change_side_effect_plan(
        in, /*player_id=*/0x00040005u,
        /*current_stage=*/LEGACY_ESTAGE_HWA,
        /*ability_group=*/77);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.discard_item);
    EXPECT_TRUE(plan.change_stage_ability);
    EXPECT_TRUE(plan.set_stage);
    EXPECT_TRUE(plan.log_item_money);
    ASSERT_EQ(plan.effects.size(), 5u);
    EXPECT_EQ(plan.effects[0].kind,
              JobChangeSideEffectKind::DiscardChangeJobItem);
    EXPECT_EQ(plan.effects[1].kind,
              JobChangeSideEffectKind::ChangeCharacterStageAbility);
    EXPECT_EQ(plan.effects[2].kind,
              JobChangeSideEffectKind::SetStage);
    EXPECT_EQ(plan.effects[3].kind,
              JobChangeSideEffectKind::SendAckToPlayer);
    EXPECT_EQ(plan.effects[4].kind,
              JobChangeSideEffectKind::LogItemMoney);
    EXPECT_EQ(plan.effects[2].new_stage, LEGACY_ESTAGE_GEUK);

    RecordingSink sink;
    auto out = apply_job_change_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 5u);
    EXPECT_EQ(out.discards, 1u);
    EXPECT_EQ(out.ability_changes, 1u);
    EXPECT_EQ(out.stage_sets, 1u);
    EXPECT_EQ(out.acks_sent, 1u);
    EXPECT_EQ(out.money_logs, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_TRUE(out.ack_flag_consumed);
    EXPECT_TRUE(out.discard_flag_consumed);
    EXPECT_TRUE(out.ability_flag_consumed);
    EXPECT_TRUE(out.stage_flag_consumed);
    EXPECT_TRUE(out.log_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>(
                  {"discard", "ability", "setstage", "ack", "log"}));
    EXPECT_EQ(sink.last_player_id, 0x00040005u);
    EXPECT_EQ(sink.last_current_stage, LEGACY_ESTAGE_HWA);
    EXPECT_EQ(sink.last_new_stage, LEGACY_ESTAGE_GEUK);
    EXPECT_EQ(sink.last_ability_group, 77u);
    EXPECT_EQ(sink.ack_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyJobChangeSideEffects, GeukToHwaFlipsStage) {
    auto in = PassingGates();
    auto plan = job_change_side_effect_plan(
        in, 1, LEGACY_ESTAGE_GEUK, 0);
    EXPECT_EQ(plan.effects[2].new_stage, LEGACY_ESTAGE_HWA);

    RecordingSink sink;
    (void)apply_job_change_side_effects(plan, sink);
    EXPECT_EQ(sink.last_current_stage, LEGACY_ESTAGE_GEUK);
    EXPECT_EQ(sink.last_new_stage, LEGACY_ESTAGE_HWA);
}

TEST(ApplyJobChangeSideEffects, BadStageEmitsNackCode1) {
    auto in = PassingGates();
    in.stage_is_hwa_or_geuk = false;
    auto plan = job_change_side_effect_plan(in, 7, LEGACY_ESTAGE_HWA, 0);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_FALSE(plan.send_ack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              JobChangeSideEffectKind::SendNackToPlayer);
    EXPECT_EQ(plan.effects[0].nack_code, 1u);

    RecordingSink sink;
    auto out = apply_job_change_side_effects(plan, sink);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"nack"}));
    EXPECT_EQ(sink.last_nack_code, 1u);
    EXPECT_EQ(sink.last_player_id, 7u);
}

TEST(ApplyJobChangeSideEffects, BadItemEmitsNackCode2) {
    auto in = PassingGates();
    in.slot_exists = false;
    auto plan = job_change_side_effect_plan(in, 7, 1, 0);
    EXPECT_EQ(plan.effects[0].nack_code, 2u);

    auto in2 = PassingGates();
    in2.item_icon_is_change_job = false;
    auto plan2 = job_change_side_effect_plan(in2, 7, 1, 0);
    EXPECT_EQ(plan2.effects[0].nack_code, 2u);

    auto in3 = PassingGates();
    in3.item_db_idx_matches = false;
    auto plan3 = job_change_side_effect_plan(in3, 7, 1, 0);
    EXPECT_EQ(plan3.effects[0].nack_code, 2u);
}

TEST(ApplyJobChangeSideEffects, DiscardFailedEmitsNackCode3) {
    auto in = PassingGates();
    in.discard_returned_true = false;
    auto plan = job_change_side_effect_plan(in, 7, 1, 0);
    EXPECT_EQ(plan.effects[0].nack_code, 3u);

    RecordingSink sink;
    (void)apply_job_change_side_effects(plan, sink);
    EXPECT_EQ(sink.last_nack_code, 3u);
}

TEST(ApplyJobChangeSideEffects, GatePrecedenceLocked) {
    // Bad stage outranks bad item.
    auto in = PassingGates();
    in.stage_is_hwa_or_geuk = false;
    in.slot_exists = false;
    auto plan = job_change_side_effect_plan(in, 7, 1, 0);
    EXPECT_EQ(plan.effects[0].nack_code, 1u);

    // Bad item outranks discard failure.
    auto in2 = PassingGates();
    in2.item_exists = false;
    in2.discard_returned_true = false;
    auto plan2 = job_change_side_effect_plan(in2, 7, 1, 0);
    EXPECT_EQ(plan2.effects[0].nack_code, 2u);
}

TEST(ApplyJobChangeSideEffects, EmptyPlanIsNoOp) {
    mxh::server::JobChangeSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_job_change_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.discards, 0u);
    EXPECT_EQ(out.ability_changes, 0u);
    EXPECT_EQ(out.stage_sets, 0u);
    EXPECT_EQ(out.money_logs, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_FALSE(out.discard_flag_consumed);
    EXPECT_FALSE(out.ability_flag_consumed);
    EXPECT_FALSE(out.stage_flag_consumed);
    EXPECT_FALSE(out.log_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}
