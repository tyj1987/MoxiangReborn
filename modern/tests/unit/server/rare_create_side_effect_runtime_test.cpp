// rare_create_side_effect_runtime_test.cpp
//
// Verifies apply_rare_create_side_effects() (the runtime orchestrator
// for the CItemManager::MP_ITEM_SHOPITEM_RARECREATE_SYN side-effect
// chain) walks the data-plane plan and dispatches each entry: the
// 5-step success chain (NO RARECREATE_ACK) / the 11-way gate NACK.

#include <mxh/server/rare_create_side_effect.hpp>
#include <mxh/server/rare_create_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::RareCreateSideEffectKind;
using mxh::server::RareCreateSideEffectSink;
using mxh::server::RareCreateValidationInput;
using mxh::server::apply_rare_create_side_effects;
using mxh::server::rare_create_side_effect_plan;

class RecordingSink final : public RareCreateSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_player_id = 0;
    std::uint32_t last_nack_code = 0;
    std::uint16_t last_shop_item_idx = 0;
    std::uint16_t last_shop_item_pos = 0;
    std::uint32_t last_target_w_icon_idx = 0;
    std::uint32_t last_target_position = 0;
    std::uint32_t last_target_db_idx = 0;
    std::size_t nack_count = 0;
    std::size_t use_ack_count = 0;

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
        ++use_ack_count;
    }
    void generate_rare_option(std::uint32_t player_id,
                              std::uint32_t target_w_icon_idx) override {
        calls.push_back("gen");
        last_player_id = player_id;
        last_target_w_icon_idx = target_w_icon_idx;
    }
    void discard_shop_item(std::uint32_t player_id,
                           std::uint16_t shop_item_idx,
                           std::uint16_t shop_item_pos) override {
        calls.push_back("discard");
        last_player_id = player_id;
        last_shop_item_idx = shop_item_idx;
        last_shop_item_pos = shop_item_pos;
    }
    void shop_item_rare_insert_to_db(
        std::uint32_t player_id, std::uint32_t target_w_icon_idx,
        std::uint32_t target_position,
        std::uint32_t target_db_idx) override {
        calls.push_back("db");
        last_player_id = player_id;
        last_target_w_icon_idx = target_w_icon_idx;
        last_target_position = target_position;
        last_target_db_idx = target_db_idx;
    }
    void log_item_money(std::uint32_t player_id) override {
        calls.push_back("log");
        last_player_id = player_id;
    }
};

RareCreateValidationInput PassingGates() {
    RareCreateValidationInput in;
    in.shop_item_is_useable = true;
    in.shop_item_exists = true;
    in.target_item_exists = true;
    in.shop_item_info_exists = true;
    in.target_item_info_exists = true;
    in.shop_item_icon_is_create_50_70_90_99 = true;
    in.target_is_equip_kind = true;
    in.target_durability_zero = true;
    in.target_option_idx_zero = true;
    in.target_w_icon_idx_suffix_zero = true;
    in.level_in_range = true;
    in.is_rare_item_able = true;
    in.get_rare_returned_true = true;
    in.discard_returned_true = true;
    return in;
}

}  // namespace

TEST(ApplyRareCreateSideEffects, SuccessEmitsFiveStepChainInOrder) {
    auto in = PassingGates();
    auto plan = rare_create_side_effect_plan(
        in, /*player_id=*/0x00080009u,
        /*shop_item_idx=*/200, /*shop_item_pos=*/3,
        /*target_w_icon_idx=*/4321,
        /*target_position=*/17, /*target_db_idx=*/5555);
    EXPECT_TRUE(plan.send_use_ack);
    EXPECT_TRUE(plan.generate_rare_option);
    EXPECT_TRUE(plan.discard_shop_item);
    EXPECT_TRUE(plan.db_rare_insert);
    EXPECT_TRUE(plan.log_item_money);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 5u);
    EXPECT_EQ(plan.effects[0].kind,
              RareCreateSideEffectKind::GenerateRareOption);
    EXPECT_EQ(plan.effects[1].kind,
              RareCreateSideEffectKind::DiscardShopItem);
    EXPECT_EQ(plan.effects[2].kind,
              RareCreateSideEffectKind::ShopItemRareInsertToDB);
    EXPECT_EQ(plan.effects[3].kind,
              RareCreateSideEffectKind::LogItemMoney);
    EXPECT_EQ(plan.effects[4].kind,
              RareCreateSideEffectKind::SendUseAckToPlayer);
    EXPECT_EQ(plan.effects[0].target_w_icon_idx, 4321u);
    EXPECT_EQ(plan.effects[2].target_position, 17u);
    EXPECT_EQ(plan.effects[2].target_db_idx, 5555u);

    RecordingSink sink;
    auto out = apply_rare_create_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 5u);
    EXPECT_EQ(out.generated, 1u);
    EXPECT_EQ(out.discards, 1u);
    EXPECT_EQ(out.db_inserts, 1u);
    EXPECT_EQ(out.money_logs, 1u);
    EXPECT_EQ(out.use_acks_sent, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_TRUE(out.use_ack_flag_consumed);
    EXPECT_TRUE(out.generate_flag_consumed);
    EXPECT_TRUE(out.discard_flag_consumed);
    EXPECT_TRUE(out.db_flag_consumed);
    EXPECT_TRUE(out.log_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>(
                  {"gen", "discard", "db", "log", "useack"}));
    EXPECT_EQ(sink.last_player_id, 0x00080009u);
    EXPECT_EQ(sink.last_target_w_icon_idx, 4321u);
    EXPECT_EQ(sink.last_target_position, 17u);
    EXPECT_EQ(sink.last_target_db_idx, 5555u);
    EXPECT_EQ(sink.last_shop_item_idx, 200u);
    EXPECT_EQ(sink.last_shop_item_pos, 3u);
    EXPECT_EQ(sink.use_ack_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyRareCreateSideEffects, FirstFiveGateNackCodes) {
    struct Case {
        void (*mutate)(RareCreateValidationInput&);
        std::uint32_t expected_code;
    };
    const Case cases[] = {
        {[](RareCreateValidationInput& i) { i.shop_item_is_useable = false; }, 1u},
        {[](RareCreateValidationInput& i) { i.shop_item_exists = false; }, 2u},
        {[](RareCreateValidationInput& i) { i.shop_item_info_exists = false; }, 3u},
        {[](RareCreateValidationInput& i) { i.shop_item_icon_is_create_50_70_90_99 = false; }, 4u},
        {[](RareCreateValidationInput& i) { i.target_is_equip_kind = false; }, 5u},
    };
    for (const auto& c : cases) {
        auto in = PassingGates();
        c.mutate(in);
        auto plan = rare_create_side_effect_plan(
            in, 7, 1, 2, 3, 4, 5);
        EXPECT_TRUE(plan.send_nack);
        EXPECT_FALSE(plan.send_use_ack);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind,
                  RareCreateSideEffectKind::SendNackToPlayer);
        EXPECT_EQ(plan.effects[0].nack_code, c.expected_code);

        RecordingSink sink;
        (void)apply_rare_create_side_effects(plan, sink);
        EXPECT_EQ(sink.last_nack_code, c.expected_code);
    }
}

TEST(ApplyRareCreateSideEffects, RemainingGateNackCodes) {
    struct Case {
        void (*mutate)(RareCreateValidationInput&);
        std::uint32_t expected_code;
    };
    const Case cases[] = {
        {[](RareCreateValidationInput& i) { i.target_durability_zero = false; }, 6u},
        {[](RareCreateValidationInput& i) { i.target_option_idx_zero = false; }, 6u},
        {[](RareCreateValidationInput& i) { i.target_w_icon_idx_suffix_zero = false; }, 7u},
        {[](RareCreateValidationInput& i) { i.level_in_range = false; }, 8u},
        {[](RareCreateValidationInput& i) { i.is_rare_item_able = false; }, 9u},
        {[](RareCreateValidationInput& i) { i.get_rare_returned_true = false; }, 10u},
        {[](RareCreateValidationInput& i) { i.discard_returned_true = false; }, 11u},
    };
    for (const auto& c : cases) {
        auto in = PassingGates();
        c.mutate(in);
        auto plan = rare_create_side_effect_plan(
            in, 7, 1, 2, 3, 4, 5);
        EXPECT_TRUE(plan.send_nack);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].nack_code, c.expected_code);

        RecordingSink sink;
        (void)apply_rare_create_side_effects(plan, sink);
        EXPECT_EQ(sink.last_nack_code, c.expected_code);
    }
}

TEST(ApplyRareCreateSideEffects, GatePrecedenceLocked) {
    // Not-usable outranks bad item; bad item outranks missing info.
    auto in = PassingGates();
    in.shop_item_is_useable = false;
    in.shop_item_exists = false;
    auto plan = rare_create_side_effect_plan(in, 7, 1, 2, 3, 4, 5);
    EXPECT_EQ(plan.effects[0].nack_code, 1u);

    auto in2 = PassingGates();
    in2.shop_item_exists = false;
    in2.shop_item_info_exists = false;
    auto plan2 = rare_create_side_effect_plan(in2, 7, 1, 2, 3, 4, 5);
    EXPECT_EQ(plan2.effects[0].nack_code, 2u);

    // Already rare outranks wrong suffix.
    auto in3 = PassingGates();
    in3.target_durability_zero = false;
    in3.target_w_icon_idx_suffix_zero = false;
    auto plan3 = rare_create_side_effect_plan(in3, 7, 1, 2, 3, 4, 5);
    EXPECT_EQ(plan3.effects[0].nack_code, 6u);
}

TEST(ApplyRareCreateSideEffects, EmptyPlanIsNoOp) {
    mxh::server::RareCreateSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_rare_create_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.use_acks_sent, 0u);
    EXPECT_EQ(out.generated, 0u);
    EXPECT_EQ(out.discards, 0u);
    EXPECT_EQ(out.db_inserts, 0u);
    EXPECT_EQ(out.money_logs, 0u);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_FALSE(out.use_ack_flag_consumed);
    EXPECT_FALSE(out.generate_flag_consumed);
    EXPECT_FALSE(out.discard_flag_consumed);
    EXPECT_FALSE(out.db_flag_consumed);
    EXPECT_FALSE(out.log_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}
