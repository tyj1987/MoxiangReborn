// reinforce_reset_side_effect_runtime_test.cpp
//
// Verifies apply_reinforce_reset_side_effects() (the runtime
// orchestrator for the CItemManager::
// MP_ITEM_SHOPITEM_REINFORCERESET_SYN side-effect chain) walks the
// data-plane plan and dispatches each entry: the 9-step success chain
// in legacy order / the 7-way gate NACK (codes 1-6, 9).

#include <mxh/server/reinforce_reset_side_effect.hpp>
#include <mxh/server/reinforce_reset_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::ReinforceResetSideEffectKind;
using mxh::server::ReinforceResetSideEffectSink;
using mxh::server::ReinforceResetValidationInput;
using mxh::server::apply_reinforce_reset_side_effects;
using mxh::server::reinforce_reset_side_effect_plan;

class RecordingSink final : public ReinforceResetSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_player_id = 0;
    std::uint32_t last_nack_code = 0;
    std::uint16_t last_shop_item_idx = 0;
    std::uint16_t last_shop_item_pos = 0;
    std::uint32_t last_target_db_idx = 0;
    std::uint32_t last_target_durability = 0;
    std::size_t ack_count = 0;
    std::size_t nack_count = 0;

    void send_ack_to_player(std::uint32_t player_id) override {
        calls.push_back("ack");
        last_player_id = player_id;
        ++ack_count;
    }
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
        calls.push_back("discard");
        last_player_id = player_id;
        last_shop_item_idx = shop_item_idx;
        last_shop_item_pos = shop_item_pos;
    }
    void remove_item_option(std::uint32_t player_id,
                            std::uint32_t target_db_idx,
                            std::uint32_t target_durability) override {
        calls.push_back("remove");
        last_player_id = player_id;
        last_target_db_idx = target_db_idx;
        last_target_durability = target_durability;
    }
    void character_item_option_delete(
        std::uint32_t player_id, std::uint32_t target_db_idx,
        std::uint32_t target_durability) override {
        calls.push_back("dbo");
        last_player_id = player_id;
        last_target_db_idx = target_db_idx;
        last_target_durability = target_durability;
    }
    void item_update_to_db(std::uint32_t player_id,
                           std::uint32_t target_db_idx) override {
        calls.push_back("itemupd");
        last_player_id = player_id;
        last_target_db_idx = target_db_idx;
    }
    void log_item_money_use(std::uint32_t player_id) override {
        calls.push_back("loguse");
        last_player_id = player_id;
    }
    void log_item_money_reset(std::uint32_t player_id) override {
        calls.push_back("logreset");
        last_player_id = player_id;
    }
    void clear_target_durability(std::uint32_t player_id,
                                 std::uint32_t target_db_idx) override {
        calls.push_back("clear");
        last_player_id = player_id;
        last_target_db_idx = target_db_idx;
    }
};

ReinforceResetValidationInput PassingGates() {
    ReinforceResetValidationInput in;
    in.shop_item_is_useable = true;
    in.shop_item_exists = true;
    in.target_item_exists = true;
    in.target_item_info_exists = true;
    in.shop_item_icon_is_reinforce_reset = true;
    in.target_is_equip_kind = true;
    in.target_has_option = true;
    in.discard_returned_true = true;
    return in;
}

}  // namespace

TEST(ApplyReinforceResetSideEffects, SuccessEmitsNineStepChainInOrder) {
    auto in = PassingGates();
    auto plan = reinforce_reset_side_effect_plan(
        in, /*player_id=*/0x00060007u,
        /*shop_item_idx=*/100, /*shop_item_pos=*/5,
        /*target_db_idx=*/9001, /*target_durability=*/12345);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_TRUE(plan.send_use_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.discard_shop_item);
    EXPECT_TRUE(plan.remove_item_option);
    EXPECT_TRUE(plan.db_item_option_delete);
    EXPECT_TRUE(plan.db_item_update);
    EXPECT_TRUE(plan.log_item_money_use);
    EXPECT_TRUE(plan.log_item_money_reset);
    EXPECT_TRUE(plan.clear_target_durability);
    ASSERT_EQ(plan.effects.size(), 9u);
    const ReinforceResetSideEffectKind expected[] = {
        ReinforceResetSideEffectKind::DiscardShopItem,
        ReinforceResetSideEffectKind::LogItemMoneyUse,
        ReinforceResetSideEffectKind::RemoveItemOption,
        ReinforceResetSideEffectKind::CharacterItemOptionDelete,
        ReinforceResetSideEffectKind::ItemUpdateToDB,
        ReinforceResetSideEffectKind::LogItemMoneyReset,
        ReinforceResetSideEffectKind::ClearTargetDurability,
        ReinforceResetSideEffectKind::SendUseAckToPlayer,
        ReinforceResetSideEffectKind::SendAckToPlayer,
    };
    for (std::size_t i = 0u; i < 9u; ++i) {
        EXPECT_EQ(plan.effects[i].kind, expected[i]);
    }
    EXPECT_EQ(plan.effects[0].shop_item_idx, 100u);
    EXPECT_EQ(plan.effects[0].shop_item_pos, 5u);
    EXPECT_EQ(plan.effects[2].target_db_idx, 9001u);
    EXPECT_EQ(plan.effects[2].target_durability, 12345u);
    EXPECT_EQ(plan.effects[7].shop_item_idx, 100u);
    EXPECT_EQ(plan.effects[7].shop_item_pos, 5u);

    RecordingSink sink;
    auto out = apply_reinforce_reset_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 9u);
    EXPECT_EQ(out.discards, 1u);
    EXPECT_EQ(out.use_logs, 1u);
    EXPECT_EQ(out.option_removals, 1u);
    EXPECT_EQ(out.db_option_deletes, 1u);
    EXPECT_EQ(out.db_item_updates, 1u);
    EXPECT_EQ(out.reset_logs, 1u);
    EXPECT_EQ(out.durability_clears, 1u);
    EXPECT_EQ(out.use_acks_sent, 1u);
    EXPECT_EQ(out.acks_sent, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_TRUE(out.discard_flag_consumed);
    EXPECT_TRUE(out.remove_flag_consumed);
    EXPECT_TRUE(out.db_delete_flag_consumed);
    EXPECT_TRUE(out.db_update_flag_consumed);
    EXPECT_TRUE(out.clear_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>({"discard", "loguse", "remove",
                                        "dbo", "itemupd", "logreset",
                                        "clear", "useack", "ack"}));
    EXPECT_EQ(sink.last_player_id, 0x00060007u);
    EXPECT_EQ(sink.last_shop_item_idx, 100u);
    EXPECT_EQ(sink.last_shop_item_pos, 5u);
    EXPECT_EQ(sink.last_target_db_idx, 9001u);
    EXPECT_EQ(sink.last_target_durability, 12345u);
    EXPECT_EQ(sink.ack_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyReinforceResetSideEffects, GateNackCodesSweep) {
    struct Case {
        void (*mutate)(ReinforceResetValidationInput&);
        std::uint32_t expected_code;
    };
    const Case cases[] = {
        {[](ReinforceResetValidationInput& i) { i.shop_item_is_useable = false; }, 1u},
        {[](ReinforceResetValidationInput& i) { i.shop_item_exists = false; }, 2u},
        {[](ReinforceResetValidationInput& i) { i.target_item_info_exists = false; }, 3u},
        {[](ReinforceResetValidationInput& i) { i.shop_item_icon_is_reinforce_reset = false; }, 4u},
        {[](ReinforceResetValidationInput& i) { i.target_is_equip_kind = false; }, 5u},
        {[](ReinforceResetValidationInput& i) { i.target_has_option = false; }, 6u},
        {[](ReinforceResetValidationInput& i) { i.discard_returned_true = false; }, 9u},
    };
    for (const auto& c : cases) {
        auto in = PassingGates();
        c.mutate(in);
        auto plan = reinforce_reset_side_effect_plan(
            in, 7, 1, 2, 3, 4);
        EXPECT_TRUE(plan.send_nack);
        EXPECT_FALSE(plan.send_ack);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind,
                  ReinforceResetSideEffectKind::SendNackToPlayer);
        EXPECT_EQ(plan.effects[0].nack_code, c.expected_code);

        RecordingSink sink;
        (void)apply_reinforce_reset_side_effects(plan, sink);
        EXPECT_EQ(sink.last_nack_code, c.expected_code);
    }
}

TEST(ApplyReinforceResetSideEffects, GatePrecedenceLocked) {
    // Not-usable outranks bad item.
    auto in = PassingGates();
    in.shop_item_is_useable = false;
    in.shop_item_exists = false;
    auto plan = reinforce_reset_side_effect_plan(in, 7, 1, 2, 3, 4);
    EXPECT_EQ(plan.effects[0].nack_code, 1u);

    // Wrong icon outranks not-equip.
    auto in2 = PassingGates();
    in2.shop_item_icon_is_reinforce_reset = false;
    in2.target_is_equip_kind = false;
    auto plan2 = reinforce_reset_side_effect_plan(in2, 7, 1, 2, 3, 4);
    EXPECT_EQ(plan2.effects[0].nack_code, 4u);

    // No option outranks discard failure.
    auto in3 = PassingGates();
    in3.target_has_option = false;
    in3.discard_returned_true = false;
    auto plan3 = reinforce_reset_side_effect_plan(in3, 7, 1, 2, 3, 4);
    EXPECT_EQ(plan3.effects[0].nack_code, 6u);
}

TEST(ApplyReinforceResetSideEffects, NackDoesNotTouchAckState) {
    auto bad = PassingGates();
    bad.shop_item_is_useable = false;
    auto nack_plan = reinforce_reset_side_effect_plan(
        bad, 7, 1, 2, 3, 4);
    RecordingSink nack_sink;
    auto nack_out =
        apply_reinforce_reset_side_effects(nack_plan, nack_sink);
    EXPECT_EQ(nack_out.acks_sent, 0u);
    EXPECT_EQ(nack_sink.ack_count, 0u);
    EXPECT_EQ(nack_out.nacks_sent, 1u);

    auto ok = PassingGates();
    auto ack_plan = reinforce_reset_side_effect_plan(
        ok, 7, 1, 2, 3, 4);
    RecordingSink ack_sink;
    auto ack_out =
        apply_reinforce_reset_side_effects(ack_plan, ack_sink);
    EXPECT_EQ(ack_out.nacks_sent, 0u);
    EXPECT_EQ(ack_sink.nack_count, 0u);
    EXPECT_EQ(ack_out.acks_sent, 1u);
}

TEST(ApplyReinforceResetSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ReinforceResetSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_reinforce_reset_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.use_acks_sent, 0u);
    EXPECT_EQ(out.discards, 0u);
    EXPECT_EQ(out.option_removals, 0u);
    EXPECT_EQ(out.db_option_deletes, 0u);
    EXPECT_EQ(out.db_item_updates, 0u);
    EXPECT_EQ(out.use_logs, 0u);
    EXPECT_EQ(out.reset_logs, 0u);
    EXPECT_EQ(out.durability_clears, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_FALSE(out.use_ack_flag_consumed);
    EXPECT_FALSE(out.discard_flag_consumed);
    EXPECT_FALSE(out.remove_flag_consumed);
    EXPECT_FALSE(out.db_delete_flag_consumed);
    EXPECT_FALSE(out.db_update_flag_consumed);
    EXPECT_FALSE(out.use_log_flag_consumed);
    EXPECT_FALSE(out.reset_log_flag_consumed);
    EXPECT_FALSE(out.clear_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}
