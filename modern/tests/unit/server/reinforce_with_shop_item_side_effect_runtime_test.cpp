// reinforce_with_shop_item_side_effect_runtime_test.cpp
//
// Verifies apply_reinforce_with_shop_item_side_effects() (the runtime
// orchestrator for the CItemManager::
// MP_ITEM_REINFORCE_WITHSHOPITEM_SYN side-effect chain) walks the
// data-plane plan and dispatches the entry: silent success (rt==0) /
// failed-ACK (rt==99) / NACK with the raw error code.

#include <mxh/server/reinforce_with_shop_item_side_effect.hpp>
#include <mxh/server/reinforce_with_shop_item_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::ReinforceWithShopItemSideEffectKind;
using mxh::server::ReinforceWithShopItemSideEffectSink;
using mxh::server::ReinforceWithShopItemValidationInput;
using mxh::server::apply_reinforce_with_shop_item_side_effects;
using mxh::server::reinforce_with_shop_item_side_effect_plan;

class RecordingSink final : public ReinforceWithShopItemSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_player_id = 0;
    int last_error_code = 0;
    std::size_t failed_ack_count = 0;
    std::size_t nack_count = 0;

    void send_failed_ack_to_player(std::uint32_t player_id) override {
        calls.push_back("failedack");
        last_player_id = player_id;
        ++failed_ack_count;
    }
    void send_nack_to_player(std::uint32_t player_id,
                             int nack_error_code) override {
        calls.push_back("nack");
        last_player_id = player_id;
        last_error_code = nack_error_code;
        ++nack_count;
    }
};

}  // namespace

TEST(ApplyReinforceWithShopItemSideEffects, SuccessIsSilent) {
    ReinforceWithShopItemValidationInput in;
    in.rt = 0;  // EI_TRUE
    auto plan = reinforce_with_shop_item_side_effect_plan(in, 7);
    EXPECT_FALSE(plan.send_failed_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_reinforce_with_shop_item_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.failed_acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_FALSE(out.failed_ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyReinforceWithShopItemSideEffects, FailedRt99EmitsFailedAck) {
    ReinforceWithShopItemValidationInput in;
    in.rt = 99;
    auto plan = reinforce_with_shop_item_side_effect_plan(in, 42);
    EXPECT_TRUE(plan.send_failed_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ReinforceWithShopItemSideEffectKind::SendFailedAckToPlayer);

    RecordingSink sink;
    auto out = apply_reinforce_with_shop_item_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.failed_acks_sent, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_TRUE(out.failed_ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"failedack"}));
    EXPECT_EQ(sink.last_player_id, 42u);
}

TEST(ApplyReinforceWithShopItemSideEffects, NackEmitsNackWithRt) {
    ReinforceWithShopItemValidationInput in;
    in.rt = 5;
    auto plan = reinforce_with_shop_item_side_effect_plan(in, 9);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.nack_error_code, 5);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ReinforceWithShopItemSideEffectKind::SendNackToPlayer);
    EXPECT_EQ(plan.effects[0].nack_error_code, 5);

    RecordingSink sink;
    auto out = apply_reinforce_with_shop_item_side_effects(plan, sink);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_EQ(out.failed_acks_sent, 0u);
    EXPECT_TRUE(out.nack_flag_consumed);
    EXPECT_FALSE(out.failed_ack_flag_consumed);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"nack"}));
    EXPECT_EQ(sink.last_player_id, 9u);
    EXPECT_EQ(sink.last_error_code, 5);
}

TEST(ApplyReinforceWithShopItemSideEffects, FailureCodeSweep) {
    for (int rt : {1, 2, 5, 100, -2, 1000}) {
        ReinforceWithShopItemValidationInput in;
        in.rt = rt;
        auto plan = reinforce_with_shop_item_side_effect_plan(in, 7);
        EXPECT_TRUE(plan.send_nack);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].nack_error_code, rt);

        RecordingSink sink;
        (void)apply_reinforce_with_shop_item_side_effects(plan, sink);
        EXPECT_EQ(sink.last_error_code, rt);
        EXPECT_EQ(sink.failed_ack_count, 0u);
    }
}

TEST(ApplyReinforceWithShopItemSideEffects, NackDoesNotTouchFailedAckState) {
    ReinforceWithShopItemValidationInput nack_in;
    nack_in.rt = 7;
    auto nack_plan = reinforce_with_shop_item_side_effect_plan(nack_in, 1);
    RecordingSink nack_sink;
    auto nack_out =
        apply_reinforce_with_shop_item_side_effects(nack_plan, nack_sink);
    EXPECT_EQ(nack_out.failed_acks_sent, 0u);
    EXPECT_EQ(nack_sink.failed_ack_count, 0u);
    EXPECT_EQ(nack_out.nacks_sent, 1u);

    ReinforceWithShopItemValidationInput ack_in;
    ack_in.rt = 99;
    auto ack_plan = reinforce_with_shop_item_side_effect_plan(ack_in, 2);
    RecordingSink ack_sink;
    auto ack_out =
        apply_reinforce_with_shop_item_side_effects(ack_plan, ack_sink);
    EXPECT_EQ(ack_out.nacks_sent, 0u);
    EXPECT_EQ(ack_sink.nack_count, 0u);
    EXPECT_EQ(ack_out.failed_acks_sent, 1u);
}

TEST(ApplyReinforceWithShopItemSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ReinforceWithShopItemSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_reinforce_with_shop_item_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.failed_acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_FALSE(out.failed_ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}
