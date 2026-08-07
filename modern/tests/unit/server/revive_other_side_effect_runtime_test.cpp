// revive_other_side_effect_runtime_test.cpp
//
// Verifies apply_revive_other_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEM_SHOPITEM_REVIVEOTHER_SYN
// side-effect chain) walks the data-plane plan and dispatches each
// entry: the 3-step success chain in legacy order / the 3-way gate
// NACK (codes 2/7/3).

#include <mxh/server/revive_other_side_effect.hpp>
#include <mxh/server/revive_other_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::LEGACY_ESHOPITEM_REVIVE_NOTDEAD;
using mxh::server::LEGACY_ESHOPITEM_REVIVE_NOTREADY;
using mxh::server::LEGACY_ESHOPITEM_REVIVE_NOTUSE;
using mxh::server::LEGACY_REVIVETIME_60SEC_MS;
using mxh::server::ReviveOtherSideEffectKind;
using mxh::server::ReviveOtherSideEffectSink;
using mxh::server::ReviveOtherValidationInput;
using mxh::server::apply_revive_other_side_effects;
using mxh::server::revive_other_side_effect_plan;

class RecordingSink final : public ReviveOtherSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_target_data1 = 0;
    std::uint16_t last_item_idx = 0;
    std::uint16_t last_item_pos = 0;
    std::uint32_t last_revive_time = 0;
    std::uint32_t last_nack_code = 0;
    std::size_t forward_count = 0;
    std::size_t nack_count = 0;

    void forward_revive_other_syn(std::uint32_t target_data1,
                                  std::uint16_t item_idx,
                                  std::uint16_t item_pos) override {
        calls.push_back("forward");
        last_target_data1 = target_data1;
        last_item_idx = item_idx;
        last_item_pos = item_pos;
        ++forward_count;
    }
    void set_revive_data(std::uint32_t target_data1,
                         std::uint16_t item_idx,
                         std::uint16_t item_pos) override {
        calls.push_back("data");
        last_target_data1 = target_data1;
        last_item_idx = item_idx;
        last_item_pos = item_pos;
    }
    void set_revive_time(std::uint32_t revive_time_ms) override {
        calls.push_back("time");
        last_revive_time = revive_time_ms;
    }
    void broadcast_revive_nack(std::uint32_t target_data1,
                               std::uint16_t item_idx,
                               std::uint16_t item_pos,
                               std::uint32_t nack_code) override {
        calls.push_back("nack");
        last_target_data1 = target_data1;
        last_item_idx = item_idx;
        last_item_pos = item_pos;
        last_nack_code = nack_code;
        ++nack_count;
    }
};

}  // namespace

TEST(ApplyReviveOtherSideEffects, SuccessEmitsThreeStepChainInOrder) {
    ReviveOtherValidationInput in;
    in.target_found = true;
    in.target_is_dead = true;
    in.is_useable_shop_item = true;
    auto plan = revive_other_side_effect_plan(
        in, /*target_data1=*/0x001A001Bu,
        /*item_idx=*/500, /*item_pos=*/8);
    EXPECT_TRUE(plan.forward_syn);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 3u);
    EXPECT_EQ(plan.effects[0].kind,
              ReviveOtherSideEffectKind::ForwardReviveOtherSyn);
    EXPECT_EQ(plan.effects[1].kind,
              ReviveOtherSideEffectKind::SetReviveData);
    EXPECT_EQ(plan.effects[2].kind,
              ReviveOtherSideEffectKind::SetReviveTime);
    EXPECT_EQ(plan.effects[2].revive_time_ms,
              LEGACY_REVIVETIME_60SEC_MS);

    RecordingSink sink;
    auto out = apply_revive_other_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 3u);
    EXPECT_EQ(out.forwards, 1u);
    EXPECT_EQ(out.revive_data_sets, 1u);
    EXPECT_EQ(out.revive_time_sets, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_TRUE(out.forward_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>({"forward", "data", "time"}));
    EXPECT_EQ(sink.last_target_data1, 0x001A001Bu);
    EXPECT_EQ(sink.last_item_idx, 500u);
    EXPECT_EQ(sink.last_item_pos, 8u);
    EXPECT_EQ(sink.last_revive_time, LEGACY_REVIVETIME_60SEC_MS);
    EXPECT_EQ(sink.forward_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyReviveOtherSideEffects, GateNackCodesSweep) {
    struct Case {
        void (*mutate)(ReviveOtherValidationInput&);
        std::uint32_t expected_code;
    };
    const Case cases[] = {
        {[](ReviveOtherValidationInput& i) { i.target_found = false; },
         LEGACY_ESHOPITEM_REVIVE_NOTDEAD},
        {[](ReviveOtherValidationInput& i) { i.target_is_dead = false; },
         LEGACY_ESHOPITEM_REVIVE_NOTDEAD},
        {[](ReviveOtherValidationInput& i) {
             i.siege_war_active = true;
             i.observer_team = true;
             i.incantation_limit_level = true;
         },
         LEGACY_ESHOPITEM_REVIVE_NOTREADY},
        {[](ReviveOtherValidationInput& i) { i.is_useable_shop_item = false; },
         LEGACY_ESHOPITEM_REVIVE_NOTUSE},
    };
    for (const auto& c : cases) {
        ReviveOtherValidationInput in;
        in.target_found = true;
        in.target_is_dead = true;
        in.is_useable_shop_item = true;
        c.mutate(in);
        auto plan = revive_other_side_effect_plan(in, 7, 1, 2);
        EXPECT_TRUE(plan.send_nack);
        EXPECT_EQ(plan.nack_code, c.expected_code);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind,
                  ReviveOtherSideEffectKind::BroadcastReviveNack);
        EXPECT_EQ(plan.effects[0].nack_code, c.expected_code);

        RecordingSink sink;
        (void)apply_revive_other_side_effects(plan, sink);
        EXPECT_EQ(sink.last_nack_code, c.expected_code);
    }
}

TEST(ApplyReviveOtherSideEffects, GatePrecedenceLocked) {
    // NotDead outranks NotReady.
    ReviveOtherValidationInput in;
    in.target_found = false;
    in.target_is_dead = false;
    in.siege_war_active = true;
    in.observer_team = true;
    in.incantation_limit_level = true;
    in.is_useable_shop_item = true;
    auto plan = revive_other_side_effect_plan(in, 7, 1, 2);
    EXPECT_EQ(plan.nack_code, LEGACY_ESHOPITEM_REVIVE_NOTDEAD);

    // NotReady outranks NotUsable.
    ReviveOtherValidationInput in2;
    in2.target_found = true;
    in2.target_is_dead = true;
    in2.siege_war_active = true;
    in2.observer_team = true;
    in2.incantation_limit_level = true;
    in2.is_useable_shop_item = false;
    auto plan2 = revive_other_side_effect_plan(in2, 7, 1, 2);
    EXPECT_EQ(plan2.nack_code, LEGACY_ESHOPITEM_REVIVE_NOTREADY);
}

TEST(ApplyReviveOtherSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ReviveOtherSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_revive_other_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.forwards, 0u);
    EXPECT_EQ(out.revive_data_sets, 0u);
    EXPECT_EQ(out.revive_time_sets, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_FALSE(out.forward_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyReviveOtherSideEffects, BoundaryFieldsPassthrough) {
    ReviveOtherValidationInput in;
    in.target_found = true;
    in.target_is_dead = true;
    in.is_useable_shop_item = true;
    auto plan = revive_other_side_effect_plan(
        in, /*target_data1=*/0xFFFFFFFFu,
        /*item_idx=*/0xFFFFu, /*item_pos=*/0xFFFFu);
    RecordingSink sink;
    (void)apply_revive_other_side_effects(plan, sink);
    EXPECT_EQ(sink.last_target_data1, 0xFFFFFFFFu);
    EXPECT_EQ(sink.last_item_idx, 0xFFFFu);
    EXPECT_EQ(sink.last_item_pos, 0xFFFFu);
}

TEST(ApplyReviveOtherSideEffects, NackDoesNotTouchForwardState) {
    ReviveOtherValidationInput in;
    in.target_found = false;
    auto nack_plan = revive_other_side_effect_plan(in, 7, 1, 2);
    RecordingSink nack_sink;
    auto nack_out = apply_revive_other_side_effects(nack_plan, nack_sink);
    EXPECT_EQ(nack_out.forwards, 0u);
    EXPECT_EQ(nack_sink.forward_count, 0u);
    EXPECT_EQ(nack_out.nacks_sent, 1u);

    ReviveOtherValidationInput ok;
    ok.target_found = true;
    ok.target_is_dead = true;
    ok.is_useable_shop_item = true;
    auto fwd_plan = revive_other_side_effect_plan(ok, 7, 1, 2);
    RecordingSink fwd_sink;
    auto fwd_out = apply_revive_other_side_effects(fwd_plan, fwd_sink);
    EXPECT_EQ(fwd_out.nacks_sent, 0u);
    EXPECT_EQ(fwd_sink.nack_count, 0u);
    EXPECT_EQ(fwd_out.forwards, 1u);
}
