// playtime_decrement_test.cpp - 1:1 data-plane tests for
// the legacy CShopItemManager::CheckEndTime PLAYTIME branch
// from [Server]Map/ShopItemManager.cpp. Locks the clamp + the
// underflow-to-zero semantics.

#include <mxh/server/playtime_decrement.hpp>

#include <gtest/gtest.h>

#include <cstdint>

using namespace mxh::server;

TEST(PlaytimeDecrement, NormalSubtractsElapsed) {
    auto r = playtime_decrement(/*remtime=*/60000, /*last=*/1000, /*now=*/11000);
    EXPECT_EQ(r.elapsed_clamped_ms, 10000u);
    EXPECT_EQ(r.new_remaintime, 50000u);
}

TEST(PlaytimeDecrement, ClampTo30000Ms) {
    // 60s elapsed, clamped to 30s.
    auto r = playtime_decrement(/*remtime=*/120000, /*last=*/0, /*now=*/60000);
    EXPECT_EQ(r.elapsed_clamped_ms, 30000u);
    EXPECT_EQ(r.new_remaintime, 90000u);
}

TEST(PlaytimeDecrement, ClampExact30000Ms) {
    // exactly 30s elapsed, no clamping needed.
    auto r = playtime_decrement(/*remtime=*/60000, /*last=*/0, /*now=*/30000);
    EXPECT_EQ(r.elapsed_clamped_ms, 30000u);
    EXPECT_EQ(r.new_remaintime, 30000u);
}

TEST(PlaytimeDecrement, UnderflowToZero) {
    // elapsed > remtime -> remtime becomes 0.
    auto r = playtime_decrement(/*remtime=*/5000, /*last=*/0, /*now=*/10000);
    EXPECT_EQ(r.elapsed_clamped_ms, 10000u);
    EXPECT_EQ(r.new_remaintime, 0u);
}

TEST(PlaytimeDecrement, EqualElapsedEqualsZero) {
    // elapsed == remtime -> remtime becomes 0.
    auto r = playtime_decrement(/*remtime=*/5000, /*last=*/0, /*now=*/5000);
    EXPECT_EQ(r.elapsed_clamped_ms, 5000u);
    EXPECT_EQ(r.new_remaintime, 0u);
}

TEST(PlaytimeDecrement, ZeroElapsedZeroDecrement) {
    // now == last -> elapsed 0, no decrement.
    auto r = playtime_decrement(/*remtime=*/5000, /*last=*/1000, /*now=*/1000);
    EXPECT_EQ(r.elapsed_clamped_ms, 0u);
    EXPECT_EQ(r.new_remaintime, 5000u);
}

TEST(PlaytimeDecrement, ZeroRemaintimeStaysZero) {
    auto r = playtime_decrement(/*remtime=*/0, /*last=*/0, /*now=*/1000);
    EXPECT_EQ(r.elapsed_clamped_ms, 1000u);
    EXPECT_EQ(r.new_remaintime, 0u);
}

TEST(PlaytimeDecrement, UnderflowLastGreaterThanNow) {
    // Legacy uses DWORD arithmetic: last > now underflows to a huge
    // value, which is then clamped to 30000 ms. Result: full 30s
    // decrement regardless of sign.
    auto r = playtime_decrement(/*remtime=*/120000, /*last=*/2000, /*now=*/1000);
    EXPECT_EQ(r.elapsed_clamped_ms, 30000u);
    EXPECT_EQ(r.new_remaintime, 90000u);
}

TEST(PlaytimeDecrement, RemtimeOneBelowChecktimeGivesZero) {
    // Boundary: remtime - 1 == elapsed - 1 -> exact equality.
    auto r = playtime_decrement(/*remtime=*/100, /*last=*/0, /*now=*/100);
    EXPECT_EQ(r.new_remaintime, 0u);
    r = playtime_decrement(/*remtime=*/99, /*last=*/0, /*now=*/100);
    EXPECT_EQ(r.new_remaintime, 0u);
    r = playtime_decrement(/*remtime=*/101, /*last=*/0, /*now=*/100);
    EXPECT_EQ(r.new_remaintime, 1u);
}
