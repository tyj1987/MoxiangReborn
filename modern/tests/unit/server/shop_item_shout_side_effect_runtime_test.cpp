// shop_item_shout_side_effect_runtime_test.cpp
//
// Verifies apply_shop_item_shout_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEM_SHOPITEM_SHOUT_SYN
// side-effect chain) walks the data-plane plan and dispatches each
// entry: discard + use-ACK + agent forward for the once variant /
// forward-only for the reusable variant / NACK for the not-usable
// and discard-fail branches.

#include <mxh/server/shop_item_shout_side_effect.hpp>
#include <mxh/server/shop_item_shout_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::ShopItemShoutSideEffectKind;
using mxh::server::ShopItemShoutSideEffectSink;
using mxh::server::ShopItemShoutValidationInput;
using mxh::server::apply_shop_item_shout_side_effects;
using mxh::server::shop_item_shout_side_effect_plan;

class RecordingSink final : public ShopItemShoutSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint16_t last_item_idx = 0;
    std::uint16_t last_item_pos = 0;
    std::uint32_t last_character_idx = 0;
    bool last_is_once_variant = false;
    std::size_t forward_count = 0;
    std::size_t nack_count = 0;

    void discard_shout_item(std::uint16_t item_idx,
                            std::uint16_t item_pos,
                            bool is_once_variant) override {
        calls.push_back("discard");
        last_item_idx = item_idx;
        last_item_pos = item_pos;
        last_is_once_variant = is_once_variant;
    }
    void broadcast_use_ack(std::uint16_t item_idx,
                           std::uint16_t item_pos) override {
        calls.push_back("useack");
        last_item_idx = item_idx;
        last_item_pos = item_pos;
    }
    void forward_shout_ack(std::uint32_t character_idx,
                           std::uint16_t item_idx,
                           std::uint16_t item_pos) override {
        calls.push_back("forward");
        last_character_idx = character_idx;
        last_item_idx = item_idx;
        last_item_pos = item_pos;
        ++forward_count;
    }
    void broadcast_shout_nack(std::uint16_t item_idx,
                              std::uint16_t item_pos,
                              bool is_once_variant) override {
        calls.push_back("nack");
        last_item_idx = item_idx;
        last_item_pos = item_pos;
        last_is_once_variant = is_once_variant;
        ++nack_count;
    }
};

}  // namespace

TEST(ApplyShopItemShoutSideEffects,
     OnceVariantEmitsDiscardUseAckForwardInOrder) {
    ShopItemShoutValidationInput in;
    in.player_found = true;
    in.usable_shop_item = true;
    in.is_once_variant = true;
    in.discard_rt = 0;
    auto plan = shop_item_shout_side_effect_plan(
        in, /*item_idx=*/100, /*item_pos=*/7,
        /*character_idx=*/0x00030004u);
    EXPECT_TRUE(plan.forward_shout);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 3u);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemShoutSideEffectKind::DiscardShoutItem);
    EXPECT_EQ(plan.effects[1].kind,
              ShopItemShoutSideEffectKind::BroadcastUseAck);
    EXPECT_EQ(plan.effects[2].kind,
              ShopItemShoutSideEffectKind::ForwardShoutAck);
    EXPECT_EQ(plan.effects[0].is_once_variant, true);

    RecordingSink sink;
    auto out = apply_shop_item_shout_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 3u);
    EXPECT_EQ(out.discards, 1u);
    EXPECT_EQ(out.use_acks_sent, 1u);
    EXPECT_EQ(out.forwards_sent, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_TRUE(out.forward_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>({"discard", "useack", "forward"}));
    EXPECT_EQ(sink.last_character_idx, 0x00030004u);
    EXPECT_EQ(sink.last_item_idx, 100u);
    EXPECT_EQ(sink.last_item_pos, 7u);
    EXPECT_EQ(sink.last_is_once_variant, true);
    EXPECT_EQ(sink.forward_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyShopItemShoutSideEffects, NonOnceVariantEmitsForwardOnly) {
    ShopItemShoutValidationInput in;
    in.player_found = true;
    in.usable_shop_item = true;
    in.is_once_variant = false;
    in.discard_rt = 0;
    auto plan = shop_item_shout_side_effect_plan(
        in, 100, 7, 99);
    EXPECT_TRUE(plan.forward_shout);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemShoutSideEffectKind::ForwardShoutAck);

    RecordingSink sink;
    auto out = apply_shop_item_shout_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.discards, 0u);
    EXPECT_EQ(out.use_acks_sent, 0u);
    EXPECT_EQ(out.forwards_sent, 1u);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"forward"}));
    EXPECT_EQ(sink.last_character_idx, 99u);
}

TEST(ApplyShopItemShoutSideEffects, NotUsableEmitsNack) {
    ShopItemShoutValidationInput in;
    in.player_found = true;
    in.usable_shop_item = false;
    in.is_once_variant = false;
    in.discard_rt = 0;
    auto plan = shop_item_shout_side_effect_plan(
        in, 5, 6, 7);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_FALSE(plan.forward_shout);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemShoutSideEffectKind::BroadcastShoutNack);
    EXPECT_EQ(plan.effects[0].is_once_variant, false);

    RecordingSink sink;
    auto out = apply_shop_item_shout_side_effects(plan, sink);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_EQ(out.forwards_sent, 0u);
    EXPECT_TRUE(out.nack_flag_consumed);
    EXPECT_FALSE(out.forward_flag_consumed);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"nack"}));
    EXPECT_EQ(sink.last_item_idx, 5u);
    EXPECT_EQ(sink.last_item_pos, 6u);
    EXPECT_EQ(sink.last_is_once_variant, false);
}

TEST(ApplyShopItemShoutSideEffects, DiscardFailEmitsNackWithOnceVariant) {
    ShopItemShoutValidationInput in;
    in.player_found = true;
    in.usable_shop_item = true;
    in.is_once_variant = true;
    in.discard_rt = 5;  // legacy: any non-EI_TRUE return
    auto plan = shop_item_shout_side_effect_plan(
        in, 11, 12, 13);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemShoutSideEffectKind::BroadcastShoutNack);
    EXPECT_EQ(plan.effects[0].is_once_variant, true);

    RecordingSink sink;
    (void)apply_shop_item_shout_side_effects(plan, sink);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"nack"}));
    EXPECT_EQ(sink.last_is_once_variant, true);
    EXPECT_EQ(sink.nack_count, 1u);
}

TEST(ApplyShopItemShoutSideEffects, NoPlayerEmitsEmptyPlan) {
    ShopItemShoutValidationInput in;
    in.player_found = false;
    in.usable_shop_item = true;
    in.is_once_variant = true;
    in.discard_rt = 0;
    auto plan = shop_item_shout_side_effect_plan(
        in, 1, 2, 3);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_FALSE(plan.forward_shout);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_shop_item_shout_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.forwards_sent, 0u);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyShopItemShoutSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ShopItemShoutSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_shop_item_shout_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.discards, 0u);
    EXPECT_EQ(out.use_acks_sent, 0u);
    EXPECT_EQ(out.forwards_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_FALSE(out.forward_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyShopItemShoutSideEffects, NackDoesNotTouchForwardState) {
    auto nack_in = ShopItemShoutValidationInput{};
    nack_in.player_found = true;
    nack_in.usable_shop_item = false;
    auto nack_plan = shop_item_shout_side_effect_plan(
        nack_in, 1, 2, 3);
    RecordingSink nack_sink;
    auto nack_out = apply_shop_item_shout_side_effects(nack_plan, nack_sink);
    EXPECT_EQ(nack_out.forwards_sent, 0u);
    EXPECT_EQ(nack_sink.forward_count, 0u);
    EXPECT_EQ(nack_out.nacks_sent, 1u);

    auto fwd_in = ShopItemShoutValidationInput{};
    fwd_in.player_found = true;
    fwd_in.usable_shop_item = true;
    auto fwd_plan = shop_item_shout_side_effect_plan(
        fwd_in, 1, 2, 3);
    RecordingSink fwd_sink;
    auto fwd_out = apply_shop_item_shout_side_effects(fwd_plan, fwd_sink);
    EXPECT_EQ(fwd_out.nacks_sent, 0u);
    EXPECT_EQ(fwd_sink.nack_count, 0u);
    EXPECT_EQ(fwd_out.forwards_sent, 1u);
}
