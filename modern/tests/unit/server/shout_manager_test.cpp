// shout_manager_test.cpp - Phase 6.3 ShoutManager 1:1 port tests.

#include "mxh/server/shout_manager.hpp"

#include <gtest/gtest.h>

#include <cstring>

namespace {

using mxh::server::MAX_SHOUT_LENGTH;
using mxh::server::MAX_SHOUT_PER_CHARACTER;
using mxh::server::SHOUT_BROADCAST_INTERVAL_MS;
using mxh::server::ShoutBase;
using mxh::server::ShoutBatch;
using mxh::server::ShoutReceive;
using mxh::server::add_shout_msg;
using mxh::server::make_shout_manager;
using mxh::server::process;
using mxh::server::shout_manager_init;
using mxh::server::shout_manager_release;
using mxh::server::shout_queue_size;
using mxh::server::take_shout_batch;

ShoutBase shout(std::uint32_t character, const char* message) {
    ShoutBase base;
    base.CharacterIdx = character;
    std::strncpy(base.Msg, message, MAX_SHOUT_LENGTH);
    base.Msg[MAX_SHOUT_LENGTH] = 0;
    return base;
}

} // namespace

TEST(ShoutConstants, LegacyLimitsMatch) {
    EXPECT_EQ(MAX_SHOUT_LENGTH, 60u);
    EXPECT_EQ(MAX_SHOUT_PER_CHARACTER, 3u);
    EXPECT_EQ(SHOUT_BROADCAST_INTERVAL_MS, 5000u);
    EXPECT_EQ(sizeof(ShoutBase::Msg), 61u);
}

TEST(ShoutPOD, DefaultsAreZero) {
    ShoutBase base;
    ShoutReceive receive;
    EXPECT_EQ(base.CharacterIdx, 0u);
    EXPECT_EQ(base.Msg[0], 0);
    EXPECT_EQ(receive.Count, 0u);
    EXPECT_EQ(receive.Time, 0u);
    EXPECT_EQ(receive.CharacterIdx, 0u);
}

TEST(ShoutLifecycle, MakeAndReleaseAreEmpty) {
    auto manager = make_shout_manager();
    add_shout_msg(manager, shout(1u, "one"));
    ASSERT_EQ(shout_queue_size(manager), 1u);
    shout_manager_release(manager);
    EXPECT_EQ(shout_queue_size(manager), 0u);
}

TEST(ShoutLifecycle, InitClearsQueueAndResetsClock) {
    auto manager = make_shout_manager();
    add_shout_msg(manager, shout(1u, "one"));
    manager.m_lastbrodtime = 9999u;
    shout_manager_init(manager);
    EXPECT_EQ(shout_queue_size(manager), 0u);
    EXPECT_EQ(manager.m_lastbrodtime, 0u);
}

TEST(ShoutAdd, CharacterCountAndDelayMatchLegacy) {
    auto manager = make_shout_manager();
    add_shout_msg(manager, shout(2u, "queued"));
    ShoutReceive receive;
    receive.CharacterIdx = 2u;
    ASSERT_TRUE(add_shout_msg(manager, shout(2u, "next"), receive));
    EXPECT_EQ(receive.Count, 2u);
    EXPECT_EQ(receive.Time, 0u);
    EXPECT_EQ(receive.CharacterIdx, 2u);
}

TEST(ShoutAdd, DelayUsesQueueGroupsBeforeAppend) {
    auto manager = make_shout_manager();
    for (std::uint32_t i = 0; i < 6u; ++i) {
        add_shout_msg(manager, shout(100u + i, "queued"));
    }
    ShoutReceive receive;
    ASSERT_TRUE(add_shout_msg(manager, shout(999u, "last"), receive));
    EXPECT_EQ(receive.Count, 1u);
    EXPECT_EQ(receive.Time, 10u);
}

TEST(ShoutAdd, ThirdMessageIsAccepted) {
    auto manager = make_shout_manager();
    ShoutReceive receive;
    EXPECT_TRUE(add_shout_msg(manager, shout(7u, "one"), receive));
    EXPECT_EQ(receive.Count, 1u);
    EXPECT_TRUE(add_shout_msg(manager, shout(7u, "two"), receive));
    EXPECT_EQ(receive.Count, 2u);
    EXPECT_TRUE(add_shout_msg(manager, shout(7u, "three"), receive));
    EXPECT_EQ(receive.Count, 3u);
    EXPECT_EQ(shout_queue_size(manager), 3u);
}

TEST(ShoutAdd, FourthCharacterMessageIsRejectedAndCountZeroed) {
    auto manager = make_shout_manager();
    for (const char* text : {"one", "two", "three"}) {
        ShoutReceive receive;
        ASSERT_TRUE(add_shout_msg(manager, shout(7u, text), receive));
    }
    ShoutReceive receive;
    receive.Count = 99u;
    receive.Time = 88u;
    ASSERT_FALSE(add_shout_msg(manager, shout(7u, "four"), receive));
    EXPECT_EQ(receive.Count, 0u);
    EXPECT_EQ(receive.Time, 88u);
    EXPECT_EQ(shout_queue_size(manager), 3u);
}

TEST(ShoutAdd, CharacterLimitCountsWholeQueueNotConsecutiveMessages) {
    auto manager = make_shout_manager();
    add_shout_msg(manager, shout(7u, "one"));
    add_shout_msg(manager, shout(8u, "other"));
    add_shout_msg(manager, shout(7u, "two"));
    add_shout_msg(manager, shout(9u, "other"));
    ShoutReceive receive;
    ASSERT_TRUE(add_shout_msg(manager, shout(7u, "three"), receive));
    EXPECT_EQ(receive.Count, 3u);
}

TEST(ShoutAdd, UnconditionalOverloadBypassesCharacterLimit) {
    auto manager = make_shout_manager();
    for (std::uint32_t i = 0; i < 4u; ++i) {
        add_shout_msg(manager, shout(7u, "trusted"));
    }
    EXPECT_EQ(shout_queue_size(manager), 4u);
}

TEST(ShoutProcess, DoesNotDrainBeforeFiveSeconds) {
    auto manager = make_shout_manager();
    add_shout_msg(manager, shout(1u, "one"));
    ShoutBatch batch;
    EXPECT_FALSE(process(manager, 4999u, batch));
    EXPECT_EQ(shout_queue_size(manager), 1u);
}

TEST(ShoutProcess, DrainsAtExactlyFiveSeconds) {
    auto manager = make_shout_manager();
    add_shout_msg(manager, shout(1u, "one"));
    auto batch = take_shout_batch(manager, 5000u);
    ASSERT_TRUE(batch.has_value());
    EXPECT_EQ(batch->Count, 1u);
    EXPECT_EQ(batch->ShoutMsg[0].CharacterIdx, 1u);
    EXPECT_STREQ(batch->ShoutMsg[0].Msg, "one");
    EXPECT_EQ(shout_queue_size(manager), 0u);
}

TEST(ShoutProcess, TakesOnlyFirstThreeAndPreservesFifoOrder) {
    auto manager = make_shout_manager();
    for (std::uint32_t i = 1u; i <= 4u; ++i) {
        add_shout_msg(manager, shout(i, "fifo"));
    }
    auto batch = take_shout_batch(manager, 5000u);
    ASSERT_TRUE(batch.has_value());
    ASSERT_EQ(batch->Count, 3u);
    EXPECT_EQ(batch->ShoutMsg[0].CharacterIdx, 1u);
    EXPECT_EQ(batch->ShoutMsg[1].CharacterIdx, 2u);
    EXPECT_EQ(batch->ShoutMsg[2].CharacterIdx, 3u);
    EXPECT_EQ(shout_queue_size(manager), 1u);
}

TEST(ShoutProcess, NextBatchWaitsAnotherFiveSeconds) {
    auto manager = make_shout_manager();
    add_shout_msg(manager, shout(1u, "one"));
    add_shout_msg(manager, shout(2u, "two"));
    ASSERT_TRUE(take_shout_batch(manager, 5000u).has_value());
    add_shout_msg(manager, shout(3u, "three"));
    EXPECT_FALSE(take_shout_batch(manager, 9999u).has_value());
    auto batch = take_shout_batch(manager, 10000u);
    ASSERT_TRUE(batch.has_value());
    EXPECT_EQ(batch->ShoutMsg[0].CharacterIdx, 3u);
}

TEST(ShoutProcess, EmptyQueueDoesNotAdvanceClock) {
    auto manager = make_shout_manager();
    EXPECT_FALSE(take_shout_batch(manager, 5000u).has_value());
    EXPECT_EQ(manager.m_lastbrodtime, 0u);
    add_shout_msg(manager, shout(1u, "one"));
    ASSERT_TRUE(take_shout_batch(manager, 5000u).has_value());
}

TEST(ShoutProcess, ProcessCopiesBatchAndReportsWhetherSent) {
    auto manager = make_shout_manager();
    add_shout_msg(manager, shout(55u, "copy"));
    ShoutBatch batch;
    ASSERT_TRUE(process(manager, 5000u, batch));
    EXPECT_EQ(batch.Count, 1u);
    EXPECT_EQ(batch.ShoutMsg[0].CharacterIdx, 55u);
    EXPECT_STREQ(batch.ShoutMsg[0].Msg, "copy");
}

TEST(ShoutProcess, UnsignedClockWrapRetainsIntervalSemantics) {
    auto manager = make_shout_manager();
    manager.m_lastbrodtime = 0xfffffff0u;
    add_shout_msg(manager, shout(1u, "wrap"));
    EXPECT_FALSE(take_shout_batch(manager, 0x00000f00u).has_value());
    EXPECT_TRUE(take_shout_batch(manager, 0x00001378u).has_value());
}