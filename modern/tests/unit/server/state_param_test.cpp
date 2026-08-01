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

TEST(StateParam, SetStateReturnsResultStruct) {
    StateParam s{};
    state_param_set_state(s, 7u);
    const auto r = state_param_set_state(s, 8u);
    EXPECT_EQ(r.new_state, 8u);
    EXPECT_EQ(r.cur_state, 8u);
    EXPECT_EQ(r.old_state, 7u);  // previous cur becomes old
    EXPECT_TRUE(r.state_updated);
}

TEST(StateParam, SetStateResultReflectsFirstTransition) {
    // After init, set 1->2->3, the second call returns old_state=1.
    StateParam s{};
    state_param_set_state(s, 1u);
    const auto r2 = state_param_set_state(s, 2u);
    EXPECT_EQ(r2.old_state, 1u);
    EXPECT_EQ(r2.cur_state, 2u);
    EXPECT_EQ(r2.new_state, 2u);
}

TEST(StateParam, ThreeStepChainKeepsOldAtTwoStepsBack) {
    StateParam s{};
    state_param_set_state(s, 1u);
    state_param_set_state(s, 2u);
    state_param_set_state(s, 3u);
    EXPECT_EQ(s.stateCur, 3u);
    EXPECT_EQ(s.stateNew, 3u);
    EXPECT_EQ(s.stateOld, 2u);
    EXPECT_TRUE(s.bStateUpdate);
}

TEST(StateParam, InitAfterSetStateResetsAllFields) {
    StateParam s{};
    state_param_set_state(s, 5u);
    state_param_set_state(s, 6u);
    state_param_init(s);
    EXPECT_EQ(s.stateNew, 0u);
    EXPECT_EQ(s.stateCur, 0u);
    EXPECT_EQ(s.stateOld, 0u);
    EXPECT_FALSE(s.bStateUpdate);
    s.stateStartTime = 999u;
    state_param_init(s);
    EXPECT_EQ(s.stateStartTime, 0u);
}

TEST(StateParam, StateMidAndEndTimeUnchangedBySetAndInit) {
    StateParam s{};
    s.stateEndTime = 1234u;
    s.stateMidTime = 5678u;
    state_param_set_state(s, 9u);
    EXPECT_EQ(s.stateEndTime, 1234u);
    EXPECT_EQ(s.stateMidTime, 5678u);
}

TEST(StateParam, ElapsedAtMaxDelta) {
    StateParam s{};
    s.stateStartTime = 0u;
    EXPECT_EQ(state_param_elapsed(s, 0xFFFFFFFFu), 0xFFFFFFFFu);
}

TEST(StateParam, ElapsedZeroForAllZeros) {
    StateParam s{};
    EXPECT_EQ(state_param_elapsed(s, 0u), 0u);
    EXPECT_EQ(state_param_elapsed(s, 1000u), 1000u);
}

TEST(StateParam, StructCanHoldLargeStateCodes) {
    StateParam s{};
    state_param_set_state(s, 0xFFFFFFFFu);
    EXPECT_EQ(s.stateCur, 0xFFFFFFFFu);
    EXPECT_EQ(state_param_set_state(s, 0xFFFFFFFFu).new_state, 0xFFFFFFFFu);
}
}  // namespace
