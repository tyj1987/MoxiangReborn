// decoration_on_side_effect_runtime_test.cpp
//
// Verifies apply_decoration_on_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEMEXT_SHOPITEM_DECORATION_ON
// side-effect chain) walks the data-plane plan and dispatches the
// broadcast entry / empty plan when the player is missing.

#include <mxh/server/decoration_on_side_effect.hpp>
#include <mxh/server/decoration_on_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::DecorationOnSideEffectKind;
using mxh::server::DecorationOnSideEffectSink;
using mxh::server::DecorationOnValidationInput;
using mxh::server::apply_decoration_on_side_effects;
using mxh::server::decoration_on_side_effect_plan;

class RecordingSink final : public DecorationOnSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_player_id = 0;
    std::uint32_t last_data1 = 0;
    std::uint32_t last_data2 = 0;
    std::size_t broadcast_count = 0;

    void broadcast_to_others(std::uint32_t player_id,
                             std::uint32_t data1,
                             std::uint32_t data2) override {
        calls.push_back("broadcast");
        last_player_id = player_id;
        last_data1 = data1;
        last_data2 = data2;
        ++broadcast_count;
    }
};

}  // namespace

TEST(ApplyDecorationOnSideEffects, BroadcastEmitsToOthers) {
    DecorationOnValidationInput in;
    in.player_found = true;
    auto plan = decoration_on_side_effect_plan(
        in, /*player_id=*/0x00220023u,
        /*data1=*/11, /*data2=*/22);
    EXPECT_TRUE(plan.broadcast);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              DecorationOnSideEffectKind::BroadcastToOthers);
    EXPECT_EQ(plan.effects[0].data1, 11u);
    EXPECT_EQ(plan.effects[0].data2, 22u);

    RecordingSink sink;
    auto out = apply_decoration_on_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.broadcasts_sent, 1u);
    EXPECT_TRUE(out.broadcast_flag_consumed);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"broadcast"}));
    EXPECT_EQ(sink.last_player_id, 0x00220023u);
    EXPECT_EQ(sink.last_data1, 11u);
    EXPECT_EQ(sink.last_data2, 22u);
}

TEST(ApplyDecorationOnSideEffects, NoPlayerEmitsEmptyPlan) {
    DecorationOnValidationInput in;
    in.player_found = false;
    auto plan = decoration_on_side_effect_plan(in, 1, 2, 3);
    EXPECT_FALSE(plan.broadcast);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_decoration_on_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.broadcasts_sent, 0u);
    EXPECT_FALSE(out.broadcast_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyDecorationOnSideEffects, EmptyPlanIsNoOp) {
    mxh::server::DecorationOnSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_decoration_on_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.broadcasts_sent, 0u);
    EXPECT_FALSE(out.broadcast_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyDecorationOnSideEffects, BoundaryDataPassthrough) {
    DecorationOnValidationInput in;
    in.player_found = true;
    auto plan = decoration_on_side_effect_plan(
        in, /*player_id=*/0xFFFFFFFFu,
        /*data1=*/0xFFFFFFFFu, /*data2=*/0xFFFFFFFFu);
    RecordingSink sink;
    (void)apply_decoration_on_side_effects(plan, sink);
    EXPECT_EQ(sink.last_player_id, 0xFFFFFFFFu);
    EXPECT_EQ(sink.last_data1, 0xFFFFFFFFu);
    EXPECT_EQ(sink.last_data2, 0xFFFFFFFFu);
}
