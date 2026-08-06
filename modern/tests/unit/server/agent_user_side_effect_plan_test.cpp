//
// agent_user_side_effect_plan_test.cpp -- D4.119
//
// 1:1 lock the legacy state-side-effects applied by [Server]Agent/AgentServer.cpp
// + AgentNetworkMsgParser.cpp for the AgentUser lifecycle:
//   insert_agent_user / remove_agent_user / assign_agent_user_map /
//   toggle_agent_user_force_move. This test covers the plan-builders + the
//   apply_agent_user_side_effect_plan() orchestrator entry point.
//

#include <gtest/gtest.h>
#include "mxh/server/agent_user.hpp"
#include "mxh/server/agent_user_side_effect_plan.hpp"

using namespace mxh::server;

// ---------------------- plan-builders ----------------------

TEST(UserPlan, InsertEmitsInsertEffect) {
    const auto plan = agent_user_insert_side_effect_plan(0x12345678u, 100u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentUserSideEffectKind::InsertAgentUser);
    EXPECT_EQ(plan.effects[0].auth_key, 0x12345678u);
    EXPECT_EQ(plan.effects[0].object_id, 100u);
}

TEST(UserPlan, RemoveEmitsRemoveEffect) {
    const auto plan = agent_user_remove_side_effect_plan();
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentUserSideEffectKind::RemoveAgentUser);
    EXPECT_TRUE(plan.effects[0].clear_info);
}

TEST(UserPlan, AssignMapEmitsAssignEffectWhenChannelNonZero) {
    const auto plan = agent_user_assign_map_side_effect_plan(7u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentUserSideEffectKind::AssignAgentUserMap);
    EXPECT_EQ(plan.effects[0].map_channel, 7u);
}

TEST(UserPlan, AssignMapZeroDrops) {
    const auto plan = agent_user_assign_map_side_effect_plan(0u);
    EXPECT_TRUE(plan.drop);
    ASSERT_FALSE(plan.effects.empty());
    EXPECT_EQ(plan.effects[0].kind, AgentUserSideEffectKind::Drop);
}

TEST(UserPlan, ToggleForceMoveEmitsToggleEffect) {
    const auto plan = agent_user_toggle_force_move_side_effect_plan();
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentUserSideEffectKind::ToggleAgentUserForceMove);
}

// ---------------------- apply: insert ----------------------

TEST(UserApplyPlan, InsertEmptyRecordSetsFieldsAndInUse) {
    AgentUserRecord r{};
    ASSERT_FALSE(r.in_use);
    const auto plan = agent_user_insert_side_effect_plan(0xCAFEBABEu, 0xDEADBEEFu);
    EXPECT_TRUE(apply_agent_user_side_effect_plan(r, plan));
    EXPECT_TRUE(r.in_use);
    EXPECT_EQ(r.info.dwAuthKey, 0xCAFEBABEu);
    EXPECT_EQ(r.info.dwObjectID, 0xDEADBEEFu);
}
TEST(UserApplyPlan, InsertOnInUseRecordIsNoOp) {
    AgentUserRecord r{};
    ASSERT_TRUE(insert_agent_user(r, 1u, 1u));
    const auto plan = agent_user_insert_side_effect_plan(2u, 2u);
    EXPECT_FALSE(apply_agent_user_side_effect_plan(r, plan));
    // Original values preserved.
    EXPECT_TRUE(r.in_use);
    EXPECT_EQ(r.info.dwAuthKey, 1u);
    EXPECT_EQ(r.info.dwObjectID, 1u);
}

// ---------------------- apply: remove ----------------------

TEST(UserApplyPlan, RemoveInUseRecordClearsInfoAndInUse) {
    AgentUserRecord r{};
    ASSERT_TRUE(insert_agent_user(r, 0xAABBCCDDu, 42u));
    const auto plan = agent_user_remove_side_effect_plan();
    EXPECT_TRUE(apply_agent_user_side_effect_plan(r, plan));
    EXPECT_FALSE(r.in_use);
    EXPECT_EQ(r.info.dwAuthKey, 0u);
    EXPECT_EQ(r.info.dwObjectID, 0u);
    EXPECT_EQ(r.info.dwUserID, 0u);
    EXPECT_EQ(r.info.dwMapChannel, 0u);
}

TEST(UserApplyPlan, RemoveEmptyRecordIsNoOp) {
    AgentUserRecord r{};
    const auto plan = agent_user_remove_side_effect_plan();
    EXPECT_FALSE(apply_agent_user_side_effect_plan(r, plan));
    EXPECT_FALSE(r.in_use);
}
// ---------------------- apply: assign_map ----------------------

TEST(UserApplyPlan, AssignMapNonZeroUpdatesChannel) {
    AgentUserRecord r{};
    ASSERT_TRUE(insert_agent_user(r, 1u, 1u));
    const auto plan = agent_user_assign_map_side_effect_plan(17u);
    EXPECT_TRUE(apply_agent_user_side_effect_plan(r, plan));
    EXPECT_EQ(r.info.dwMapChannel, 17u);
}

TEST(UserApplyPlan, AssignMapZeroIsNoOp) {
    AgentUserRecord r{};
    ASSERT_TRUE(insert_agent_user(r, 1u, 1u));
    r.info.dwMapChannel = 5u;  // pre-set non-zero
    const auto plan = agent_user_assign_map_side_effect_plan(0u);
    EXPECT_FALSE(apply_agent_user_side_effect_plan(r, plan));
    // Channel unchanged because apply rejects zero sentinel.
    EXPECT_EQ(r.info.dwMapChannel, 5u);
}

TEST(UserApplyPlan, AssignMapPlanDroppedReturnsFalse) {
    AgentUserRecord r{};
    ASSERT_TRUE(insert_agent_user(r, 1u, 1u));
    // Channel==0 -> builder emits a Drop plan with drop=true.
    const auto plan = agent_user_assign_map_side_effect_plan(0u);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(apply_agent_user_side_effect_plan(r, plan));
    EXPECT_EQ(r.info.dwMapChannel, 0u);  // unchanged
}

// ---------------------- apply: toggle_force_move ----------------------

TEST(UserApplyPlan, ToggleForceMoveFlipsZeroToOne) {
    AgentUserRecord r{};
    ASSERT_TRUE(insert_agent_user(r, 1u, 1u));
    EXPECT_EQ(r.info.bForceMove, 0u);
    const auto plan = agent_user_toggle_force_move_side_effect_plan();
    EXPECT_TRUE(apply_agent_user_side_effect_plan(r, plan));
    EXPECT_EQ(r.info.bForceMove, 1u);
}
TEST(UserApplyPlan, ToggleForceMoveFlipsOneToZero) {
    AgentUserRecord r{};
    ASSERT_TRUE(insert_agent_user(r, 1u, 1u));
    r.info.bForceMove = 1u;
    const auto plan = agent_user_toggle_force_move_side_effect_plan();
    EXPECT_TRUE(apply_agent_user_side_effect_plan(r, plan));
    EXPECT_EQ(r.info.bForceMove, 0u);
}

TEST(UserApplyPlan, ToggleForceMoveTwiceRestoresOriginal) {
    AgentUserRecord r{};
    ASSERT_TRUE(insert_agent_user(r, 1u, 1u));
    const auto plan1 = agent_user_toggle_force_move_side_effect_plan();
    const auto plan2 = agent_user_toggle_force_move_side_effect_plan();
    EXPECT_TRUE(apply_agent_user_side_effect_plan(r, plan1));
    EXPECT_TRUE(apply_agent_user_side_effect_plan(r, plan2));
    EXPECT_EQ(r.info.bForceMove, 0u);
}

// ---------------------- apply: plan-level control ----------------------

TEST(UserApplyPlan, DropPlanReturnsFalse) {
    AgentUserRecord r{};
    AgentUserSideEffectPlan plan;
    plan.drop = true;
    plan.effects.push_back({AgentUserSideEffectKind::Drop, 0u, 0u, 0u, 0u, false});
    EXPECT_FALSE(apply_agent_user_side_effect_plan(r, plan));
    EXPECT_FALSE(r.in_use);
}

TEST(UserApplyPlan, EmptyEffectsPlanReturnsFalse) {
    AgentUserRecord r{};
    AgentUserSideEffectPlan plan;
    plan.dispatched = true;
    plan.drop = false;
    EXPECT_FALSE(apply_agent_user_side_effect_plan(r, plan));
    EXPECT_FALSE(r.in_use);
}
// ---------------------- 1:1 lock: full sequence ----------------------

TEST(UserApplyPlan, FullLifecycleSequenceMatchesLegacy) {
    // Insert -> AssignMap -> ToggleForceMove -> Remove.
    AgentUserRecord r{};
    EXPECT_TRUE(apply_agent_user_side_effect_plan(r, agent_user_insert_side_effect_plan(0xFEEDFACEu, 999u)));
    EXPECT_TRUE(r.in_use);
    EXPECT_EQ(r.info.dwAuthKey, 0xFEEDFACEu);
    EXPECT_EQ(r.info.dwObjectID, 999u);
    EXPECT_EQ(r.info.dwMapChannel, 0u);
    EXPECT_EQ(r.info.bForceMove, 0u);

    EXPECT_TRUE(apply_agent_user_side_effect_plan(r, agent_user_assign_map_side_effect_plan(12u)));
    EXPECT_EQ(r.info.dwMapChannel, 12u);

    EXPECT_TRUE(apply_agent_user_side_effect_plan(r, agent_user_toggle_force_move_side_effect_plan()));
    EXPECT_EQ(r.info.bForceMove, 1u);

    EXPECT_TRUE(apply_agent_user_side_effect_plan(r, agent_user_remove_side_effect_plan()));
    EXPECT_FALSE(r.in_use);
    EXPECT_EQ(r.info.dwAuthKey, 0u);
    EXPECT_EQ(r.info.dwObjectID, 0u);
    EXPECT_EQ(r.info.dwMapChannel, 0u);
    EXPECT_EQ(r.info.bForceMove, 0u);
}

// 1:1 mirror: plan and direct mutator produce identical state.

TEST(UserApplyPlan, InsertPlanMatchesInsertAgentUser) {
    AgentUserRecord a{};
    AgentUserRecord b{};
    EXPECT_TRUE(apply_agent_user_side_effect_plan(a, agent_user_insert_side_effect_plan(0xABCDEF01u, 7u)));
    EXPECT_TRUE(insert_agent_user(b, 0xABCDEF01u, 7u));
    EXPECT_EQ(a.in_use, b.in_use);
    EXPECT_EQ(a.info.dwAuthKey, b.info.dwAuthKey);
    EXPECT_EQ(a.info.dwObjectID, b.info.dwObjectID);
}

TEST(UserApplyPlan, RemovePlanMatchesRemoveAgentUser) {
    AgentUserRecord a{};
    AgentUserRecord b{};
    ASSERT_TRUE(insert_agent_user(a, 0xAAu, 0xBBu));
    ASSERT_TRUE(insert_agent_user(b, 0xAAu, 0xBBu));
    EXPECT_TRUE(apply_agent_user_side_effect_plan(a, agent_user_remove_side_effect_plan()));
    EXPECT_TRUE(remove_agent_user(b));
    EXPECT_EQ(a.in_use, b.in_use);
    EXPECT_EQ(a.info.dwAuthKey, b.info.dwAuthKey);
    EXPECT_EQ(a.info.dwObjectID, b.info.dwObjectID);
}

TEST(UserApplyPlan, AssignMapPlanMatchesAssignAgentUserMap) {
    AgentUserRecord a{};
    AgentUserRecord b{};
    ASSERT_TRUE(insert_agent_user(a, 1u, 1u));
    ASSERT_TRUE(insert_agent_user(b, 1u, 1u));
    EXPECT_TRUE(apply_agent_user_side_effect_plan(a, agent_user_assign_map_side_effect_plan(42u)));
    EXPECT_TRUE(assign_agent_user_map(b, 42u));
    EXPECT_EQ(a.info.dwMapChannel, b.info.dwMapChannel);
}

TEST(UserApplyPlan, TogglePlanMatchesToggleAgentUserForceMove) {
    AgentUserRecord a{};
    AgentUserRecord b{};
    ASSERT_TRUE(insert_agent_user(a, 1u, 1u));
    ASSERT_TRUE(insert_agent_user(b, 1u, 1u));
    EXPECT_TRUE(apply_agent_user_side_effect_plan(a, agent_user_toggle_force_move_side_effect_plan()));
    EXPECT_TRUE(toggle_agent_user_force_move(b));
    EXPECT_EQ(a.info.bForceMove, b.info.bForceMove);
}
