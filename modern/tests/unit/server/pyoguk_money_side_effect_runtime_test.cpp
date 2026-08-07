// pyoguk_money_side_effect_runtime_test.cpp
//
// Verifies apply_pyoguk_money_side_effects() (the runtime
// orchestrator for the CPyoGukManager::PutInMoneyPyoguk /
// PutOutMoneyPyoguk side-effect chains) walks the data-plane plan and
// dispatches each entry to the correct subsystem: the 6-step success
// chain in legacy order / put-in NACK when clamped to 0 / silent
// put-out when clamped to 0.

#include <mxh/server/pyoguk_money_side_effect.hpp>
#include <mxh/server/pyoguk_money_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::LEGACY_ELOG_ITEM_MOVE_INVEN_TO_PYOGUK;
using mxh::server::LEGACY_ELOG_ITEM_MOVE_PYOGUK_TO_INVEN;
using mxh::server::LEGACY_EMONEYLOG_GET_PYOGUK;
using mxh::server::LEGACY_EMONEYLOG_LOSE_PYOGUK;
using mxh::server::PyogukMoneySideEffectKind;
using mxh::server::PyogukMoneySideEffectSink;
using mxh::server::PyogukMoneySource;
using mxh::server::apply_pyoguk_money_side_effects;
using mxh::server::pyoguk_money_put_in_side_effect_plan;
using mxh::server::pyoguk_money_put_out_side_effect_plan;

class RecordingSink final : public PyogukMoneySideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_player_id = 0;
    PyogukMoneySource last_source = PyogukMoneySource::PutIn;
    std::uint64_t last_amount = 0;
    std::uint64_t last_new_inven = 0;
    std::uint64_t last_new_pyoguk = 0;
    std::uint32_t last_money_log_code = 0;
    std::uint32_t last_item_log_code = 0;
    std::size_t ack_count = 0;
    std::size_t nack_count = 0;
    std::size_t skip_count = 0;

    void subtract_from_source(std::uint32_t player_id,
                              PyogukMoneySource source,
                              std::uint64_t amount,
                              std::uint64_t new_inven_money,
                              std::uint64_t new_pyoguk_money) override {
        calls.push_back("sub");
        last_player_id = player_id;
        last_source = source;
        last_amount = amount;
        last_new_inven = new_inven_money;
        last_new_pyoguk = new_pyoguk_money;
    }
    void add_to_target(std::uint32_t player_id,
                       PyogukMoneySource source,
                       std::uint64_t amount,
                       std::uint64_t new_inven_money,
                       std::uint64_t new_pyoguk_money) override {
        calls.push_back("add");
        last_player_id = player_id;
        last_source = source;
        last_amount = amount;
        last_new_inven = new_inven_money;
        last_new_pyoguk = new_pyoguk_money;
    }
    void update_pyoguk_money_db(std::uint32_t player_id,
                                std::uint64_t amount,
                                std::uint64_t new_pyoguk_money) override {
        calls.push_back("db");
        last_player_id = player_id;
        last_amount = amount;
        last_new_pyoguk = new_pyoguk_money;
    }
    void insert_log_money(std::uint32_t player_id,
                          std::uint32_t money_log_code,
                          std::uint64_t amount,
                          std::uint64_t new_inven_money,
                          std::uint64_t new_pyoguk_money) override {
        calls.push_back("logm");
        last_player_id = player_id;
        last_money_log_code = money_log_code;
        last_amount = amount;
        last_new_inven = new_inven_money;
        last_new_pyoguk = new_pyoguk_money;
    }
    void log_item_money(std::uint32_t player_id,
                        std::uint32_t item_log_code,
                        std::uint64_t amount,
                        std::uint64_t new_inven_money,
                        std::uint64_t new_pyoguk_money) override {
        calls.push_back("logi");
        last_player_id = player_id;
        last_item_log_code = item_log_code;
        last_amount = amount;
        last_new_inven = new_inven_money;
        last_new_pyoguk = new_pyoguk_money;
    }
    void broadcast_ack(std::uint32_t player_id,
                       PyogukMoneySource source,
                       std::uint64_t amount,
                       std::uint64_t new_pyoguk_money) override {
        calls.push_back("ack");
        last_player_id = player_id;
        last_source = source;
        last_amount = amount;
        last_new_pyoguk = new_pyoguk_money;
        ++ack_count;
    }
    void broadcast_nack(std::uint32_t player_id,
                        PyogukMoneySource source) override {
        calls.push_back("nack");
        last_player_id = player_id;
        last_source = source;
        ++nack_count;
    }
    void silent_skip(std::uint32_t player_id,
                     PyogukMoneySource source) override {
        calls.push_back("skip");
        last_player_id = player_id;
        last_source = source;
        ++skip_count;
    }
};

}  // namespace

TEST(ApplyPyogukMoneySideEffects, PutInSuccessEmitsSixStepChainInOrder) {
    auto plan = pyoguk_money_put_in_side_effect_plan(
        /*requested=*/1000, /*inven_money=*/5000,
        /*pyoguk_money=*/2000, /*pyoguk_max=*/10000);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_FALSE(plan.silent);
    EXPECT_EQ(plan.source, PyogukMoneySource::PutIn);
    ASSERT_EQ(plan.effects.size(), 6u);
    EXPECT_EQ(plan.effects[0].kind,
              PyogukMoneySideEffectKind::SubtractFromSource);
    EXPECT_EQ(plan.effects[1].kind,
              PyogukMoneySideEffectKind::AddToTarget);
    EXPECT_EQ(plan.effects[2].kind,
              PyogukMoneySideEffectKind::UpdatePyogukMoneyDB);
    EXPECT_EQ(plan.effects[3].kind,
              PyogukMoneySideEffectKind::InsertLogMoney);
    EXPECT_EQ(plan.effects[4].kind,
              PyogukMoneySideEffectKind::LogItemMoney);
    EXPECT_EQ(plan.effects[5].kind,
              PyogukMoneySideEffectKind::BroadcastAck);

    RecordingSink sink;
    auto out = apply_pyoguk_money_side_effects(
        /*player_id=*/0x00010002u, plan, sink);
    EXPECT_EQ(out.effects_applied, 6u);
    EXPECT_EQ(out.subtractions, 1u);
    EXPECT_EQ(out.additions, 1u);
    EXPECT_EQ(out.db_updates, 1u);
    EXPECT_EQ(out.money_logs, 1u);
    EXPECT_EQ(out.item_logs, 1u);
    EXPECT_EQ(out.acks_sent, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.silent_skips, 0u);
    EXPECT_TRUE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_FALSE(out.silent_flag_consumed);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>(
                  {"sub", "add", "db", "logm", "logi", "ack"}));
    EXPECT_EQ(sink.last_player_id, 0x00010002u);
    EXPECT_EQ(sink.last_source, PyogukMoneySource::PutIn);
    EXPECT_EQ(sink.last_amount, 1000u);
    EXPECT_EQ(sink.last_new_inven, 4000u);
    EXPECT_EQ(sink.last_new_pyoguk, 3000u);
    EXPECT_EQ(sink.last_money_log_code, LEGACY_EMONEYLOG_LOSE_PYOGUK);
    EXPECT_EQ(sink.last_item_log_code,
              LEGACY_ELOG_ITEM_MOVE_INVEN_TO_PYOGUK);
    EXPECT_EQ(sink.ack_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
    EXPECT_EQ(sink.skip_count, 0u);
}

TEST(ApplyPyogukMoneySideEffects, PutInZeroEmitsNackOnly) {
    auto plan = pyoguk_money_put_in_side_effect_plan(
        /*requested=*/0, /*inven_money=*/5000,
        /*pyoguk_money=*/2000, /*pyoguk_max=*/10000);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              PyogukMoneySideEffectKind::BroadcastNack);

    RecordingSink sink;
    auto out = apply_pyoguk_money_side_effects(7u, plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_TRUE(out.nack_flag_consumed);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"nack"}));
    EXPECT_EQ(sink.last_player_id, 7u);
    EXPECT_EQ(sink.last_source, PyogukMoneySource::PutIn);
}

TEST(ApplyPyogukMoneySideEffects, PutInClampsToSourceAndSpace) {
    // requested > inven money -> clamped to 5000 (source cap).
    auto plan = pyoguk_money_put_in_side_effect_plan(
        10000, 5000, 2000, 10000);
    EXPECT_EQ(plan.effects[0].amount, 5000u);
    EXPECT_EQ(plan.effects[0].new_inven_money, 0u);
    EXPECT_EQ(plan.effects[1].new_pyoguk_money, 7000u);
    EXPECT_EQ(plan.effects[5].new_pyoguk_money, 7000u);

    // pyoguk space (10000-9500=500) caps below the source cap.
    auto plan2 = pyoguk_money_put_in_side_effect_plan(
        10000, 5000, 9500, 10000);
    EXPECT_EQ(plan2.effects[0].amount, 500u);
    EXPECT_EQ(plan2.effects[0].new_inven_money, 4500u);
    EXPECT_EQ(plan2.effects[1].new_pyoguk_money, 10000u);
    EXPECT_EQ(plan2.effects[5].new_pyoguk_money, 10000u);

    // Both caps exhausted -> NACK (clamped to 0).
    auto plan3 = pyoguk_money_put_in_side_effect_plan(
        10000, 0, 9500, 10000);
    EXPECT_TRUE(plan3.send_nack);
    ASSERT_EQ(plan3.effects.size(), 1u);
    EXPECT_EQ(plan3.effects[0].kind,
              PyogukMoneySideEffectKind::BroadcastNack);
}

TEST(ApplyPyogukMoneySideEffects, PutOutSuccessEmitsSixStepChainInOrder) {
    auto plan = pyoguk_money_put_out_side_effect_plan(
        /*requested=*/500, /*pyoguk_money=*/2000,
        /*inven_money=*/5000, /*inven_max=*/10000);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_FALSE(plan.silent);
    EXPECT_EQ(plan.source, PyogukMoneySource::PutOut);
    ASSERT_EQ(plan.effects.size(), 6u);
    EXPECT_EQ(plan.effects[0].kind,
              PyogukMoneySideEffectKind::SubtractFromSource);
    EXPECT_EQ(plan.effects[1].kind,
              PyogukMoneySideEffectKind::AddToTarget);
    EXPECT_EQ(plan.effects[2].kind,
              PyogukMoneySideEffectKind::UpdatePyogukMoneyDB);
    EXPECT_EQ(plan.effects[3].kind,
              PyogukMoneySideEffectKind::InsertLogMoney);
    EXPECT_EQ(plan.effects[4].kind,
              PyogukMoneySideEffectKind::LogItemMoney);
    EXPECT_EQ(plan.effects[5].kind,
              PyogukMoneySideEffectKind::BroadcastAck);

    RecordingSink sink;
    auto out = apply_pyoguk_money_side_effects(42u, plan, sink);
    EXPECT_EQ(out.effects_applied, 6u);
    EXPECT_EQ(out.acks_sent, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>(
                  {"sub", "add", "db", "logm", "logi", "ack"}));
    EXPECT_EQ(sink.last_player_id, 42u);
    EXPECT_EQ(sink.last_source, PyogukMoneySource::PutOut);
    EXPECT_EQ(sink.last_amount, 500u);
    EXPECT_EQ(sink.last_new_inven, 5500u);
    EXPECT_EQ(sink.last_new_pyoguk, 1500u);
    EXPECT_EQ(sink.last_money_log_code, LEGACY_EMONEYLOG_GET_PYOGUK);
    EXPECT_EQ(sink.last_item_log_code,
              LEGACY_ELOG_ITEM_MOVE_PYOGUK_TO_INVEN);
}

TEST(ApplyPyogukMoneySideEffects, PutOutZeroIsSilentSkip) {
    auto plan = pyoguk_money_put_out_side_effect_plan(
        /*requested=*/0, /*pyoguk_money=*/2000,
        /*inven_money=*/5000, /*inven_max=*/10000);
    EXPECT_TRUE(plan.silent);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_pyoguk_money_side_effects(9u, plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.silent_skips, 1u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_TRUE(out.silent_flag_consumed);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"skip"}));
    EXPECT_EQ(sink.last_player_id, 9u);
    EXPECT_EQ(sink.last_source, PyogukMoneySource::PutOut);
}

TEST(ApplyPyogukMoneySideEffects, PutOutClampsToPyogukAndInvenSpace) {
    // requested > pyoguk money -> clamped to 2000 (source cap).
    auto plan = pyoguk_money_put_out_side_effect_plan(
        10000, 2000, 5000, 10000);
    EXPECT_EQ(plan.effects[0].amount, 2000u);
    EXPECT_EQ(plan.effects[0].new_pyoguk_money, 0u);
    EXPECT_EQ(plan.effects[1].new_inven_money, 7000u);
    EXPECT_EQ(plan.effects[5].new_pyoguk_money, 0u);

    // inven space (10000-9000=1000) caps below the source cap.
    auto plan2 = pyoguk_money_put_out_side_effect_plan(
        10000, 2000, 9000, 10000);
    EXPECT_EQ(plan2.effects[0].amount, 1000u);
    EXPECT_EQ(plan2.effects[0].new_pyoguk_money, 1000u);
    EXPECT_EQ(plan2.effects[1].new_inven_money, 10000u);
    EXPECT_EQ(plan2.effects[5].new_pyoguk_money, 1000u);

    // Inven full -> silent (clamped to 0).
    auto plan3 = pyoguk_money_put_out_side_effect_plan(
        10000, 2000, 10000, 10000);
    EXPECT_TRUE(plan3.silent);
    EXPECT_TRUE(plan3.effects.empty());
}

TEST(ApplyPyogukMoneySideEffects, NackDoesNotTouchAckState) {
    auto nack_plan = pyoguk_money_put_in_side_effect_plan(
        0, 0, 0, 10000);
    RecordingSink nack_sink;
    auto nack_out = apply_pyoguk_money_side_effects(1u, nack_plan, nack_sink);
    EXPECT_EQ(nack_out.acks_sent, 0u);
    EXPECT_EQ(nack_sink.ack_count, 0u);
    EXPECT_EQ(nack_out.nacks_sent, 1u);

    auto ack_plan = pyoguk_money_put_in_side_effect_plan(
        100, 500, 0, 10000);
    RecordingSink ack_sink;
    auto ack_out = apply_pyoguk_money_side_effects(2u, ack_plan, ack_sink);
    EXPECT_EQ(ack_out.nacks_sent, 0u);
    EXPECT_EQ(ack_sink.nack_count, 0u);
    EXPECT_EQ(ack_out.acks_sent, 1u);
}

TEST(ApplyPyogukMoneySideEffects, EmptyPlanIsNoOp) {
    mxh::server::PyogukMoneySideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_pyoguk_money_side_effects(3u, plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.subtractions, 0u);
    EXPECT_EQ(out.additions, 0u);
    EXPECT_EQ(out.db_updates, 0u);
    EXPECT_EQ(out.money_logs, 0u);
    EXPECT_EQ(out.item_logs, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.silent_skips, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_FALSE(out.silent_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}
