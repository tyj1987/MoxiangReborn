//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_JACKPOTUserMsgParser (lines 4616-4619) and MP_JACKPOTServerMsgParser
// (lines 4621-4673). Each test pins one branch of the legacy dispatch to its
// modern side-effect plan output so future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_jackpot.hpp"
#include "mxh/server/agent_jackpot_side_effect_plan.hpp"

using namespace mxh::server;

namespace {
constexpr std::uint32_t kObjectId = 0xAABBCCDDu;
constexpr std::uint32_t kMoney = 0x12345678u;
}

TEST(JackpotPlan, UserSideAlwaysEmitsDrop) {
    JackpotUserAction a;
    a.kind = JackpotUserActionKind::drop_no_user;
    a.reply_protocol = jackpot_prize_notify;
    const auto plan = jackpot_user_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, JackpotSideEffectKind::Drop);
    EXPECT_EQ(plan.effects[0].reply_protocol, jackpot_prize_notify);
}

TEST(JackpotPlan, ServerPrizeNotifySetsTotalMoneyAndBroadcasts) {
    JackpotServerRequest r;
    r.protocol = jackpot_prize_notify;
    r.prize.dw_object_id = kObjectId;
    r.prize.dw_rest_total_money = kMoney;
    auto action = classify_jackpot_server(r);
    const auto plan = jackpot_server_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_TRUE(plan.broadcast);
    EXPECT_FALSE(plan.broadcast_in_map);
    EXPECT_TRUE(plan.set_total_money);
    EXPECT_FALSE(plan.rewrite_protocol);
    // 2 effects: SetJackpotTotalMoney + BroadcastAllUsers
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind, JackpotSideEffectKind::SetJackpotTotalMoney);
    EXPECT_EQ(plan.effects[0].jackpot_total_money, kMoney);
    EXPECT_EQ(plan.effects[1].kind, JackpotSideEffectKind::BroadcastAllUsers);
    EXPECT_EQ(plan.effects[1].reply_protocol, jackpot_prize_notify);
    EXPECT_EQ(plan.effects[1].jackpot_total_money, kMoney);
}

TEST(JackpotPlan, ServerTotalMoneyToAgentSetsAndBroadcastsInMap) {
    JackpotServerRequest r;
    r.protocol = jackpot_totalmoney_notify_to_agent;
    r.total.dw_object_id = kObjectId;
    r.total.dw_data = kMoney;
    auto action = classify_jackpot_server(r);
    const auto plan = jackpot_server_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_TRUE(plan.broadcast_in_map);
    EXPECT_FALSE(plan.broadcast);
    EXPECT_TRUE(plan.set_total_money);
    EXPECT_TRUE(plan.rewrite_protocol);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[1].kind, JackpotSideEffectKind::BroadcastInMapUsers);
    EXPECT_EQ(plan.effects[1].reply_protocol, jackpot_totalmoney_notify);
    EXPECT_EQ(plan.effects[1].rewritten_protocol, jackpot_totalmoney_notify);
    EXPECT_TRUE(plan.effects[1].rewrite_protocol);
}

TEST(JackpotPlan, ServerPrizeEffectForwardsToOriginatingClient) {
    JackpotServerRequest r;
    r.protocol = jackpot_prize_effect;
    auto action = classify_jackpot_server(r);
    const auto plan = jackpot_server_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_FALSE(plan.broadcast);
    EXPECT_FALSE(plan.broadcast_in_map);
    EXPECT_FALSE(plan.set_total_money);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, JackpotSideEffectKind::ForwardToOriginatingClient);
    EXPECT_EQ(plan.effects[0].reply_protocol, jackpot_prize_effect);
}

TEST(JackpotPlan, ServerCheatMapTotalMoneyForwardsToOriginatingClient) {
    JackpotServerRequest r;
    r.protocol = jackpot_cheat_maptotalmoney;
    auto action = classify_jackpot_server(r);
    const auto plan = jackpot_server_side_effect_plan(action);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, JackpotSideEffectKind::ForwardToOriginatingClient);
    EXPECT_EQ(plan.effects[0].reply_protocol, jackpot_cheat_maptotalmoney);
}

TEST(JackpotPlan, ServerTotalMoneyNotifyDefaultForwards) {
    // legacy commented-out branch falls through to TransToClientMsgParser.
    JackpotServerRequest r;
    r.protocol = jackpot_totalmoney_notify;
    auto action = classify_jackpot_server(r);
    const auto plan = jackpot_server_side_effect_plan(action);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, JackpotSideEffectKind::ForwardToOriginatingClient);
}

TEST(JackpotPlan, ServerUnknownProtocolEmitsDrop) {
    JackpotServerRequest r;
    r.protocol = 200u;
    auto action = classify_jackpot_server(r);
    const auto plan = jackpot_server_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, JackpotSideEffectKind::Drop);
    EXPECT_EQ(plan.effects[0].reply_protocol, 200u);
}

TEST(JackpotPlan, ServerTotalMoneyToAgentEchoesObjectId) {
    JackpotServerRequest r;
    r.protocol = jackpot_totalmoney_notify_to_agent;
    r.total.dw_object_id = kObjectId;
    auto action = classify_jackpot_server(r);
    const auto plan = jackpot_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[1].dw_object_id, kObjectId);
}

TEST(JackpotPlan, ServerPrizeNotifyWithZeroTotalMoneyStillUpdates) {
    JackpotServerRequest r;
    r.protocol = jackpot_prize_notify;
    r.prize.dw_rest_total_money = 0u;
    auto action = classify_jackpot_server(r);
    const auto plan = jackpot_server_side_effect_plan(action);
    EXPECT_TRUE(plan.set_total_money);
    EXPECT_EQ(plan.effects[0].jackpot_total_money, 0u);
}

TEST(JackpotPlan, ServerBroadcastEffectsNeverMarkDrop) {
    JackpotServerRequest r;
    r.protocol = jackpot_prize_notify;
    auto action = classify_jackpot_server(r);
    const auto plan = jackpot_server_side_effect_plan(action);
    EXPECT_FALSE(plan.drop);
}
