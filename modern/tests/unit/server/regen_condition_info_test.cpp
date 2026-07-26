// regen_condition_info_test.cpp - Phase D6 RegenConditionInfo 1:1 port tests.

#include "mxh/server/regen_condition_info.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::RegenConditionInfo;
using mxh::server::regen_condition_info_init;
using mxh::server::regen_condition_info_should_regen;

TEST(RegenConditionInfo, DefaultConstructZeroesAll) {
    RegenConditionInfo c{};
    EXPECT_EQ(c.dwTargetGroupID, 0u);
    EXPECT_FLOAT_EQ(c.fRemainderRatio, 0.0f);
    EXPECT_EQ(c.dwStartRegenTick, 0u);
    EXPECT_EQ(c.dwRegenDelay, 0u);
    EXPECT_FALSE(c.bRegen);
}

TEST(RegenConditionInfo, InitMatchesLegacyDefaults) {
    RegenConditionInfo c{};
    c.dwTargetGroupID = 7u;
    c.fRemainderRatio = 0.5f;
    regen_condition_info_init(c);
    EXPECT_EQ(c.dwTargetGroupID, 0u);
    EXPECT_FLOAT_EQ(c.fRemainderRatio, 0.0f);
    EXPECT_FALSE(c.bRegen);
}

TEST(RegenConditionInfo, ShouldRegenFalseWhenDisabled) {
    RegenConditionInfo c{};
    c.bRegen = false;
    c.dwStartRegenTick = 0u;
    c.dwRegenDelay     = 0u;
    EXPECT_FALSE(regen_condition_info_should_regen(c, 100u, 0u));
}

TEST(RegenConditionInfo, ShouldRegenFalseWhenAliveExist) {
    RegenConditionInfo c{};
    c.bRegen = true;
    EXPECT_FALSE(regen_condition_info_should_regen(c, 1000u, 1u));
}

TEST(RegenConditionInfo, ShouldRegenTrueAtZeroDelay) {
    RegenConditionInfo c{};
    c.bRegen = true;
    c.dwRegenDelay = 0u;
    EXPECT_TRUE(regen_condition_info_should_regen(c, 100u, 0u));
}

TEST(RegenConditionInfo, ShouldRegenBeforeDelay) {
    RegenConditionInfo c{};
    c.bRegen = true;
    c.dwStartRegenTick = 100u;
    c.dwRegenDelay     = 500u;
    EXPECT_FALSE(regen_condition_info_should_regen(c, 200u, 0u));
}

TEST(RegenConditionInfo, ShouldRegenAfterDelay) {
    RegenConditionInfo c{};
    c.bRegen = true;
    c.dwStartRegenTick = 100u;
    c.dwRegenDelay     = 500u;
    EXPECT_TRUE(regen_condition_info_should_regen(c, 600u, 0u));
}

TEST(RegenConditionInfo, ShouldRegenAtExactDelay) {
    RegenConditionInfo c{};
    c.bRegen = true;
    c.dwStartRegenTick = 100u;
    c.dwRegenDelay     = 500u;
    EXPECT_TRUE(regen_condition_info_should_regen(c, 600u, 0u));
}

}  // namespace
