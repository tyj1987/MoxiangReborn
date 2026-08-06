// D4.68 GuildMove (MP_ITEM_GUILD_MOVE_SYN) side-effect
// dispatcher tests.

#include <mxh/server/guild_move_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

GuildMoveValidationInput ok() {
    GuildMoveValidationInput in{};
    in.player_found = true;
    in.can_move = true;
    in.move_rt = 0;
    return in;
}

TEST(GuildMoveOutcome, ZeroRtIsSuccess) {
    auto in = ok();
    EXPECT_EQ(classify_guild_move_outcome(in),
              GuildMoveOutcome::Success);
}

TEST(GuildMoveOutcome, NonZeroRtIsFailure) {
    auto in = ok();
    in.move_rt = 5;
    EXPECT_EQ(classify_guild_move_outcome(in),
              GuildMoveOutcome::Failure);
}

TEST(GuildMoveOutcome, NotMoveableTakesPrecedence) {
    auto in = ok();
    in.can_move = false;
    in.move_rt = 0;
    EXPECT_EQ(classify_guild_move_outcome(in),
              GuildMoveOutcome::NotMoveable);
}

TEST(GuildMoveOutcome, NoPlayerTakesPrecedence) {
    auto in = ok();
    in.player_found = false;
    in.can_move = false;
    in.move_rt = 1;
    EXPECT_EQ(classify_guild_move_outcome(in),
              GuildMoveOutcome::NoPlayer);
}

TEST(GuildMovePlan, SuccessEmitsAck) {
    auto in = ok();
    auto plan = guild_move_side_effect_plan(
        in, /*from_pos=*/10, /*to_pos=*/20,
        /*from_item_idx=*/1, /*to_item_idx=*/2);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              GuildMoveSideEffectKind::BroadcastGuildMoveAck);
    EXPECT_EQ(plan.effects[0].from_pos, 10u);
    EXPECT_EQ(plan.effects[0].to_pos, 20u);
    EXPECT_EQ(plan.effects[0].from_item_idx, 1u);
    EXPECT_EQ(plan.effects[0].to_item_idx, 2u);
}

TEST(GuildMovePlan, FailureEmitsNackWithRt) {
    auto in = ok();
    in.move_rt = 5;
    auto plan = guild_move_side_effect_plan(in, 1, 2, 3, 4);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.error_code, 5);
    EXPECT_EQ(plan.effects[0].error_code, 5);
}

TEST(GuildMovePlan, NotMoveableEmitsNackWithCode4) {
    auto in = ok();
    in.can_move = false;
    auto plan = guild_move_side_effect_plan(in, 1, 2, 3, 4);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.error_code, LEGACY_GUILD_MOVE_ERR_NOT_MOVEABLE);
    EXPECT_EQ(plan.effects[0].error_code,
              LEGACY_GUILD_MOVE_ERR_NOT_MOVEABLE);
}

TEST(GuildMovePlan, NoPlayerEmitsEmptyPlan) {
    auto in = ok();
    in.player_found = false;
    auto plan = guild_move_side_effect_plan(in, 1, 2, 3, 4);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(GuildMovePlan, PlanIsIdempotent) {
    auto in = ok();
    auto a = guild_move_side_effect_plan(in, 1, 2, 3, 4);
    auto b = guild_move_side_effect_plan(in, 1, 2, 3, 4);
    EXPECT_EQ(a.send_ack, b.send_ack);
    EXPECT_EQ(a.send_nack, b.send_nack);
    EXPECT_EQ(a.error_code, b.error_code);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].from_pos, b.effects[i].from_pos);
        EXPECT_EQ(a.effects[i].to_pos, b.effects[i].to_pos);
    }
}
