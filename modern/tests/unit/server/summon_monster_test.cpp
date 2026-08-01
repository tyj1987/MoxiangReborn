// summon_monster_test.cpp - Phase D6 SummonMonster 1:1 port tests.

#include "mxh/server/summon_monster.hpp"

#include <gtest/gtest.h>

#include <string>
#include <type_traits>

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

TEST(SummonMonster, InitResetsAllThreeFields) {
    SummonMonster s{};
    s.m_SummmonerID = 10u;
    s.m_RegenTime   = 200u;
    s.m_DieTime     = 500u;
    summon_monster_init(s);
    EXPECT_EQ(s.m_SummmonerID, 0u);
    EXPECT_EQ(s.m_RegenTime,   0u);
    EXPECT_EQ(s.m_DieTime,     0u);
}

TEST(SummonMonster, InitAfterNonZeroRestoresZero) {
    SummonMonster s{};
    s.m_SummmonerID = 42u;
    s.m_RegenTime = 1000u;
    s.m_DieTime = 2000u;
    EXPECT_NE(s.m_SummmonerID, 0u);
    summon_monster_init(s);
    EXPECT_EQ(s.m_SummmonerID, 0u);
    EXPECT_EQ(s.m_RegenTime, 0u);
    EXPECT_EQ(s.m_DieTime, 0u);
}

TEST(SummonMonster, AgeAtExactRegenIsZero) {
    SummonMonster s{};
    s.m_RegenTime = 1000u;
    EXPECT_EQ(summon_monster_age_ms(s, 1000u), 0u);
}

TEST(SummonMonster, AgeFromZeroRegenIsNow) {
    SummonMonster s{};  // m_RegenTime=0
    EXPECT_EQ(summon_monster_age_ms(s, 500u), 500u);
    EXPECT_EQ(summon_monster_age_ms(s, 0u), 0u);
}

TEST(SummonMonster, AgeHandlesLargeDeltas) {
    SummonMonster s{};
    s.m_RegenTime = 1u;
    EXPECT_EQ(summon_monster_age_ms(s, 0xFFFFFFFFu), 0xFFFFFFFFu - 1u);
}

TEST(SummonMonster, DieTimeFieldIsIndependent) {
    // m_DieTime stores the absolute expiry timestamp; it is not
    // consumed by age_ms. The helper returns the age, the caller
    // computes age-vs-DieTime. Verify the field persists untouched.
    SummonMonster s{};
    s.m_DieTime = 7000u;
    EXPECT_EQ(summon_monster_age_ms(s, 6500u), 6500u);
    EXPECT_EQ(s.m_DieTime, 7000u);
}

TEST(SummonMonster, SummmonerIdFieldIsDWordSized) {
    SummonMonster s{};
    static_assert(sizeof(s.m_SummmonerID) == 4, "summoner id must be 4 bytes");
    s.m_SummmonerID = 0xDEADBEEFu;
    EXPECT_EQ(s.m_SummmonerID, 0xDEADBEEFu);
}

TEST(SummonMonster, LegacyTypoImpossibleToMiss) {
    // Field-name typo `m_SummmonerID` (double m) is intentional.
    SummonMonster s{};
    static_assert(std::string("m_SummmonerID") != std::string("m_SummonerID"),
                  "must preserve legacy identifier spelling");
    s.m_SummmonerID = 7u;
    EXPECT_EQ(s.m_SummmonerID, 7u);
}

TEST(SummonMonster, StructSatisfiesTriviallyCopyable) {
    // The struct is used as a value type passed by const ref; ensure POD.
    static_assert(std::is_trivially_copyable<SummonMonster>::value,
                  "SummonMonster must be trivially copyable");
    SUCCEED();
}
}  // namespace
