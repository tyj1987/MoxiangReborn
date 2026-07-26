// state_param_test.cpp - Phase D6 StateParam 1:1 port tests.

#include "mxh/server/state_param.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::StateParam;
using mxh::server::state_param_init;
using mxh::server::state_param_set_state;
using mxh::server::state_param_elapsed;

TEST(StateParam, DefaultFieldsZeroed) {
    StateParam s{};
    EXPECT_EQ(s.stateNew, 0u);
    EXPECT_EQ(s.stateCur, 0u);
    EXPECT_EQ(s.stateOld, 0u);
    EXPECT_FALSE(s.bStateUpdate);
}

TEST(StateParam, InitMatchesLegacyDefaults) {
    StateParam s{};
    state_param_init(s);
    EXPECT_EQ(s.stateNew, 0u);
    EXPECT_EQ(s.stateCur, 0u);
    EXPECT_EQ(s.stateOld, 0u);
    EXPECT_FALSE(s.bStateUpdate);
}

TEST(StateParam, SetStateCopiesToCurAndNewAndOld) {
    StateParam s{};
    state_param_set_state(s, 5u);
    EXPECT_EQ(s.stateNew, 5u);
    EXPECT_EQ(s.stateCur, 5u);
    EXPECT_EQ(s.stateOld, 0u);
    EXPECT_TRUE(s.bStateUpdate);
}

TEST(StateParam, SetStateTwicePromotesCurToOld) {
    StateParam s{};
    state_param_set_state(s, 1u);
    state_param_set_state(s, 2u);
    EXPECT_EQ(s.stateCur, 2u);
    EXPECT_EQ(s.stateNew, 2u);
    EXPECT_EQ(s.stateOld, 1u);
}

TEST(StateParam, SetStateSetsUpdateFlag) {
    StateParam s{};
    state_param_set_state(s, 7u);
    EXPECT_TRUE(s.bStateUpdate);
}

TEST(StateParam, ElapsedReturnsNowMinusStart) {
    StateParam s{};
    s.stateStartTime = 1000u;
    EXPECT_EQ(state_param_elapsed(s, 1500u), 500u);
}

TEST(StateParam, ElapsedClampsAtZeroWhenNowBeforeStart) {
    StateParam s{};
    s.stateStartTime = 1000u;
    EXPECT_EQ(state_param_elapsed(s, 500u), 0u);
}

TEST(StateParam, ElapsedZeroAtExactStart) {
    StateParam s{};
    s.stateStartTime = 1000u;
    EXPECT_EQ(state_param_elapsed(s, 1000u), 0u);
}

}  // namespace
