// item_unseal_side_effect_runtime_test.cpp
//
// Verifies apply_item_unseal_side_effects() (the runtime orchestrator
// for the CItemManager::MP_ITEM_SHOPITEM_UNSEAL_SYN side-effect chain)
// walks the data-plane plan and dispatches the single entry to its
// subsystem: ACK / NACK via protocol flip with dwData preserved.

#include <mxh/server/item_unseal_side_effect.hpp>
#include <mxh/server/item_unseal_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using mxh::server::ItemUnsealSideEffectKind;
using mxh::server::ItemUnsealSideEffectSink;
using mxh::server::apply_item_unseal_side_effects;
using mxh::server::item_unseal_side_effect_plan;

class RecordingSink final : public ItemUnsealSideEffectSink {
public:
    std::string last_call;
    std::uint32_t last_dw_data = 0;
    std::uint16_t last_target_pos = 0;
    std::size_t ack_count = 0;
    std::size_t nack_count = 0;

    void broadcast_unseal_ack(std::uint32_t dw_data,
                              std::uint16_t target_pos) override {
        last_call = "ack";
        last_dw_data = dw_data;
        last_target_pos = target_pos;
        ++ack_count;
    }
    void broadcast_unseal_nack(std::uint32_t dw_data,
                               std::uint16_t target_pos) override {
        last_call = "nack";
        last_dw_data = dw_data;
        last_target_pos = target_pos;
        ++nack_count;
    }
};

}  // namespace

TEST(ApplyItemUnsealSideEffects, SuccessEmitsAckWithDwData) {
    auto plan = item_unseal_side_effect_plan(
        /*unseal_ok=*/true, /*dw_data=*/0x00010005u);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemUnsealSideEffectKind::BroadcastUnsealAck);
    EXPECT_EQ(plan.effects[0].dw_data, 0x00010005u);
    EXPECT_EQ(plan.effects[0].target_pos, 5u);  // low word of dwData

    RecordingSink sink;
    auto out = apply_item_unseal_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.acks_sent, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_TRUE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "ack");
    EXPECT_EQ(sink.last_dw_data, 0x00010005u);
    EXPECT_EQ(sink.last_target_pos, 5u);
    EXPECT_EQ(sink.ack_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyItemUnsealSideEffects, FailureEmitsNackWithDwData) {
    auto plan = item_unseal_side_effect_plan(
        /*unseal_ok=*/false, /*dw_data=*/0x00020007u);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemUnsealSideEffectKind::BroadcastUnsealNack);
    EXPECT_EQ(plan.effects[0].dw_data, 0x00020007u);

    RecordingSink sink;
    auto out = apply_item_unseal_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_TRUE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_dw_data, 0x00020007u);
    EXPECT_EQ(sink.last_target_pos, 7u);
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 1u);
}

TEST(ApplyItemUnsealSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ItemUnsealSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_item_unseal_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyItemUnsealSideEffects, TargetPosIsLowWordOfDwData) {
    // Legacy POSTYPE is the low 16 bits of dwData; the high word is a
    // separate field carried in dw_data.
    auto plan = item_unseal_side_effect_plan(
        /*unseal_ok=*/true, /*dw_data=*/0xFFFFABCDu);
    EXPECT_EQ(plan.effects[0].target_pos, 0xABCDu);

    RecordingSink sink;
    (void)apply_item_unseal_side_effects(plan, sink);
    EXPECT_EQ(sink.last_target_pos, 0xABCDu);
    EXPECT_EQ(sink.last_dw_data, 0xFFFFABCDu);
}

TEST(ApplyItemUnsealSideEffects, NackDoesNotTouchAckState) {
    auto plan = item_unseal_side_effect_plan(false, 0x00000009u);
    RecordingSink sink;
    (void)apply_item_unseal_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 1u);
}

TEST(ApplyItemUnsealSideEffects, ZeroDwDataStillDispatches) {
    auto plan = item_unseal_side_effect_plan(true, 0u);
    RecordingSink sink;
    (void)apply_item_unseal_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "ack");
    EXPECT_EQ(sink.last_dw_data, 0u);
    EXPECT_EQ(sink.last_target_pos, 0u);
    EXPECT_EQ(sink.ack_count, 1u);
}
