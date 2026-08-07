// titan_delegate_side_effect_runtime_test.cpp
//
// Verifies apply_titan_delegate_side_effects() (the runtime
// orchestrator for the CItemManager MP_ITEM_TITAN_*_SYN /
// _ADDITEM_SYN / _RELEASEITEM delegations) walks the data-plane plan
// and dispatches the entry: TITANITEMMGR delegate for the 12 craft
// actions / slot-lock release for the 6 release actions / empty plan
// when the player is missing.

#include <mxh/server/titan_delegate_side_effect.hpp>
#include <mxh/server/titan_delegate_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::TitanDelegateAction;
using mxh::server::TitanDelegateSideEffectKind;
using mxh::server::TitanDelegateSideEffectSink;
using mxh::server::TitanDelegateValidationInput;
using mxh::server::apply_titan_delegate_side_effects;
using mxh::server::titan_delegate_side_effect_plan;

class RecordingSink final : public TitanDelegateSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_player_id = 0;
    TitanDelegateAction last_action = TitanDelegateAction::TitanRegister;
    std::uint16_t last_slot_pos = 0;
    std::size_t delegate_count = 0;
    std::size_t release_count = 0;

    void delegate_to_titan_manager(std::uint32_t player_id,
                                   TitanDelegateAction action) override {
        calls.push_back("delegate");
        last_player_id = player_id;
        last_action = action;
        ++delegate_count;
    }
    void release_slot_lock(std::uint32_t player_id,
                           std::uint16_t slot_pos) override {
        calls.push_back("release");
        last_player_id = player_id;
        last_slot_pos = slot_pos;
        ++release_count;
    }
};

}  // namespace

TEST(ApplyTitanDelegateSideEffects, RegisterDelegatesToTitanManager) {
    TitanDelegateValidationInput in;
    in.player_found = true;
    auto plan = titan_delegate_side_effect_plan(
        in, TitanDelegateAction::TitanRegister,
        /*player_id=*/77, /*slot_pos=*/0);
    EXPECT_TRUE(plan.delegated);
    EXPECT_FALSE(plan.release_lock);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              TitanDelegateSideEffectKind::DelegateToTitanManager);
    EXPECT_EQ(plan.effects[0].action, TitanDelegateAction::TitanRegister);

    RecordingSink sink;
    auto out = apply_titan_delegate_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.delegations, 1u);
    EXPECT_EQ(out.releases, 0u);
    EXPECT_TRUE(out.delegated_flag_consumed);
    EXPECT_FALSE(out.release_flag_consumed);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"delegate"}));
    EXPECT_EQ(sink.last_player_id, 77u);
    EXPECT_EQ(sink.last_action, TitanDelegateAction::TitanRegister);
}

TEST(ApplyTitanDelegateSideEffects, AllCraftActionsDelegate) {
    const TitanDelegateAction actions[] = {
        TitanDelegateAction::TitanRegister,
        TitanDelegateAction::TitanRegisterAdditem,
        TitanDelegateAction::TitanCancellation,
        TitanDelegateAction::TitanDissolutionAdditem,
        TitanDelegateAction::TitanMix,
        TitanDelegateAction::TitanMixAdditem,
        TitanDelegateAction::TitanUpgrade,
        TitanDelegateAction::TitanUpgradeAdditem,
        TitanDelegateAction::TitanBreak,
        TitanDelegateAction::TitanBreakAdditem,
        TitanDelegateAction::TitanPartsMake,
        TitanDelegateAction::TitanPartsMakeAdditem,
    };
    TitanDelegateValidationInput in;
    in.player_found = true;
    for (const auto action : actions) {
        auto plan = titan_delegate_side_effect_plan(
            in, action, 1, 0);
        EXPECT_TRUE(plan.delegated);
        EXPECT_FALSE(plan.release_lock);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind,
                  TitanDelegateSideEffectKind::DelegateToTitanManager);
        EXPECT_EQ(plan.effects[0].action, action);

        RecordingSink sink;
        (void)apply_titan_delegate_side_effects(plan, sink);
        EXPECT_EQ(sink.last_action, action);
        EXPECT_EQ(sink.delegate_count, 1u);
        EXPECT_EQ(sink.release_count, 0u);
    }
}

TEST(ApplyTitanDelegateSideEffects, ReleaseItemLockReleasesSlotLock) {
    TitanDelegateValidationInput in;
    in.player_found = true;
    auto plan = titan_delegate_side_effect_plan(
        in, TitanDelegateAction::ReleaseItemLock,
        /*player_id=*/0x000E000Fu, /*slot_pos=*/12);
    EXPECT_TRUE(plan.delegated);
    EXPECT_TRUE(plan.release_lock);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              TitanDelegateSideEffectKind::ReleaseSlotLock);
    EXPECT_EQ(plan.effects[0].action,
              TitanDelegateAction::ReleaseItemLock);
    EXPECT_EQ(plan.effects[0].slot_pos, 12u);

    RecordingSink sink;
    auto out = apply_titan_delegate_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.releases, 1u);
    EXPECT_EQ(out.delegations, 0u);
    EXPECT_TRUE(out.release_flag_consumed);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"release"}));
    EXPECT_EQ(sink.last_player_id, 0x000E000Fu);
    EXPECT_EQ(sink.last_slot_pos, 12u);
}

TEST(ApplyTitanDelegateSideEffects, NoPlayerEmitsEmptyPlan) {
    for (const auto action :
         {TitanDelegateAction::TitanMix, TitanDelegateAction::ReleaseItemLock}) {
        TitanDelegateValidationInput in;
        in.player_found = false;
        auto plan = titan_delegate_side_effect_plan(in, action, 5, 9);
        EXPECT_FALSE(plan.delegated);
        EXPECT_FALSE(plan.release_lock);
        EXPECT_TRUE(plan.effects.empty());

        RecordingSink sink;
        auto out = apply_titan_delegate_side_effects(plan, sink);
        EXPECT_EQ(out.effects_applied, 0u);
        EXPECT_EQ(out.delegations, 0u);
        EXPECT_EQ(out.releases, 0u);
        EXPECT_TRUE(sink.calls.empty());
    }
}

TEST(ApplyTitanDelegateSideEffects, EmptyPlanIsNoOp) {
    mxh::server::TitanDelegateSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_titan_delegate_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.delegations, 0u);
    EXPECT_EQ(out.releases, 0u);
    EXPECT_FALSE(out.delegated_flag_consumed);
    EXPECT_FALSE(out.release_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyTitanDelegateSideEffects, BoundaryPlayerIdAndSlotPos) {
    TitanDelegateValidationInput in;
    in.player_found = true;
    auto plan = titan_delegate_side_effect_plan(
        in, TitanDelegateAction::ReleaseItemLock,
        /*player_id=*/0xFFFFFFFFu, /*slot_pos=*/0xFFFFu);
    RecordingSink sink;
    (void)apply_titan_delegate_side_effects(plan, sink);
    EXPECT_EQ(sink.last_player_id, 0xFFFFFFFFu);
    EXPECT_EQ(sink.last_slot_pos, 0xFFFFu);
}

TEST(ApplyTitanDelegateSideEffects, DelegateDoesNotTouchReleaseFlag) {
    TitanDelegateValidationInput in;
    in.player_found = true;
    auto plan = titan_delegate_side_effect_plan(
        in, TitanDelegateAction::TitanRegisterAdditem, 1, 0);
    RecordingSink sink;
    auto out = apply_titan_delegate_side_effects(plan, sink);
    EXPECT_TRUE(out.delegated_flag_consumed);
    EXPECT_FALSE(out.release_flag_consumed);
    EXPECT_EQ(sink.release_count, 0u);
}
