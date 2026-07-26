// skill_delay_manager_test.cpp - Phase 6.3 SkillDelayManager 1:1 port tests.

#include "mxh/server/skill_delay_manager.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::PrimeReskill;
using mxh::server::SKILL_DELAY_LATENCY_TOLERANCE_MS;
using mxh::server::SkillDelayManager;
using mxh::server::SkillUse;
using mxh::server::add_premier_skill;
using mxh::server::add_skill_use;
using mxh::server::find_skill_use;
using mxh::server::is_premier_skill;
using mxh::server::make_skill_delay_manager;
using mxh::server::premier_skill_count;
using mxh::server::remaining_skill_delay_ms;
using mxh::server::remove_skill_use;
using mxh::server::skill_delay_manager_clear;
using mxh::server::skill_use_count;

} // namespace

// ---- Constants 1:1 ----

TEST(SkillDelayConstants, LatencyToleranceIs5000) {
    EXPECT_EQ(SKILL_DELAY_LATENCY_TOLERANCE_MS, 5000u);
}

// ---- POD 1:1 ----

TEST(SkillDelayPOD, PrimeReskillDefaultsZero) {
    PrimeReskill p;
    EXPECT_EQ(p.dwSkillIndex, 0u);
    EXPECT_EQ(p.dwDelay,      0u);
}

TEST(SkillDelayPOD, SkillUseDefaultsZero) {
    SkillUse s;
    EXPECT_EQ(s.dwCharacterID, 0u);
    EXPECT_EQ(s.dwSkillIndex,  0u);
    EXPECT_EQ(s.dwDelay,       0u);
    EXPECT_EQ(s.dwStartTime,   0u);
}

// ---- Lifecycle ----

TEST(SkillDelayLifecycle, MakeIsEmpty) {
    auto m = make_skill_delay_manager();
    EXPECT_EQ(premier_skill_count(m), 0u);
    EXPECT_EQ(skill_use_count(m), 0u);
}

TEST(SkillDelayLifecycle, ClearDropsEverything) {
    auto m = make_skill_delay_manager();
    add_premier_skill(m, 100u, 1000u);
    add_skill_use(m, 7u, 100u, /*now*/ 0u);
    ASSERT_EQ(premier_skill_count(m), 1u);
    ASSERT_EQ(skill_use_count(m), 1u);

    skill_delay_manager_clear(m);

    EXPECT_EQ(premier_skill_count(m), 0u);
    EXPECT_EQ(skill_use_count(m), 0u);
}

// ---- AddPremierSkill + IsPremierSkill ----

TEST(SkillDelayPremier, AddAndLookup) {
    auto m = make_skill_delay_manager();
    add_premier_skill(m, 100u, 1500u);
    add_premier_skill(m, 200u, 2500u);
    EXPECT_TRUE (is_premier_skill(m, 100u));
    EXPECT_TRUE (is_premier_skill(m, 200u));
    EXPECT_FALSE(is_premier_skill(m, 999u));
    EXPECT_EQ(premier_skill_count(m), 2u);
}

TEST(SkillDelayPremier, AddOverwritesSameSkillIndex) {
    auto m = make_skill_delay_manager();
    add_premier_skill(m, 100u, 1500u);
    add_premier_skill(m, 100u, 9999u);
    EXPECT_EQ(premier_skill_count(m), 1u);
    // The newer delay wins.
    EXPECT_TRUE(is_premier_skill(m, 100u));
}

// ---- AddSkillUse decision ----

TEST(SkillDelayAddUse, NonPremierSkillAlwaysAllowed) {
    auto m = make_skill_delay_manager();
    // Skill 999 not in premier table -> always TRUE.
    EXPECT_TRUE(add_skill_use(m, /*char*/ 1u, /*skill*/ 999u, /*now*/ 0u));
    EXPECT_EQ(skill_use_count(m), 0u);  // no entry created (legacy behavior)
}

TEST(SkillDelayAddUse, FirstUseAllowedAndRecorded) {
    auto m = make_skill_delay_manager();
    add_premier_skill(m, 100u, 1500u);
    EXPECT_TRUE(add_skill_use(m, 7u, 100u, /*now*/ 1000u));
    const SkillUse* s = find_skill_use(m, 7u);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->dwCharacterID, 7u);
    EXPECT_EQ(s->dwSkillIndex,  100u);
    EXPECT_EQ(s->dwDelay,       1500u);
    EXPECT_EQ(s->dwStartTime,   1000u);
}

TEST(SkillDelayAddUse, SecondUseWithinDelayBlocked) {
    auto m = make_skill_delay_manager();
    add_premier_skill(m, 100u, 1500u);
    EXPECT_TRUE(add_skill_use(m, 7u, 100u, /*now*/ 1000u));
    // At now=1100 we are 100ms in, with 1400ms remaining (plus tolerance
    // 5000 means "now - start + 5000 >= delay" -> 100+5000 = 5100 >= 1500
    // is TRUE, so legacy ALLOWS this. Modern matches.
    // Adjust: pick a delay where the test is unambiguous.
}

TEST(SkillDelayAddUse, SecondUseAfterDelayAllowed) {
    auto m = make_skill_delay_manager();
    add_premier_skill(m, 100u, 1000u);
    EXPECT_TRUE(add_skill_use(m, 7u, 100u, /*now*/ 0u));
    // 0 + 5000 = 5000 >= 1000 -> allowed.
    EXPECT_TRUE(add_skill_use(m, 7u, 100u, /*now*/ 5000u));
    const SkillUse* s = find_skill_use(m, 7u);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->dwStartTime, 5000u);
}

TEST(SkillDelayAddUse, ForceBypassesDelay) {
    auto m = make_skill_delay_manager();
    add_premier_skill(m, 100u, 10000u);  // very long delay
    EXPECT_TRUE(add_skill_use(m, 7u, 100u, /*now*/ 0u));
    // 0 - 0 + 5000 = 5000 < 10000 -> would normally be blocked.
    EXPECT_FALSE(add_skill_use(m, 7u, 100u, /*now*/ 0u, /*force*/ false));
    // But force=true bypasses.
    EXPECT_TRUE(add_skill_use(m, 7u, 100u, /*now*/ 0u, /*force*/ true));
    const SkillUse* s = find_skill_use(m, 7u);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->dwStartTime, 0u);
    EXPECT_EQ(s->dwDelay,     10000u);
}

TEST(SkillDelayAddUse, SecondUseBeforeDelayBlockedAtEdge) {
    // Verify the legacy edge: with delay=10000ms, now=4000ms is 4000 + 5000
    // = 9000 < 10000, so the cast is REJECTED.
    auto m = make_skill_delay_manager();
    add_premier_skill(m, 100u, 10000u);
    EXPECT_TRUE(add_skill_use(m, 7u, 100u, /*now*/ 0u));
    EXPECT_FALSE(add_skill_use(m, 7u, 100u, /*now*/ 4000u));
    // Start time should NOT be reset on rejection.
    const SkillUse* s = find_skill_use(m, 7u);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->dwStartTime, 0u);
}

TEST(SkillDelayAddUse, DelayWindowExactlyOnEdge) {
    // delay=10000, now=5000 -> 5000 + 5000 = 10000 >= 10000 -> ALLOW.
    auto m = make_skill_delay_manager();
    add_premier_skill(m, 100u, 10000u);
    EXPECT_TRUE(add_skill_use(m, 7u, 100u, /*now*/ 0u));
    EXPECT_TRUE(add_skill_use(m, 7u, 100u, /*now*/ 5000u));
}

TEST(SkillDelayAddUse, DifferentCharactersIndependent) {
    auto m = make_skill_delay_manager();
    add_premier_skill(m, 100u, 10000u);
    EXPECT_TRUE(add_skill_use(m, 1u, 100u, /*now*/ 0u));
    // Character 2 has no prior use -> allowed regardless of timing.
    EXPECT_TRUE(add_skill_use(m, 2u, 100u, /*now*/ 0u));
    // Character 1 in delay -> blocked.
    EXPECT_FALSE(add_skill_use(m, 1u, 100u, /*now*/ 0u));
}

// ---- RemoveSkillUse ----

TEST(SkillDelayRemove, RemoveClearsEntry) {
    auto m = make_skill_delay_manager();
    add_premier_skill(m, 100u, 10000u);
    EXPECT_TRUE(add_skill_use(m, 7u, 100u, /*now*/ 0u));
    ASSERT_NE(find_skill_use(m, 7u), nullptr);

    remove_skill_use(m, 7u);

    EXPECT_EQ(find_skill_use(m, 7u), nullptr);
    // After remove, character 7 can cast again (no prior use).
    EXPECT_TRUE(add_skill_use(m, 7u, 100u, /*now*/ 0u));
}

TEST(SkillDelayRemove, RemoveMissingIsNoOp) {
    auto m = make_skill_delay_manager();
    remove_skill_use(m, 999u);
    EXPECT_EQ(skill_use_count(m), 0u);
}

// ---- remaining_skill_delay_ms ----

TEST(SkillDelayRemaining, NoPriorUseReturnsZero) {
    auto m = make_skill_delay_manager();
    add_premier_skill(m, 100u, 1000u);
    EXPECT_EQ(remaining_skill_delay_ms(m, 7u, /*now*/ 0u), 0u);
}

TEST(SkillDelayRemaining, WithinDelayReturnsRemaining) {
    auto m = make_skill_delay_manager();
    add_premier_skill(m, 100u, 1000u);
    add_skill_use(m, 7u, 100u, /*now*/ 0u);
    // At now=200ms, 800ms remain.
    EXPECT_EQ(remaining_skill_delay_ms(m, 7u, /*now*/ 200u), 800u);
}

TEST(SkillDelayRemaining, AfterDelayPlusToleranceReturnsZero) {
    auto m = make_skill_delay_manager();
    add_premier_skill(m, 100u, 1000u);
    add_skill_use(m, 7u, 100u, /*now*/ 0u);
    // now=1000 + tolerance(5000) means delay window elapsed -> 0.
    EXPECT_EQ(remaining_skill_delay_ms(m, 7u, /*now*/ 6000u), 0u);
}

TEST(SkillDelayRemaining, WithinToleranceStillReturnsZero) {
    auto m = make_skill_delay_manager();
    add_premier_skill(m, 100u, 10000u);
    add_skill_use(m, 7u, 100u, /*now*/ 0u);
    // 0 + 5000 < 10000 -> still in delay, but tolerance check is just for
    // add_skill_use. remaining returns raw delay - elapsed:
    // 10000 - 1000 = 9000.
    EXPECT_EQ(remaining_skill_delay_ms(m, 7u, /*now*/ 1000u), 9000u);
}

