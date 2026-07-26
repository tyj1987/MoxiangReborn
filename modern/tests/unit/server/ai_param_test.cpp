// ai_param_test.cpp - Phase D6 AIParam 1:1 port tests.

#include "mxh/server/ai_param.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::AIPARAM;
using mxh::server::ai_param_init;
using mxh::server::ai_param_get_cur_attack_kind;
using mxh::server::ai_param_set_current_attack_pattern;
using mxh::server::RunawayType;
using mxh::server::AIPARAM_SEARCH_PERIOD_MS;

TEST(AIParam, DefaultConstructZeroesMembers) {
    AIPARAM p{};
    EXPECT_EQ(p.AttackStartTime, 0u);
    EXPECT_EQ(p.CurAttackKind, 0u);
    EXPECT_EQ(p.CurAttackPatternNum, 0u);
    EXPECT_EQ(p.CurAttackPatternIndex, 0u);
    EXPECT_EQ(p.RunawayTypeVal, RunawayType::None);
    EXPECT_EQ(p.pTarget, nullptr);
    EXPECT_EQ(p.pHelperMonster, nullptr);
}

TEST(AIParam, InitSetsFiveSecondOffsets) {
    AIPARAM p{};
    ai_param_init(p, 1000u);
    EXPECT_EQ(p.SearchLastTime, 1000u + AIPARAM_SEARCH_PERIOD_MS);
    EXPECT_EQ(p.CollSearchLastTime, 1000u + AIPARAM_SEARCH_PERIOD_MS);
    EXPECT_EQ(p.PursuitForgiveStartTime, 1000u);
    EXPECT_EQ(p.RunawayTypeVal, RunawayType::None);
}

TEST(AIParam, InitResetsPatternCounters) {
    AIPARAM p{};
    p.CurAttackPatternNum = 7;
    p.CurAttackPatternIndex = 3;
    ai_param_init(p, 5000u);
    EXPECT_EQ(p.CurAttackPatternNum, 0u);
    EXPECT_EQ(p.CurAttackPatternIndex, 0u);
}

TEST(AIParam, GetCurAttackKindReturnsField) {
    AIPARAM p{};
    p.CurAttackKind = 42u;
    EXPECT_EQ(ai_param_get_cur_attack_kind(p), 42u);
}

TEST(AIParam, SetCurrentAttackPatternResetsIndex) {
    AIPARAM p{};
    p.CurAttackPatternIndex = 9;
    ai_param_set_current_attack_pattern(p, 5);
    EXPECT_EQ(p.CurAttackPatternNum, 5u);
    EXPECT_EQ(p.CurAttackPatternIndex, 0u);
}

TEST(AIParam, InitIdempotent) {
    AIPARAM p{};
    ai_param_init(p, 1000u);
    const auto first_search = p.SearchLastTime;
    ai_param_init(p, 2000u);
    EXPECT_EQ(p.SearchLastTime, 2000u + AIPARAM_SEARCH_PERIOD_MS);
    EXPECT_NE(p.SearchLastTime, first_search);
}

TEST(AIParam, RunawayTypeEnumValue) {
    EXPECT_EQ(static_cast<std::uint16_t>(RunawayType::None), 0u);
}

}  // namespace
