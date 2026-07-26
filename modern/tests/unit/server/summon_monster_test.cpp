// summon_monster_test.cpp - Phase D6 SummonMonster 1:1 port tests.

#include "mxh/server/summon_monster.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::SummonMonster;
using mxh::server::summon_monster_init;
using mxh::server::summon_monster_age_ms;

TEST(SummonMonster, DefaultFieldsAreZero) {
    SummonMonster s{};
    EXPECT_EQ(s.m_SummmonerID, 0u);
    EXPECT_EQ(s.m_RegenTime,   0u);
    EXPECT_EQ(s.m_DieTime,     0u);
}

TEST(SummonMonster, InitMatchesLegacyDefaults) {
    SummonMonster s{};
    s.m_SummmonerID = 99u;
    summon_monster_init(s);
    EXPECT_EQ(s.m_SummmonerID, 0u);
}

TEST(SummonMonster, AgeReturnsNowMinusRegen) {
    SummonMonster s{};
    s.m_RegenTime = 1000u;
    EXPECT_EQ(summon_monster_age_ms(s, 1500u), 500u);
}

TEST(SummonMonster, AgeZeroWhenNowBeforeRegen) {
    SummonMonster s{};
    s.m_RegenTime = 1000u;
    EXPECT_EQ(summon_monster_age_ms(s, 500u), 0u);
}

TEST(SummonMonster, LegacyTypoPreserved) {
    // Modern field name retains the legacy typo m_SummmonerID
    // (double "m").  This test pins the legacy identifier so a
    // future cleanup can be a deliberate decision.
    SummonMonster s{};
    static_assert(sizeof(s.m_SummmonerID) >= 4, "summoner id is DWORD");
    s.m_SummmonerID = 42u;
    EXPECT_EQ(s.m_SummmonerID, 42u);
}

}  // namespace
