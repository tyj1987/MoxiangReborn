// guild_move_side_effect_runtime_test.cpp
//
// Verifies apply_guild_move_side_effects() (the runtime orchestrator
// for the CItemManager::MP_ITEM_GUILD_MOVE_SYN side-effect chain)
// walks the data-plane plan and dispatches the single entry: ACK on
// MoveItem success / NACK with error code on failure / no-op when the
// player is missing.

#include <mxh/server/guild_move_side_effect.hpp>
#include <mxh/server/guild_move_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using mxh::server::GuildMoveSideEffectKind;
using mxh::server::GuildMoveSideEffectSink;
using mxh::server::LEGACY_GUILD_MOVE_ERR_NOT_MOVEABLE;
using mxh::server::apply_guild_move_side_effects;
using mxh::server::guild_move_side_effect_plan;

class RecordingSink final : public GuildMoveSideEffectSink {
public:
    std::string last_call;
    int last_rt = 0;
    int last_error_code = 0;
    std::uint16_t last_from_pos = 0;
    std::uint16_t last_to_pos = 0;
    std::uint16_t last_from_item_idx = 0;
    std::uint16_t last_to_item_idx = 0;
    std::size_t ack_count = 0;
    std::size_t nack_count = 0;

    void broadcast_guild_move_ack(std::uint16_t from_pos,
                                  std::uint16_t to_pos,
                                  std::uint16_t from_item_idx,
                                  std::uint16_t to_item_idx,
                                  int original_rt) override {
        last_call = "ack";
        last_from_pos = from_pos;
        last_to_pos = to_pos;
        last_from_item_idx = from_item_idx;
        last_to_item_idx = to_item_idx;
        last_rt = original_rt;
        ++ack_count;
    }
    void broadcast_guild_move_nack(std::uint16_t from_pos,
                                   std::uint16_t to_pos,
                                   std::uint16_t from_item_idx,
                                   std::uint16_t to_item_idx,
                                   int original_rt,
                                   int error_code) override {
        last_call = "nack";
        last_from_pos = from_pos;
        last_to_pos = to_pos;
        last_from_item_idx = from_item_idx;
        last_to_item_idx = to_item_idx;
        last_rt = original_rt;
        last_error_code = error_code;
        ++nack_count;
    }
};

}  // namespace

TEST(ApplyGuildMoveSideEffects, SuccessEmitsAck) {
    mxh::server::GuildMoveValidationInput in;
    in.player_found = true;
    in.can_move = true;
    in.move_rt = 0;
    auto plan = guild_move_side_effect_plan(
        in, 1, 2, 3, 4);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              GuildMoveSideEffectKind::BroadcastGuildMoveAck);

    RecordingSink sink;
    auto out = apply_guild_move_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.acks_sent, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_TRUE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "ack");
    EXPECT_EQ(sink.last_from_pos, 1u);
    EXPECT_EQ(sink.last_to_pos, 2u);
    EXPECT_EQ(sink.last_from_item_idx, 3u);
    EXPECT_EQ(sink.last_to_item_idx, 4u);
    EXPECT_EQ(sink.last_rt, 0);
    EXPECT_EQ(sink.ack_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyGuildMoveSideEffects, NotMoveableEmitsNackWithCode4) {
    mxh::server::GuildMoveValidationInput in;
    in.player_found = true;
    in.can_move = false;
    in.move_rt = 0;
    auto plan = guild_move_side_effect_plan(in, 1, 2, 3, 4);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              GuildMoveSideEffectKind::BroadcastGuildMoveNack);
    EXPECT_EQ(plan.effects[0].error_code,
              LEGACY_GUILD_MOVE_ERR_NOT_MOVEABLE);

    RecordingSink sink;
    auto out = apply_guild_move_side_effects(plan, sink);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_TRUE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_error_code, LEGACY_GUILD_MOVE_ERR_NOT_MOVEABLE);
    EXPECT_EQ(sink.nack_count, 1u);
}

TEST(ApplyGuildMoveSideEffects, FailureEmitsNackWithRt) {
    mxh::server::GuildMoveValidationInput in;
    in.player_found = true;
    in.can_move = true;
    in.move_rt = 7;
    auto plan = guild_move_side_effect_plan(in, 1, 2, 3, 4);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              GuildMoveSideEffectKind::BroadcastGuildMoveNack);
    EXPECT_EQ(plan.effects[0].error_code, 7);

    RecordingSink sink;
    (void)apply_guild_move_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_rt, 7);
    EXPECT_EQ(sink.last_error_code, 7);
}

TEST(ApplyGuildMoveSideEffects, NoPlayerIsNoOp) {
    mxh::server::GuildMoveValidationInput in;
    in.player_found = false;
    in.can_move = true;
    in.move_rt = 0;
    auto plan = guild_move_side_effect_plan(in, 1, 2, 3, 4);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_guild_move_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyGuildMoveSideEffects, EmptyPlanIsNoOp) {
    mxh::server::GuildMoveSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_guild_move_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyGuildMoveSideEffects, GatePrecedenceNoPlayerOverOthers) {
    // classify: NoPlayer wins over NotMoveable and Failure.
    mxh::server::GuildMoveValidationInput in;
    in.player_found = false;
    in.can_move = false;
    in.move_rt = 5;
    auto plan = guild_move_side_effect_plan(in, 1, 2, 3, 4);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    (void)apply_guild_move_side_effects(plan, sink);
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyGuildMoveSideEffects, FieldPassthroughOnNack) {
    mxh::server::GuildMoveValidationInput in;
    in.player_found = true;
    in.can_move = true;
    in.move_rt = 9;
    auto plan = guild_move_side_effect_plan(in, 11, 12, 13, 14);
    RecordingSink sink;
    (void)apply_guild_move_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_from_pos, 11u);
    EXPECT_EQ(sink.last_to_pos, 12u);
    EXPECT_EQ(sink.last_from_item_idx, 13u);
    EXPECT_EQ(sink.last_to_item_idx, 14u);
    EXPECT_EQ(sink.last_rt, 9);
    EXPECT_EQ(sink.last_error_code, 9);
    EXPECT_EQ(sink.ack_count, 0u);
}
