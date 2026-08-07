// skin_select_side_effect_runtime_test.cpp
//
// Verifies apply_skin_select_success_side_effects() and
// apply_skin_select_nack_side_effects() (the runtime orchestrators
// for the legacy ItemManager::MP_ITEMEXT_SKINTITEM_SELECT handler
// side-effect chains) walk the data-plane plans and dispatch each
// entry to its respective subsystem.
//
// Locks the 3-step success chain in legacy order, the single-step
// NACK payload (3 DWORDs), and the empty-plan paths.

#include <mxh/server/skin_select_side_effect.hpp>
#include <mxh/server/skin_select_side_effect_runtime.hpp>
#include <mxh/server/skin_select_transition.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace {

using mxh::server::SkinSelectResult;
using mxh::server::SkinSelectSideEffectKind;
using mxh::server::SkinSelectSideEffectSink;
using mxh::server::apply_skin_select_nack_side_effects;
using mxh::server::apply_skin_select_success_side_effects;
using mxh::server::skin_select_success_side_effect_plan;
using mxh::server::skin_select_nack_side_effect_plan;

class RecordingSink final : public SkinSelectSideEffectSink {
public:
    std::vector<std::string> calls;

    void start_skin_delay() override { calls.push_back("start_delay"); }
    void character_skin_info_update() override { calls.push_back("db_update"); }
    void broadcast_skin_info() override { calls.push_back("broadcast"); }
    void send_nack(std::uint32_t result_code,
                   std::uint32_t skin_delay_remaining,
                   std::uint32_t dw_limit_level) override {
        calls.push_back("nack " + std::to_string(result_code) + "/" +
                        std::to_string(skin_delay_remaining) + "/" +
                        std::to_string(dw_limit_level));
    }
};

}  // namespace

// ----- success path: 3-step chain in legacy order -----

TEST(ApplySkinSelectSuccessSideEffects, ThreeStepsInLegacyOrder) {
    auto plan = skin_select_success_side_effect_plan();
    RecordingSink sink;
    auto out = apply_skin_select_success_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 3u);
    EXPECT_EQ(out.delays_started, 1u);
    EXPECT_EQ(out.db_updates, 1u);
    EXPECT_EQ(out.broadcasts, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_TRUE(out.broadcast_flag_consumed);
    const std::vector<std::string> kExpected = {
        "start_delay", "db_update", "broadcast",
    };
    EXPECT_EQ(sink.calls, kExpected);
}

// ----- NACK path: single step with 3 DWORDs -----

TEST(ApplySkinSelectNackSideEffects, FailResultEmitsNackWithCorrectDwords) {
    auto plan = skin_select_nack_side_effect_plan(
        SkinSelectResult::Fail,
        /*skin_delay_remaining=*/30000,
        /*dw_limit_level=*/0);
    RecordingSink sink;
    auto out = apply_skin_select_nack_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_TRUE(out.nack_flag_consumed);
    const std::vector<std::string> kExpected = {"nack 1/30000/0"};
    EXPECT_EQ(sink.calls, kExpected);
}

TEST(ApplySkinSelectNackSideEffects, DelayFailResultEmitsNackWithDelay) {
    auto plan = skin_select_nack_side_effect_plan(
        SkinSelectResult::DelayFail,
        /*skin_delay_remaining=*/60000,
        /*dw_limit_level=*/0);
    RecordingSink sink;
    auto out = apply_skin_select_nack_side_effects(plan, sink);
    EXPECT_EQ(out.nacks_sent, 1u);
    const std::vector<std::string> kExpected = {"nack 2/60000/0"};
    EXPECT_EQ(sink.calls, kExpected);
}

TEST(ApplySkinSelectNackSideEffects, LevelFailResultEmitsNackWithLimit) {
    auto plan = skin_select_nack_side_effect_plan(
        SkinSelectResult::LevelFail,
        /*skin_delay_remaining=*/0,
        /*dw_limit_level=*/42);
    RecordingSink sink;
    auto out = apply_skin_select_nack_side_effects(plan, sink);
    EXPECT_EQ(out.nacks_sent, 1u);
    const std::vector<std::string> kExpected = {"nack 3/0/42"};
    EXPECT_EQ(sink.calls, kExpected);
}

// ----- empty plan paths -----

TEST(ApplySkinSelectSuccessSideEffects, EmptyPlanIsNoOp) {
    mxh::server::SkinSelectSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_skin_select_success_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_FALSE(out.broadcast_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplySkinSelectNackSideEffects, SuccessPathReturnsEmptyNackPlan) {
    // Success is routed through the success plan, not the NACK plan.
    // The data plane returns an empty NackPlan for Success -- the
    // orchestrator must therefore not emit a NACK.
    auto plan = skin_select_nack_side_effect_plan(
        SkinSelectResult::Success, 0, 0);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.steps.empty());
    RecordingSink sink;
    auto out = apply_skin_select_nack_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

// ----- success and nack flows are independent -----

TEST(ApplySkinSelectSuccessSideEffects, SuccessFlowDoesNotEmitNack) {
    auto plan = skin_select_success_side_effect_plan();
    RecordingSink sink;
    apply_skin_select_success_side_effects(plan, sink);
    EXPECT_EQ(sink.calls.size(), 3u);
    for (const auto& c : sink.calls) {
        EXPECT_EQ(c.find("nack"), std::string::npos);
    }
}
