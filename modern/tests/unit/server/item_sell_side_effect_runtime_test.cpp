// item_sell_side_effect_runtime_test.cpp
//
// Verifies apply_item_sell_side_effects() (the runtime orchestrator
// for the CItemManager::MP_ITEM_SELL_SYN side-effect chain) walks the
// data-plane plan and dispatches the single entry to its respective
// subsystem (ACK on SellItem success / NACK on failure or NPC-gate
// rejection).

#include <mxh/server/item_sell_side_effect.hpp>
#include <mxh/server/item_sell_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using mxh::server::ItemSellSideEffectKind;
using mxh::server::ItemSellSideEffectSink;
using mxh::server::LEGACY_NOT_EXIST;
using mxh::server::apply_item_sell_side_effects;
using mxh::server::item_sell_side_effect_plan;

class RecordingSink final : public ItemSellSideEffectSink {
public:
    std::string last_call;
    int last_rt = 0;
    int last_ecode = 0;
    std::uint16_t last_target_pos = 0;
    std::uint16_t last_item_idx = 0;
    std::uint16_t last_item_num = 0;
    std::uint16_t last_dealer_idx = 0;
    std::size_t ack_count = 0;
    std::size_t nack_count = 0;

    void broadcast_sell_ack(std::uint16_t target_pos,
                            std::uint16_t item_idx,
                            std::uint16_t item_num,
                            std::uint16_t dealer_idx,
                            int original_rt) override {
        last_call = "ack";
        last_target_pos = target_pos;
        last_item_idx = item_idx;
        last_item_num = item_num;
        last_dealer_idx = dealer_idx;
        last_rt = original_rt;
        ++ack_count;
    }
    void broadcast_sell_nack(std::uint16_t target_pos,
                             std::uint16_t item_idx,
                             std::uint16_t item_num,
                             std::uint16_t dealer_idx,
                             int original_rt,
                             int ecode) override {
        last_call = "nack";
        last_target_pos = target_pos;
        last_item_idx = item_idx;
        last_item_num = item_num;
        last_dealer_idx = dealer_idx;
        last_rt = original_rt;
        last_ecode = ecode;
        ++nack_count;
    }
};

}  // namespace

TEST(ApplyItemSellSideEffects, SuccessRtEmitsAck) {
    auto plan = item_sell_side_effect_plan(
        /*sell_rt=*/0, /*npc_gate_ok=*/true,
        /*target_pos=*/10, /*item_idx=*/100, /*item_num=*/1,
        /*dealer_idx=*/50);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemSellSideEffectKind::BroadcastSellAck);

    RecordingSink sink;
    auto out = apply_item_sell_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.acks_sent, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_TRUE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "ack");
    EXPECT_EQ(sink.last_target_pos, 10u);
    EXPECT_EQ(sink.last_item_idx, 100u);
    EXPECT_EQ(sink.last_item_num, 1u);
    EXPECT_EQ(sink.last_dealer_idx, 50u);
    EXPECT_EQ(sink.last_rt, 0);
    EXPECT_EQ(sink.ack_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyItemSellSideEffects, FailureRtEmitsNackWithRt) {
    auto plan = item_sell_side_effect_plan(
        /*sell_rt=*/LEGACY_NOT_EXIST, /*npc_gate_ok=*/true,
        /*target_pos=*/10, /*item_idx=*/100, /*item_num=*/2,
        /*dealer_idx=*/50);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemSellSideEffectKind::BroadcastSellNack);

    RecordingSink sink;
    auto out = apply_item_sell_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_TRUE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_rt, LEGACY_NOT_EXIST);
    EXPECT_EQ(sink.last_ecode, LEGACY_NOT_EXIST);
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 1u);
}

TEST(ApplyItemSellSideEffects, NpcGateFailureEmitsNackWithNotExistCode) {
    // Legacy: CheckHackNpc failure short-circuits before SellItem, so
    // the NACK carries ECode = NOT_EXIST and original_rt = -1.
    auto plan = item_sell_side_effect_plan(
        /*sell_rt=*/0, /*npc_gate_ok=*/false,
        /*target_pos=*/10, /*item_idx=*/100, /*item_num=*/1,
        /*dealer_idx=*/50);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemSellSideEffectKind::BroadcastSellNack);

    RecordingSink sink;
    auto out = apply_item_sell_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_rt, -1);
    EXPECT_EQ(sink.last_ecode, mxh::server::LEGACY_NOT_EXIST);
    EXPECT_EQ(sink.nack_count, 1u);
}

TEST(ApplyItemSellSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ItemSellSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_item_sell_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyItemSellSideEffects, VariousFailureCodesAllEmitNack) {
    // Legacy: any non-zero SellItem return code emits NACK with the
    // same code, independent of the payload fields.
    for (int rt : {1, 2, 5, 99, 101, 108, 1000, -2}) {
        auto plan = item_sell_side_effect_plan(
            rt, /*npc_gate_ok=*/true,
            /*target_pos=*/1, /*item_idx=*/2, /*item_num=*/3,
            /*dealer_idx=*/4);
        EXPECT_TRUE(plan.send_nack);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind,
                  ItemSellSideEffectKind::BroadcastSellNack);
        EXPECT_EQ(plan.effects[0].ecode, rt);
        EXPECT_EQ(plan.effects[0].original_rt, rt);

        RecordingSink sink;
        (void)apply_item_sell_side_effects(plan, sink);
        EXPECT_EQ(sink.last_call, "nack");
        EXPECT_EQ(sink.last_rt, rt);
        EXPECT_EQ(sink.last_ecode, rt);
    }
}

TEST(ApplyItemSellSideEffects, AckDoesNotTouchNackState) {
    auto plan = item_sell_side_effect_plan(0, true, 7, 8, 9, 10);
    RecordingSink sink;
    (void)apply_item_sell_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "ack");
    EXPECT_EQ(sink.last_rt, 0);
    EXPECT_EQ(sink.last_ecode, 0);  // untouched on the ACK path
    EXPECT_EQ(sink.ack_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyItemSellSideEffects, FieldPassthroughOnNack) {
    auto plan = item_sell_side_effect_plan(55, true, 21, 22, 23, 24);
    RecordingSink sink;
    (void)apply_item_sell_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_target_pos, 21u);
    EXPECT_EQ(sink.last_item_idx, 22u);
    EXPECT_EQ(sink.last_item_num, 23u);
    EXPECT_EQ(sink.last_dealer_idx, 24u);
    EXPECT_EQ(sink.last_rt, 55);
    EXPECT_EQ(sink.last_ecode, 55);
}
