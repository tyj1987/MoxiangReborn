//
// 1:1 lock the implicit default-branch dispatch for MP_MONSTER in
// [Server]Agent/AgentNetworkMsgParser.cpp. Each test pins one branch
// of the legacy behavior to its modern side-effect plan output so
// future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_monster.hpp"
#include "mxh/server/agent_monster_side_effect_plan.hpp"

using namespace mxh::server;

TEST(AgentMonsterPlan, UserFoundEmitsForwardEffect) {
    AgentMonsterRequest r;
    r.protocol = monster_life_notify;
    r.user_found = true;
    r.object_id = 0xCAFE1234u;
    const auto plan = agent_monster_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentMonsterSideEffectKind::ForwardToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, monster_life_notify);
    EXPECT_EQ(plan.effects[0].connection_index, 17u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFE1234u);
    EXPECT_TRUE(monster_effect_targets_user(plan.effects[0]));
}

TEST(AgentMonsterPlan, UserMissingEmitsDropEffect) {
    AgentMonsterRequest r;
    r.protocol = monster_life_notify;
    r.user_found = false;
    r.object_id = 0x12345678u;
    const auto plan = agent_monster_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentMonsterSideEffectKind::Drop);
    EXPECT_FALSE(monster_effect_targets_user(plan.effects[0]));
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
    EXPECT_EQ(plan.effects[0].object_id, 0x12345678u);
}

TEST(AgentMonsterPlan, ForwardPlanPreservesObjectId) {
    AgentMonsterRequest r;
    r.protocol = monster_life_notify;
    r.user_found = true;
    r.object_id = 0xABCDEF01u;
    const auto plan = agent_monster_side_effect_plan(r, 99u);
    EXPECT_EQ(plan.effects[0].object_id, 0xABCDEF01u);
    EXPECT_EQ(plan.effects[0].connection_index, 99u);
}

TEST(AgentMonsterPlan, ForwardPlanPreservesProtocolByte) {
    const std::uint8_t all[] = {
        monster_life_notify,
        monster_reststart_notify,
        monster_restend_notify,
        monster_recall_notify
    };
    for (std::uint8_t p : all) {
        AgentMonsterRequest r;
        r.protocol = p;
        r.user_found = true;
        const auto plan = agent_monster_side_effect_plan(r, 0u);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].reply_protocol, p)
            << "protocol=" << +p;
    }
}

TEST(AgentMonsterPlan, DropEffectAlwaysEmittedWhenUserMissing) {
    const std::uint8_t all[] = {
        monster_life_notify,
        monster_reststart_notify,
        monster_restend_notify,
        monster_recall_notify
    };
    for (std::uint8_t p : all) {
        AgentMonsterRequest r;
        r.protocol = p;
        r.user_found = false;
        const auto plan = agent_monster_side_effect_plan(r, 0u);
        EXPECT_TRUE(plan.drop);
        EXPECT_EQ(plan.effects[0].kind, AgentMonsterSideEffectKind::Drop);
    }
}

TEST(AgentMonsterPlan, DefaultPlanStructFieldsAreStable) {
    AgentMonsterSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentMonsterPlan, EffectTargetsUserPredicateMatchesForward) {
    AgentMonsterSideEffect forward{};
    forward.kind = AgentMonsterSideEffectKind::ForwardToUser;
    AgentMonsterSideEffect drop{};
    drop.kind = AgentMonsterSideEffectKind::Drop;
    EXPECT_TRUE(monster_effect_targets_user(forward));
    EXPECT_FALSE(monster_effect_targets_user(drop));
}

TEST(AgentMonsterPlan, ForwardConnectionIndexEqualsResolvedConnection) {
    AgentMonsterRequest r;
    r.user_found = true;
    r.object_id = 0x11223344u;
    const auto plan = agent_monster_side_effect_plan(r, 0xFFFFAA00u);
    EXPECT_EQ(plan.effects[0].connection_index, 0xFFFFAA00u);
}

TEST(AgentMonsterPlan, DropEffectCarriesObjectIdEvenWhenConnectionZero) {
    AgentMonsterRequest r;
    r.user_found = false;
    r.object_id = 0xCAFEBABEu;
    const auto plan = agent_monster_side_effect_plan(r, 1u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFEBABEu);
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
}
