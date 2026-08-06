// D4.44 PyogukMoney (MP_PYOGUK_PUTIN_MONEY_SYN / PUTOUT_MONEY_SYN) data
// plane + side-effect dispatcher tests.

#include <mxh/server/pyoguk_money_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

// ---------- PutIn clamp ----------

TEST(PyogukPutInClamp, RequestedBelowAllCapsStaysRequested) {
    EXPECT_EQ(pyoguk_put_in_clamp(100, 1000, 0, 10000), 100u);
}

TEST(PyogukPutInClamp, CapsAtInvenMoney) {
    EXPECT_EQ(pyoguk_put_in_clamp(2000, 500, 0, 10000), 500u);
}

TEST(PyogukPutInClamp, CapsAtPyogukSpace) {
    EXPECT_EQ(pyoguk_put_in_clamp(8000, 10000, 9000, 10000), 1000u);
}

TEST(PyogukPutInClamp, CapsAtPyogukAlreadyFull) {
    // legacy: maxmon - pyogukmon < 0 yields setMoney = 0 (NACK)
    EXPECT_EQ(pyoguk_put_in_clamp(100, 500, 10000, 10000), 0u);
}

TEST(PyogukPutInClamp, ZeroRequestedYieldsZero) {
    EXPECT_EQ(pyoguk_put_in_clamp(0, 1000, 0, 10000), 0u);
}

// ---------- PutOut clamp ----------

TEST(PyogukPutOutClamp, RequestedBelowAllCapsStaysRequested) {
    EXPECT_EQ(pyoguk_put_out_clamp(100, 500, 0, 10000), 100u);
}

TEST(PyogukPutOutClamp, CapsAtPyogukMoney) {
    EXPECT_EQ(pyoguk_put_out_clamp(2000, 500, 0, 10000), 500u);
}

TEST(PyogukPutOutClamp, CapsAtInvenSpace) {
    EXPECT_EQ(pyoguk_put_out_clamp(8000, 10000, 9000, 10000), 1000u);
}

TEST(PyogukPutOutClamp, CapsAtInvenAlreadyFull) {
    EXPECT_EQ(pyoguk_put_out_clamp(100, 5000, 10000, 10000), 0u);
}

// ---------- PutIn plan ----------

TEST(PyogukPutInPlan, SuccessEmitsSixSteps) {
    auto plan = pyoguk_money_put_in_side_effect_plan(
        /*requested=*/100, /*inven_mon=*/1000,
        /*pyoguk_mon=*/0, /*pyoguk_max=*/10000);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_FALSE(plan.silent);
    EXPECT_EQ(plan.source, PyogukMoneySource::PutIn);
    ASSERT_EQ(plan.effects.size(), 6u);
    EXPECT_EQ(plan.effects[0].kind,
              PyogukMoneySideEffectKind::SubtractFromSource);
    EXPECT_EQ(plan.effects[0].amount, 100u);
    EXPECT_EQ(plan.effects[0].new_inven_money, 900u);
    EXPECT_EQ(plan.effects[1].kind,
              PyogukMoneySideEffectKind::AddToTarget);
    EXPECT_EQ(plan.effects[1].new_pyoguk_money, 100u);
    EXPECT_EQ(plan.effects[2].kind,
              PyogukMoneySideEffectKind::UpdatePyogukMoneyDB);
    EXPECT_EQ(plan.effects[2].new_pyoguk_money, 100u);
    EXPECT_EQ(plan.effects[3].kind,
              PyogukMoneySideEffectKind::InsertLogMoney);
    EXPECT_EQ(plan.effects[3].money_log_code,
              LEGACY_EMONEYLOG_LOSE_PYOGUK);
    EXPECT_EQ(plan.effects[4].kind,
              PyogukMoneySideEffectKind::LogItemMoney);
    EXPECT_EQ(plan.effects[4].item_log_code,
              LEGACY_ELOG_ITEM_MOVE_INVEN_TO_PYOGUK);
    EXPECT_EQ(plan.effects[5].kind,
              PyogukMoneySideEffectKind::BroadcastAck);
}

TEST(PyogukPutInPlan, ZeroClampedEmitsSingleNack) {
    auto plan = pyoguk_money_put_in_side_effect_plan(
        /*requested=*/100, /*inven_mon=*/1000,
        /*pyoguk_mon=*/10000, /*pyoguk_max=*/10000);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_FALSE(plan.silent);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              PyogukMoneySideEffectKind::BroadcastNack);
}

// ---------- PutOut plan ----------

TEST(PyogukPutOutPlan, SuccessEmitsSixSteps) {
    auto plan = pyoguk_money_put_out_side_effect_plan(
        /*requested=*/50, /*pyoguk_mon=*/500,
        /*inven_mon=*/0, /*inven_max=*/10000);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_FALSE(plan.silent);
    EXPECT_EQ(plan.source, PyogukMoneySource::PutOut);
    ASSERT_EQ(plan.effects.size(), 6u);
    EXPECT_EQ(plan.effects[0].kind,
              PyogukMoneySideEffectKind::SubtractFromSource);
    EXPECT_EQ(plan.effects[0].amount, 50u);
    EXPECT_EQ(plan.effects[0].new_pyoguk_money, 450u);
    EXPECT_EQ(plan.effects[1].kind,
              PyogukMoneySideEffectKind::AddToTarget);
    EXPECT_EQ(plan.effects[1].new_inven_money, 50u);
    EXPECT_EQ(plan.effects[3].kind,
              PyogukMoneySideEffectKind::InsertLogMoney);
    EXPECT_EQ(plan.effects[3].money_log_code,
              LEGACY_EMONEYLOG_GET_PYOGUK);
    EXPECT_EQ(plan.effects[4].kind,
              PyogukMoneySideEffectKind::LogItemMoney);
    EXPECT_EQ(plan.effects[4].item_log_code,
              LEGACY_ELOG_ITEM_MOVE_PYOGUK_TO_INVEN);
}

TEST(PyogukPutOutPlan, ZeroClampedIsSilent) {
    auto plan = pyoguk_money_put_out_side_effect_plan(
        /*requested=*/100, /*pyoguk_mon=*/500,
        /*inven_mon=*/10000, /*inven_max=*/10000);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.silent);
    EXPECT_TRUE(plan.effects.empty());
}

TEST(PyogukPutOutPlan, Idempotent) {
    auto a = pyoguk_money_put_out_side_effect_plan(
        50, 500, 0, 10000);
    auto b = pyoguk_money_put_out_side_effect_plan(
        50, 500, 0, 10000);
    EXPECT_EQ(a.send_ack, b.send_ack);
    EXPECT_EQ(a.send_nack, b.send_nack);
    EXPECT_EQ(a.silent, b.silent);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].amount, b.effects[i].amount);
    }
}
