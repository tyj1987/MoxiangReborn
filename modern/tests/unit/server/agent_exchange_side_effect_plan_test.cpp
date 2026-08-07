// D4.171 -- 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_EXCHANGEUserMsgParser (lines 5095-5104). Each test pins one branch of the
// legacy dispatch to its modern side-effect plan output so future drift triggers a
// test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_exchange.hpp"
#include "mxh/server/agent_exchange_side_effect_plan.hpp"

using namespace mxh::server;

namespace {
constexpr std::uint32_t kObjectId = 0x12345678u;
constexpr std::uint32_t kCharId = 0x87654321u;
}

TEST(ExchangePlan, UserNotFoundEmitsDrop) {
    ExchangeUserRequest r;
    r.protocol = exchange_apply_syn;
    r.dw_object_id = kObjectId;
    r.user_found = false;
    auto a = classify_exchange_user(r);
    EXPECT_EQ(a.kind, ExchangeUserActionKind::drop_no_user);
    const auto plan = exchange_user_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    EXPECT_FALSE(plan.forward_to_map_server);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, ExchangeSideEffectKind::Drop);
    EXPECT_EQ(plan.effects[0].reply_protocol, exchange_apply_syn);
    EXPECT_EQ(plan.effects[0].dw_object_id, kObjectId);
    EXPECT_FALSE(plan.effects[0].forward_payload);
}

TEST(ExchangePlan, UserObjectIdMismatchEmitsDrop) {
    ExchangeUserRequest r;
    r.protocol = exchange_additem_syn;
    r.dw_object_id = kObjectId;
    r.dw_user_character_id = kCharId;
    r.user_found = true;
    auto a = classify_exchange_user(r);
    EXPECT_EQ(a.kind, ExchangeUserActionKind::drop_object_id_mismatch);
    const auto plan = exchange_user_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, ExchangeSideEffectKind::Drop);
    EXPECT_EQ(plan.effects[0].reply_protocol, exchange_additem_syn);
    EXPECT_FALSE(plan.effects[0].forward_payload);
}

TEST(ExchangePlan, UserObjectIdMatchForwardsToMapServer) {
    ExchangeUserRequest r;
    r.protocol = exchange_additem_syn;
    r.dw_object_id = kCharId;
    r.dw_user_character_id = kCharId;
    r.user_found = true;
    auto a = classify_exchange_user(r);
    EXPECT_EQ(a.kind, ExchangeUserActionKind::forward_to_map_server);
    const auto plan = exchange_user_side_effect_plan(a);
    EXPECT_FALSE(plan.drop);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_TRUE(plan.forward_to_map_server);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, ExchangeSideEffectKind::ForwardToMapServer);
    EXPECT_EQ(plan.effects[0].reply_protocol, exchange_additem_syn);
    EXPECT_EQ(plan.effects[0].dw_object_id, kCharId);
    EXPECT_TRUE(plan.effects[0].forward_payload);
}

TEST(ExchangePlan, UserStartProtocolForwardsToMapServer) {
    ExchangeUserRequest r;
    r.protocol = exchange_start;
    r.dw_object_id = kObjectId;
    r.dw_user_character_id = kObjectId;
    r.user_found = true;
    auto a = classify_exchange_user(r);
    EXPECT_EQ(a.kind, ExchangeUserActionKind::forward_to_map_server);
    const auto plan = exchange_user_side_effect_plan(a);
    EXPECT_TRUE(plan.forward_to_map_server);
    EXPECT_EQ(plan.effects[0].reply_protocol, exchange_start);
}

TEST(ExchangePlan, UserExchangeProtocolForwardsToMapServer) {
    ExchangeUserRequest r;
    r.protocol = exchange_exchange_syn;
    r.dw_object_id = kObjectId;
    r.dw_user_character_id = kObjectId;
    r.user_found = true;
    auto a = classify_exchange_user(r);
    EXPECT_EQ(a.kind, ExchangeUserActionKind::forward_to_map_server);
    const auto plan = exchange_user_side_effect_plan(a);
    EXPECT_TRUE(plan.forward_to_map_server);
    EXPECT_EQ(plan.effects[0].reply_protocol, exchange_exchange_syn);
}

TEST(ExchangePlan, PlanDefaultsAreConservative) {
    ExchangeSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.forward_to_map_server);
    EXPECT_TRUE(plan.effects.empty());
}

