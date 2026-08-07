// item_upgrade_side_effect_runtime_test.cpp
//
// Verifies apply_item_upgrade_side_effects() (the runtime orchestrator
// for the CItemManager::MP_ITEM_UPGRADE_SYN side-effect chain) walks
// the data-plane plan and dispatches the single entry: ACK on
// UpgradeItem success / error-NACK with eItemUseErr_Upgrade on
// failure.

#include <mxh/server/item_upgrade_side_effect.hpp>
#include <mxh/server/item_upgrade_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using mxh::server::ItemUpgradeSideEffectKind;
using mxh::server::ItemUpgradeSideEffectSink;
using mxh::server::LEGACY_EITEMUSE_UPGRADE;
using mxh::server::apply_item_upgrade_side_effects;
using mxh::server::item_upgrade_side_effect_plan;

class RecordingSink final : public ItemUpgradeSideEffectSink {
public:
    std::string last_call;
    int last_rt = 0;
    int last_error_code = 0;
    std::uint16_t last_item_idx = 0;
    std::uint16_t last_item_pos = 0;
    std::uint16_t last_material_item_idx = 0;
    std::uint16_t last_material_item_pos = 0;
    std::size_t ack_count = 0;
    std::size_t nack_count = 0;

    void broadcast_upgrade_success_ack(
        std::uint16_t item_idx, std::uint16_t item_pos,
        std::uint16_t material_item_idx, std::uint16_t material_item_pos,
        int original_rt) override {
        last_call = "ack";
        last_item_idx = item_idx;
        last_item_pos = item_pos;
        last_material_item_idx = material_item_idx;
        last_material_item_pos = material_item_pos;
        last_rt = original_rt;
        ++ack_count;
    }
    void broadcast_upgrade_error_nack(
        std::uint16_t item_idx, std::uint16_t item_pos,
        std::uint16_t material_item_idx, std::uint16_t material_item_pos,
        int original_rt, int error_code) override {
        last_call = "nack";
        last_item_idx = item_idx;
        last_item_pos = item_pos;
        last_material_item_idx = material_item_idx;
        last_material_item_pos = material_item_pos;
        last_rt = original_rt;
        last_error_code = error_code;
        ++nack_count;
    }
};

}  // namespace

TEST(ApplyItemUpgradeSideEffects, SuccessRtEmitsAck) {
    auto plan = item_upgrade_side_effect_plan(
        /*upgrade_rt=*/0, /*item_idx=*/100, /*item_pos=*/7,
        /*material_item_idx=*/200, /*material_item_pos=*/8);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemUpgradeSideEffectKind::BroadcastUpgradeSuccessAck);

    RecordingSink sink;
    auto out = apply_item_upgrade_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.acks_sent, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_TRUE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "ack");
    EXPECT_EQ(sink.last_item_idx, 100u);
    EXPECT_EQ(sink.last_item_pos, 7u);
    EXPECT_EQ(sink.last_material_item_idx, 200u);
    EXPECT_EQ(sink.last_material_item_pos, 8u);
    EXPECT_EQ(sink.last_rt, 0);
    EXPECT_EQ(sink.ack_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyItemUpgradeSideEffects, FailureRtEmitsNackWithUpgradeEcode) {
    // Legacy: any non-zero rt -> MSG_ITEM_ERROR with
    // ECode = eItemUseErr_Upgrade (= 10).
    auto plan = item_upgrade_side_effect_plan(
        /*upgrade_rt=*/5, /*item_idx=*/100, /*item_pos=*/7,
        /*material_item_idx=*/200, /*material_item_pos=*/8);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemUpgradeSideEffectKind::BroadcastUpgradeErrorNack);
    EXPECT_EQ(plan.effects[0].error_code, LEGACY_EITEMUSE_UPGRADE);

    RecordingSink sink;
    auto out = apply_item_upgrade_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_TRUE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_rt, 5);
    EXPECT_EQ(sink.last_error_code, LEGACY_EITEMUSE_UPGRADE);
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 1u);
}

TEST(ApplyItemUpgradeSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ItemUpgradeSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_item_upgrade_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyItemUpgradeSideEffects, VariousFailureCodesAllEmitNack) {
    for (int rt : {1, 5, 10, 99, -1}) {
        auto plan = item_upgrade_side_effect_plan(
            rt, 1, 2, 3, 4);
        EXPECT_TRUE(plan.send_nack);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind,
                  ItemUpgradeSideEffectKind::BroadcastUpgradeErrorNack);
        EXPECT_EQ(plan.effects[0].error_code, LEGACY_EITEMUSE_UPGRADE);

        RecordingSink sink;
        (void)apply_item_upgrade_side_effects(plan, sink);
        EXPECT_EQ(sink.last_call, "nack");
        EXPECT_EQ(sink.last_rt, rt);
        EXPECT_EQ(sink.last_error_code, LEGACY_EITEMUSE_UPGRADE);
    }
}

TEST(ApplyItemUpgradeSideEffects, NackDoesNotTouchAckState) {
    auto plan = item_upgrade_side_effect_plan(3, 1, 2, 3, 4);
    RecordingSink sink;
    (void)apply_item_upgrade_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_error_code, LEGACY_EITEMUSE_UPGRADE);
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 1u);
}

TEST(ApplyItemUpgradeSideEffects, FieldPassthroughOnAck) {
    auto plan = item_upgrade_side_effect_plan(0, 11, 12, 13, 14);
    RecordingSink sink;
    (void)apply_item_upgrade_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "ack");
    EXPECT_EQ(sink.last_item_idx, 11u);
    EXPECT_EQ(sink.last_item_pos, 12u);
    EXPECT_EQ(sink.last_material_item_idx, 13u);
    EXPECT_EQ(sink.last_material_item_pos, 14u);
    EXPECT_EQ(sink.last_rt, 0);
    EXPECT_EQ(sink.nack_count, 0u);
}
