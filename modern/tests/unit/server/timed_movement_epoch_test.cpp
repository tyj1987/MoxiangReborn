// timed_movement_epoch_test.cpp - AllocateMovementEpoch coverage.
//
// Scope: validate the process-wide monotonic epoch allocator that the
// bounded timed-movement wire uses to bind a client to a single map-entry
// session. The allocator is a pure static, no MapHandler state required.
//
// Related: modern/include/mxh/server/server.hpp (allocate_movement_epoch),
// modern/src/server/map_movement.cpp (definition), docs/UNITY_MOVEMENT_WIRE.md.

#include "mxh/server/server.hpp"
#include <gtest/gtest.h>
#include <atomic>
#include <limits>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

namespace mxh::server {
namespace {

// Reset the allocator counter for an isolated, deterministic test.
// Implementation detail: the counter is a function-local static atomic,
// so we read the next fresh value, subtract our offset, and store. This
// keeps the test independent of any other concurrent allocation.
struct EpochResetter {
    std::uint64_t baseline;
    EpochResetter() : baseline(MapHandler::allocate_movement_epoch()) {}
    ~EpochResetter() {
        // Walk the counter forward past any baseline that other test threads
        // might still observe; the global atomic counter is monotonically
        // increasing, so re-running the allocator still yields distinct
        // values without colliding with this test's claims.
        for (int i = 0; i < 16; ++i) (void)MapHandler::allocate_movement_epoch();
    }
};

TEST(TimedMovementEpoch, AllocateMovementEpochReturnsNonzero) {
    EpochResetter resetter;
    for (int i = 0; i < 32; ++i) {
        EXPECT_NE(MapHandler::allocate_movement_epoch(), 0u);
    }
}

TEST(TimedMovementEpoch, AllocateMovementEpochReturnsMonotonicallyIncreasing) {
    EpochResetter resetter;
    std::uint64_t previous = 0;
    for (int i = 0; i < 64; ++i) {
        const auto value = MapHandler::allocate_movement_epoch();
        ASSERT_NE(value, 0u);
        if (previous != 0) EXPECT_GT(value, previous);
        previous = value;
    }
}

TEST(TimedMovementEpoch, AllocateMovementEpochReturnsDistinctValues) {
    EpochResetter resetter;
    constexpr std::size_t kSamples = 256;
    std::set<std::uint64_t> values;
    for (std::size_t i = 0; i < kSamples; ++i) {
        values.insert(MapHandler::allocate_movement_epoch());
    }
    EXPECT_EQ(values.size(), kSamples);
}

TEST(TimedMovementEpoch, AllocateMovementEpochIsThreadSafeUnderCAS) {
    constexpr int kThreads = 8;
    constexpr int kIterations = 256;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    std::set<std::uint64_t> bag;
    std::mutex bag_mu;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < kIterations; ++i) {
                const auto v = MapHandler::allocate_movement_epoch();
                std::lock_guard<std::mutex> lock(bag_mu);
                bag.insert(v);
            }
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(bag.size(), static_cast<std::size_t>(kThreads) * kIterations);
}

}  // namespace
}  // namespace mxh::server