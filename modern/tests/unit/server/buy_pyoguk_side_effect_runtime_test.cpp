// buy_pyoguk_side_effect_runtime_test.cpp
//
// Verifies apply_buy_pyoguk_side_effects() (the runtime orchestrator
// for the CPyoGukManager::BuyPyogukSyn side-effect chain) walks the
// data-plane plan and dispatches each entry: the 5-step success
// chain in legacy order / NACK for the cap or money gate.

#include <mxh/server/buy_pyoguk_side_effect.hpp>
#include <mxh/server/buy_pyoguk_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::BuyPyogukSideEffectKind;
using mxh::server::BuyPyogukSideEffectSink;
using mxh::server::LEGACY_TAB_PYOGUK_NUM;
using mxh::server::apply_buy_pyoguk_side_effects;
using mxh::server::buy_pyoguk_side_effect_plan;

class RecordingSink final : public BuyPyogukSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_player_id = 0;
    std::uint64_t last_buy_price = 0;
    std::uint8_t last_new_pyoguk_num = 0;
    std::uint64_t last_new_max_money = 0;
    std::size_t ack_count = 0;
    std::size_t nack_count = 0;

    void subtract_buy_price(std::uint32_t player_id,
                            std::uint64_t buy_price) override {
        calls.push_back("sub");
        last_player_id = player_id;
        last_buy_price = buy_price;
    }
    void increment_pyoguk_num(std::uint32_t player_id,
                              std::uint8_t new_pyoguk_num) override {
        calls.push_back("inc");
        last_player_id = player_id;
        last_new_pyoguk_num = new_pyoguk_num;
    }
    void update_max_purse_money(std::uint32_t player_id,
                                std::uint8_t new_pyoguk_num,
                                std::uint64_t new_max_money) override {
        calls.push_back("max");
        last_player_id = player_id;
        last_new_pyoguk_num = new_pyoguk_num;
        last_new_max_money = new_max_money;
    }
    void insert_pyoguk_buy_db(std::uint32_t player_id,
                              std::uint8_t new_pyoguk_num) override {
        calls.push_back("db");
        last_player_id = player_id;
        last_new_pyoguk_num = new_pyoguk_num;
    }
    void broadcast_buy_ack(std::uint32_t player_id,
                           std::uint8_t new_pyoguk_num) override {
        calls.push_back("ack");
        last_player_id = player_id;
        last_new_pyoguk_num = new_pyoguk_num;
        ++ack_count;
    }
    void broadcast_buy_nack(std::uint32_t player_id) override {
        calls.push_back("nack");
        last_player_id = player_id;
        ++nack_count;
    }
};

}  // namespace

TEST(ApplyBuyPyogukSideEffects, SuccessEmitsFiveStepChainInOrder) {
    auto plan = buy_pyoguk_side_effect_plan(
        /*current_pyoguk_num=*/2, /*inven_money=*/10000,
        /*buy_price=*/3000, /*new_max_money_after_buy=*/600000);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 5u);
    EXPECT_EQ(plan.effects[0].kind,
              BuyPyogukSideEffectKind::SubtractBuyPrice);
    EXPECT_EQ(plan.effects[1].kind,
              BuyPyogukSideEffectKind::IncrementPyogukNum);
    EXPECT_EQ(plan.effects[2].kind,
              BuyPyogukSideEffectKind::UpdateMaxPurseMoney);
    EXPECT_EQ(plan.effects[3].kind,
              BuyPyogukSideEffectKind::InsertPyogukBuyDB);
    EXPECT_EQ(plan.effects[4].kind,
              BuyPyogukSideEffectKind::BroadcastBuyAck);
    EXPECT_EQ(plan.effects[0].buy_price, 3000u);
    EXPECT_EQ(plan.effects[1].new_pyoguk_num, 3u);
    EXPECT_EQ(plan.effects[2].new_max_money, 600000u);

    RecordingSink sink;
    auto out = apply_buy_pyoguk_side_effects(
        /*player_id=*/0x00020001u, plan, sink);
    EXPECT_EQ(out.effects_applied, 5u);
    EXPECT_EQ(out.subtractions, 1u);
    EXPECT_EQ(out.increments, 1u);
    EXPECT_EQ(out.max_money_updates, 1u);
    EXPECT_EQ(out.db_inserts, 1u);
    EXPECT_EQ(out.acks_sent, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_TRUE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>({"sub", "inc", "max", "db", "ack"}));
    EXPECT_EQ(sink.last_player_id, 0x00020001u);
    EXPECT_EQ(sink.last_buy_price, 3000u);
    EXPECT_EQ(sink.last_new_pyoguk_num, 3u);
    EXPECT_EQ(sink.last_new_max_money, 600000u);
    EXPECT_EQ(sink.ack_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyBuyPyogukSideEffects, PyogukCapFailureEmitsNack) {
    auto plan = buy_pyoguk_side_effect_plan(
        /*current_pyoguk_num=*/LEGACY_TAB_PYOGUK_NUM,
        /*inven_money=*/1000000, /*buy_price=*/100,
        /*new_max_money_after_buy=*/0);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              BuyPyogukSideEffectKind::BroadcastBuyNack);

    RecordingSink sink;
    auto out = apply_buy_pyoguk_side_effects(7u, plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"nack"}));
    EXPECT_EQ(sink.last_player_id, 7u);
}

TEST(ApplyBuyPyogukSideEffects, InsufficientMoneyEmitsNack) {
    auto plan = buy_pyoguk_side_effect_plan(
        /*current_pyoguk_num=*/1, /*inven_money=*/2999,
        /*buy_price=*/3000, /*new_max_money_after_buy=*/0);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              BuyPyogukSideEffectKind::BroadcastBuyNack);

    RecordingSink sink;
    (void)apply_buy_pyoguk_side_effects(8u, plan, sink);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"nack"}));
    EXPECT_EQ(sink.nack_count, 1u);
}

TEST(ApplyBuyPyogukSideEffects, CapGatePrecedesMoneyGate) {
    // Cap failure wins even when money is plentiful (legacy gate
    // order: pyoguknum >= TAB_PYOGUK_NUM checked first).
    auto plan = buy_pyoguk_side_effect_plan(
        /*current_pyoguk_num=*/LEGACY_TAB_PYOGUK_NUM,
        /*inven_money=*/UINT64_MAX, /*buy_price=*/1,
        /*new_max_money_after_buy=*/0);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_FALSE(plan.send_ack);
}

TEST(ApplyBuyPyogukSideEffects, BoundaryCapValue) {
    // current == TAB_PYOGUK_NUM - 1 is the last allowed slot.
    auto ok = buy_pyoguk_side_effect_plan(
        LEGACY_TAB_PYOGUK_NUM - 1, 5000, 100, 999);
    EXPECT_TRUE(ok.send_ack);
    EXPECT_EQ(ok.effects[1].new_pyoguk_num, LEGACY_TAB_PYOGUK_NUM);

    // current == TAB_PYOGUK_NUM -> cap NACK.
    auto capped = buy_pyoguk_side_effect_plan(
        LEGACY_TAB_PYOGUK_NUM, 5000, 100, 999);
    EXPECT_TRUE(capped.send_nack);
}

TEST(ApplyBuyPyogukSideEffects, EmptyPlanIsNoOp) {
    mxh::server::BuyPyogukSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_buy_pyoguk_side_effects(3u, plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.subtractions, 0u);
    EXPECT_EQ(out.increments, 0u);
    EXPECT_EQ(out.max_money_updates, 0u);
    EXPECT_EQ(out.db_inserts, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyBuyPyogukSideEffects, NackDoesNotTouchAckState) {
    auto nack_plan = buy_pyoguk_side_effect_plan(
        LEGACY_TAB_PYOGUK_NUM, 100, 10, 0);
    RecordingSink nack_sink;
    auto nack_out =
        apply_buy_pyoguk_side_effects(1u, nack_plan, nack_sink);
    EXPECT_EQ(nack_out.acks_sent, 0u);
    EXPECT_EQ(nack_sink.ack_count, 0u);
    EXPECT_EQ(nack_out.nacks_sent, 1u);

    auto ack_plan = buy_pyoguk_side_effect_plan(0, 100, 10, 1000);
    RecordingSink ack_sink;
    auto ack_out = apply_buy_pyoguk_side_effects(2u, ack_plan, ack_sink);
    EXPECT_EQ(ack_out.nacks_sent, 0u);
    EXPECT_EQ(ack_sink.nack_count, 0u);
    EXPECT_EQ(ack_out.acks_sent, 1u);
}
