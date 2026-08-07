//
// 1:1 lock the implicit default-branch dispatch for MP_KYUNGGONG in
// [Server]Agent/AgentNetworkMsgParser.cpp. Each test pins one branch
// of the legacy behavior to its modern side-effect plan output so
// future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_kyunggong.hpp"
#include "mxh/server/agent_kyunggong_side_effect_plan.hpp"

using namespace mxh::server;

TEST(AgentKyungGongPlan, UserFoundEmitsForwardEffect) {
    AgentKyungGongRequest r;
    r.protocol = kyunggong_change_notify;
    r.user_found = true;
    r.object_id = 0xCAFE1234u;
    const auto plan = agent_kyunggong_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentKyungGongSideEffectKind::ForwardToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, kyunggong_change_notify);
    EXPECT_EQ(plan.effects[0].connection_index, 17u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFE1234u);
    EXPECT_TRUE(kyunggong_effect_targets_user(plan.effects[0]));
}

TEST(AgentKyungGongPlan, UserMissingEmitsDropEffect) {
    AgentKyungGongRequest r;
    r.protocol = kyunggong_change_notify;
    r.user_found = false;
    r.object_id = 0x12345678u;
    const auto plan = agent_kyunggong_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentKyungGongSideEffectKind::Drop);
    EXPECT_FALSE(kyunggong_effect_targets_user(plan.effects[0]));
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
    EXPECT_EQ(plan.effects[0].object_id, 0x12345678u);
}

TEST(AgentKyungGongPlan, ForwardPlanPreservesObjectId) {
    AgentKyungGongRequest r;
    r.protocol = kyunggong_change_notify;
    r.user_found = true;
    r.object_id = 0xABCDEF01u;
    const auto plan = agent_kyunggong_side_effect_plan(r, 99u);
    EXPECT_EQ(plan.effects[0].object_id, 0xABCDEF01u);
    EXPECT_EQ(plan.effects[0].connection_index, 99u);
}

TEST(AgentKyungGongPlan, ForwardPlanPreservesProtocolByte) {
    const std::uint8_t all[] = {
        kyunggong_change_notify,
        kyunggong_ability_change_notify
    };
    for (std::uint8_t p : all) {
        AgentKyungGongRequest r;
        r.protocol = p;
        r.user_found = true;
        const auto plan = agent_kyunggong_side_effect_plan(r, 0u);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].reply_protocol, p)
            << "protocol=" << +p;
    }
}

TEST(AgentKyungGongPlan, DropEffectAlwaysEmittedWhenUserMissing) {
    const std::uint8_t all[] = {
        kyunggong_change_notify,
        kyunggong_ability_change_notify
    };
    for (std::uint8_t p : all) {
        AgentKyungGongRequest r;
        r.protocol = p;
        r.user_found = false;
        const auto plan = agent_kyunggong_side_effect_plan(r, 0u);
        EXPECT_TRUE(plan.drop);
        EXPECT_EQ(plan.effects[0].kind, AgentKyungGongSideEffectKind::Drop);
    }
}

TEST(AgentKyungGongPlan, DefaultPlanStructFieldsAreStable) {
    AgentKyungGongSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentKyungGongPlan, EffectTargetsUserPredicateMatchesForward) {
    AgentKyungGongSideEffect forward{};
    forward.kind = AgentKyungGongSideEffectKind::ForwardToUser;
    AgentKyungGongSideEffect drop{};
    drop.kind = AgentKyungGongSideEffectKind::Drop;
    EXPECT_TRUE(kyunggong_effect_targets_user(forward));
    EXPECT_FALSE(kyunggong_effect_targets_user(drop));
}

TEST(AgentKyungGongPlan, ForwardConnectionIndexEqualsResolvedConnection) {
    AgentKyungGongRequest r;
    r.user_found = true;
    r.object_id = 0x11223344u;
    const auto plan = agent_kyunggong_side_effect_plan(r, 0xFFFFAA00u);
    EXPECT_EQ(plan.effects[0].connection_index, 0xFFFFAA00u);
}

TEST(AgentKyungGongPlan, DropEffectCarriesObjectIdEvenWhenConnectionZero) {
    AgentKyungGongRequest r;
    r.user_found = false;
    r.object_id = 0xCAFEBABEu;
    const auto plan = agent_kyunggong_side_effect_plan(r, 1u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFEBABEu);
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
}
