//
// agent_simple_auth_forward_side_effect_plan_test.cpp -- D4.122
//
// 1:1 lock the legacy auth-gate side-effects applied by
// [Server]Agent/AgentNetworkMsgParser.cpp MP_STREETSTALLUserMsgParser +
// MP_EXCHANGEUserMsgParser. Both share the same gate: drop when user not found OR
// character_id mismatch, else forward to map.
//

#include <gtest/gtest.h>

#include "mxh/server/agent_simple_auth_forward.hpp"
#include "mxh/server/agent_simple_auth_forward_side_effect_plan.hpp"

using namespace mxh::server;


//
// agent_simple_auth_forward_side_effect_plan_test.cpp -- D4.122
//
// 1:1 lock the legacy auth-gate side-effects applied by
// [Server]Agent/AgentNetworkMsgParser.cpp MP_STREETSTALLUserMsgParser +
// MP_EXCHANGEUserMsgParser. Both share the same gate: drop when user not found OR
// character_id mismatch, else forward to map.
//

#include <gtest/gtest.h>

#include "mxh/server/agent_simple_auth_forward.hpp"
#include "mxh/server/agent_simple_auth_forward_side_effect_plan.hpp"

using namespace mxh::server;

// ---------------------- generic plan-builder ----------------------

TEST(SimpleAuthPlan, ForwardActionEmitsForwardPlan) {
    const auto plan = agent_simple_auth_forward_side_effect_plan(
        7u, 0u, streetstall_category, 0u);  // forward_to_map
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentSimpleAuthForwardSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].connection_index, 7u);
    EXPECT_EQ(plan.effects[0].category, streetstall_category);
    EXPECT_TRUE(agent_simple_auth_forward_effect_targets_map(plan.effects[0]));
}

TEST(SimpleAuthPlan, DropNoUserEmitsDropPlan) {
    const auto plan = agent_simple_auth_forward_side_effect_plan(
        7u, 0u, streetstall_category, 1u);  // drop_no_user
    EXPECT_TRUE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentSimpleAuthForwardSideEffectKind::Drop);
    EXPECT_EQ(plan.effects[0].connection_index, 7u);
    EXPECT_EQ(plan.effects[0].protocol, 0u);  // cleared
}

TEST(SimpleAuthPlan, DropObjectMismatchEmitsDropPlan) {
    const auto plan = agent_simple_auth_forward_side_effect_plan(
        7u, 0u, exchange_category, 2u);  // drop_object_mismatch
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentSimpleAuthForwardSideEffectKind::Drop);
    EXPECT_EQ(plan.effects[0].category, exchange_category);
}
// ---------------------- StreetStallUserAction ----------------------

TEST(StreetStallUserPlan, ForwardActionEmitsForwardEffect) {
    StreetStallUserAction a{};
    a.kind = StreetStallUserActionKind::forward_to_map;
    a.connection_index = 5u;
    const auto plan = agent_streetstall_user_side_effect_plan(a);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentSimpleAuthForwardSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].connection_index, 5u);
    EXPECT_EQ(plan.effects[0].category, streetstall_category);
}

TEST(StreetStallUserPlan, DropNoUserEmitsDropEffect) {
    StreetStallUserAction a{};
    a.kind = StreetStallUserActionKind::drop_no_user;
    const auto plan = agent_streetstall_user_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentSimpleAuthForwardSideEffectKind::Drop);
}

TEST(StreetStallUserPlan, DropObjectMismatchEmitsDropEffect) {
    StreetStallUserAction a{};
    a.kind = StreetStallUserActionKind::drop_object_mismatch;
    const auto plan = agent_streetstall_user_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentSimpleAuthForwardSideEffectKind::Drop);
}

// ---------------------- StreetStallUserRequest (classify-style) ----------------------

TEST(StreetStallUserClassify, UserFoundAndIdMatchForwards) {
    StreetStallUserRequest r{};
    r.user_found = true;
    r.object_id = 100u;
    r.character_id = 100u;
    r.connection_index = 7u;
    const auto plan = agent_streetstall_user_side_effect_plan(r);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentSimpleAuthForwardSideEffectKind::ForwardRawToMap);
}

TEST(StreetStallUserClassify, UserNotFoundDrops) {
    StreetStallUserRequest r{};
    r.user_found = false;
    r.object_id = 100u;
    r.character_id = 100u;
    const auto plan = agent_streetstall_user_side_effect_plan(r);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentSimpleAuthForwardSideEffectKind::Drop);
}

TEST(StreetStallUserClassify, CharacterIdMismatchDrops) {
    StreetStallUserRequest r{};
    r.user_found = true;
    r.object_id = 100u;
    r.character_id = 999u;
    const auto plan = agent_streetstall_user_side_effect_plan(r);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentSimpleAuthForwardSideEffectKind::Drop);
}
// ---------------------- ExchangeUserAction ----------------------

TEST(ExchangeUserPlan, ForwardActionEmitsForwardEffect) {
    ExchangeUserAction a{};
    a.kind = ExchangeUserActionKind::forward_to_map;
    a.connection_index = 5u;
    const auto plan = agent_exchange_user_side_effect_plan(a);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentSimpleAuthForwardSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].connection_index, 5u);
    EXPECT_EQ(plan.effects[0].category, exchange_category);
}

TEST(ExchangeUserPlan, DropNoUserEmitsDropEffect) {
    ExchangeUserAction a{};
    a.kind = ExchangeUserActionKind::drop_no_user;
    const auto plan = agent_exchange_user_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentSimpleAuthForwardSideEffectKind::Drop);
}

TEST(ExchangeUserPlan, DropObjectMismatchEmitsDropEffect) {
    ExchangeUserAction a{};
    a.kind = ExchangeUserActionKind::drop_object_mismatch;
    const auto plan = agent_exchange_user_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentSimpleAuthForwardSideEffectKind::Drop);
}

// ---------------------- ExchangeUserRequest (classify-style) ----------------------

TEST(ExchangeUserClassify, UserFoundAndIdMatchForwards) {
    ExchangeUserRequest r{};
    r.user_found = true;
    r.object_id = 200u;
    r.character_id = 200u;
    r.connection_index = 7u;
    const auto plan = agent_exchange_user_side_effect_plan(r);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentSimpleAuthForwardSideEffectKind::ForwardRawToMap);
}

TEST(ExchangeUserClassify, UserNotFoundDrops) {
    ExchangeUserRequest r{};
    r.user_found = false;
    const auto plan = agent_exchange_user_side_effect_plan(r);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentSimpleAuthForwardSideEffectKind::Drop);
}

TEST(ExchangeUserClassify, CharacterIdMismatchDrops) {
    ExchangeUserRequest r{};
    r.user_found = true;
    r.object_id = 200u;
    r.character_id = 300u;
    const auto plan = agent_exchange_user_side_effect_plan(r);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentSimpleAuthForwardSideEffectKind::Drop);
}
// ---------------------- apply ----------------------

TEST(SimpleAuthApplyPlan, ForwardPlanReturnsTrue) {
    StreetStallUserAction a{};
    a.kind = StreetStallUserActionKind::forward_to_map;
    const auto plan = agent_streetstall_user_side_effect_plan(a);
    EXPECT_TRUE(apply_agent_simple_auth_forward_side_effect_plan(plan));
}

TEST(SimpleAuthApplyPlan, DropPlanReturnsFalse) {
    StreetStallUserAction a{};
    a.kind = StreetStallUserActionKind::drop_no_user;
    const auto plan = agent_streetstall_user_side_effect_plan(a);
    EXPECT_FALSE(apply_agent_simple_auth_forward_side_effect_plan(plan));
}

TEST(SimpleAuthApplyPlan, EmptyEffectsPlanReturnsFalse) {
    AgentSimpleAuthForwardSideEffectPlan plan;
    plan.dispatched = true;
    plan.drop = false;
    EXPECT_FALSE(apply_agent_simple_auth_forward_side_effect_plan(plan));
}

// ---------------------- 1:1 mirror ----------------------

TEST(SimpleAuthApplyPlan, StreetstallPlanMirrorsClassify) {
    StreetStallUserRequest r{};
    r.user_found = true;
    r.object_id = 50u;
    r.character_id = 50u;
    r.connection_index = 7u;
    const auto a = classify_streetstall_user(r);
    const auto plan = agent_streetstall_user_side_effect_plan(r);
    EXPECT_EQ(a.kind, StreetStallUserActionKind::forward_to_map);
    EXPECT_EQ(plan.effects[0].kind, AgentSimpleAuthForwardSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].connection_index, a.connection_index);
}

TEST(SimpleAuthApplyPlan, ExchangePlanMirrorsClassify) {
    ExchangeUserRequest r{};
    r.user_found = false;
    r.object_id = 50u;
    r.character_id = 50u;
    r.connection_index = 7u;
    const auto a = classify_exchange_user(r);
    const auto plan = agent_exchange_user_side_effect_plan(r);
    EXPECT_EQ(a.kind, ExchangeUserActionKind::drop_no_user);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentSimpleAuthForwardSideEffectKind::Drop);
    EXPECT_EQ(plan.effects[0].category, exchange_category);
}

// ---------------------- category lock ----------------------

TEST(SimpleAuthCategory, StreetstallCategoryIs29) {
    EXPECT_EQ(streetstall_category, 29u);
}

TEST(SimpleAuthCategory, ExchangeCategoryIs28) {
    EXPECT_EQ(exchange_category, 28u);
}

// ---------------------- predicate coverage ----------------------

TEST(SimpleAuthPredicates, TargetsMapOnlyForward) {
    AgentSimpleAuthForwardSideEffect fwd{AgentSimpleAuthForwardSideEffectKind::ForwardRawToMap, 0u, 0u, 0u};
    AgentSimpleAuthForwardSideEffect drop{AgentSimpleAuthForwardSideEffectKind::Drop, 0u, 0u, 0u};
    EXPECT_TRUE(agent_simple_auth_forward_effect_targets_map(fwd));
    EXPECT_FALSE(agent_simple_auth_forward_effect_targets_map(drop));
}
