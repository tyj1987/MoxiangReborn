// object_state_manager_test.cpp - Phase D6 ObjectStateManager 1:1 port tests.

#include "mxh/server/object_state_manager.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::start_object_state;
using mxh::server::end_object_state;
using mxh::server::StartStateResult;
using mxh::server::EndStateResult;
using mxh::server::ObjectState;

TEST(ObjectStateManagerValues, EnumMatchesLegacyNumericSequence) {
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectState::None), 0u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectState::Enter), 1u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectState::Move), 2u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectState::Ungijosik), 3u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectState::Tactic), 4u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectState::Rest), 5u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectState::Deal), 6u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectState::Exchange), 7u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectState::StreetStall_Owner), 8u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectState::StreetStall_Guest), 9u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectState::PrivateWarehouse), 10u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectState::Munpa), 11u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectState::TiedUp), 19u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectState::Die), 20u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectState::BattleReady), 21u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectState::Exit), 22u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectState::Society), 24u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectState::ItemUse), 25u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectState::TitanRecall), 31u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectState::Max), 32u);
}

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

TEST(ObjectStateManagerStart, DieRejectsExitLikeEveryOtherStart) {
    EXPECT_EQ(start_object_state(ObjectState::Die, ObjectState::Exit),
              StartStateResult::RejectedDieBlocks);
}

TEST(ObjectStateManagerStart, LockedStateAcceptsDie) {
    EXPECT_EQ(start_object_state(ObjectState::Move, ObjectState::Die),
              StartStateResult::Accepted);
}

TEST(ObjectStateManagerStart, LockedStateInvalidRequestStillAccepted) {
    // Legacy asserts in this branch but continues to OnStartObjectState
    // + SetState and returns TRUE. The pure modern seam preserves the
    // observable return value without reproducing the process abort.
    EXPECT_EQ(start_object_state(ObjectState::Ungijosik, ObjectState::Move),
              StartStateResult::Accepted);
    EXPECT_EQ(start_object_state(ObjectState::Exchange, ObjectState::Tactic),
              StartStateResult::Accepted);
}

TEST(ObjectStateManagerStart, LockedStateAcceptsExit) {
    EXPECT_EQ(start_object_state(ObjectState::Deal, ObjectState::Exit),
              StartStateResult::Accepted);
}

TEST(ObjectStateManagerEnd, MismatchedNonDieReturnsWithoutMutation) {
    std::uint32_t t = 777u;
    bool b = true;
    EXPECT_EQ(end_object_state(ObjectState::Move, ObjectState::Die, 0, 100, t, b),
              EndStateResult::MismatchedAndNotDie);
    EXPECT_EQ(t, 777u);
    EXPECT_TRUE(b);
}

TEST(ObjectStateManagerEnd, MismatchedDieSilentlyReturnsWithoutMutation) {
    std::uint32_t t = 99u;
    bool b = true;
    EXPECT_EQ(end_object_state(ObjectState::Die, ObjectState::Move, 0, 100, t, b),
              EndStateResult::MismatchedButDie);
    EXPECT_EQ(t, 99u);
    EXPECT_TRUE(b);
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

TEST(ObjectStateManagerEnd, DelayedEndUsesLegacyDwordWrap) {
    std::uint32_t t = 0;
    bool b = false;
    EXPECT_EQ(end_object_state(ObjectState::Deal, ObjectState::Deal, 100u,
                               0xFFFFFFF0u, t, b),
              EndStateResult::EndedDelayed);
    EXPECT_EQ(t, 84u);
    EXPECT_TRUE(b);
}

}  // namespace
