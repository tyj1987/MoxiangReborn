//
// 1:1 lock the implicit default-branch dispatch for MP_PYOGUK in
// [Server]Agent/AgentNetworkMsgParser.cpp. Each test pins one branch
// of the legacy behavior to its modern side-effect plan output so
// future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_pyoguk.hpp"
#include "mxh/server/agent_pyoguk_side_effect_plan.hpp"

using namespace mxh::server;

TEST(AgentPyogukPlan, UserFoundEmitsForwardEffect) {
    AgentPyogukRequest r;
    r.protocol = pyoguk_listinfo_syn;
    r.user_found = true;
    r.object_id = 0xCAFE1234u;
    const auto plan = agent_pyoguk_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentPyogukSideEffectKind::ForwardToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, pyoguk_listinfo_syn);
    EXPECT_EQ(plan.effects[0].connection_index, 17u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFE1234u);
    EXPECT_TRUE(pyoguk_effect_targets_user(plan.effects[0]));
}

TEST(AgentPyogukPlan, UserMissingEmitsDropEffect) {
    AgentPyogukRequest r;
    r.protocol = pyoguk_listinfo_syn;
    r.user_found = false;
    r.object_id = 0x12345678u;
    const auto plan = agent_pyoguk_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentPyogukSideEffectKind::Drop);
    EXPECT_FALSE(pyoguk_effect_targets_user(plan.effects[0]));
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
    EXPECT_EQ(plan.effects[0].object_id, 0x12345678u);
}

TEST(AgentPyogukPlan, ForwardPlanPreservesObjectId) {
    AgentPyogukRequest r;
    r.protocol = pyoguk_listinfo_syn;
    r.user_found = true;
    r.object_id = 0xABCDEF01u;
    const auto plan = agent_pyoguk_side_effect_plan(r, 99u);
    EXPECT_EQ(plan.effects[0].object_id, 0xABCDEF01u);
    EXPECT_EQ(plan.effects[0].connection_index, 99u);
}

TEST(AgentPyogukPlan, ForwardPlanPreservesProtocolByte) {
    const std::uint8_t all[] = {
        pyoguk_listinfo_syn,
        pyoguk_listinfo_ack,
        pyoguk_listinfo_nack,
        pyoguk_buy_syn,
        pyoguk_buy_ack,
        pyoguk_buy_nack,
        pyoguk_del_syn,
        pyoguk_del_ack,
        pyoguk_del_nack,
        pyoguk_putin_money_syn,
        pyoguk_putin_money_ack,
        pyoguk_putin_money_nack,
        pyoguk_putout_money_syn,
        pyoguk_putout_money_ack,
        pyoguk_putout_money_nack,
        pyoguk_info
    };
    for (std::uint8_t p : all) {
        AgentPyogukRequest r;
        r.protocol = p;
        r.user_found = true;
        const auto plan = agent_pyoguk_side_effect_plan(r, 0u);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].reply_protocol, p)
            << "protocol=" << +p;
    }
}

TEST(AgentPyogukPlan, DropEffectAlwaysEmittedWhenUserMissing) {
    const std::uint8_t all[] = {
        pyoguk_listinfo_syn,
        pyoguk_listinfo_ack,
        pyoguk_listinfo_nack,
        pyoguk_buy_syn,
        pyoguk_buy_ack,
        pyoguk_buy_nack,
        pyoguk_del_syn,
        pyoguk_del_ack,
        pyoguk_del_nack,
        pyoguk_putin_money_syn,
        pyoguk_putin_money_ack,
        pyoguk_putin_money_nack,
        pyoguk_putout_money_syn,
        pyoguk_putout_money_ack,
        pyoguk_putout_money_nack,
        pyoguk_info
    };
    for (std::uint8_t p : all) {
        AgentPyogukRequest r;
        r.protocol = p;
        r.user_found = false;
        const auto plan = agent_pyoguk_side_effect_plan(r, 0u);
        EXPECT_TRUE(plan.drop);
        EXPECT_EQ(plan.effects[0].kind, AgentPyogukSideEffectKind::Drop);
    }
}

TEST(AgentPyogukPlan, DefaultPlanStructFieldsAreStable) {
    AgentPyogukSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentPyogukPlan, EffectTargetsUserPredicateMatchesForward) {
    AgentPyogukSideEffect forward{};
    forward.kind = AgentPyogukSideEffectKind::ForwardToUser;
    AgentPyogukSideEffect drop{};
    drop.kind = AgentPyogukSideEffectKind::Drop;
    EXPECT_TRUE(pyoguk_effect_targets_user(forward));
    EXPECT_FALSE(pyoguk_effect_targets_user(drop));
}

TEST(AgentPyogukPlan, ForwardConnectionIndexEqualsResolvedConnection) {
    AgentPyogukRequest r;
    r.user_found = true;
    r.object_id = 0x11223344u;
    const auto plan = agent_pyoguk_side_effect_plan(r, 0xFFFFAA00u);
    EXPECT_EQ(plan.effects[0].connection_index, 0xFFFFAA00u);
}

TEST(AgentPyogukPlan, DropEffectCarriesObjectIdEvenWhenConnectionZero) {
    AgentPyogukRequest r;
    r.user_found = false;
    r.object_id = 0xCAFEBABEu;
    const auto plan = agent_pyoguk_side_effect_plan(r, 1u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFEBABEu);
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
}
