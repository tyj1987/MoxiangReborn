//
// agent_skill_side_effect_plan_test.cpp -- D4.120
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_SkillUserMsgParser + MP_SkillServerMsgParser. Covers both plan-builders
// (from AgentSkillAction + from classify-style inputs) + the apply_agent_skill_side_effect_plan()
// orchestrator entry point.
//

#include <gtest/gtest.h>

#include "mxh/server/agent_skill.hpp"
#include "mxh/server/agent_skill_side_effect_plan.hpp"
#include "mxh/server/skill_delay_manager.hpp"

using namespace mxh::server;


// ---------------------- plan-builder from AgentSkillAction ----------------------

TEST(SkillPlan, ForwardToMapEmitsForwardEffect) {
    AgentSkillAction a{};
    a.kind = AgentSkillActionKind::forward_to_map;
    a.character_id = 7u;
    a.skill_index = 100u;
    const auto plan = agent_skill_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentSkillSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].character_id, 7u);
    EXPECT_EQ(plan.effects[0].skill_index, 100u);
    EXPECT_TRUE(agent_skill_effect_targets_map(plan.effects[0]));
    EXPECT_FALSE(agent_skill_effect_targets_user(plan.effects[0]));
    EXPECT_FALSE(agent_skill_effect_mutates_state(plan.effects[0]));
}

TEST(SkillPlan, SendStartNackEmitsNackEffect) {
    AgentSkillAction a{};
    a.kind = AgentSkillActionKind::send_start_nack;
    a.character_id = 7u;
    a.skill_index = 100u;
    a.protocol = 2u;
    const auto plan = agent_skill_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentSkillSideEffectKind::SendStartNackToUser);
    EXPECT_EQ(plan.effects[0].character_id, 7u);
    EXPECT_EQ(plan.effects[0].skill_index, 100u);
    EXPECT_FALSE(plan.effects[0].skill_allowed);
    EXPECT_TRUE(agent_skill_effect_targets_user(plan.effects[0]));
    EXPECT_FALSE(agent_skill_effect_targets_map(plan.effects[0]));
    EXPECT_FALSE(agent_skill_effect_mutates_state(plan.effects[0]));
}
// ---------------------- classify-style plan-builders ----------------------

TEST(SkillClassifyPlan, UserNonPremierEmitsForwardPlan) {
    SkillDelayManager m;
    const auto plan = agent_skill_user_side_effect_plan(m, 7u, 99u, 0u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    // Non-premier -> allowed=true, ForwardRawToMap emitted.
    // ApplySkillDelayUser is also recorded (mirrors legacy add_skill_use call).
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind, AgentSkillSideEffectKind::ApplySkillDelayUser);
    EXPECT_EQ(plan.effects[1].kind, AgentSkillSideEffectKind::ForwardRawToMap);
    EXPECT_TRUE(plan.effects[0].skill_allowed);
    EXPECT_TRUE(plan.effects[1].skill_allowed);
}

TEST(SkillClassifyPlan, UserFirstPremierEmitsForwardPlan) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    const auto plan = agent_skill_user_side_effect_plan(m, 7u, 100u, 0u);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind, AgentSkillSideEffectKind::ApplySkillDelayUser);
    EXPECT_EQ(plan.effects[1].kind, AgentSkillSideEffectKind::ForwardRawToMap);
    EXPECT_TRUE(plan.effects[0].skill_allowed);
    // State was mutated (add_skill_use recorded the use).
    EXPECT_EQ(skill_use_count(m), 1u);
}

TEST(SkillClassifyPlan, UserCooldownEmitsNackPlan) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    process_agent_skill_user(m, 7u, 100u, 0u);
    const auto plan = agent_skill_user_side_effect_plan(m, 7u, 100u, 4000u);
    // Within 5s+latency window -> rejected -> SendStartNackToUser only.
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentSkillSideEffectKind::SendStartNackToUser);
    EXPECT_FALSE(plan.effects[0].skill_allowed);
    // Legacy AddSkillUse returns false WITHOUT mutating state.
    // (m_SkillUses still has the original entry from the first call.)
    const SkillUse* prior = find_skill_use(m, 7u);
    ASSERT_NE(prior, nullptr);
    EXPECT_EQ(prior->dwStartTime, 0u);  // unchanged
}

TEST(SkillClassifyPlan, UserCooldownElapsedEmitsForwardPlan) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    process_agent_skill_user(m, 7u, 100u, 0u);
    // 6000ms after start: 6000-0+5000=11000 >= 10000, so allowed.
    const auto plan = agent_skill_user_side_effect_plan(m, 7u, 100u, 6000u);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[1].kind, AgentSkillSideEffectKind::ForwardRawToMap);
    EXPECT_TRUE(plan.effects[0].skill_allowed);
    // dwStartTime was reset to 6000 by the second add_skill_use call.
    const SkillUse* prior = find_skill_use(m, 7u);
    ASSERT_NE(prior, nullptr);
    EXPECT_EQ(prior->dwStartTime, 6000u);
}
TEST(SkillClassifyPlan, ServerEmitsForceForwardPlan) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    const auto plan = agent_skill_server_side_effect_plan(m, 7u, 100u, 0u);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind, AgentSkillSideEffectKind::ApplySkillDelayServer);
    EXPECT_EQ(plan.effects[1].kind, AgentSkillSideEffectKind::ForwardRawToMap);
    EXPECT_TRUE(plan.effects[0].skill_allowed);
    EXPECT_EQ(skill_use_count(m), 1u);
}

TEST(SkillClassifyPlan, ServerForceResetsEvenDuringCooldown) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    process_agent_skill_server(m, 7u, 100u, 0u);
    // Cooldown still active (4s elapsed < 10s). Server force=true bypasses.
    const auto plan = agent_skill_server_side_effect_plan(m, 7u, 100u, 4000u);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[1].kind, AgentSkillSideEffectKind::ForwardRawToMap);
    EXPECT_TRUE(plan.effects[0].skill_allowed);
    // dwStartTime reset to 4000.
    const SkillUse* prior = find_skill_use(m, 7u);
    ASSERT_NE(prior, nullptr);
    EXPECT_EQ(prior->dwStartTime, 4000u);
}

TEST(SkillClassifyPlan, OtherEmitsSingleForwardEffect) {
    const auto plan = agent_skill_other_side_effect_plan(7u, 100u);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentSkillSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].character_id, 7u);
    EXPECT_EQ(plan.effects[0].skill_index, 100u);
}

// ---------------------- apply: state mutation ----------------------

TEST(SkillApplyPlan, DropPlanReturnsFalse) {
    SkillDelayManager m;
    AgentSkillSideEffectPlan plan;
    plan.drop = true;
    plan.effects.push_back({AgentSkillSideEffectKind::ForwardRawToMap, 7u, 100u, 0u, true});
    EXPECT_FALSE(apply_agent_skill_side_effect_plan(m, plan));
    EXPECT_EQ(skill_use_count(m), 0u);
}

TEST(SkillApplyPlan, EmptyEffectsPlanReturnsFalse) {
    SkillDelayManager m;
    AgentSkillSideEffectPlan plan;
    plan.dispatched = true;
    plan.drop = false;
    EXPECT_FALSE(apply_agent_skill_side_effect_plan(m, plan));
}

TEST(SkillApplyPlan, ApplySkillDelayUserMutatesManager) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    AgentSkillSideEffectPlan plan;
    plan.dispatched = true;
    plan.drop = false;
    plan.effects.push_back({AgentSkillSideEffectKind::ApplySkillDelayUser, 7u, 100u, 0u, true});
    EXPECT_TRUE(apply_agent_skill_side_effect_plan(m, plan));
    EXPECT_EQ(skill_use_count(m), 1u);
    const SkillUse* prior = find_skill_use(m, 7u);
    ASSERT_NE(prior, nullptr);
    EXPECT_EQ(prior->dwStartTime, 0u);
    EXPECT_EQ(prior->dwDelay, 10000u);
}
TEST(SkillApplyPlan, ApplySkillDelayServerMutatesManager) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    AgentSkillSideEffectPlan plan;
    plan.dispatched = true;
    plan.drop = false;
    plan.effects.push_back({AgentSkillSideEffectKind::ApplySkillDelayServer, 7u, 100u, 0u, true});
    EXPECT_TRUE(apply_agent_skill_side_effect_plan(m, plan));
    EXPECT_EQ(skill_use_count(m), 1u);
}

TEST(SkillApplyPlan, ForwardRawToMapDoesNotMutateState) {
    SkillDelayManager m;
    AgentSkillSideEffectPlan plan;
    plan.dispatched = true;
    plan.drop = false;
    plan.effects.push_back({AgentSkillSideEffectKind::ForwardRawToMap, 7u, 100u, 0u, true});
    EXPECT_TRUE(apply_agent_skill_side_effect_plan(m, plan));
    EXPECT_EQ(skill_use_count(m), 0u);
}

TEST(SkillApplyPlan, SendStartNackDoesNotMutateState) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    AgentSkillSideEffectPlan plan;
    plan.dispatched = true;
    plan.drop = false;
    plan.effects.push_back({AgentSkillSideEffectKind::SendStartNackToUser, 7u, 100u, 0u, false});
    EXPECT_TRUE(apply_agent_skill_side_effect_plan(m, plan));
    EXPECT_EQ(skill_use_count(m), 0u);
}

TEST(SkillApplyPlan, MultiEffectPlanAppliesAllInOrder) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    AgentSkillSideEffectPlan plan;
    plan.dispatched = true;
    plan.drop = false;
    plan.effects.push_back({AgentSkillSideEffectKind::ApplySkillDelayUser, 7u, 100u, 0u, true});
    plan.effects.push_back({AgentSkillSideEffectKind::ForwardRawToMap, 7u, 100u, 0u, true});
    EXPECT_TRUE(apply_agent_skill_side_effect_plan(m, plan));
    EXPECT_EQ(skill_use_count(m), 1u);
    // Forward is a no-op for state but the plan as a whole is applied.
    EXPECT_TRUE(agent_skill_effect_targets_map(plan.effects[1]));
}

// ---------------------- 1:1 mirror with data plane ----------------------

TEST(SkillApplyPlan, UserPlanMirrorsProcessAgentSkillUser) {
    SkillDelayManager m1;
    SkillDelayManager m2;
    add_premier_skill(m1, 100u, 10000u);
    add_premier_skill(m2, 100u, 10000u);
    const auto a = process_agent_skill_user(m1, 7u, 100u, 0u);
    const auto plan = agent_skill_user_side_effect_plan(m2, 7u, 100u, 0u);
    // Data plane + plan must agree on outcome.
    EXPECT_EQ(a.kind, AgentSkillActionKind::forward_to_map);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[1].kind, AgentSkillSideEffectKind::ForwardRawToMap);
    // State should match (both added the skill use at start=0).
    EXPECT_EQ(skill_use_count(m1), skill_use_count(m2));
    const SkillUse* p1 = find_skill_use(m1, 7u);
    const SkillUse* p2 = find_skill_use(m2, 7u);
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    EXPECT_EQ(p1->dwStartTime, p2->dwStartTime);
    EXPECT_EQ(p1->dwDelay, p2->dwDelay);
}
TEST(SkillApplyPlan, UserPlanMirrorsProcessAgentSkillUserCooldown) {
    SkillDelayManager m1;
    SkillDelayManager m2;
    add_premier_skill(m1, 100u, 10000u);
    add_premier_skill(m2, 100u, 10000u);
    process_agent_skill_user(m1, 7u, 100u, 0u);
    process_agent_skill_user(m2, 7u, 100u, 0u);
    // Now in cooldown (start=0, current=4000, 4000-0+5000=9000 < 10000).
    const auto a = process_agent_skill_user(m1, 7u, 100u, 4000u);
    const auto plan = agent_skill_user_side_effect_plan(m2, 7u, 100u, 4000u);
    EXPECT_EQ(a.kind, AgentSkillActionKind::send_start_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentSkillSideEffectKind::SendStartNackToUser);
    // State unchanged (legacy AddSkillUse returns false without mutating).
    const SkillUse* p1 = find_skill_use(m1, 7u);
    const SkillUse* p2 = find_skill_use(m2, 7u);
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    EXPECT_EQ(p1->dwStartTime, 0u);
    EXPECT_EQ(p2->dwStartTime, 0u);
}

TEST(SkillApplyPlan, ServerPlanMirrorsProcessAgentSkillServer) {
    SkillDelayManager m1;
    SkillDelayManager m2;
    add_premier_skill(m1, 100u, 10000u);
    add_premier_skill(m2, 100u, 10000u);
    const auto a = process_agent_skill_server(m1, 7u, 100u, 0u);
    const auto plan = agent_skill_server_side_effect_plan(m2, 7u, 100u, 0u);
    EXPECT_EQ(a.kind, AgentSkillActionKind::forward_to_map);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[1].kind, AgentSkillSideEffectKind::ForwardRawToMap);
    // State should match.
    const SkillUse* p1 = find_skill_use(m1, 7u);
    const SkillUse* p2 = find_skill_use(m2, 7u);
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    EXPECT_EQ(p1->dwStartTime, p2->dwStartTime);
    EXPECT_EQ(p1->dwDelay, p2->dwDelay);
}

TEST(SkillApplyPlan, OtherPlanMirrorsProcessAgentSkillOther) {
    const auto a = process_agent_skill_other(7u, 100u);
    const auto plan = agent_skill_other_side_effect_plan(7u, 100u);
    EXPECT_EQ(a.kind, AgentSkillActionKind::forward_to_map);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentSkillSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].character_id, 7u);
    EXPECT_EQ(plan.effects[0].skill_index, 100u);
}
// ---------------------- predicate coverage ----------------------

TEST(SkillPlanPredicates, TargetsMapOnlyForward) {
    AgentSkillSideEffect fwd{AgentSkillSideEffectKind::ForwardRawToMap, 0u, 0u, 0u, true};
    AgentSkillSideEffect nack{AgentSkillSideEffectKind::SendStartNackToUser, 0u, 0u, 0u, false};
    AgentSkillSideEffect user{AgentSkillSideEffectKind::ApplySkillDelayUser, 0u, 0u, 0u, true};
    AgentSkillSideEffect srv{AgentSkillSideEffectKind::ApplySkillDelayServer, 0u, 0u, 0u, true};
    EXPECT_TRUE(agent_skill_effect_targets_map(fwd));
    EXPECT_FALSE(agent_skill_effect_targets_map(nack));
    EXPECT_FALSE(agent_skill_effect_targets_map(user));
    EXPECT_FALSE(agent_skill_effect_targets_map(srv));
}

TEST(SkillPlanPredicates, TargetsUserOnlyNack) {
    AgentSkillSideEffect fwd{AgentSkillSideEffectKind::ForwardRawToMap, 0u, 0u, 0u, true};
    AgentSkillSideEffect nack{AgentSkillSideEffectKind::SendStartNackToUser, 0u, 0u, 0u, false};
    AgentSkillSideEffect user{AgentSkillSideEffectKind::ApplySkillDelayUser, 0u, 0u, 0u, true};
    AgentSkillSideEffect srv{AgentSkillSideEffectKind::ApplySkillDelayServer, 0u, 0u, 0u, true};
    EXPECT_FALSE(agent_skill_effect_targets_user(fwd));
    EXPECT_TRUE(agent_skill_effect_targets_user(nack));
    EXPECT_FALSE(agent_skill_effect_targets_user(user));
    EXPECT_FALSE(agent_skill_effect_targets_user(srv));
}

TEST(SkillPlanPredicates, MutatesStateOnlyApplyKinds) {
    AgentSkillSideEffect fwd{AgentSkillSideEffectKind::ForwardRawToMap, 0u, 0u, 0u, true};
    AgentSkillSideEffect nack{AgentSkillSideEffectKind::SendStartNackToUser, 0u, 0u, 0u, false};
    AgentSkillSideEffect user{AgentSkillSideEffectKind::ApplySkillDelayUser, 0u, 0u, 0u, true};
    AgentSkillSideEffect srv{AgentSkillSideEffectKind::ApplySkillDelayServer, 0u, 0u, 0u, true};
    AgentSkillSideEffect drop{};
    EXPECT_FALSE(agent_skill_effect_mutates_state(fwd));
    EXPECT_FALSE(agent_skill_effect_mutates_state(nack));
    EXPECT_TRUE(agent_skill_effect_mutates_state(user));
    EXPECT_TRUE(agent_skill_effect_mutates_state(srv));
    EXPECT_FALSE(agent_skill_effect_mutates_state(drop));
}

// ---------------------- 1:1 lock: full sequence ----------------------

TEST(SkillApplyPlan, FullLifecycleSequenceMatchesLegacy) {
    // User first cast -> User cooldown -> Server force reset -> User elapsed.
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    // 1) First user cast: plan + apply state mutation.
    const auto p1 = agent_skill_user_side_effect_plan(m, 7u, 100u, 0u);
    EXPECT_TRUE(apply_agent_skill_side_effect_plan(m, p1));
    EXPECT_EQ(skill_use_count(m), 1u);
    // 2) Cooldown user cast: rejected -> only NACK in plan, no state change.
    const auto p2 = agent_skill_user_side_effect_plan(m, 7u, 100u, 4000u);
    const std::size_t count_before = skill_use_count(m);
    EXPECT_TRUE(apply_agent_skill_side_effect_plan(m, p2));
    EXPECT_EQ(skill_use_count(m), count_before);
    EXPECT_EQ(p2.effects.size(), 1u);
    // 3) Server force resets dwStartTime even during cooldown.
    const auto p3 = agent_skill_server_side_effect_plan(m, 7u, 100u, 4000u);
    EXPECT_TRUE(apply_agent_skill_side_effect_plan(m, p3));
    const SkillUse* p = find_skill_use(m, 7u);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->dwStartTime, 4000u);
    // 4) User cast 8000ms after force: 8000-4000+5000=9000 < 10000 -> NACK.
    const auto p4 = agent_skill_user_side_effect_plan(m, 7u, 100u, 8000u);
    ASSERT_EQ(p4.effects.size(), 1u);
    EXPECT_EQ(p4.effects[0].kind, AgentSkillSideEffectKind::SendStartNackToUser);
}
