// revive_other_ack_side_effect_runtime_test.cpp
//
// Verifies apply_revive_other_ack_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEM_SHOPITEM_REVIVEOTHER_ACK
// side-effect chain) walks the data-plane plan and dispatches each
// entry: NACK pairs for NotDead/NotUsable/Fail, the 5-step success
// chain (with or without the initial discard), and the always-clear
// step.

#include <mxh/server/revive_other_ack_side_effect.hpp>
#include <mxh/server/revive_other_ack_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::ReviveOtherAckSideEffectKind;
using mxh::server::ReviveOtherAckSideEffectSink;
using mxh::server::LEGACY_ESHOPITEM_REVIVE_FAIL;
using mxh::server::LEGACY_ESHOPITEM_REVIVE_NOTDEAD;
using mxh::server::LEGACY_ESHOPITEM_REVIVE_NOTUSE;
using mxh::server::apply_revive_other_ack_side_effects;
using mxh::server::revive_other_ack_side_effect_plan;

class RecordingSink final : public ReviveOtherAckSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_target_id = 0;
    std::uint32_t last_resurrector_id = 0;
    std::uint32_t last_nack_code = 0;
    std::uint16_t last_shop_item_idx = 0;
    std::uint16_t last_shop_item_pos = 0;

    void record_common(std::uint32_t target_id,
                       std::uint32_t resurrector_id,
                       std::uint32_t nack_code) {
        last_target_id = target_id;
        last_resurrector_id = resurrector_id;
        last_nack_code = nack_code;
    }
    void send_not_dead_nack_to_target(
        std::uint32_t target_id, std::uint32_t resurrector_id,
        std::uint32_t nack_code) override {
        calls.push_back("not_dead_target");
        record_common(target_id, resurrector_id, nack_code);
    }
    void send_not_dead_nack_to_resurrector(
        std::uint32_t target_id, std::uint32_t resurrector_id,
        std::uint32_t nack_code) override {
        calls.push_back("not_dead_res");
        record_common(target_id, resurrector_id, nack_code);
    }
    void send_not_usable_nack_to_target(
        std::uint32_t target_id, std::uint32_t resurrector_id,
        std::uint32_t nack_code) override {
        calls.push_back("not_use_target");
        record_common(target_id, resurrector_id, nack_code);
    }
    void send_not_usable_nack_to_resurrector(
        std::uint32_t target_id, std::uint32_t resurrector_id,
        std::uint32_t nack_code) override {
        calls.push_back("not_use_res");
        record_common(target_id, resurrector_id, nack_code);
    }
    void send_failed_nack_to_target(
        std::uint32_t target_id, std::uint32_t resurrector_id,
        std::uint32_t nack_code) override {
        calls.push_back("fail_target");
        record_common(target_id, resurrector_id, nack_code);
    }
    void send_failed_nack_to_resurrector(
        std::uint32_t target_id, std::uint32_t resurrector_id,
        std::uint32_t nack_code) override {
        calls.push_back("fail_res");
        record_common(target_id, resurrector_id, nack_code);
    }
    void send_revive_ack_to_target(
        std::uint32_t target_id, std::uint32_t resurrector_id) override {
        calls.push_back("revive_ack_target");
        last_target_id = target_id;
        last_resurrector_id = resurrector_id;
    }
    void send_revive_ack_to_resurrector(
        std::uint32_t target_id, std::uint32_t resurrector_id) override {
        calls.push_back("revive_ack_res");
        last_target_id = target_id;
        last_resurrector_id = resurrector_id;
    }
    void send_use_ack_to_target(
        std::uint32_t target_id, std::uint16_t shop_item_idx,
        std::uint16_t shop_item_pos) override {
        calls.push_back("use_ack_target");
        last_target_id = target_id;
        last_shop_item_idx = shop_item_idx;
        last_shop_item_pos = shop_item_pos;
    }
    void revive_shop_item_on_resurrector(
        std::uint32_t resurrector_id, std::uint16_t shop_item_idx) override {
        calls.push_back("revive_shop");
        last_resurrector_id = resurrector_id;
        last_shop_item_idx = shop_item_idx;
    }
    void discard_shop_item_from_target(
        std::uint32_t target_id, std::uint16_t shop_item_idx,
        std::uint16_t shop_item_pos) override {
        calls.push_back("discard");
        last_target_id = target_id;
        last_shop_item_idx = shop_item_idx;
        last_shop_item_pos = shop_item_pos;
    }
    void clear_revive_data(std::uint32_t target_id) override {
        calls.push_back("clear");
        last_target_id = target_id;
    }
};

}  // namespace

TEST(ApplyReviveOtherAckSideEffects, NotDeadEmitsNackPairAndClears) {
    mxh::server::ReviveOtherAckValidationInput in;
    in.resurrector_state_is_die = false;
    in.item_is_useable = true;
    in.item_info_exists = true;
    in.item_kind_is_incantation = true;
    in.item_limit_level_nonzero = true;
    in.resurrector_is_able = true;
    auto plan = revive_other_ack_side_effect_plan(
        in, /*target_id=*/10, /*resurrector_id=*/20, 100, 7);
    EXPECT_TRUE(plan.send_not_dead_nack);
    EXPECT_TRUE(plan.clear_revive_data);
    EXPECT_FALSE(plan.send_revive_ack);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              ReviveOtherAckSideEffectKind::SendNotDeadNackToTarget);
    EXPECT_EQ(plan.effects[1].kind,
              ReviveOtherAckSideEffectKind::SendNotDeadNackToResurrector);

    RecordingSink sink;
    auto out = apply_revive_other_ack_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 2u);
    EXPECT_EQ(out.nacks_target, 1u);
    EXPECT_EQ(out.nacks_resurrector, 1u);
    EXPECT_EQ(out.clears, 1u);
    EXPECT_TRUE(out.clear_flag_consumed);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>(
                  {"not_dead_target", "not_dead_res", "clear"}));
    EXPECT_EQ(sink.last_nack_code, LEGACY_ESHOPITEM_REVIVE_NOTDEAD);
    EXPECT_EQ(sink.last_target_id, 10u);
}

TEST(ApplyReviveOtherAckSideEffects, NotUsableEmitsMixedNackPairAndClears) {
    mxh::server::ReviveOtherAckValidationInput in;
    in.resurrector_state_is_die = true;
    in.item_is_useable = false;
    in.item_info_exists = true;
    in.item_kind_is_incantation = true;
    in.item_limit_level_nonzero = true;
    in.resurrector_is_able = true;
    auto plan = revive_other_ack_side_effect_plan(in, 10, 20, 100, 7);
    EXPECT_TRUE(plan.send_not_usable_nack);
    EXPECT_TRUE(plan.clear_revive_data);

    RecordingSink sink;
    (void)apply_revive_other_ack_side_effects(plan, sink);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>(
                  {"not_use_target", "not_use_res", "clear"}));
    // Target gets NotUse(3), resurrector gets Fail(1) -- legacy split.
    EXPECT_EQ(plan.effects[0].nack_code, LEGACY_ESHOPITEM_REVIVE_NOTUSE);
    EXPECT_EQ(plan.effects[1].nack_code, LEGACY_ESHOPITEM_REVIVE_FAIL);
}

TEST(ApplyReviveOtherAckSideEffects, FailEmitsFailedNackPairAndClears) {
    mxh::server::ReviveOtherAckValidationInput in;
    in.resurrector_state_is_die = true;
    in.item_is_useable = true;
    in.item_info_exists = true;
    in.item_kind_is_incantation = true;
    in.item_limit_level_nonzero = true;
    in.resurrector_is_able = false;  // NotAbleToRevive -> Fail
    auto plan = revive_other_ack_side_effect_plan(in, 10, 20, 100, 7);
    EXPECT_TRUE(plan.send_failed_nack);
    EXPECT_TRUE(plan.clear_revive_data);

    RecordingSink sink;
    (void)apply_revive_other_ack_side_effects(plan, sink);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>(
                  {"fail_target", "fail_res", "clear"}));
    EXPECT_EQ(plan.effects[0].nack_code, LEGACY_ESHOPITEM_REVIVE_FAIL);
    EXPECT_EQ(plan.effects[1].nack_code, LEGACY_ESHOPITEM_REVIVE_FAIL);
}

TEST(ApplyReviveOtherAckSideEffects, SuccessEmitsFullChainWithDiscard) {
    mxh::server::ReviveOtherAckValidationInput in;
    in.resurrector_state_is_die = true;
    in.item_is_useable = true;
    in.item_info_exists = true;
    in.item_kind_is_incantation = true;
    in.item_limit_level_nonzero = true;
    in.resurrector_is_able = true;
    in.item_in_using_list = false;
    in.item_sell_price_zero = true;
    in.discard_returned_true = true;
    auto plan = revive_other_ack_side_effect_plan(
        in, /*target_id=*/10, /*resurrector_id=*/20,
        /*shop_item_idx=*/100, /*shop_item_pos=*/7);
    EXPECT_TRUE(plan.send_revive_ack);
    EXPECT_TRUE(plan.revive_shop_item);
    EXPECT_TRUE(plan.discard_shop_item);
    EXPECT_TRUE(plan.send_use_ack_to_target);
    EXPECT_TRUE(plan.clear_revive_data);
    ASSERT_EQ(plan.effects.size(), 5u);

    RecordingSink sink;
    auto out = apply_revive_other_ack_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 5u);
    EXPECT_EQ(out.discards, 1u);
    EXPECT_EQ(out.revives, 1u);
    EXPECT_EQ(out.use_acks, 1u);
    EXPECT_EQ(out.revive_acks, 2u);
    EXPECT_EQ(out.clears, 1u);
    EXPECT_TRUE(out.clear_flag_consumed);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>({
                  "discard", "revive_shop", "use_ack_target",
                  "revive_ack_target", "revive_ack_res", "clear"}));
    EXPECT_EQ(sink.last_target_id, 10u);
    EXPECT_EQ(sink.last_resurrector_id, 20u);
    EXPECT_EQ(sink.last_shop_item_idx, 100u);
    EXPECT_EQ(sink.last_shop_item_pos, 7u);
}

TEST(ApplyReviveOtherAckSideEffects, AlreadyUsedEmitsChainWithoutDiscard) {
    mxh::server::ReviveOtherAckValidationInput in;
    in.resurrector_state_is_die = true;
    in.item_is_useable = true;
    in.item_info_exists = true;
    in.item_kind_is_incantation = true;
    in.item_limit_level_nonzero = true;
    in.resurrector_is_able = true;
    in.item_in_using_list = true;   // legacy skips the discard path
    in.item_sell_price_zero = true;
    in.discard_returned_true = true;
    auto plan = revive_other_ack_side_effect_plan(in, 10, 20, 100, 7);
    EXPECT_TRUE(plan.send_revive_ack);
    EXPECT_TRUE(plan.revive_shop_item);
    EXPECT_FALSE(plan.discard_shop_item);
    ASSERT_EQ(plan.effects.size(), 4u);

    RecordingSink sink;
    (void)apply_revive_other_ack_side_effects(plan, sink);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>({
                  "revive_shop", "use_ack_target",
                  "revive_ack_target", "revive_ack_res", "clear"}));
}

TEST(ApplyReviveOtherAckSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ReviveOtherAckSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_revive_other_ack_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.nacks_target, 0u);
    EXPECT_EQ(out.nacks_resurrector, 0u);
    EXPECT_EQ(out.revive_acks, 0u);
    EXPECT_EQ(out.use_acks, 0u);
    EXPECT_EQ(out.revives, 0u);
    EXPECT_EQ(out.discards, 0u);
    EXPECT_EQ(out.clears, 0u);
    EXPECT_FALSE(out.clear_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}
