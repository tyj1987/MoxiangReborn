// item_discard_side_effect_runtime_test.cpp
//
// Verifies apply_item_discard_side_effects() (the runtime orchestrator
// for the CItemManager::MP_ITEM_DISCARD_SYN side-effect chain) walks
// the data-plane plan and dispatches each entry to its respective
// subsystem: ACK + money-log on success (legacy order), discard-NACK
// on failure, error-NACK for looted players.

#include <mxh/server/item_discard_side_effect.hpp>
#include <mxh/server/item_discard_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::ItemDiscardSideEffectKind;
using mxh::server::ItemDiscardSideEffectSink;
using mxh::server::LEGACY_DISCARD_RT_LOOTED;
using mxh::server::LEGACY_EITEMUSE_DISCARD;
using mxh::server::LEGACY_ELOG_ITEM_DISCARD;
using mxh::server::apply_item_discard_side_effects;
using mxh::server::item_discard_side_effect_plan;

class RecordingSink final : public ItemDiscardSideEffectSink {
public:
    std::vector<std::string> calls;
    int last_rt = 0;
    int last_ecode = 0;
    std::uint32_t last_log_code = 0;
    std::uint16_t last_target_pos = 0;
    std::uint16_t last_item_idx = 0;
    std::uint16_t last_item_num = 0;

    void broadcast_discard_ack(std::uint16_t target_pos,
                               std::uint16_t item_idx,
                               std::uint16_t item_num,
                               int original_rt) override {
        calls.push_back("ack");
        last_target_pos = target_pos;
        last_item_idx = item_idx;
        last_item_num = item_num;
        last_rt = original_rt;
    }
    void log_discarded_item(std::uint16_t target_pos,
                            std::uint16_t item_idx,
                            std::uint16_t item_num,
                            std::uint32_t log_code) override {
        calls.push_back("log");
        last_target_pos = target_pos;
        last_item_idx = item_idx;
        last_item_num = item_num;
        last_log_code = log_code;
    }
    void broadcast_discard_nack(std::uint16_t target_pos,
                                std::uint16_t item_idx,
                                std::uint16_t item_num,
                                int original_rt,
                                int ecode) override {
        calls.push_back("nack");
        last_target_pos = target_pos;
        last_item_idx = item_idx;
        last_item_num = item_num;
        last_rt = original_rt;
        last_ecode = ecode;
    }
    void broadcast_error_nack(std::uint16_t target_pos,
                              std::uint16_t item_idx,
                              std::uint16_t item_num,
                              int original_rt,
                              int ecode) override {
        calls.push_back("error_nack");
        last_target_pos = target_pos;
        last_item_idx = item_idx;
        last_item_num = item_num;
        last_rt = original_rt;
        last_ecode = ecode;
    }
};

}  // namespace

TEST(ApplyItemDiscardSideEffects, SuccessEmitsAckThenLogInLegacyOrder) {
    // Legacy: MP_ITEM_DISCARD_SYN success echoes the ACK first, then
    // LogItemMoney with eLog_ItemDiscard.
    auto plan = item_discard_side_effect_plan(
        /*discard_rt=*/0, /*is_looted=*/false,
        /*target_pos=*/10, /*item_idx=*/100, /*item_num=*/1);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_FALSE(plan.send_error_nack);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemDiscardSideEffectKind::BroadcastDiscardAck);
    EXPECT_EQ(plan.effects[1].kind,
              ItemDiscardSideEffectKind::LogDiscardedItem);

    RecordingSink sink;
    auto out = apply_item_discard_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 2u);
    EXPECT_EQ(out.acks_sent, 1u);
    EXPECT_EQ(out.logs_sent, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.error_nacks_sent, 0u);
    EXPECT_TRUE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_FALSE(out.error_nack_flag_consumed);
    ASSERT_EQ(sink.calls.size(), 2u);
    EXPECT_EQ(sink.calls[0], "ack");
    EXPECT_EQ(sink.calls[1], "log");
    EXPECT_EQ(sink.last_target_pos, 10u);
    EXPECT_EQ(sink.last_item_idx, 100u);
    EXPECT_EQ(sink.last_item_num, 1u);
    EXPECT_EQ(sink.last_log_code, LEGACY_ELOG_ITEM_DISCARD);
    EXPECT_EQ(sink.last_rt, 0);
}

TEST(ApplyItemDiscardSideEffects, FailureRtEmitsDiscardNackWithRt) {
    // Legacy: DiscardItem non-zero -> MSG_ITEM_DISCARD_NACK with
    // ECode = the return code.
    auto plan = item_discard_side_effect_plan(
        /*discard_rt=*/7, /*is_looted=*/false,
        /*target_pos=*/10, /*item_idx=*/100, /*item_num=*/1);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_FALSE(plan.send_error_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemDiscardSideEffectKind::BroadcastDiscardNack);

    RecordingSink sink;
    auto out = apply_item_discard_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.logs_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_EQ(out.error_nacks_sent, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_TRUE(out.nack_flag_consumed);
    EXPECT_FALSE(out.error_nack_flag_consumed);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"nack"}));
    EXPECT_EQ(sink.last_rt, 7);
    EXPECT_EQ(sink.last_ecode, 7);
}

TEST(ApplyItemDiscardSideEffects, LootedPlayerEmitsErrorNack) {
    // Legacy: IsLootedPlayer -> MSG_ITEM_ERROR_NACK with
    // ECode = eItemUseErr_Discard and looted rt (= 10) as aux.
    auto plan = item_discard_side_effect_plan(
        /*discard_rt=*/0, /*is_looted=*/true,
        /*target_pos=*/10, /*item_idx=*/100, /*item_num=*/1);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.send_error_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemDiscardSideEffectKind::BroadcastErrorNack);

    RecordingSink sink;
    auto out = apply_item_discard_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.logs_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.error_nacks_sent, 1u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_TRUE(out.error_nack_flag_consumed);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>({"error_nack"}));
    EXPECT_EQ(sink.last_rt, LEGACY_DISCARD_RT_LOOTED);
    EXPECT_EQ(sink.last_ecode, LEGACY_EITEMUSE_DISCARD);
}

TEST(ApplyItemDiscardSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ItemDiscardSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_item_discard_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.logs_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.error_nacks_sent, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_FALSE(out.error_nack_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyItemDiscardSideEffects, VariousFailureCodesAllEmitDiscardNack) {
    // Every non-zero rt (with looted=false) maps to the discard NACK
    // with the same code; the looted sentinel is only special when the
    // looted flag is set.
    for (int rt : {1, 5, 10, 99, -1}) {
        auto plan = item_discard_side_effect_plan(
            rt, /*is_looted=*/false, 1, 2, 3);
        EXPECT_TRUE(plan.send_nack);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind,
                  ItemDiscardSideEffectKind::BroadcastDiscardNack);
        EXPECT_EQ(plan.effects[0].ecode, rt);

        RecordingSink sink;
        (void)apply_item_discard_side_effects(plan, sink);
        EXPECT_EQ(sink.calls,
                  std::vector<std::string>({"nack"}));
        EXPECT_EQ(sink.last_rt, rt);
        EXPECT_EQ(sink.last_ecode, rt);
    }
}

TEST(ApplyItemDiscardSideEffects, LootedFlagOverridesZeroRt) {
    // classify_item_discard_outcome: is_looted wins over rt==0.
    auto plan = item_discard_side_effect_plan(
        /*discard_rt=*/0, /*is_looted=*/true, 4, 5, 6);
    EXPECT_TRUE(plan.send_error_nack);
    EXPECT_FALSE(plan.send_ack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemDiscardSideEffectKind::BroadcastErrorNack);
    EXPECT_EQ(plan.effects[0].ecode, LEGACY_EITEMUSE_DISCARD);

    RecordingSink sink;
    (void)apply_item_discard_side_effects(plan, sink);
    EXPECT_EQ(sink.last_target_pos, 4u);
    EXPECT_EQ(sink.last_item_idx, 5u);
    EXPECT_EQ(sink.last_item_num, 6u);
    EXPECT_EQ(sink.last_ecode, LEGACY_EITEMUSE_DISCARD);
}
