// revive_other_nack_side_effect_runtime_test.cpp
//
// Verifies apply_revive_other_nack_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEM_SHOPITEM_REVIVEOTHER_NACK
// side-effect chain) walks the data-plane plan and dispatches each
// entry: NACK forward -> revive-data clear -> revive-time clear in
// legacy order / empty plan when either player is missing.

#include <mxh/server/revive_other_nack_side_effect.hpp>
#include <mxh/server/revive_other_nack_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::ReviveOtherNackSideEffectKind;
using mxh::server::ReviveOtherNackSideEffectSink;
using mxh::server::ReviveOtherNackValidationInput;
using mxh::server::apply_revive_other_nack_side_effects;
using mxh::server::revive_other_nack_side_effect_plan;

class RecordingSink final : public ReviveOtherNackSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_target_id = 0;
    std::uint32_t last_nack_code = 0;
    std::size_t forward_count = 0;
    std::size_t data_clear_count = 0;
    std::size_t time_clear_count = 0;

    void forward_nack_to_target(std::uint32_t target_id,
                                std::uint32_t nack_code) override {
        calls.push_back("nack");
        last_target_id = target_id;
        last_nack_code = nack_code;
        ++forward_count;
    }
    void clear_revive_data_on_target(std::uint32_t target_id) override {
        calls.push_back("cleardata");
        last_target_id = target_id;
        ++data_clear_count;
    }
    void clear_revive_time_on_target(std::uint32_t target_id) override {
        calls.push_back("cleartime");
        last_target_id = target_id;
        ++time_clear_count;
    }
};

}  // namespace

TEST(ApplyReviveOtherNackSideEffects, ForwardedEmitsThreeStepChainInOrder) {
    ReviveOtherNackValidationInput in;
    in.player_found = true;
    in.target_found = true;
    auto plan = revive_other_nack_side_effect_plan(
        in, /*target_id=*/0x00100011u, /*nack_code=*/2);
    EXPECT_TRUE(plan.forward_nack);
    EXPECT_TRUE(plan.clear_revive_data);
    EXPECT_TRUE(plan.clear_revive_time);
    ASSERT_EQ(plan.effects.size(), 3u);
    EXPECT_EQ(plan.effects[0].kind,
              ReviveOtherNackSideEffectKind::ForwardNackToTarget);
    EXPECT_EQ(plan.effects[1].kind,
              ReviveOtherNackSideEffectKind::ClearReviveDataOnTarget);
    EXPECT_EQ(plan.effects[2].kind,
              ReviveOtherNackSideEffectKind::ClearReviveTimeOnTarget);
    EXPECT_EQ(plan.effects[0].nack_code, 2u);

    RecordingSink sink;
    auto out = apply_revive_other_nack_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 3u);
    EXPECT_EQ(out.nacks_forwarded, 1u);
    EXPECT_EQ(out.revive_data_clears, 1u);
    EXPECT_EQ(out.revive_time_clears, 1u);
    EXPECT_TRUE(out.forward_flag_consumed);
    EXPECT_TRUE(out.data_flag_consumed);
    EXPECT_TRUE(out.time_flag_consumed);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>({"nack", "cleardata", "cleartime"}));
    EXPECT_EQ(sink.last_target_id, 0x00100011u);
    EXPECT_EQ(sink.last_nack_code, 2u);
    EXPECT_EQ(sink.forward_count, 1u);
    EXPECT_EQ(sink.data_clear_count, 1u);
    EXPECT_EQ(sink.time_clear_count, 1u);
}

TEST(ApplyReviveOtherNackSideEffects, NoPlayerEmitsEmptyPlan) {
    ReviveOtherNackValidationInput no_player;
    no_player.player_found = false;
    no_player.target_found = true;
    auto plan1 = revive_other_nack_side_effect_plan(
        no_player, 7, 3);
    EXPECT_FALSE(plan1.forward_nack);
    EXPECT_TRUE(plan1.effects.empty());

    ReviveOtherNackValidationInput no_target;
    no_target.player_found = true;
    no_target.target_found = false;
    auto plan2 = revive_other_nack_side_effect_plan(
        no_target, 7, 3);
    EXPECT_FALSE(plan2.forward_nack);
    EXPECT_TRUE(plan2.effects.empty());

    RecordingSink sink;
    auto out = apply_revive_other_nack_side_effects(plan1, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.nacks_forwarded, 0u);
    EXPECT_EQ(out.revive_data_clears, 0u);
    EXPECT_EQ(out.revive_time_clears, 0u);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyReviveOtherNackSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ReviveOtherNackSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_revive_other_nack_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.nacks_forwarded, 0u);
    EXPECT_EQ(out.revive_data_clears, 0u);
    EXPECT_EQ(out.revive_time_clears, 0u);
    EXPECT_FALSE(out.forward_flag_consumed);
    EXPECT_FALSE(out.data_flag_consumed);
    EXPECT_FALSE(out.time_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyReviveOtherNackSideEffects, BoundaryNackCodePassthrough) {
    ReviveOtherNackValidationInput in;
    in.player_found = true;
    in.target_found = true;
    auto plan = revive_other_nack_side_effect_plan(
        in, /*target_id=*/7, /*nack_code=*/0xFFFFFFFFu);
    RecordingSink sink;
    (void)apply_revive_other_nack_side_effects(plan, sink);
    EXPECT_EQ(sink.last_nack_code, 0xFFFFFFFFu);
    EXPECT_EQ(sink.last_target_id, 7u);
}

TEST(ApplyReviveOtherNackSideEffects, BoundaryTargetIdPassthrough) {
    ReviveOtherNackValidationInput in;
    in.player_found = true;
    in.target_found = true;
    auto plan = revive_other_nack_side_effect_plan(
        in, /*target_id=*/0xFFFFFFFFu, /*nack_code=*/1);
    RecordingSink sink;
    (void)apply_revive_other_nack_side_effects(plan, sink);
    EXPECT_EQ(sink.last_target_id, 0xFFFFFFFFu);
    EXPECT_EQ(sink.last_nack_code, 1u);
}

TEST(ApplyReviveOtherNackSideEffects, AllClearsFollowForward) {
    // The clear steps never run without the forward step in the same
    // plan (legacy returns before any mutation when a player is
    // missing).
    ReviveOtherNackValidationInput in;
    in.player_found = true;
    in.target_found = true;
    auto plan = revive_other_nack_side_effect_plan(in, 3, 9);
    RecordingSink sink;
    auto out = apply_revive_other_nack_side_effects(plan, sink);
    EXPECT_EQ(out.nacks_forwarded, 1u);
    EXPECT_EQ(out.revive_data_clears, 1u);
    EXPECT_EQ(out.revive_time_clears, 1u);
    EXPECT_EQ(sink.forward_count, 1u);
    EXPECT_EQ(sink.data_clear_count, 1u);
    EXPECT_EQ(sink.time_clear_count, 1u);
}
