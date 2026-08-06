// Tests for MP_ITEM_TITAN_* delegate side-effect dispatcher.

#include <mxh/server/titan_delegate_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TitanDelegateValidationInput found() {
    TitanDelegateValidationInput in{};
    in.player_found = true;
    return in;
}

TitanDelegateValidationInput missing() {
    TitanDelegateValidationInput in{};
    in.player_found = false;
    return in;
}

TEST(TitanDelegateOutcome, PlayerFoundIsDelegated) {
    EXPECT_EQ(classify_titan_delegate_outcome(found()),
              TitanDelegateOutcome::Delegated);
}

TEST(TitanDelegateOutcome, PlayerMissingIsNoPlayer) {
    EXPECT_EQ(classify_titan_delegate_outcome(missing()),
              TitanDelegateOutcome::NoPlayer);
}

TEST(TitanDelegatePlan, RegisterSynEmitsTitanManagerCall) {
    auto in = found();
    auto plan = titan_delegate_side_effect_plan(
        in, TitanDelegateAction::TitanRegister, 100, 0);
    EXPECT_TRUE(plan.delegated);
    EXPECT_FALSE(plan.release_lock);
    EXPECT_EQ(plan.action, TitanDelegateAction::TitanRegister);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              TitanDelegateSideEffectKind::DelegateToTitanManager);
    EXPECT_EQ(plan.effects[0].action,
              TitanDelegateAction::TitanRegister);
    EXPECT_EQ(plan.effects[0].player_id, 100u);
}

TEST(TitanDelegatePlan, RegisterReleaseItemEmitsSlotLockRelease) {
    auto in = found();
    auto plan = titan_delegate_side_effect_plan(
        in, TitanDelegateAction::ReleaseItemLock, 100, 25);
    EXPECT_TRUE(plan.delegated);
    EXPECT_TRUE(plan.release_lock);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              TitanDelegateSideEffectKind::ReleaseSlotLock);
    EXPECT_EQ(plan.effects[0].slot_pos, 25u);
}

TEST(TitanDelegatePlan, AllSynActionsDispatchToManager) {
    using A = TitanDelegateAction;
    const A syn_actions[] = {
        A::TitanRegister, A::TitanRegisterAdditem,
        A::TitanCancellation, A::TitanDissolutionAdditem,
        A::TitanMix, A::TitanMixAdditem,
        A::TitanUpgrade, A::TitanUpgradeAdditem,
        A::TitanBreak, A::TitanBreakAdditem,
        A::TitanPartsMake, A::TitanPartsMakeAdditem,
    };
    for (auto a : syn_actions) {
        auto in = found();
        auto plan = titan_delegate_side_effect_plan(in, a, 100, 0);
        EXPECT_TRUE(plan.delegated);
        EXPECT_FALSE(plan.release_lock);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind,
                  TitanDelegateSideEffectKind::DelegateToTitanManager);
        EXPECT_EQ(plan.effects[0].action, a);
    }
}

TEST(TitanDelegatePlan, NoPlayerEmitsEmptyPlan) {
    auto in = missing();
    auto plan = titan_delegate_side_effect_plan(
        in, TitanDelegateAction::TitanRegister, 100, 0);
    EXPECT_FALSE(plan.delegated);
    EXPECT_FALSE(plan.release_lock);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(TitanDelegatePlan, PlanIsIdempotent) {
    auto in = found();
    auto a = titan_delegate_side_effect_plan(
        in, TitanDelegateAction::TitanRegister, 100, 0);
    auto b = titan_delegate_side_effect_plan(
        in, TitanDelegateAction::TitanRegister, 100, 0);
    EXPECT_EQ(a.delegated, b.delegated);
    EXPECT_EQ(a.action, b.action);
    EXPECT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].action, b.effects[i].action);
    }
}
