// Tests for MP_ITEM_SHOPITEM_AVATAR_USE_SYN side-effect dispatcher.

#include <mxh/server/avatar_use_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

AvatarUseValidationInput success_input() {
    AvatarUseValidationInput in{};
    in.state_is_none_or_immortal = true;
    in.item_is_useable = true;
    in.item_base_exists = true;
    in.weapon_to_shop_item_ok = true;
    in.item_in_using_list = true;
    in.using_list_db_idx_matches = true;
    in.put_on_avatar_item_ok = true;
    return in;
}

TEST(AvatarUseOutcome, AllGatesPassAndPutOnReturnsSuccess) {
    EXPECT_EQ(classify_avatar_use_outcome(success_input()),
              AvatarUseOutcome::Success);
}

TEST(AvatarUseOutcome, StateGateFailIsGateFailed) {
    auto in = success_input();
    in.state_is_none_or_immortal = false;
    EXPECT_EQ(classify_avatar_use_outcome(in),
              AvatarUseOutcome::GateFailed);
}

TEST(AvatarUseOutcome, UseableGateFailIsGateFailed) {
    auto in = success_input();
    in.item_is_useable = false;
    EXPECT_EQ(classify_avatar_use_outcome(in),
              AvatarUseOutcome::GateFailed);
}

TEST(AvatarUseOutcome, ItemBaseGateFailIsGateFailed) {
    auto in = success_input();
    in.item_base_exists = false;
    EXPECT_EQ(classify_avatar_use_outcome(in),
              AvatarUseOutcome::GateFailed);
}

TEST(AvatarUseOutcome, WeaponGateFailIsGateFailed) {
    auto in = success_input();
    in.weapon_to_shop_item_ok = false;
    EXPECT_EQ(classify_avatar_use_outcome(in),
              AvatarUseOutcome::GateFailed);
}

TEST(AvatarUseOutcome, NotInUsingListIsAsyncDbQuery) {
    auto in = success_input();
    in.item_in_using_list = false;
    EXPECT_EQ(classify_avatar_use_outcome(in),
              AvatarUseOutcome::AsyncDbQuery);
}

TEST(AvatarUseOutcome, UsingListDbIdxMismatchIsUsingListMismatch) {
    auto in = success_input();
    in.using_list_db_idx_matches = false;
    EXPECT_EQ(classify_avatar_use_outcome(in),
              AvatarUseOutcome::UsingListMismatch);
}

TEST(AvatarUseOutcome, PutOnReturnsFalseIsPutOnFailed) {
    auto in = success_input();
    in.put_on_avatar_item_ok = false;
    EXPECT_EQ(classify_avatar_use_outcome(in),
              AvatarUseOutcome::PutOnFailed);
}

TEST(AvatarUseOutcome, GateFailedTakesPrecedenceOverAsyncDb) {
    auto in = success_input();
    in.state_is_none_or_immortal = false;
    in.item_in_using_list = false;
    EXPECT_EQ(classify_avatar_use_outcome(in),
              AvatarUseOutcome::GateFailed);
}

TEST(AvatarUsePlan, SuccessEmitsPutOnThenAck) {
    auto in = success_input();
    auto plan = avatar_use_side_effect_plan(in, 100, 50, 6, 999, 1234, 7);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_TRUE(plan.put_on_avatar_item);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_FALSE(plan.query_db);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              AvatarUseSideEffectKind::PutOnAvatarItem);
    EXPECT_EQ(plan.effects[0].player_id, 100u);
    EXPECT_EQ(plan.effects[0].item_idx, 50u);
    EXPECT_EQ(plan.effects[1].kind,
              AvatarUseSideEffectKind::SendAckToPlayer);
}

TEST(AvatarUsePlan, GateFailedSendsNack) {
    auto in = success_input();
    in.state_is_none_or_immortal = false;
    auto plan = avatar_use_side_effect_plan(in, 100, 50, 6, 999, 1234, 7);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_FALSE(plan.put_on_avatar_item);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              AvatarUseSideEffectKind::SendNackToPlayer);
    EXPECT_EQ(plan.effects[0].player_id, 100u);
}

TEST(AvatarUsePlan, UsingListMismatchSendsNack) {
    auto in = success_input();
    in.using_list_db_idx_matches = false;
    auto plan = avatar_use_side_effect_plan(in, 100, 50, 6, 999, 1234, 7);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              AvatarUseSideEffectKind::SendNackToPlayer);
}

TEST(AvatarUsePlan, PutOnFailedSendsNack) {
    auto in = success_input();
    in.put_on_avatar_item_ok = false;
    auto plan = avatar_use_side_effect_plan(in, 100, 50, 6, 999, 1234, 7);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              AvatarUseSideEffectKind::SendNackToPlayer);
}

TEST(AvatarUsePlan, AsyncDbQueryEmitsDbCallOnly) {
    auto in = success_input();
    in.item_in_using_list = false;
    auto plan = avatar_use_side_effect_plan(in, 100, 50, 6, 999, 1234, 7);
    EXPECT_TRUE(plan.query_db);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_FALSE(plan.put_on_avatar_item);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              AvatarUseSideEffectKind::QueryDbForAvatarItem);
    EXPECT_EQ(plan.effects[0].player_id, 100u);
    EXPECT_EQ(plan.effects[0].item_db_idx, 999u);
    EXPECT_EQ(plan.effects[0].item_icon_idx, 1234u);
    EXPECT_EQ(plan.effects[0].item_position, 7u);
}

TEST(AvatarUsePlan, PlanIsIdempotent) {
    auto in = success_input();
    auto a = avatar_use_side_effect_plan(in, 100, 50, 6, 999, 1234, 7);
    auto b = avatar_use_side_effect_plan(in, 100, 50, 6, 999, 1234, 7);
    EXPECT_EQ(a.send_ack, b.send_ack);
    EXPECT_EQ(a.put_on_avatar_item, b.put_on_avatar_item);
    EXPECT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].player_id, b.effects[i].player_id);
        EXPECT_EQ(a.effects[i].item_idx, b.effects[i].item_idx);
    }
}
