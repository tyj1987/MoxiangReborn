// skin_discard_side_effect_runtime_test.cpp
//
// Verifies apply_skin_discard_side_effects() (the runtime
// orchestrator for the CShopItemManager::DiscardSkinItem
// side-effect chain) walks the data-plane plan and dispatches each
// entry to its respective subsystem in the legacy order.

#include <mxh/server/skin_discard_side_effect.hpp>
#include <mxh/server/skin_discard_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace {

using mxh::server::SkinDiscardSideEffectKind;
using mxh::server::SkinDiscardSideEffectSink;
using mxh::server::apply_skin_discard_side_effects;
using mxh::server::skin_discard_side_effect_plan;

class RecordingSink final : public SkinDiscardSideEffectSink {
public:
    std::vector<std::string> calls;

    void write_skin_item_update() override { calls.push_back("write_skin"); }
    void character_skin_info_update() override { calls.push_back("db_update"); }
    void broadcast_skin_info() override { calls.push_back("broadcast"); }
};

}  // namespace

TEST(ApplySkinDiscardSideEffects, ThreeStepsInLegacyOrder) {
    auto plan = skin_discard_side_effect_plan();
    RecordingSink sink;
    auto out = apply_skin_discard_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 3u);
    EXPECT_EQ(out.skin_writes, 1u);
    EXPECT_EQ(out.db_updates, 1u);
    EXPECT_EQ(out.broadcasts, 1u);
    EXPECT_TRUE(out.broadcast_flag_consumed);
    const std::vector<std::string> kExpected = {
        "write_skin", "db_update", "broadcast",
    };
    EXPECT_EQ(sink.calls, kExpected);
}

TEST(ApplySkinDiscardSideEffects, WriteSkinItemUpdateHappensBeforeDb) {
    // The legacy order: RemoveEquipSkin (write back) -> DB -> broadcast.
    // Locks the relative ordering of the first two steps.
    auto plan = skin_discard_side_effect_plan();
    RecordingSink sink;
    apply_skin_discard_side_effects(plan, sink);
    ASSERT_EQ(sink.calls.size(), 3u);
    EXPECT_EQ(sink.calls[0], "write_skin");
    EXPECT_EQ(sink.calls[1], "db_update");
    EXPECT_EQ(sink.calls[2], "broadcast");
}

TEST(ApplySkinDiscardSideEffects, EmptyPlanIsNoOp) {
    mxh::server::SkinDiscardSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_skin_discard_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_FALSE(out.broadcast_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplySkinDiscardSideEffects, NoDelayResetOnDiscard) {
    // The legacy code does NOT reset the skin delay timer on discard
    // (delay is only set when a skin is applied, not when it is
    // removed). The runtime therefore has no StartSkinDelay step.
    // We verify the recording sink's call list contains no
    // "start_delay" entry.
    auto plan = skin_discard_side_effect_plan();
    RecordingSink sink;
    apply_skin_discard_side_effects(plan, sink);
    for (const auto& c : sink.calls) {
        EXPECT_NE(c, "start_delay");
    }
}
