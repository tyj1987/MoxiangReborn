// item_mix_side_effect_runtime_test.cpp
//
// Verifies apply_item_mix_side_effects() (the runtime orchestrator
// for the CItemManager::MP_ITEM_MIX_SYN side-effect chain) walks the
// data-plane plan and dispatches the single entry to its respective
// subsystem: success / big-fail / fail ACKs, the DWORD2 mix message,
// or the error NACK.

#include <mxh/server/item_mix_side_effect.hpp>
#include <mxh/server/item_mix_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using mxh::server::ItemMixSideEffectKind;
using mxh::server::ItemMixSideEffectSink;
using mxh::server::LEGACY_MIX_RT_BIGFAIL;
using mxh::server::LEGACY_MIX_RT_FAIL;
using mxh::server::LEGACY_MIX_RT_ASSERT;
using mxh::server::apply_item_mix_side_effects;
using mxh::server::item_mix_side_effect_plan;

class RecordingSink final : public ItemMixSideEffectSink {
public:
    std::string last_call;
    int last_rt = 0;
    int last_ecode = 0;
    std::uint16_t last_basic_item_idx = 0;
    std::uint16_t last_basic_item_pos = 0;
    std::uint16_t last_result_index = 0;
    std::uint16_t last_material_num = 0;
    std::uint16_t last_shop_item_idx = 0;
    std::uint16_t last_shop_item_pos = 0;

    void record_common(std::uint16_t basic_item_idx,
                       std::uint16_t basic_item_pos,
                       std::uint16_t result_index,
                       std::uint16_t material_num,
                       std::uint16_t shop_item_idx,
                       std::uint16_t shop_item_pos,
                       int original_rt) {
        last_basic_item_idx = basic_item_idx;
        last_basic_item_pos = basic_item_pos;
        last_result_index = result_index;
        last_material_num = material_num;
        last_shop_item_idx = shop_item_idx;
        last_shop_item_pos = shop_item_pos;
        last_rt = original_rt;
    }
    void broadcast_success_ack(
        std::uint16_t basic_item_idx, std::uint16_t basic_item_pos,
        std::uint16_t result_index, std::uint16_t material_num,
        std::uint16_t shop_item_idx, std::uint16_t shop_item_pos,
        int original_rt) override {
        last_call = "success_ack";
        record_common(basic_item_idx, basic_item_pos, result_index,
                      material_num, shop_item_idx, shop_item_pos,
                      original_rt);
    }
    void broadcast_big_fail_ack(
        std::uint16_t basic_item_idx, std::uint16_t basic_item_pos,
        std::uint16_t result_index, std::uint16_t material_num,
        std::uint16_t shop_item_idx, std::uint16_t shop_item_pos,
        int original_rt) override {
        last_call = "big_fail_ack";
        record_common(basic_item_idx, basic_item_pos, result_index,
                      material_num, shop_item_idx, shop_item_pos,
                      original_rt);
    }
    void broadcast_fail_ack(
        std::uint16_t basic_item_idx, std::uint16_t basic_item_pos,
        std::uint16_t result_index, std::uint16_t material_num,
        std::uint16_t shop_item_idx, std::uint16_t shop_item_pos,
        int original_rt) override {
        last_call = "fail_ack";
        record_common(basic_item_idx, basic_item_pos, result_index,
                      material_num, shop_item_idx, shop_item_pos,
                      original_rt);
    }
    void broadcast_mix_msg(std::uint16_t basic_item_pos,
                           int original_rt) override {
        last_call = "mix_msg";
        last_basic_item_pos = basic_item_pos;
        last_rt = original_rt;
    }
    void broadcast_error_nack(
        std::uint16_t basic_item_idx, std::uint16_t basic_item_pos,
        std::uint16_t result_index, std::uint16_t material_num,
        std::uint16_t shop_item_idx, std::uint16_t shop_item_pos,
        int original_rt, int ecode) override {
        last_call = "error_nack";
        record_common(basic_item_idx, basic_item_pos, result_index,
                      material_num, shop_item_idx, shop_item_pos,
                      original_rt);
        last_ecode = ecode;
    }
};

}  // namespace

TEST(ApplyItemMixSideEffects, SuccessRtEmitsSuccessAck) {
    auto plan = item_mix_side_effect_plan(
        /*mix_rt=*/0, 1, 2, 3, 4, 5, 6);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_msg);
    EXPECT_FALSE(plan.send_error_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemMixSideEffectKind::BroadcastSuccessAck);

    RecordingSink sink;
    auto out = apply_item_mix_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.success_acks, 1u);
    EXPECT_EQ(out.big_fail_acks, 0u);
    EXPECT_EQ(out.fail_acks, 0u);
    EXPECT_EQ(out.msgs_sent, 0u);
    EXPECT_EQ(out.error_nacks_sent, 0u);
    EXPECT_TRUE(out.ack_flag_consumed);
    EXPECT_FALSE(out.msg_flag_consumed);
    EXPECT_FALSE(out.error_nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "success_ack");
    EXPECT_EQ(sink.last_basic_item_idx, 1u);
    EXPECT_EQ(sink.last_basic_item_pos, 2u);
    EXPECT_EQ(sink.last_result_index, 3u);
    EXPECT_EQ(sink.last_material_num, 4u);
    EXPECT_EQ(sink.last_shop_item_idx, 5u);
    EXPECT_EQ(sink.last_shop_item_pos, 6u);
    EXPECT_EQ(sink.last_rt, 0);
}

TEST(ApplyItemMixSideEffects, BigFailRtEmitsBigFailAck) {
    auto plan = item_mix_side_effect_plan(
        /*mix_rt=*/LEGACY_MIX_RT_BIGFAIL, 1, 2, 3, 4, 5, 6);
    EXPECT_TRUE(plan.send_ack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemMixSideEffectKind::BroadcastBigFailAck);

    RecordingSink sink;
    auto out = apply_item_mix_side_effects(plan, sink);
    EXPECT_EQ(out.big_fail_acks, 1u);
    EXPECT_EQ(out.success_acks, 0u);
    EXPECT_TRUE(out.ack_flag_consumed);
    EXPECT_EQ(sink.last_call, "big_fail_ack");
    EXPECT_EQ(sink.last_rt, LEGACY_MIX_RT_BIGFAIL);
}

TEST(ApplyItemMixSideEffects, FailRtEmitsFailAck) {
    auto plan = item_mix_side_effect_plan(
        /*mix_rt=*/LEGACY_MIX_RT_FAIL, 1, 2, 3, 4, 5, 6);
    EXPECT_TRUE(plan.send_ack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemMixSideEffectKind::BroadcastFailAck);

    RecordingSink sink;
    auto out = apply_item_mix_side_effects(plan, sink);
    EXPECT_EQ(out.fail_acks, 1u);
    EXPECT_TRUE(out.ack_flag_consumed);
    EXPECT_EQ(sink.last_call, "fail_ack");
    EXPECT_EQ(sink.last_rt, LEGACY_MIX_RT_FAIL);
}

TEST(ApplyItemMixSideEffects, MsgRtEmitsDword2WithBasicItemPos) {
    // Legacy: rt in {20..23} -> MSG_DWORD2(MP_ITEM_MIX_MSG,
    // dwData1=rt, dwData2=BasicItemPos).
    for (int rt : {20, 21, 22, 23}) {
        auto plan = item_mix_side_effect_plan(
            rt, 1, 77, 3, 4, 5, 6);
        EXPECT_FALSE(plan.send_ack);
        EXPECT_TRUE(plan.send_msg);
        EXPECT_FALSE(plan.send_error_nack);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind,
                  ItemMixSideEffectKind::BroadcastMixMsg);

        RecordingSink sink;
        auto out = apply_item_mix_side_effects(plan, sink);
        EXPECT_EQ(out.effects_applied, 1u);
        EXPECT_EQ(out.msgs_sent, 1u);
        EXPECT_TRUE(out.msg_flag_consumed);
        EXPECT_FALSE(out.ack_flag_consumed);
        EXPECT_FALSE(out.error_nack_flag_consumed);
        EXPECT_EQ(sink.last_call, "mix_msg");
        EXPECT_EQ(sink.last_basic_item_pos, 77u);
        EXPECT_EQ(sink.last_rt, rt);
    }
}

TEST(ApplyItemMixSideEffects, ErrorNackRtEmitsErrorNack) {
    auto plan = item_mix_side_effect_plan(
        /*mix_rt=*/LEGACY_MIX_RT_ASSERT, 1, 2, 3, 4, 5, 6);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_FALSE(plan.send_msg);
    EXPECT_TRUE(plan.send_error_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemMixSideEffectKind::BroadcastErrorNack);
    EXPECT_EQ(plan.effects[0].ecode, LEGACY_MIX_RT_ASSERT);

    RecordingSink sink;
    auto out = apply_item_mix_side_effects(plan, sink);
    EXPECT_EQ(out.error_nacks_sent, 1u);
    EXPECT_TRUE(out.error_nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "error_nack");
    EXPECT_EQ(sink.last_rt, LEGACY_MIX_RT_ASSERT);
    EXPECT_EQ(sink.last_ecode, LEGACY_MIX_RT_ASSERT);
}

TEST(ApplyItemMixSideEffects, VariousOtherRtsEmitErrorNack) {
    for (int rt : {1, 3, 19, 24, 999, 1002, -1}) {
        auto plan = item_mix_side_effect_plan(
            rt, 1, 2, 3, 4, 5, 6);
        EXPECT_TRUE(plan.send_error_nack);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind,
                  ItemMixSideEffectKind::BroadcastErrorNack);
        EXPECT_EQ(plan.effects[0].ecode, rt);

        RecordingSink sink;
        (void)apply_item_mix_side_effects(plan, sink);
        EXPECT_EQ(sink.last_call, "error_nack");
        EXPECT_EQ(sink.last_rt, rt);
        EXPECT_EQ(sink.last_ecode, rt);
    }
}

TEST(ApplyItemMixSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ItemMixSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_item_mix_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.success_acks, 0u);
    EXPECT_EQ(out.big_fail_acks, 0u);
    EXPECT_EQ(out.fail_acks, 0u);
    EXPECT_EQ(out.msgs_sent, 0u);
    EXPECT_EQ(out.error_nacks_sent, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.msg_flag_consumed);
    EXPECT_FALSE(out.error_nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
}

TEST(ApplyItemMixSideEffects, FieldPassthroughOnErrorNack) {
    auto plan = item_mix_side_effect_plan(9, 11, 12, 13, 14, 15, 16);
    RecordingSink sink;
    (void)apply_item_mix_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "error_nack");
    EXPECT_EQ(sink.last_basic_item_idx, 11u);
    EXPECT_EQ(sink.last_basic_item_pos, 12u);
    EXPECT_EQ(sink.last_result_index, 13u);
    EXPECT_EQ(sink.last_material_num, 14u);
    EXPECT_EQ(sink.last_shop_item_idx, 15u);
    EXPECT_EQ(sink.last_shop_item_pos, 16u);
    EXPECT_EQ(sink.last_rt, 9);
    EXPECT_EQ(sink.last_ecode, 9);
}
