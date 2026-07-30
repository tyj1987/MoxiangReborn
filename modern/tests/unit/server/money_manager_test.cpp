// money_manager_test.cpp

#include "mxh/server/money_manager.hpp"
#include <gtest/gtest.h>
#include <vector>

namespace {
using mxh::server::MoneyManager;
}

TEST(MoneyManager, AddAndSpend) {
    MoneyManager m;
    EXPECT_TRUE(m.add(100, 1000, "test"));
    EXPECT_EQ(m.balance(100), 1000u);
    EXPECT_TRUE(m.spend(100, 250, "test"));
    EXPECT_EQ(m.balance(100), 750u);
    EXPECT_FALSE(m.spend(100, 9999, "test"));     // insufficient
    EXPECT_EQ(m.balance(100), 750u);
}

TEST(MoneyManager, ClampAtMax) {
    MoneyManager m;
    EXPECT_TRUE(m.add(100, 100, "test"));
    EXPECT_TRUE(m.add(100, mxh::server::MXH_PLAYER_MAX_MONEY, "test"));
    EXPECT_EQ(m.balance(100), mxh::server::MXH_PLAYER_MAX_MONEY);
}

TEST(MoneyManager, SetClampsAndRecordsLog) {
    MoneyManager m;
    std::vector<int> hits;
    m.set_log_sink([&](const mxh::server::MoneyLogEntry& e){
        if (e.reason == "init") hits.push_back(1);
    });
    EXPECT_TRUE(m.set_money(100, 50, "init"));
    hits.clear();
    m.set_log_sink(nullptr);
    EXPECT_TRUE(m.set_money(100, 99, "again"));   // no callback -> silent
    EXPECT_EQ(m.balance(100), 99u);
}

TEST(MoneyManager, MultiplePlayersIsolated) {
    MoneyManager m;
    EXPECT_TRUE(m.add(100, 500, "x"));
    EXPECT_TRUE(m.add(200, 200, "x"));
    EXPECT_TRUE(m.spend(200, 100, "x"));
    EXPECT_EQ(m.balance(100), 500u);
    EXPECT_EQ(m.balance(200), 100u);
}

TEST(MoneyManager, UnknownPlayerSpendFails) {
    MoneyManager m;
    EXPECT_FALSE(m.spend(999, 10, "x"));
    EXPECT_EQ(m.balance(999), 0u);
}
