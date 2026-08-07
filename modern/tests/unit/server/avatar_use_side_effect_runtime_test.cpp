// avatar_use_side_effect_runtime_test.cpp
//
// Verifies apply_avatar_use_side_effects() (the runtime orchestrator
// for the CItemManager::MP_ITEM_SHOPITEM_AVATAR_USE_SYN side-effect
// chain) walks the data-plane plan and dispatches each entry:
// put-on-then-ack on success / async DB query when not in the using
// list / NACK for the three failure categories.

#include <mxh/server/avatar_use_side_effect.hpp>
#include <mxh/server/avatar_use_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::AvatarUseSideEffectKind;
using mxh::server::AvatarUseSideEffectSink;
using mxh::server::apply_avatar_use_side_effects;
using mxh::server::avatar_use_side_effect_plan;

class RecordingSink final : public AvatarUseSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_player_id = 0;
    std::uint16_t last_item_idx = 0;
    std::uint16_t last_item_pos = 0;
    std::uint32_t last_item_db_idx = 0;
    std::uint32_t last_item_icon_idx = 0;
    std::uint32_t last_item_position = 0;

    void send_ack_to_player(std::uint32_t player_id,
                            std::uint16_t item_idx,
                            std::uint16_t item_pos) override {
        calls.push_back("ack");
        last_player_id = player_id;
        last_item_idx = item_idx;
        last_item_pos = item_pos;
    }
    void send_nack_to_player(std::uint32_t player_id,
                             std::uint16_t item_idx,
                             std::uint16_t item_pos) override {
        calls.push_back("nack");
        last_player_id = player_id;
        last_item_idx = item_idx;
        last_item_pos = item_pos;
    }
    void put_on_avatar_item(std::uint32_t player_id,
                            std::uint16_t item_idx,
                            std::uint16_t item_pos) override {
        calls.push_back("put");
        last_player_id = player_id;
        last_item_idx = item_idx;
        last_item_pos = item_pos;
    }
    void query_db_for_avatar_item(
        std::uint32_t player_id, std::uint32_t item_db_idx,
        std::uint32_t item_icon_idx, std::uint32_t item_position) override {
        calls.push_back("db");
        last_player_id = player_id;
        last_item_db_idx = item_db_idx;
        last_item_icon_idx = item_icon_idx;
        last_item_position = item_position;
    }
};

}  // namespace

TEST(ApplyAvatarUseSideEffects, SuccessEmitsPutThenAckInOrder) {
    mxh::server::AvatarUseValidationInput in;
    in.state_is_none_or_immortal = true;
    in.item_is_useable = true;
    in.item_base_exists = true;
    in.weapon_to_shop_item_ok = true;
    in.item_in_using_list = true;
    in.using_list_db_idx_matches = true;
    in.put_on_avatar_item_ok = true;
    auto plan = avatar_use_side_effect_plan(
        in, /*player_id=*/0x00010002u, /*item_idx=*/100,
        /*item_pos=*/7, 0, 0, 0);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_TRUE(plan.put_on_avatar_item);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_FALSE(plan.query_db);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              AvatarUseSideEffectKind::PutOnAvatarItem);
    EXPECT_EQ(plan.effects[1].kind,
              AvatarUseSideEffectKind::SendAckToPlayer);

    RecordingSink sink;
    auto out = apply_avatar_use_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 2u);
    EXPECT_EQ(out.puts, 1u);
    EXPECT_EQ(out.acks_sent, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.db_queries, 0u);
    EXPECT_TRUE(out.put_flag_consumed);
    EXPECT_TRUE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_FALSE(out.db_flag_consumed);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"put", "ack"}));
    EXPECT_EQ(sink.last_player_id, 0x00010002u);
    EXPECT_EQ(sink.last_item_idx, 100u);
    EXPECT_EQ(sink.last_item_pos, 7u);
}

TEST(ApplyAvatarUseSideEffects, NotInUsingListEmitsAsyncDbQuery) {
    mxh::server::AvatarUseValidationInput in;
    in.state_is_none_or_immortal = true;
    in.item_is_useable = true;
    in.item_base_exists = true;
    in.weapon_to_shop_item_ok = true;
    in.item_in_using_list = false;
    auto plan = avatar_use_side_effect_plan(
        in, /*player_id=*/7, 100, 7,
        /*item_db_idx=*/1, /*item_icon_idx=*/2, /*item_position=*/3);
    EXPECT_TRUE(plan.query_db);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              AvatarUseSideEffectKind::QueryDbForAvatarItem);

    RecordingSink sink;
    auto out = apply_avatar_use_side_effects(plan, sink);
    EXPECT_EQ(out.db_queries, 1u);
    EXPECT_TRUE(out.db_flag_consumed);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"db"}));
    EXPECT_EQ(sink.last_player_id, 7u);
    EXPECT_EQ(sink.last_item_db_idx, 1u);
    EXPECT_EQ(sink.last_item_icon_idx, 2u);
    EXPECT_EQ(sink.last_item_position, 3u);
}

TEST(ApplyAvatarUseSideEffects, GateFailedEmitsNack) {
    mxh::server::AvatarUseValidationInput in;
    in.state_is_none_or_immortal = false;  // gate 1 fails
    in.item_is_useable = true;
    in.item_base_exists = true;
    in.weapon_to_shop_item_ok = true;
    in.item_in_using_list = true;
    in.using_list_db_idx_matches = true;
    in.put_on_avatar_item_ok = true;
    auto plan = avatar_use_side_effect_plan(in, 7, 100, 7, 0, 0, 0);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              AvatarUseSideEffectKind::SendNackToPlayer);

    RecordingSink sink;
    auto out = apply_avatar_use_side_effects(plan, sink);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_TRUE(out.nack_flag_consumed);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"nack"}));
    EXPECT_EQ(sink.last_player_id, 7u);
    EXPECT_EQ(sink.last_item_idx, 100u);
    EXPECT_EQ(sink.last_item_pos, 7u);
}

TEST(ApplyAvatarUseSideEffects, UsingListMismatchEmitsNack) {
    mxh::server::AvatarUseValidationInput in;
    in.state_is_none_or_immortal = true;
    in.item_is_useable = true;
    in.item_base_exists = true;
    in.weapon_to_shop_item_ok = true;
    in.item_in_using_list = true;
    in.using_list_db_idx_matches = false;
    in.put_on_avatar_item_ok = true;
    auto plan = avatar_use_side_effect_plan(in, 7, 100, 7, 0, 0, 0);
    EXPECT_TRUE(plan.send_nack);

    RecordingSink sink;
    (void)apply_avatar_use_side_effects(plan, sink);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"nack"}));
}

TEST(ApplyAvatarUseSideEffects, PutOnFailedEmitsNack) {
    mxh::server::AvatarUseValidationInput in;
    in.state_is_none_or_immortal = true;
    in.item_is_useable = true;
    in.item_base_exists = true;
    in.weapon_to_shop_item_ok = true;
    in.item_in_using_list = true;
    in.using_list_db_idx_matches = true;
    in.put_on_avatar_item_ok = false;
    auto plan = avatar_use_side_effect_plan(in, 7, 100, 7, 0, 0, 0);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_FALSE(plan.send_ack);

    RecordingSink sink;
    (void)apply_avatar_use_side_effects(plan, sink);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"nack"}));
}

TEST(ApplyAvatarUseSideEffects, EmptyPlanIsNoOp) {
    mxh::server::AvatarUseSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_avatar_use_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.puts, 0u);
    EXPECT_EQ(out.db_queries, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_FALSE(out.put_flag_consumed);
    EXPECT_FALSE(out.db_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}
