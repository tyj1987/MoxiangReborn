// curse_cancellation_side_effect_runtime_test.cpp
//
// Verifies apply_curse_cancellation_side_effects() (the runtime
// orchestrator for the CItemManager::
// MP_ITEMEXT_SHOPITEM_CURSE_CANCELLATION_SYN side-effect chain) walks
// the data-plane plan and dispatches each entry: the 7-step full
// cancel chain / 6-step no-space chain / 3-way gate NACK.

#include <mxh/server/curse_cancellation_side_effect.hpp>
#include <mxh/server/curse_cancellation_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::CurseCancellationSideEffectKind;
using mxh::server::CurseCancellationSideEffectSink;
using mxh::server::CurseCancellationValidationInput;
using mxh::server::apply_curse_cancellation_side_effects;
using mxh::server::curse_cancellation_side_effect_plan;

class RecordingSink final : public CurseCancellationSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_player_id = 0;
    std::uint32_t last_nack_code = 0;
    std::uint16_t last_shop_item_idx = 0;
    std::uint16_t last_shop_item_pos = 0;
    std::uint32_t last_curse_count = 0;
    std::size_t nack_count = 0;
    std::size_t obtain_count = 0;

    void send_nack_to_player(std::uint32_t player_id,
                             std::uint32_t nack_code) override {
        calls.push_back("nack");
        last_player_id = player_id;
        last_nack_code = nack_code;
        ++nack_count;
    }
    void send_use_ack_to_player(std::uint32_t player_id,
                                std::uint16_t shop_item_idx,
                                std::uint16_t shop_item_pos) override {
        calls.push_back("useack");
        last_player_id = player_id;
        last_shop_item_idx = shop_item_idx;
        last_shop_item_pos = shop_item_pos;
    }
    void discard_shop_item(std::uint32_t player_id,
                           std::uint16_t shop_item_idx,
                           std::uint16_t shop_item_pos) override {
        calls.push_back("discardshop");
        last_player_id = player_id;
        last_shop_item_idx = shop_item_idx;
        last_shop_item_pos = shop_item_pos;
    }
    void log_item_money_use(std::uint32_t player_id) override {
        calls.push_back("loguse");
        last_player_id = player_id;
    }
    void discard_cursed_item(std::uint32_t player_id) override {
        calls.push_back("discardcursed");
        last_player_id = player_id;
    }
    void send_delete_item_ack(std::uint32_t player_id) override {
        calls.push_back("deleteack");
        last_player_id = player_id;
    }
    void log_item_money_discard(std::uint32_t player_id) override {
        calls.push_back("logdisc");
        last_player_id = player_id;
    }
    void obtain_item_ex(std::uint32_t player_id,
                        std::uint32_t curse_cancellation_count) override {
        calls.push_back("obtain");
        last_player_id = player_id;
        last_curse_count = curse_cancellation_count;
        ++obtain_count;
    }
};

CurseCancellationValidationInput PassingGates() {
    CurseCancellationValidationInput in;
    in.item_exists_at_target = true;
    in.unique_item_info_exists = true;
    in.unique_item_is_cursed = true;
    in.discard_shop_returned_true = true;
    in.obtain_space_available = true;
    return in;
}

}  // namespace

TEST(ApplyCurseCancellationSideEffects, FullCancelEmitsSevenStepChainInOrder) {
    auto in = PassingGates();
    auto plan = curse_cancellation_side_effect_plan(
        in, /*player_id=*/0x00120013u,
        /*shop_item_idx=*/400, /*shop_item_pos=*/6,
        /*curse_cancellation_count=*/3);
    EXPECT_TRUE(plan.send_use_ack);
    EXPECT_TRUE(plan.discard_shop);
    EXPECT_TRUE(plan.log_use);
    EXPECT_TRUE(plan.discard_cursed);
    EXPECT_TRUE(plan.send_delete_ack);
    EXPECT_TRUE(plan.log_discard);
    EXPECT_TRUE(plan.obtain_ex);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 7u);
    const CurseCancellationSideEffectKind expected[] = {
        CurseCancellationSideEffectKind::DiscardShopItem,
        CurseCancellationSideEffectKind::LogItemMoneyUse,
        CurseCancellationSideEffectKind::SendUseAckToPlayer,
        CurseCancellationSideEffectKind::DiscardCursedItem,
        CurseCancellationSideEffectKind::SendDeleteItemAck,
        CurseCancellationSideEffectKind::LogItemMoneyDiscard,
        CurseCancellationSideEffectKind::ObtainItemEx,
    };
    for (std::size_t i = 0u; i < 7u; ++i) {
        EXPECT_EQ(plan.effects[i].kind, expected[i]);
    }
    EXPECT_EQ(plan.effects[6].curse_cancellation_count, 3u);

    RecordingSink sink;
    auto out = apply_curse_cancellation_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 7u);
    EXPECT_EQ(out.shop_discards, 1u);
    EXPECT_EQ(out.use_logs, 1u);
    EXPECT_EQ(out.use_acks_sent, 1u);
    EXPECT_EQ(out.cursed_discards, 1u);
    EXPECT_EQ(out.delete_acks_sent, 1u);
    EXPECT_EQ(out.discard_logs, 1u);
    EXPECT_EQ(out.obtains, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_TRUE(out.shop_flag_consumed);
    EXPECT_TRUE(out.use_ack_flag_consumed);
    EXPECT_TRUE(out.obtain_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>({"discardshop", "loguse", "useack",
                                        "discardcursed", "deleteack",
                                        "logdisc", "obtain"}));
    EXPECT_EQ(sink.last_player_id, 0x00120013u);
    EXPECT_EQ(sink.last_shop_item_idx, 400u);
    EXPECT_EQ(sink.last_shop_item_pos, 6u);
    EXPECT_EQ(sink.last_curse_count, 3u);
    EXPECT_EQ(sink.obtain_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyCurseCancellationSideEffects, NoSpaceForRestoreOmitsObtain) {
    auto in = PassingGates();
    in.obtain_space_available = false;
    auto plan = curse_cancellation_side_effect_plan(
        in, 7, 1, 2, 3);
    EXPECT_TRUE(plan.send_use_ack);
    EXPECT_FALSE(plan.obtain_ex);
    ASSERT_EQ(plan.effects.size(), 6u);
    EXPECT_EQ(plan.effects[5].kind,
              CurseCancellationSideEffectKind::LogItemMoneyDiscard);

    RecordingSink sink;
    auto out = apply_curse_cancellation_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 6u);
    EXPECT_EQ(out.obtains, 0u);
    EXPECT_FALSE(out.obtain_flag_consumed);
    EXPECT_TRUE(out.use_ack_flag_consumed);
    EXPECT_EQ(sink.obtain_count, 0u);
}

TEST(ApplyCurseCancellationSideEffects, GateNackCodesSweep) {
    struct Case {
        void (*mutate)(CurseCancellationValidationInput&);
        std::uint32_t expected_code;
    };
    const Case cases[] = {
        {[](CurseCancellationValidationInput& i) { i.item_exists_at_target = false; }, 1u},
        {[](CurseCancellationValidationInput& i) { i.unique_item_info_exists = false; }, 2u},
        {[](CurseCancellationValidationInput& i) { i.unique_item_is_cursed = false; }, 2u},
        {[](CurseCancellationValidationInput& i) { i.discard_shop_returned_true = false; }, 3u},
    };
    for (const auto& c : cases) {
        auto in = PassingGates();
        c.mutate(in);
        auto plan = curse_cancellation_side_effect_plan(
            in, 7, 1, 2, 3);
        EXPECT_TRUE(plan.send_nack);
        EXPECT_EQ(plan.nack_code, c.expected_code);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind,
                  CurseCancellationSideEffectKind::SendNackToPlayer);
        EXPECT_EQ(plan.effects[0].nack_code, c.expected_code);

        RecordingSink sink;
        (void)apply_curse_cancellation_side_effects(plan, sink);
        EXPECT_EQ(sink.last_nack_code, c.expected_code);
    }
}

TEST(ApplyCurseCancellationSideEffects, GatePrecedenceLocked) {
    // Item-not-exist outranks invalid unique info.
    auto in = PassingGates();
    in.item_exists_at_target = false;
    in.unique_item_info_exists = false;
    auto plan = curse_cancellation_side_effect_plan(
        in, 7, 1, 2, 3);
    EXPECT_EQ(plan.nack_code, 1u);

    // Invalid unique info outranks discard failure.
    auto in2 = PassingGates();
    in2.unique_item_is_cursed = false;
    in2.discard_shop_returned_true = false;
    auto plan2 = curse_cancellation_side_effect_plan(
        in2, 7, 1, 2, 3);
    EXPECT_EQ(plan2.nack_code, 2u);
}

TEST(ApplyCurseCancellationSideEffects, EmptyPlanIsNoOp) {
    mxh::server::CurseCancellationSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_curse_cancellation_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.use_acks_sent, 0u);
    EXPECT_EQ(out.shop_discards, 0u);
    EXPECT_EQ(out.use_logs, 0u);
    EXPECT_EQ(out.cursed_discards, 0u);
    EXPECT_EQ(out.delete_acks_sent, 0u);
    EXPECT_EQ(out.discard_logs, 0u);
    EXPECT_EQ(out.obtains, 0u);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_FALSE(out.use_ack_flag_consumed);
    EXPECT_FALSE(out.shop_flag_consumed);
    EXPECT_FALSE(out.use_log_flag_consumed);
    EXPECT_FALSE(out.cursed_flag_consumed);
    EXPECT_FALSE(out.delete_flag_consumed);
    EXPECT_FALSE(out.discard_log_flag_consumed);
    EXPECT_FALSE(out.obtain_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}
