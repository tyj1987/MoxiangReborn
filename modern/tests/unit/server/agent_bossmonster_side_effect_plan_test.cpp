//
// 1:1 lock the implicit default-branch dispatch for MP_BOSSMONSTER in
// [Server]Agent/AgentNetworkMsgParser.cpp. Each test pins one branch
// of the legacy behavior to its modern side-effect plan output so
// future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_bossmonster.hpp"
#include "mxh/server/agent_bossmonster_side_effect_plan.hpp"

using namespace mxh::server;

TEST(AgentBossMonsterPlan, UserFoundEmitsForwardEffect) {
    AgentBossMonsterRequest r;
    r.protocol = boss_rest_start_notify;
    r.user_found = true;
    r.object_id = 0xCAFE1234u;
    const auto plan = agent_bossmonster_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentBossMonsterSideEffectKind::ForwardToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, boss_rest_start_notify);
    EXPECT_EQ(plan.effects[0].connection_index, 17u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFE1234u);
    EXPECT_TRUE(bossmonster_effect_targets_user(plan.effects[0]));
}

TEST(AgentBossMonsterPlan, UserMissingEmitsDropEffect) {
    AgentBossMonsterRequest r;
    r.protocol = boss_rest_start_notify;
    r.user_found = false;
    r.object_id = 0x12345678u;
    const auto plan = agent_bossmonster_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentBossMonsterSideEffectKind::Drop);
    EXPECT_FALSE(bossmonster_effect_targets_user(plan.effects[0]));
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
    EXPECT_EQ(plan.effects[0].object_id, 0x12345678u);
}

TEST(AgentBossMonsterPlan, ForwardPlanPreservesObjectId) {
    AgentBossMonsterRequest r;
    r.protocol = boss_rest_start_notify;
    r.user_found = true;
    r.object_id = 0xABCDEF01u;
    const auto plan = agent_bossmonster_side_effect_plan(r, 99u);
    EXPECT_EQ(plan.effects[0].object_id, 0xABCDEF01u);
    EXPECT_EQ(plan.effects[0].connection_index, 99u);
}

TEST(AgentBossMonsterPlan, ForwardPlanPreservesProtocolByte) {
    const std::uint8_t all[] = {
        boss_rest_start_notify,
        boss_recall_notify,
        boss_life_notify,
        boss_shield_notify,
        boss_stand_notify,
        boss_stand_end_notify,
        field_life_notify,
        field_shield_notify
    };
    for (std::uint8_t p : all) {
        AgentBossMonsterRequest r;
        r.protocol = p;
        r.user_found = true;
        const auto plan = agent_bossmonster_side_effect_plan(r, 0u);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].reply_protocol, p)
            << "protocol=" << +p;
    }
}

TEST(AgentBossMonsterPlan, DropEffectAlwaysEmittedWhenUserMissing) {
    const std::uint8_t all[] = {
        boss_rest_start_notify,
        boss_recall_notify,
        boss_life_notify,
        boss_shield_notify,
        boss_stand_notify,
        boss_stand_end_notify,
        field_life_notify,
        field_shield_notify
    };
    for (std::uint8_t p : all) {
        AgentBossMonsterRequest r;
        r.protocol = p;
        r.user_found = false;
        const auto plan = agent_bossmonster_side_effect_plan(r, 0u);
        EXPECT_TRUE(plan.drop);
        EXPECT_EQ(plan.effects[0].kind, AgentBossMonsterSideEffectKind::Drop);
    }
}

TEST(AgentBossMonsterPlan, DefaultPlanStructFieldsAreStable) {
    AgentBossMonsterSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentBossMonsterPlan, EffectTargetsUserPredicateMatchesForward) {
    AgentBossMonsterSideEffect forward{};
    forward.kind = AgentBossMonsterSideEffectKind::ForwardToUser;
    AgentBossMonsterSideEffect drop{};
    drop.kind = AgentBossMonsterSideEffectKind::Drop;
    EXPECT_TRUE(bossmonster_effect_targets_user(forward));
    EXPECT_FALSE(bossmonster_effect_targets_user(drop));
}

TEST(AgentBossMonsterPlan, ForwardConnectionIndexEqualsResolvedConnection) {
    AgentBossMonsterRequest r;
    r.user_found = true;
    r.object_id = 0x11223344u;
    const auto plan = agent_bossmonster_side_effect_plan(r, 0xFFFFAA00u);
    EXPECT_EQ(plan.effects[0].connection_index, 0xFFFFAA00u);
}

TEST(AgentBossMonsterPlan, DropEffectCarriesObjectIdEvenWhenConnectionZero) {
    AgentBossMonsterRequest r;
    r.user_found = false;
    r.object_id = 0xCAFEBABEu;
    const auto plan = agent_bossmonster_side_effect_plan(r, 1u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFEBABEu);
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
}
