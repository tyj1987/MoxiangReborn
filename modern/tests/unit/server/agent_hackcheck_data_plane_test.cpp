// agent_hackcheck_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::classify_hackcheck (D4.143).
// Augments the legacy 6-test agent_hackcheck_test.cpp with deeper coverage of:
//   - hackcheck_category constant = 41 (MP_HACKCHECK)
//   - 3 sub-protocol constants (speedhack=0, ban_user=1, ban_user_toagent=2)
//   - speedhack_checktime=10000, speedhack_tolerance_ms=3000
//     -> effective threshold = 7000ms (server_time - client_time < 7000 -> speedhack)
//   - HackCheckActionKind enum (detect_speedhack_and_ban, ban_user_to_agent_always,
//     drop_no_user, ignore)
//   - HackCheckRequest struct defaults (protocol=0, object_id=0,
//     client_time=0, server_time=0, user_found=true)
//   - HackCheckAction struct defaults
//   - classify_hackcheck truth table:
//       speedhack + !user_found -> drop_no_user
//       speedhack + user_found + delta < 7000 -> detect_speedhack_and_ban
//       speedhack + user_found + delta >= 7000 -> ignore
//       speedhack + user_found + server_time < client_time -> ignore (no underflow)
//       ban_user_toagent + !user_found -> drop_no_user
//       ban_user_toagent + user_found -> ban_user_to_agent_always (protocol=ban_user)
//       default -> ignore with original protocol
//
// 1:1 invariants (locked):
//   - hackcheck_category = 41
//   - hackcheck_speedhack=0, hackcheck_ban_user=1, hackcheck_ban_user_toagent=2
//   - speedhack_checktime=10000, speedhack_tolerance_ms=3000
//   - Speedhack trigger: server_time >= client_time AND delta < 7000
//   - Speedhack data field = server_time - client_time
//   - ban_user_toagent always converts to hackcheck_ban_user
//   - user_found=false always drops (both speedhack and ban_user_toagent)

#pragma once

#include "mxh/server/agent_hackcheck.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <set>
#include <type_traits>

namespace {

using mxh::server::classify_hackcheck;
using mxh::server::hackcheck_ban_user;
using mxh::server::hackcheck_ban_user_toagent;
using mxh::server::hackcheck_category;
using mxh::server::hackcheck_speedhack;
using mxh::server::HackCheckAction;
using mxh::server::HackCheckActionKind;
using mxh::server::HackCheckRequest;
using mxh::server::speedhack_checktime;
using mxh::server::speedhack_tolerance_ms;

}  // namespace


// ===========================================================================
// Constants
// ===========================================================================

TEST(HackCheckDataPlane, CategoryIsFortyOne) {
    EXPECT_EQ(hackcheck_category, 41u);
}

TEST(HackCheckDataPlane, SpeedhackProtocolIsZero) { EXPECT_EQ(hackcheck_speedhack, 0u); }
TEST(HackCheckDataPlane, BanUserProtocolIsOne) { EXPECT_EQ(hackcheck_ban_user, 1u); }
TEST(HackCheckDataPlane, BanUserToAgentProtocolIsTwo) { EXPECT_EQ(hackcheck_ban_user_toagent, 2u); }

TEST(HackCheckDataPlane, ProtocolConstantsAllDistinct) {
    std::set<std::uint8_t> seen = {
        hackcheck_speedhack, hackcheck_ban_user, hackcheck_ban_user_toagent,
    };
    EXPECT_EQ(seen.size(), 3u);
}

TEST(HackCheckDataPlane, SpeedhackChecktimeIsTenThousand) {
    EXPECT_EQ(speedhack_checktime, 10000u);
}

TEST(HackCheckDataPlane, SpeedhackToleranceIsThreeThousand) {
    EXPECT_EQ(speedhack_tolerance_ms, 3000u);
}


// ===========================================================================
// Enum types
// ===========================================================================

TEST(HackCheckDataPlane, ActionKindHasFourValues) {
    auto all = {
        HackCheckActionKind::detect_speedhack_and_ban,
        HackCheckActionKind::ban_user_to_agent_always,
        HackCheckActionKind::drop_no_user,
        HackCheckActionKind::ignore,
    };
    EXPECT_EQ(all.size(), 4u);
}

TEST(HackCheckDataPlane, ActionKindUnderlyingTypeIsUint8) {
    EXPECT_TRUE((std::is_same<std::underlying_type_t<HackCheckActionKind>, std::uint8_t>::value));
}


// ===========================================================================
// Struct defaults
// ===========================================================================

TEST(HackCheckDataPlane, RequestDefaults) {
    HackCheckRequest r{};
    EXPECT_EQ(r.protocol, 0u);
    EXPECT_EQ(r.object_id, 0u);
    EXPECT_EQ(r.client_time, 0u);
    EXPECT_EQ(r.server_time, 0u);
    EXPECT_TRUE(r.user_found);
}

TEST(HackCheckDataPlane, ActionDefaults) {
    HackCheckAction a{};
    EXPECT_EQ(a.kind, HackCheckActionKind::ignore);
    EXPECT_EQ(a.protocol, 0u);
    EXPECT_EQ(a.object_id, 0u);
    EXPECT_EQ(a.data, 0u);
}


// ===========================================================================
// classify_hackcheck -- speedhack path
// ===========================================================================

TEST(HackCheckDataPlane, ClassifySpeedhackUserMissingDrops) {
    HackCheckRequest r;
    r.protocol = hackcheck_speedhack;
    r.user_found = false;
    EXPECT_EQ(classify_hackcheck(r).kind, HackCheckActionKind::drop_no_user);
}

TEST(HackCheckDataPlane, ClassifySpeedhackUserMissingPreservesProtocol) {
    HackCheckRequest r;
    r.protocol = hackcheck_speedhack;
    r.user_found = false;
    auto a = classify_hackcheck(r);
    EXPECT_EQ(a.protocol, hackcheck_speedhack);
    EXPECT_EQ(a.data, 0u);
}

TEST(HackCheckDataPlane, ClassifySpeedhackDeltaBelowThresholdDetected) {
    HackCheckRequest r;
    r.protocol = hackcheck_speedhack;
    r.user_found = true;
    r.server_time = 11000;
    r.client_time = 10500;
    auto a = classify_hackcheck(r);
    EXPECT_EQ(a.kind, HackCheckActionKind::detect_speedhack_and_ban);
    EXPECT_EQ(a.protocol, hackcheck_ban_user);
    EXPECT_EQ(a.data, 500u);
}

TEST(HackCheckDataPlane, ClassifySpeedhackDeltaZeroDetected) {
    HackCheckRequest r;
    r.protocol = hackcheck_speedhack;
    r.user_found = true;
    r.server_time = 5000;
    r.client_time = 5000;
    auto a = classify_hackcheck(r);
    EXPECT_EQ(a.kind, HackCheckActionKind::detect_speedhack_and_ban);
    EXPECT_EQ(a.data, 0u);
}

TEST(HackCheckDataPlane, ClassifySpeedhackDeltaExactlyAtThresholdIgnored) {
    // delta == 7000 is NOT < 7000, so ignored.
    HackCheckRequest r;
    r.protocol = hackcheck_speedhack;
    r.user_found = true;
    r.server_time = 17000;
    r.client_time = 10000;
    auto a = classify_hackcheck(r);
    EXPECT_EQ(a.kind, HackCheckActionKind::ignore);
}

TEST(HackCheckDataPlane, ClassifySpeedhackDeltaAboveThresholdIgnored) {
    HackCheckRequest r;
    r.protocol = hackcheck_speedhack;
    r.user_found = true;
    r.server_time = 20000;
    r.client_time = 10000;
    EXPECT_EQ(classify_hackcheck(r).kind, HackCheckActionKind::ignore);
}

TEST(HackCheckDataPlane, ClassifySpeedhackDeltaJustBelowThresholdDetected) {
    // delta = 6999 (one less than threshold) -> detected
    HackCheckRequest r;
    r.protocol = hackcheck_speedhack;
    r.user_found = true;
    r.server_time = 16999;
    r.client_time = 10000;
    auto a = classify_hackcheck(r);
    EXPECT_EQ(a.kind, HackCheckActionKind::detect_speedhack_and_ban);
    EXPECT_EQ(a.data, 6999u);
}

TEST(HackCheckDataPlane, ClassifySpeedhackServerBeforeClientIgnored) {
    // server_time < client_time means negative delta (underflow uint -> huge value).
    // The check (server_time>=client_time) should fail and return ignore.
    HackCheckRequest r;
    r.protocol = hackcheck_speedhack;
    r.user_found = true;
    r.server_time = 1000;
    r.client_time = 2000;
    auto a = classify_hackcheck(r);
    EXPECT_EQ(a.kind, HackCheckActionKind::ignore);
}

TEST(HackCheckDataPlane, ClassifySpeedhackPreservesObjectId) {
    HackCheckRequest r;
    r.protocol = hackcheck_speedhack;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    r.server_time = 11000;
    r.client_time = 10500;
    auto a = classify_hackcheck(r);
    EXPECT_EQ(a.object_id, 0xDEADBEEFu);
}


// ===========================================================================
// classify_hackcheck -- ban_user_toagent path
// ===========================================================================

TEST(HackCheckDataPlane, ClassifyBanUserToAgentUserMissingDrops) {
    HackCheckRequest r;
    r.protocol = hackcheck_ban_user_toagent;
    r.user_found = false;
    EXPECT_EQ(classify_hackcheck(r).kind, HackCheckActionKind::drop_no_user);
}

TEST(HackCheckDataPlane, ClassifyBanUserToAgentUserMissingPreservesProtocol) {
    HackCheckRequest r;
    r.protocol = hackcheck_ban_user_toagent;
    r.user_found = false;
    auto a = classify_hackcheck(r);
    EXPECT_EQ(a.protocol, hackcheck_ban_user_toagent);
}

TEST(HackCheckDataPlane, ClassifyBanUserToAgentUserFoundAlwaysBans) {
    HackCheckRequest r;
    r.protocol = hackcheck_ban_user_toagent;
    r.user_found = true;
    auto a = classify_hackcheck(r);
    EXPECT_EQ(a.kind, HackCheckActionKind::ban_user_to_agent_always);
    EXPECT_EQ(a.protocol, hackcheck_ban_user);
}

TEST(HackCheckDataPlane, ClassifyBanUserToAgentIgnoresTimeFields) {
    HackCheckRequest r;
    r.protocol = hackcheck_ban_user_toagent;
    r.user_found = true;
    r.server_time = 1;
    r.client_time = 0xFFFFFFFFu;
    auto a = classify_hackcheck(r);
    EXPECT_EQ(a.kind, HackCheckActionKind::ban_user_to_agent_always);
}

TEST(HackCheckDataPlane, ClassifyBanUserToAgentPreservesObjectId) {
    HackCheckRequest r;
    r.protocol = hackcheck_ban_user_toagent;
    r.user_found = true;
    r.object_id = 0xCAFEBABEu;
    auto a = classify_hackcheck(r);
    EXPECT_EQ(a.object_id, 0xCAFEBABEu);
}


// ===========================================================================
// classify_hackcheck -- default path
// ===========================================================================

TEST(HackCheckDataPlane, ClassifyUnknownProtocolIgnored) {
    HackCheckRequest r;
    r.protocol = 99;
    EXPECT_EQ(classify_hackcheck(r).kind, HackCheckActionKind::ignore);
}

TEST(HackCheckDataPlane, ClassifyUnknownProtocolPreservesProtocol) {
    HackCheckRequest r;
    r.protocol = 99;
    auto a = classify_hackcheck(r);
    EXPECT_EQ(a.protocol, 99u);
}

TEST(HackCheckDataPlane, ClassifyProtocol255Ignored) {
    HackCheckRequest r;
    r.protocol = 255;
    EXPECT_EQ(classify_hackcheck(r).kind, HackCheckActionKind::ignore);
}

TEST(HackCheckDataPlane, ClassifyProtocolThreeIgnored) {
    HackCheckRequest r;
    r.protocol = 3;
    EXPECT_EQ(classify_hackcheck(r).kind, HackCheckActionKind::ignore);
}


// ===========================================================================
// Threshold math invariant
// ===========================================================================

TEST(HackCheckDataPlane, ThresholdIsChecktimeMinusTolerance) {
    EXPECT_EQ(speedhack_checktime - speedhack_tolerance_ms, 7000u);
}
