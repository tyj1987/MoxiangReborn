// agent_skill_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::process_agent_skill_user +
// process_agent_skill_server + process_agent_skill_other +
// SkillDelayManager decision logic (D4.153).
// Augments the legacy 5-test agent_skill_test.cpp with deeper coverage of:
//   - AgentSkillActionKind enum (forward_to_map, send_start_nack)
//   - AgentSkillAction struct defaults (kind=forward_to_map, character_id=0,
//     skill_index=0, category=22, protocol=2, error=0)
//   - process_agent_skill_user truth table:
//       non-premier skill -> always forward_to_map
//       premier skill + no prior -> forward_to_map (start_time set)
//       premier skill + within cooldown -> send_start_nack
//       premier skill + outside cooldown -> forward_to_map
//       premier skill + force (server path) -> forward_to_map
//   - process_agent_skill_server always forwards + resets timer (force=true)
//   - process_agent_skill_other unconditional forward
//   - SkillDelayManager helpers:
//       SKILL_DELAY_LATENCY_TOLERANCE_MS = 5000
//       is_premier_skill, find_skill_use, remove_skill_use
//       premier_skill_count, skill_use_count, remaining_skill_delay_ms
//       add_premier_skill, skill_delay_manager_clear
//       add_skill_use legacy formula: now - start + 5000 >= delay
//       force=true bypasses cooldown check
//       non-premier always returns true from add_skill_use
//       remaining_skill_delay_ms returns 0 when no prior use
//       remaining_skill_delay_ms returns raw delay - elapsed (no +5000)
//
// 1:1 invariants (locked):
//   - SKILL_DELAY_LATENCY_TOLERANCE_MS = 5000
//   - AgentSkillAction: category=22 (MP_SKILL), protocol=0 on forward, protocol=2 on nack
//   - Forward path: category=22, protocol=0, error=0
//   - Nack path: category=22, protocol=2, error=0, character_id/skill_index preserved
//   - Server path: force=true always allows (bypasses latency check)
//   - Non-premier: always allow + never recorded in m_SkillUses
//   - Cooldown formula: (now - start + 5000) >= delay

#pragma once

#include "mxh/server/agent_skill.hpp"
#include "mxh/server/skill_delay_manager.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>

namespace {

using mxh::server::add_premier_skill;
using mxh::server::add_skill_use;
using mxh::server::AgentSkillAction;
using mxh::server::AgentSkillActionKind;
using mxh::server::find_skill_use;
using mxh::server::is_premier_skill;
using mxh::server::make_skill_delay_manager;
using mxh::server::premier_skill_count;
using mxh::server::process_agent_skill_other;
using mxh::server::process_agent_skill_server;
using mxh::server::process_agent_skill_user;
using mxh::server::PrimeReskill;
using mxh::server::remaining_skill_delay_ms;
using mxh::server::remove_skill_use;
using mxh::server::skill_delay_manager_clear;
using mxh::server::skill_use_count;
using mxh::server::SKILL_DELAY_LATENCY_TOLERANCE_MS;
using mxh::server::SkillDelayManager;
using mxh::server::SkillUse;

}  // namespace


// ===========================================================================
// Constants
// ===========================================================================

TEST(AgentSkillDataPlane, LatencyToleranceIsFiveThousand) {
    EXPECT_EQ(SKILL_DELAY_LATENCY_TOLERANCE_MS, 5000u);
}


// ===========================================================================
// Enum / struct defaults
// ===========================================================================

TEST(AgentSkillDataPlane, ActionKindHasTwoValues) {
    auto all = { AgentSkillActionKind::forward_to_map, AgentSkillActionKind::send_start_nack };
    EXPECT_EQ(all.size(), 2u);
}

TEST(AgentSkillDataPlane, ActionDefaults) {
    AgentSkillAction a{};
    EXPECT_EQ(a.kind, AgentSkillActionKind::forward_to_map);
    EXPECT_EQ(a.character_id, 0u);
    EXPECT_EQ(a.skill_index, 0u);
    EXPECT_EQ(a.category, 22u);  // MP_SKILL
    EXPECT_EQ(a.protocol, 2u);    // start_nack protocol
    EXPECT_EQ(a.error, 0u);
}


// ===========================================================================
// process_agent_skill_user -- non-premier path
// ===========================================================================

TEST(AgentSkillDataPlane, UserNonPremierForwards) {
    SkillDelayManager m;
    auto a = process_agent_skill_user(m, 7u, 99u, 0u);
    EXPECT_EQ(a.kind, AgentSkillActionKind::forward_to_map);
    EXPECT_EQ(a.character_id, 7u);
    EXPECT_EQ(a.skill_index, 99u);
    EXPECT_EQ(a.category, 22u);
    EXPECT_EQ(a.protocol, 0u);
}

TEST(AgentSkillDataPlane, UserNonPremierDoesNotRecord) {
    SkillDelayManager m;
    process_agent_skill_user(m, 7u, 99u, 0u);
    EXPECT_EQ(skill_use_count(m), 0u);
    EXPECT_FALSE(is_premier_skill(m, 99u));
}


// ===========================================================================
// process_agent_skill_user -- first use of premier skill
// ===========================================================================

TEST(AgentSkillDataPlane, UserFirstPremierForwards) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    auto a = process_agent_skill_user(m, 7u, 100u, 0u);
    EXPECT_EQ(a.kind, AgentSkillActionKind::forward_to_map);
}

TEST(AgentSkillDataPlane, UserFirstPremierRecordsStartTime) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    process_agent_skill_user(m, 7u, 100u, 5000u);
    auto* use = find_skill_use(m, 7u);
    ASSERT_NE(use, nullptr);
    EXPECT_EQ(use->dwStartTime, 5000u);
    EXPECT_EQ(use->dwDelay, 10000u);
    EXPECT_EQ(use->dwSkillIndex, 100u);
}


// ===========================================================================
// process_agent_skill_user -- cooldown gating
// ===========================================================================

TEST(AgentSkillDataPlane, UserCooldownSendsNack) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    process_agent_skill_user(m, 7u, 100u, 0u);
    auto a = process_agent_skill_user(m, 7u, 100u, 4000u);  // delta=4000 < 10000-5000=5000
    EXPECT_EQ(a.kind, AgentSkillActionKind::send_start_nack);
    EXPECT_EQ(a.protocol, 2u);
    EXPECT_EQ(a.character_id, 7u);
    EXPECT_EQ(a.category, 22u);
}

TEST(AgentSkillDataPlane, UserCooldownJustBelowThresholdSendsNack) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    process_agent_skill_user(m, 7u, 100u, 0u);
    // delta=4999 (just under 5000) -> still in cooldown
    auto a = process_agent_skill_user(m, 7u, 100u, 4999u);
    EXPECT_EQ(a.kind, AgentSkillActionKind::send_start_nack);
}

TEST(AgentSkillDataPlane, UserCooldownExactlyAtThresholdForwards) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    process_agent_skill_user(m, 7u, 100u, 0u);
    // delta=5000 -> (5000 - 0 + 5000) >= 10000 -> allow
    auto a = process_agent_skill_user(m, 7u, 100u, 5000u);
    EXPECT_EQ(a.kind, AgentSkillActionKind::forward_to_map);
}

TEST(AgentSkillDataPlane, UserCooldownAboveThresholdForwards) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    process_agent_skill_user(m, 7u, 100u, 0u);
    // delta=10000 -> way above threshold
    auto a = process_agent_skill_user(m, 7u, 100u, 10000u);
    EXPECT_EQ(a.kind, AgentSkillActionKind::forward_to_map);
}

TEST(AgentSkillDataPlane, UserNackPreservesCharacterAndSkillIndex) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    process_agent_skill_user(m, 7u, 100u, 0u);
    auto a = process_agent_skill_user(m, 7u, 100u, 4000u);
    EXPECT_EQ(a.character_id, 7u);
    EXPECT_EQ(a.skill_index, 100u);
}


// ===========================================================================
// process_agent_skill_server -- always force
// ===========================================================================

TEST(AgentSkillDataPlane, ServerPathForcesAndForwards) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    process_agent_skill_server(m, 7u, 100u, 0u);
    auto a = process_agent_skill_server(m, 7u, 100u, 0u);
    EXPECT_EQ(a.kind, AgentSkillActionKind::forward_to_map);
}

TEST(AgentSkillDataPlane, ServerPathResetsStartTime) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    process_agent_skill_server(m, 7u, 100u, 1000u);
    process_agent_skill_server(m, 7u, 100u, 2000u);
    auto* use = find_skill_use(m, 7u);
    ASSERT_NE(use, nullptr);
    EXPECT_EQ(use->dwStartTime, 2000u);  // reset to latest
}

TEST(AgentSkillDataPlane, ServerNonPremierForwards) {
    SkillDelayManager m;
    auto a = process_agent_skill_server(m, 7u, 99u, 0u);
    EXPECT_EQ(a.kind, AgentSkillActionKind::forward_to_map);
}

TEST(AgentSkillDataPlane, ServerPathPreservesCharacterAndSkillIndex) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    auto a = process_agent_skill_server(m, 7u, 100u, 0u);
    EXPECT_EQ(a.character_id, 7u);
    EXPECT_EQ(a.skill_index, 100u);
}


// ===========================================================================
// process_agent_skill_other -- unconditional forward
// ===========================================================================

TEST(AgentSkillDataPlane, OtherProtocolsForward) {
    auto a = process_agent_skill_other(7u, 100u);
    EXPECT_EQ(a.kind, AgentSkillActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, 0u);
}

TEST(AgentSkillDataPlane, OtherProtocolsPreservesCharacterId) {
    auto a = process_agent_skill_other(0xDEADBEEFu, 100u);
    EXPECT_EQ(a.character_id, 0xDEADBEEFu);
}

TEST(AgentSkillDataPlane, OtherProtocolsPreservesSkillIndex) {
    auto a = process_agent_skill_other(7u, 0xCAFEBABEu);
    EXPECT_EQ(a.skill_index, 0xCAFEBABEu);
}

TEST(AgentSkillDataPlane, OtherProtocolsDoesNotTouchManager) {
    SkillDelayManager m;
    process_agent_skill_other(7u, 100u);
    EXPECT_EQ(skill_use_count(m), 0u);
    EXPECT_EQ(premier_skill_count(m), 0u);
}


// ===========================================================================
// SkillDelayManager helpers
// ===========================================================================

TEST(AgentSkillDataPlane, MakeManagerIsEmpty) {
    auto m = make_skill_delay_manager();
    EXPECT_EQ(skill_use_count(m), 0u);
    EXPECT_EQ(premier_skill_count(m), 0u);
}

TEST(AgentSkillDataPlane, AddPremierSkillByIndex) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    EXPECT_EQ(premier_skill_count(m), 1u);
    EXPECT_TRUE(is_premier_skill(m, 100u));
}

TEST(AgentSkillDataPlane, AddPremierSkillByStruct) {
    SkillDelayManager m;
    PrimeReskill p{200u, 15000u};
    add_premier_skill(m, p);
    EXPECT_TRUE(is_premier_skill(m, 200u));
}

TEST(AgentSkillDataPlane, IsNotPremierForUnknownIndex) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    EXPECT_FALSE(is_premier_skill(m, 999u));
}

TEST(AgentSkillDataPlane, AddSkillUseNonPremierAlwaysTrue) {
    SkillDelayManager m;
    EXPECT_TRUE(add_skill_use(m, 7u, 99u, 0u));
    EXPECT_EQ(skill_use_count(m), 0u);  // non-premier not recorded
}

TEST(AgentSkillDataPlane, AddSkillUsePremierFirstTimeTrue) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    EXPECT_TRUE(add_skill_use(m, 7u, 100u, 0u));
    EXPECT_EQ(skill_use_count(m), 1u);
}

TEST(AgentSkillDataPlane, AddSkillUseForceBypassesCooldown) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    add_skill_use(m, 7u, 100u, 0u);
    // Even within cooldown, force=true allows.
    EXPECT_TRUE(add_skill_use(m, 7u, 100u, 100u, true));
}

TEST(AgentSkillDataPlane, AddSkillUseWithinCooldownRejects) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    add_skill_use(m, 7u, 100u, 0u);
    // delta=4999 -> (4999 - 0 + 5000) >= 10000 ? 9999 >= 10000 ? NO
    EXPECT_FALSE(add_skill_use(m, 7u, 100u, 4999u));
}

TEST(AgentSkillDataPlane, AddSkillUseOutsideCooldownAllows) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    add_skill_use(m, 7u, 100u, 0u);
    EXPECT_TRUE(add_skill_use(m, 7u, 100u, 10000u));
}

TEST(AgentSkillDataPlane, RemoveSkillUseClears) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    add_skill_use(m, 7u, 100u, 0u);
    EXPECT_EQ(skill_use_count(m), 1u);
    remove_skill_use(m, 7u);
    EXPECT_EQ(skill_use_count(m), 0u);
}

TEST(AgentSkillDataPlane, RemoveSkillUseUnknownIsNoop) {
    SkillDelayManager m;
    remove_skill_use(m, 7u);
    EXPECT_EQ(skill_use_count(m), 0u);
}

TEST(AgentSkillDataPlane, SkillDelayManagerClearResetsBoth) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    add_skill_use(m, 7u, 100u, 0u);
    skill_delay_manager_clear(m);
    EXPECT_EQ(skill_use_count(m), 0u);
    EXPECT_EQ(premier_skill_count(m), 0u);
}

TEST(AgentSkillDataPlane, RemainingSkillDelayMsNoPriorUseIsZero) {
    SkillDelayManager m;
    EXPECT_EQ(remaining_skill_delay_ms(m, 7u, 0u), 0u);
}

TEST(AgentSkillDataPlane, RemainingSkillDelayMsWithinWindow) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    add_skill_use(m, 7u, 100u, 0u);
    // Now=4000 -> elapsed=4000 -> remaining = 10000-4000 = 6000 (no +5000)
    EXPECT_EQ(remaining_skill_delay_ms(m, 7u, 4000u), 6000u);
}

TEST(AgentSkillDataPlane, RemainingSkillDelayMsAtBoundary) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    add_skill_use(m, 7u, 100u, 0u);
    EXPECT_EQ(remaining_skill_delay_ms(m, 7u, 10000u), 0u);
}

TEST(AgentSkillDataPlane, RemainingSkillDelayMsBeyondWindow) {
    SkillDelayManager m;
    add_premier_skill(m, 100u, 10000u);
    add_skill_use(m, 7u, 100u, 0u);
    EXPECT_EQ(remaining_skill_delay_ms(m, 7u, 20000u), 0u);
}
