#include "mxh/server/item_id.hpp"
#include <gtest/gtest.h>
#include <array>
#include <thread>
#include <set>
#include <vector>

TEST(ItemId, ReservesSuccessorsOfDatabaseSeed) {
    std::atomic<std::uint32_t> next{900001};
    EXPECT_EQ(mxh::server::reserve_item_id(next), 900001u);
    EXPECT_EQ(mxh::server::reserve_item_id(next), 900002u);
}
TEST(ItemId, ExhaustionDoesNotWrapOrReturnZero) {
    std::atomic<std::uint32_t> next{UINT32_MAX - 1};
    EXPECT_EQ(mxh::server::reserve_item_id(next), UINT32_MAX - 1);
    EXPECT_FALSE(mxh::server::reserve_item_id(next));
    EXPECT_EQ(next.load(), UINT32_MAX);
    next = 0;
    EXPECT_FALSE(mxh::server::reserve_item_id(next));
}
TEST(ItemId, ConcurrentReservationsNeverReuseAnIdentity) {
    std::atomic<std::uint32_t> next{700000};
    std::array<std::vector<std::uint32_t>, 8> results;
    std::vector<std::thread> threads;
    for (std::size_t i = 0; i < results.size(); ++i)
        threads.emplace_back([&, i] {
            for (int n = 0; n < 200; ++n) {
                auto id = mxh::server::reserve_item_id(next);
                if (id) results[i].push_back(*id);
            }
        });
    for (auto& thread : threads) thread.join();
    std::set<std::uint32_t> ids;
    for (const auto& row : results) {
        ASSERT_EQ(row.size(), 200u);
        ids.insert(row.begin(), row.end());
    }
    ASSERT_EQ(ids.size(), 1600u);
    EXPECT_EQ(*ids.begin(), 700000u);
    EXPECT_EQ(*ids.rbegin(), 701599u);
}
