// shop_item_chase_side_effect_runtime_test.cpp
//
// Verifies apply_shop_item_chase_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEM_SHOPITEM_CHASE_SYN
// side-effect chain) walks the data-plane plan and dispatches each
// entry: agent ACK + player tracking on success / agent NACK when the
// target is offline / no-op for non-chase kinds.

#include <mxh/server/shop_item_chase_side_effect.hpp>
#include <mxh/server/shop_item_chase_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::ShopItemChaseSideEffectKind;
using mxh::server::ShopItemChaseSideEffectSink;
using mxh::server::LEGACY_EINCANTATION_TRACKING;
using mxh::server::LEGACY_EINCANTATION_TRACKING7;
using mxh::server::LEGACY_EINCANTATION_TRACKING7_NOTRADE;
using mxh::server::apply_shop_item_chase_side_effects;
using mxh::server::shop_item_chase_side_effect_plan;

class RecordingSink final : public ShopItemChaseSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_target_id = 0;
    std::uint32_t last_requester_char_idx = 0;
    std::uint32_t last_item_kind = 0;
    int last_map_num = 0;
    int last_event_map_num = 0;

    void forward_chase_ack_to_agent(
        std::uint32_t target_id, std::uint32_t requester_char_idx,
        std::uint32_t item_kind, int map_num, int event_map_num) override {
        calls.push_back("ack");
        last_target_id = target_id;
        last_requester_char_idx = requester_char_idx;
        last_item_kind = item_kind;
        last_map_num = map_num;
        last_event_map_num = event_map_num;
    }
    void forward_chase_nack_to_agent(
        std::uint32_t requester_char_idx) override {
        calls.push_back("nack");
        last_requester_char_idx = requester_char_idx;
    }
    void broadcast_chase_tracking(std::uint32_t target_id,
                                  std::uint32_t item_kind) override {
        calls.push_back("tracking");
        last_target_id = target_id;
        last_item_kind = item_kind;
    }
};

}  // namespace

TEST(ApplyShopItemChaseSideEffects, ResolveEmitsAckThenTrackingInOrder) {
    mxh::server::ShopItemChaseValidationInput in;
    in.target_found = true;
    in.item_kind = LEGACY_EINCANTATION_TRACKING;
    auto plan = shop_item_chase_side_effect_plan(
        in, /*target_id=*/0x00010002u, /*requester_char_idx=*/7,
        /*map_num=*/101, /*event_map_num=*/44);
    EXPECT_TRUE(plan.forward_ack);
    EXPECT_FALSE(plan.forward_nack);
    EXPECT_TRUE(plan.broadcast_tracking);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemChaseSideEffectKind::ForwardChaseAckToAgent);
    EXPECT_EQ(plan.effects[1].kind,
              ShopItemChaseSideEffectKind::BroadcastChaseTracking);

    RecordingSink sink;
    auto out = apply_shop_item_chase_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 2u);
    EXPECT_EQ(out.acks_forwarded, 1u);
    EXPECT_EQ(out.nacks_forwarded, 0u);
    EXPECT_EQ(out.trackings, 1u);
    EXPECT_TRUE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_TRUE(out.tracking_flag_consumed);
    ASSERT_EQ(sink.calls.size(), 2u);
    EXPECT_EQ(sink.calls[0], "ack");
    EXPECT_EQ(sink.calls[1], "tracking");
    EXPECT_EQ(sink.last_target_id, 0x00010002u);
    EXPECT_EQ(sink.last_requester_char_idx, 7u);
    EXPECT_EQ(sink.last_item_kind, LEGACY_EINCANTATION_TRACKING);
    EXPECT_EQ(sink.last_map_num, 101);
    EXPECT_EQ(sink.last_event_map_num, 44);
}

TEST(ApplyShopItemChaseSideEffects, AllTrackingVariantsResolve) {
    for (std::uint32_t kind : {LEGACY_EINCANTATION_TRACKING,
                               LEGACY_EINCANTATION_TRACKING7,
                               LEGACY_EINCANTATION_TRACKING7_NOTRADE}) {
        mxh::server::ShopItemChaseValidationInput in;
        in.target_found = true;
        in.item_kind = kind;
        auto plan = shop_item_chase_side_effect_plan(in, 1, 2, 3, 4);
        EXPECT_TRUE(plan.forward_ack);
        EXPECT_TRUE(plan.broadcast_tracking);

        RecordingSink sink;
        (void)apply_shop_item_chase_side_effects(plan, sink);
        EXPECT_EQ(sink.calls,
                  std::vector<std::string>({"ack", "tracking"}));
        EXPECT_EQ(sink.last_item_kind, kind);
    }
}

TEST(ApplyShopItemChaseSideEffects, NoTargetEmitsAgentNack) {
    mxh::server::ShopItemChaseValidationInput in;
    in.target_found = false;
    in.item_kind = LEGACY_EINCANTATION_TRACKING;
    auto plan = shop_item_chase_side_effect_plan(
        in, 0, /*requester_char_idx=*/7, 0, 0);
    EXPECT_FALSE(plan.forward_ack);
    EXPECT_TRUE(plan.forward_nack);
    EXPECT_FALSE(plan.broadcast_tracking);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemChaseSideEffectKind::ForwardChaseNackToAgent);

    RecordingSink sink;
    auto out = apply_shop_item_chase_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.nacks_forwarded, 1u);
    EXPECT_TRUE(out.nack_flag_consumed);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.tracking_flag_consumed);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"nack"}));
    EXPECT_EQ(sink.last_requester_char_idx, 7u);
}

TEST(ApplyShopItemChaseSideEffects, NotChaseIsNoOp) {
    mxh::server::ShopItemChaseValidationInput in;
    in.target_found = true;
    in.item_kind = 999;  // not a tracking variant
    auto plan = shop_item_chase_side_effect_plan(in, 1, 2, 3, 4);
    EXPECT_FALSE(plan.forward_ack);
    EXPECT_FALSE(plan.forward_nack);
    EXPECT_FALSE(plan.broadcast_tracking);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_shop_item_chase_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.acks_forwarded, 0u);
    EXPECT_EQ(out.nacks_forwarded, 0u);
    EXPECT_EQ(out.trackings, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_FALSE(out.tracking_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyShopItemChaseSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ShopItemChaseSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_shop_item_chase_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.acks_forwarded, 0u);
    EXPECT_EQ(out.nacks_forwarded, 0u);
    EXPECT_EQ(out.trackings, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_FALSE(out.tracking_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyShopItemChaseSideEffects, NoTargetOverridesNotChase) {
    mxh::server::ShopItemChaseValidationInput in;
    in.target_found = false;
    in.item_kind = 999;
    auto plan = shop_item_chase_side_effect_plan(
        in, 0, /*requester_char_idx=*/5, 0, 0);
    EXPECT_TRUE(plan.forward_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemChaseSideEffectKind::ForwardChaseNackToAgent);

    RecordingSink sink;
    (void)apply_shop_item_chase_side_effects(plan, sink);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"nack"}));
    EXPECT_EQ(sink.last_requester_char_idx, 5u);
}
