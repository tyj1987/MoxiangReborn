// object_state_manager_test.cpp - Phase D6 ObjectStateManager 1:1 port tests.

#include "mxh/server/object_state_manager.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::start_object_state;
using mxh::server::end_object_state;
using mxh::server::StartStateResult;
using mxh::server::EndStateResult;
using mxh::server::ObjectState;

TEST(ObjectStateManagerStart, NoneToAnyAccepted) {
    EXPECT_EQ(start_object_state(ObjectState::None, ObjectState::Move), StartStateResult::Accepted);
    EXPECT_EQ(start_object_state(ObjectState::None, ObjectState::Die), StartStateResult::Accepted);
}

TEST(ObjectStateManagerStart, DieRejectsNonExit) {
    EXPECT_EQ(start_object_state(ObjectState::Die, ObjectState::Move),
              StartStateResult::RejectedDieBlocks);
    EXPECT_EQ(start_object_state(ObjectState::Die, ObjectState::Tactic),
              StartStateResult::RejectedDieBlocks);
}

TEST(ObjectStateManagerStart, DieAcceptsExit) {
    EXPECT_EQ(start_object_state(ObjectState::Die, ObjectState::Exit),
              StartStateResult::Accepted);
}

TEST(ObjectStateManagerStart, LockedStateAcceptsDie) {
    EXPECT_EQ(start_object_state(ObjectState::Move, ObjectState::Die),
              StartStateResult::Accepted);
}

TEST(ObjectStateManagerStart, LockedStateRejectsOtherTransitions) {
    EXPECT_EQ(start_object_state(ObjectState::Ungijosik, ObjectState::Move),
              StartStateResult::RejectedLocked);
    EXPECT_EQ(start_object_state(ObjectState::Exchange, ObjectState::Tactic),
              StartStateResult::RejectedLocked);
}

TEST(ObjectStateManagerStart, LockedStateAcceptsExit) {
    EXPECT_EQ(start_object_state(ObjectState::Deal, ObjectState::Exit),
              StartStateResult::Accepted);
}

TEST(ObjectStateManagerEnd, MismatchedNonDieAsserts) {
    std::uint32_t t = 0;
    bool b = true;
    EXPECT_EQ(end_object_state(ObjectState::Move, ObjectState::Die, 0, 100, t, b),
              EndStateResult::MismatchedAndNotDie);
    EXPECT_EQ(t, 0u);
    EXPECT_FALSE(b);
}

TEST(ObjectStateManagerEnd, MismatchedDieSilentlyReturns) {
    std::uint32_t t = 99;
    bool b = true;
    EXPECT_EQ(end_object_state(ObjectState::Die, ObjectState::Move, 0, 100, t, b),
              EndStateResult::MismatchedButDie);
    EXPECT_EQ(t, 0u);
    EXPECT_FALSE(b);
}

TEST(ObjectStateManagerEnd, ImmediateClearsState) {
    std::uint32_t t = 999;
    bool b = true;
    EXPECT_EQ(end_object_state(ObjectState::Move, ObjectState::Move, 0, 100, t, b),
              EndStateResult::EndedImmediate);
    EXPECT_EQ(t, 0u);
    EXPECT_FALSE(b);
}

TEST(ObjectStateManagerEnd, DelayedSchedulesEnd) {
    std::uint32_t t = 0;
    bool b = false;
    EXPECT_EQ(end_object_state(ObjectState::Tactic, ObjectState::Tactic, 500, 1000, t, b),
              EndStateResult::EndedDelayed);
    EXPECT_EQ(t, 1500u);
    EXPECT_TRUE(b);
}

}  // namespace
