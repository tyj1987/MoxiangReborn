// showdown_manager_test.cpp

#include "mxh/server/showdown_manager.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::ShowdownManager;
using mxh::server::AuctionManager;
}

TEST(ShowdownManager, OpenAndAccept) {
    ShowdownManager m;
    EXPECT_TRUE(m.open_challenge(100, 200, 5000, 1000));
    EXPECT_EQ(m.size(), 1u);
    EXPECT_TRUE(m.accept(100, 200));
    EXPECT_FALSE(m.accept(100, 201));
}

TEST(ShowdownManager, OnlyOneOpenAtATime) {
    ShowdownManager m;
    EXPECT_TRUE(m.open_challenge(100, 200, 1000, 1));
    EXPECT_FALSE(m.open_challenge(100, 300, 1000, 1));   // dup challenger
}

TEST(ShowdownManager, CancelRemovesOpen) {
    ShowdownManager m;
    m.open_challenge(100, 200, 1000, 1);
    EXPECT_TRUE(m.cancel(100));
    EXPECT_FALSE(m.cancel(100));   // already gone
}

TEST(ShowdownManager, RejectsBadInputs) {
    ShowdownManager m;
    EXPECT_FALSE(m.open_challenge(0, 200, 1000, 1));
    EXPECT_FALSE(m.open_challenge(100, 0, 1000, 1));
    EXPECT_FALSE(m.open_challenge(100, 100, 1000, 1));   // same player
}

TEST(AuctionManager, RegisterFindBid) {
    AuctionManager m;
    EXPECT_TRUE(m.register_item(100, 50, 1, 100, 1));
    EXPECT_TRUE(m.register_item(200, 51, 2, 200, 1));
    EXPECT_EQ(m.size(), 2u);
    auto* e = m.find(1);
    ASSERT_NE(e, nullptr);
    EXPECT_EQ(e->seller_id, 100u);
    EXPECT_EQ(m.bid(1, 999), true);
    EXPECT_EQ(m.bid(1, 1000), false);   // already sold
}

TEST(AuctionManager, CancelOnlyBySellerUntouched) {
    AuctionManager m;
    m.register_item(100, 1, 1, 1, 0);
    EXPECT_TRUE(m.register_item(100, 2, 1, 1, 0));
    EXPECT_TRUE(m.register_item(200, 3, 1, 1, 0));
    EXPECT_TRUE(m.cancel(1, 100));
    EXPECT_FALSE(m.cancel(2, 999));   // wrong seller
    EXPECT_FALSE(m.cancel(3, 100));   // wrong owner
    EXPECT_EQ(m.size(), 2u);
}

TEST(AuctionManager, SelfBidAndRefuseBad) {
    AuctionManager m;
    m.register_item(100, 1, 1, 1, 0);
    EXPECT_FALSE(m.bid(1, 100));     // self-bid denied
    EXPECT_FALSE(m.bid(99, 200));    // unknown item
}

TEST(AuctionManager, TickExpiresListings) {
    AuctionManager m;
    m.register_item(100, 1, 1, 1, 0);
    m.register_item(100, 2, 1, 1, 0);
    EXPECT_EQ(m.size(), 2u);
    m.tick(24ULL * 3600ULL * 1000ULL - 1);   // just before expire
    EXPECT_EQ(m.size(), 2u);
    m.tick(24ULL * 3600ULL * 1000ULL);       // at exact expire -> both removed
    EXPECT_EQ(m.size(), 0u);
}


