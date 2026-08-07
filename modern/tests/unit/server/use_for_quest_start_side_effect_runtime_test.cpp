// use_for_quest_start_side_effect_runtime_test.cpp
//
// Verifies apply_use_for_quest_start_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEM_USE_FOR_QUESTSTART_SYN
// side-effect chain) walks the data-plane plan and dispatches the
// single entry to its subsystem: ACK echo on UseItem success / NACK
// with eItemUseErr_Quest on failure.

#include <mxh/server/use_for_quest_start_side_effect.hpp>
#include <mxh/server/use_for_quest_start_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using mxh::server::UseForQuestStartSideEffectKind;
using mxh::server::UseForQuestStartSideEffectSink;
using mxh::server::LEGACY_EITEMUSE_QUEST;
using mxh::server::apply_use_for_quest_start_side_effects;
using mxh::server::use_for_quest_start_side_effect_plan;

class RecordingSink final : public UseForQuestStartSideEffectSink {
public:
    std::string last_call;
    int last_rt = 0;
    int last_error_code = 0;
    std::uint16_t last_target_pos = 0;
    std::uint16_t last_item_idx = 0;
    std::size_t ack_count = 0;
    std::size_t nack_count = 0;

    void broadcast_use_ack(std::uint16_t target_pos,
                           std::uint16_t item_idx,
                           int original_rt) override {
        last_call = "ack";
        last_target_pos = target_pos;
        last_item_idx = item_idx;
        last_rt = original_rt;
        ++ack_count;
    }
    void broadcast_use_nack(std::uint16_t target_pos,
                            std::uint16_t item_idx,
                            int original_rt,
                            int error_code) override {
        last_call = "nack";
        last_target_pos = target_pos;
        last_item_idx = item_idx;
        last_rt = original_rt;
        last_error_code = error_code;
        ++nack_count;
    }
};

}  // namespace

TEST(ApplyUseForQuestStartSideEffects, SuccessRtEmitsAck) {
    auto plan = use_for_quest_start_side_effect_plan(
        /*use_rt=*/0, /*target_pos=*/10, /*item_idx=*/100);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              UseForQuestStartSideEffectKind::BroadcastUseAck);

    RecordingSink sink;
    auto out = apply_use_for_quest_start_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.acks_sent, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_TRUE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "ack");
    EXPECT_EQ(sink.last_target_pos, 10u);
    EXPECT_EQ(sink.last_item_idx, 100u);
    EXPECT_EQ(sink.last_rt, 0);
    EXPECT_EQ(sink.ack_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyUseForQuestStartSideEffects, FailureRtEmitsNackWithQuestEcode) {
    // Legacy: any non-zero rt -> MSG_ITEM_ERROR with
    // ECode = eItemUseErr_Quest (= 7).
    auto plan = use_for_quest_start_side_effect_plan(
        /*use_rt=*/3, /*target_pos=*/10, /*item_idx=*/100);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              UseForQuestStartSideEffectKind::BroadcastUseNack);
    EXPECT_EQ(plan.effects[0].error_code, LEGACY_EITEMUSE_QUEST);

    RecordingSink sink;
    auto out = apply_use_for_quest_start_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_TRUE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_rt, 3);
    EXPECT_EQ(sink.last_error_code, LEGACY_EITEMUSE_QUEST);
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 1u);
}

TEST(ApplyUseForQuestStartSideEffects, EmptyPlanIsNoOp) {
    mxh::server::UseForQuestStartSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_use_for_quest_start_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyUseForQuestStartSideEffects, VariousFailureCodesAllEmitNack) {
    for (int rt : {1, 7, 99, -1}) {
        auto plan = use_for_quest_start_side_effect_plan(rt, 1, 2);
        EXPECT_TRUE(plan.send_nack);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind,
                  UseForQuestStartSideEffectKind::BroadcastUseNack);
        EXPECT_EQ(plan.effects[0].error_code, LEGACY_EITEMUSE_QUEST);

        RecordingSink sink;
        (void)apply_use_for_quest_start_side_effects(plan, sink);
        EXPECT_EQ(sink.last_call, "nack");
        EXPECT_EQ(sink.last_rt, rt);
        EXPECT_EQ(sink.last_error_code, LEGACY_EITEMUSE_QUEST);
    }
}

TEST(ApplyUseForQuestStartSideEffects, NackDoesNotTouchAckState) {
    auto plan = use_for_quest_start_side_effect_plan(5, 1, 2);
    RecordingSink sink;
    (void)apply_use_for_quest_start_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 1u);
}

TEST(ApplyUseForQuestStartSideEffects, FieldPassthroughOnAck) {
    auto plan = use_for_quest_start_side_effect_plan(0, 21, 22);
    RecordingSink sink;
    (void)apply_use_for_quest_start_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "ack");
    EXPECT_EQ(sink.last_target_pos, 21u);
    EXPECT_EQ(sink.last_item_idx, 22u);
    EXPECT_EQ(sink.last_rt, 0);
    EXPECT_EQ(sink.nack_count, 0u);
}
