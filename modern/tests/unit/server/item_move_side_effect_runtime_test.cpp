// item_move_side_effect_runtime_test.cpp
//
// Verifies apply_item_move_side_effects() (the runtime orchestrator
// for the CItemManager::MP_ITEM_MOVE_SYN side-effect chain) walks the
// data-plane plan and dispatches the single entry to its respective
// subsystem (ACK on MoveItem success / NACK with the fixed move error
// code on failure / silent skip on the legacy rt==99 sentinel).

#include <mxh/server/item_move_side_effect.hpp>
#include <mxh/server/item_move_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using mxh::server::ItemMoveSideEffectKind;
using mxh::server::ItemMoveSideEffectSink;
using mxh::server::LEGACY_EI_EXISTED;
using mxh::server::LEGACY_EI_LOCKED;
using mxh::server::LEGACY_EI_MAXMONEY;
using mxh::server::LEGACY_EI_NOTENOUGHMONEY;
using mxh::server::LEGACY_EI_NOTEXIST;
using mxh::server::LEGACY_EI_NOSPACE;
using mxh::server::LEGACY_EI_NOTEQUALDATA;
using mxh::server::LEGACY_EI_OUTOFPOS;
using mxh::server::LEGACY_EI_PASSWD;
using mxh::server::LEGACY_EITEMUSE_MOVE;
using mxh::server::LEGACY_MOVE_RT_SILENT;
using mxh::server::apply_item_move_side_effects;
using mxh::server::item_move_side_effect_plan;

class RecordingSink final : public ItemMoveSideEffectSink {
public:
    std::string last_call;
    int last_rt = 0;
    int last_ecode = 0;
    std::uint16_t last_from_item_idx = 0;
    std::uint16_t last_from_pos = 0;
    std::uint16_t last_to_item_idx = 0;
    std::uint16_t last_to_pos = 0;
    std::size_t ack_count = 0;
    std::size_t nack_count = 0;
    std::size_t silent_count = 0;

    void broadcast_move_ack(std::uint16_t from_item_idx,
                            std::uint16_t from_pos,
                            std::uint16_t to_item_idx,
                            std::uint16_t to_pos,
                            int original_rt) override {
        last_call = "ack";
        last_from_item_idx = from_item_idx;
        last_from_pos = from_pos;
        last_to_item_idx = to_item_idx;
        last_to_pos = to_pos;
        last_rt = original_rt;
        ++ack_count;
    }
    void broadcast_move_nack(std::uint16_t from_item_idx,
                             std::uint16_t from_pos,
                             std::uint16_t to_item_idx,
                             std::uint16_t to_pos,
                             int original_rt,
                             int ecode) override {
        last_call = "nack";
        last_from_item_idx = from_item_idx;
        last_from_pos = from_pos;
        last_to_item_idx = to_item_idx;
        last_to_pos = to_pos;
        last_rt = original_rt;
        last_ecode = ecode;
        ++nack_count;
    }
    void silent_skip() override {
        last_call = "silent";
        ++silent_count;
    }
};

}  // namespace

TEST(ApplyItemMoveSideEffects, SuccessRtEmitsAck) {
    auto plan = item_move_side_effect_plan(
        /*move_rt=*/0, /*from_item_idx=*/10, /*from_pos=*/20,
        /*to_item_idx=*/30, /*to_pos=*/40);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_FALSE(plan.silent);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemMoveSideEffectKind::BroadcastMoveAck);

    RecordingSink sink;
    auto out = apply_item_move_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.acks_sent, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.silent_skips, 0u);
    EXPECT_TRUE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_FALSE(out.silent_flag_consumed);
    EXPECT_EQ(sink.last_call, "ack");
    EXPECT_EQ(sink.last_from_item_idx, 10u);
    EXPECT_EQ(sink.last_from_pos, 20u);
    EXPECT_EQ(sink.last_to_item_idx, 30u);
    EXPECT_EQ(sink.last_to_pos, 40u);
    EXPECT_EQ(sink.last_rt, 0);
    EXPECT_EQ(sink.ack_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
    EXPECT_EQ(sink.silent_count, 0u);
}

TEST(ApplyItemMoveSideEffects, FailureRtEmitsNackWithFixedMoveEcode) {
    // Legacy: any non-zero non-99 rt emits NACK with ECode =
    // eItemUseErr_Move (= 2) and the original rt as aux code.
    auto plan = item_move_side_effect_plan(
        /*move_rt=*/LEGACY_EI_OUTOFPOS, /*from_item_idx=*/10,
        /*from_pos=*/20, /*to_item_idx=*/30, /*to_pos=*/40);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_FALSE(plan.silent);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemMoveSideEffectKind::BroadcastMoveNack);

    RecordingSink sink;
    auto out = apply_item_move_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_EQ(out.silent_skips, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_TRUE(out.nack_flag_consumed);
    EXPECT_FALSE(out.silent_flag_consumed);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_rt, LEGACY_EI_OUTOFPOS);
    EXPECT_EQ(sink.last_ecode, LEGACY_EITEMUSE_MOVE);
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 1u);
}

TEST(ApplyItemMoveSideEffects, SilentRt99ReportsSkipNoWire) {
    // Legacy: rt == 99 suppresses both ACK and NACK entirely.
    auto plan = item_move_side_effect_plan(
        /*move_rt=*/LEGACY_MOVE_RT_SILENT, /*from_item_idx=*/10,
        /*from_pos=*/20, /*to_item_idx=*/30, /*to_pos=*/40);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.silent);
    // Data plane locks: silent branch produces an empty plan.
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_item_move_side_effects(plan, sink);
    // Silent branch carries no effect entries; the callback is driven
    // by the plan.silent flag (1:1 with the empty-plan data plane).
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.silent_skips, 1u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_TRUE(out.silent_flag_consumed);
    EXPECT_EQ(sink.last_call, "silent");
    EXPECT_EQ(sink.silent_count, 1u);
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyItemMoveSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ItemMoveSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_item_move_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.silent_skips, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_FALSE(out.silent_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 0u);
    EXPECT_EQ(sink.silent_count, 0u);
}

TEST(ApplyItemMoveSideEffects, VariousFailureCodesAllEmitNack) {
    // Every ERROR_ITEM value except 0 and 99 maps to the fixed move
    // NACK with the original code preserved as aux.
    for (int rt : {LEGACY_EI_OUTOFPOS, LEGACY_EI_NOTEQUALDATA,
                   LEGACY_EI_EXISTED, LEGACY_EI_NOTEXIST,
                   LEGACY_EI_LOCKED, LEGACY_EI_PASSWD,
                   LEGACY_EI_NOTENOUGHMONEY, LEGACY_EI_NOSPACE,
                   LEGACY_EI_MAXMONEY, 100, -1}) {
        auto plan = item_move_side_effect_plan(
            rt, 1, 2, 3, 4);
        EXPECT_TRUE(plan.send_nack);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind,
                  ItemMoveSideEffectKind::BroadcastMoveNack);
        EXPECT_EQ(plan.effects[0].original_rt, rt);

        RecordingSink sink;
        (void)apply_item_move_side_effects(plan, sink);
        EXPECT_EQ(sink.last_call, "nack");
        EXPECT_EQ(sink.last_rt, rt);
        EXPECT_EQ(sink.last_ecode, LEGACY_EITEMUSE_MOVE);
    }
}

TEST(ApplyItemMoveSideEffects, NackDoesNotTouchAckOrSilentState) {
    auto plan = item_move_side_effect_plan(5, 1, 2, 3, 4);
    RecordingSink sink;
    (void)apply_item_move_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_ecode, LEGACY_EITEMUSE_MOVE);
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 1u);
    EXPECT_EQ(sink.silent_count, 0u);
}

TEST(ApplyItemMoveSideEffects, FieldPassthroughOnAck) {
    auto plan = item_move_side_effect_plan(0, 11, 12, 13, 14);
    RecordingSink sink;
    (void)apply_item_move_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "ack");
    EXPECT_EQ(sink.last_from_item_idx, 11u);
    EXPECT_EQ(sink.last_from_pos, 12u);
    EXPECT_EQ(sink.last_to_item_idx, 13u);
    EXPECT_EQ(sink.last_to_pos, 14u);
    EXPECT_EQ(sink.last_rt, 0);
    EXPECT_EQ(sink.silent_count, 0u);
}
