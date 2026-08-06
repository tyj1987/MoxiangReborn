// Tests for MP_ITEM_SHOPITEM_REVIVEOTHER_NACK side-effect dispatcher.

#include <mxh/server/revive_other_nack_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

ReviveOtherNackValidationInput both_found() {
    ReviveOtherNackValidationInput in{};
    in.player_found = true;
    in.target_found = true;
    return in;
}

TEST(ReviveOtherNackOutcome, BothPlayersFoundIsForwarded) {
    EXPECT_EQ(classify_revive_other_nack_outcome(both_found()),
              ReviveOtherNackOutcome::Forwarded);
}

TEST(ReviveOtherNackOutcome, PlayerMissingIsNoPlayer) {
    ReviveOtherNackValidationInput in{};
    in.target_found = true;
    EXPECT_EQ(classify_revive_other_nack_outcome(in),
              ReviveOtherNackOutcome::NoPlayer);
}

TEST(ReviveOtherNackOutcome, TargetMissingIsNoPlayer) {
    ReviveOtherNackValidationInput in{};
    in.player_found = true;
    EXPECT_EQ(classify_revive_other_nack_outcome(in),
              ReviveOtherNackOutcome::NoPlayer);
}

TEST(ReviveOtherNackOutcome, BothMissingIsNoPlayer) {
    EXPECT_EQ(classify_revive_other_nack_outcome({}),
              ReviveOtherNackOutcome::NoPlayer);
}

TEST(ReviveOtherNackPlan, ForwardedEmitsNackAndClearsReviveData) {
    auto in = both_found();
    auto plan = revive_other_nack_side_effect_plan(in, 100, 5);
    EXPECT_TRUE(plan.forward_nack);
    EXPECT_TRUE(plan.clear_revive_data);
    EXPECT_TRUE(plan.clear_revive_time);
    EXPECT_EQ(plan.effects.size(), 3u);
    EXPECT_EQ(plan.effects[0].kind,
              ReviveOtherNackSideEffectKind::ForwardNackToTarget);
    EXPECT_EQ(plan.effects[0].target_id, 100u);
    EXPECT_EQ(plan.effects[0].nack_code, 5u);
    EXPECT_EQ(plan.effects[1].kind,
              ReviveOtherNackSideEffectKind::ClearReviveDataOnTarget);
    EXPECT_EQ(plan.effects[2].kind,
              ReviveOtherNackSideEffectKind::ClearReviveTimeOnTarget);
}

TEST(ReviveOtherNackPlan, NoPlayerEmitsEmptyPlan) {
    ReviveOtherNackValidationInput in{};
    in.player_found = false;
    in.target_found = true;
    auto plan = revive_other_nack_side_effect_plan(in, 100, 5);
    EXPECT_FALSE(plan.forward_nack);
    EXPECT_FALSE(plan.clear_revive_data);
    EXPECT_FALSE(plan.clear_revive_time);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(ReviveOtherNackPlan, PlanIsIdempotent) {
    auto in = both_found();
    auto a = revive_other_nack_side_effect_plan(in, 100, 5);
    auto b = revive_other_nack_side_effect_plan(in, 100, 5);
    EXPECT_EQ(a.forward_nack, b.forward_nack);
    EXPECT_EQ(a.clear_revive_data, b.clear_revive_data);
    EXPECT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].target_id, b.effects[i].target_id);
        EXPECT_EQ(a.effects[i].nack_code, b.effects[i].nack_code);
    }
}
