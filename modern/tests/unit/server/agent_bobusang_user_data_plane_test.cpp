
// agent_bobusang_user_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::agent_bobusang_user (D4.137).
// Augments the legacy 4-test agent_bobusang_user_test.cpp with deeper coverage of:
//   - MP_BOBUSANG category byte = 73.
//   - BobusangUserActionKind 3-value enum (forward_to_map=0,
//     drop_no_user=1, drop_wrong_gm_power=2).
//   - classify_bobusang_user() full truth-table sweep.
//   - BobusangUserAction struct preserves protocol + object_id.
//   - BobusangUserRequest struct defaults.
//
// 1:1 invariants (locked):
//   - bobusang_user_category = 73 (MP_BOBUSANG).
//   - classify_bobusang_user truth table:
//       user_found=false (any is_gm/gm_master) -> drop_no_user
//       user_found=true, is_gm=false -> forward_to_map
//       user_found=true, is_gm=true, gm_master_or_below=true -> forward_to_map
//       user_found=true, is_gm=true, gm_master_or_below=false -> drop_wrong_gm_power
//   - BobusangUserAction preserves input protocol + object_id.

#pragma once

#include "mxh/server/agent_bobusang_user.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>

namespace {

using mxh::server::bobusang_user_category;
using mxh::server::BobusangUserAction;
using mxh::server::BobusangUserActionKind;
using mxh::server::BobusangUserRequest;
using mxh::server::classify_bobusang_user;

}  // namespace


// ===========================================================================
// Wire constants
// ===========================================================================

TEST(AgentBobusangUserDataPlane, CategoryByteIsSeventyThree) {
    EXPECT_EQ(bobusang_user_category, 73u);
}


// ===========================================================================
// BobusangUserActionKind enum values
// ===========================================================================

TEST(AgentBobusangUserDataPlane, KindForwardToMapIsZero) {
    EXPECT_EQ(static_cast<std::uint8_t>(BobusangUserActionKind::forward_to_map), 0u);
}

TEST(AgentBobusangUserDataPlane, KindDropNoUserIsOne) {
    EXPECT_EQ(static_cast<std::uint8_t>(BobusangUserActionKind::drop_no_user), 1u);
}

TEST(AgentBobusangUserDataPlane, KindDropWrongGmPowerIsTwo) {
    EXPECT_EQ(static_cast<std::uint8_t>(BobusangUserActionKind::drop_wrong_gm_power), 2u);
}

TEST(AgentBobusangUserDataPlane, AllKindsAreDistinct) {
    EXPECT_NE(BobusangUserActionKind::forward_to_map, BobusangUserActionKind::drop_no_user);
    EXPECT_NE(BobusangUserActionKind::forward_to_map, BobusangUserActionKind::drop_wrong_gm_power);
    EXPECT_NE(BobusangUserActionKind::drop_no_user, BobusangUserActionKind::drop_wrong_gm_power);
}

TEST(AgentBobusangUserDataPlane, KindsAreScopedEnum) {
    static_assert(std::is_enum<BobusangUserActionKind>::value,
                  "BobusangUserActionKind must be enum");
    static_assert(!std::is_convertible<BobusangUserActionKind, int>::value,
                  "BobusangUserActionKind must be scoped enum class");
    EXPECT_TRUE(true);
}

TEST(AgentBobusangUserDataPlane, KindsUnderlyingIsUint8) {
    static_assert(std::is_same<std::underlying_type_t<BobusangUserActionKind>,
                               std::uint8_t>::value,
                  "BobusangUserActionKind underlying must be uint8_t");
    EXPECT_TRUE(true);
}


// ===========================================================================
// BobusangUserRequest defaults
// ===========================================================================

TEST(AgentBobusangUserDataPlane, RequestDefaultValues) {
    BobusangUserRequest r{};
    EXPECT_EQ(r.protocol, 0u);
    EXPECT_EQ(r.object_id, 0u);
    EXPECT_TRUE(r.user_found);
    EXPECT_FALSE(r.is_gm);
    EXPECT_TRUE(r.gm_master_or_below);
}

TEST(AgentBobusangUserDataPlane, RequestFieldsAreAssignable) {
    BobusangUserRequest r{};
    r.protocol = 5u;
    r.object_id = 12345u;
    r.user_found = false;
    r.is_gm = true;
    r.gm_master_or_below = false;
    EXPECT_EQ(r.protocol, 5u);
    EXPECT_EQ(r.object_id, 12345u);
    EXPECT_FALSE(r.user_found);
    EXPECT_TRUE(r.is_gm);
    EXPECT_FALSE(r.gm_master_or_below);
}


// ===========================================================================
// classify_bobusang_user - 4-row truth table
// ===========================================================================

TEST(AgentBobusangUserDataPlane, NoUserDrops) {
    BobusangUserRequest r{};
    r.user_found = false;
    auto a = classify_bobusang_user(r);
    EXPECT_EQ(a.kind, BobusangUserActionKind::drop_no_user);
}

TEST(AgentBobusangUserDataPlane, NonGmForwardsToMap) {
    BobusangUserRequest r{};
    r.user_found = true;
    r.is_gm = false;
    auto a = classify_bobusang_user(r);
    EXPECT_EQ(a.kind, BobusangUserActionKind::forward_to_map);
}

TEST(AgentBobusangUserDataPlane, GmWithMasterPowerForwards) {
    BobusangUserRequest r{};
    r.user_found = true;
    r.is_gm = true;
    r.gm_master_or_below = true;
    auto a = classify_bobusang_user(r);
    EXPECT_EQ(a.kind, BobusangUserActionKind::forward_to_map);
}

TEST(AgentBobusangUserDataPlane, GmAboveMasterDrops) {
    BobusangUserRequest r{};
    r.user_found = true;
    r.is_gm = true;
    r.gm_master_or_below = false;
    auto a = classify_bobusang_user(r);
    EXPECT_EQ(a.kind, BobusangUserActionKind::drop_wrong_gm_power);
}


// ===========================================================================
// Priority test: user_found overrides is_gm
// ===========================================================================

TEST(AgentBobusangUserDataPlane, NoUserBeatsIsGm) {
    // user_found=false should always drop_no_user even if is_gm=true
    BobusangUserRequest r{};
    r.user_found = false;
    r.is_gm = true;
    r.gm_master_or_below = false;  // would normally trigger drop_wrong_gm_power
    auto a = classify_bobusang_user(r);
    EXPECT_EQ(a.kind, BobusangUserActionKind::drop_no_user);
}

TEST(AgentBobusangUserDataPlane, NoUserBeatsGmMasterBelow) {
    BobusangUserRequest r{};
    r.user_found = false;
    r.is_gm = true;
    r.gm_master_or_below = true;  // would normally forward
    auto a = classify_bobusang_user(r);
    EXPECT_EQ(a.kind, BobusangUserActionKind::drop_no_user);
}

TEST(AgentBobusangUserDataPlane, UserFoundNonGmForwardsRegardlessOfMaster) {
    BobusangUserRequest r{};
    r.user_found = true;
    r.is_gm = false;
    r.gm_master_or_below = false;  // ignored when not GM
    auto a = classify_bobusang_user(r);
    EXPECT_EQ(a.kind, BobusangUserActionKind::forward_to_map);
}


// ===========================================================================
// Action struct preserves input fields
// ===========================================================================

TEST(AgentBobusangUserDataPlane, ActionPreservesProtocolAndObjectId) {
    BobusangUserRequest r{};
    r.protocol = 7u;
    r.object_id = 99999u;
    r.user_found = false;
    auto a = classify_bobusang_user(r);
    EXPECT_EQ(a.protocol, 7u);
    EXPECT_EQ(a.object_id, 99999u);
    EXPECT_EQ(a.kind, BobusangUserActionKind::drop_no_user);
}

TEST(AgentBobusangUserDataPlane, ActionPreservesForForwardToMap) {
    BobusangUserRequest r{};
    r.protocol = 3u;
    r.object_id = 5000u;
    r.user_found = true;
    r.is_gm = false;
    auto a = classify_bobusang_user(r);
    EXPECT_EQ(a.protocol, 3u);
    EXPECT_EQ(a.object_id, 5000u);
    EXPECT_EQ(a.kind, BobusangUserActionKind::forward_to_map);
}

TEST(AgentBobusangUserDataPlane, ActionPreservesForDropWrongGm) {
    BobusangUserRequest r{};
    r.protocol = 11u;
    r.object_id = 7777u;
    r.user_found = true;
    r.is_gm = true;
    r.gm_master_or_below = false;
    auto a = classify_bobusang_user(r);
    EXPECT_EQ(a.protocol, 11u);
    EXPECT_EQ(a.object_id, 7777u);
    EXPECT_EQ(a.kind, BobusangUserActionKind::drop_wrong_gm_power);
}


// ===========================================================================
// Boundary values
// ===========================================================================

TEST(AgentBobusangUserDataPlane, ObjectIdMaxValue) {
    BobusangUserRequest r{};
    r.protocol = 0u;
    r.object_id = 0xFFFFFFFFu;
    r.user_found = false;
    auto a = classify_bobusang_user(r);
    EXPECT_EQ(a.object_id, 0xFFFFFFFFu);
}

TEST(AgentBobusangUserDataPlane, ProtocolMaxValue) {
    BobusangUserRequest r{};
    r.protocol = 0xFFu;
    r.user_found = false;
    auto a = classify_bobusang_user(r);
    EXPECT_EQ(a.protocol, 0xFFu);
}


// ===========================================================================
// Field types
// ===========================================================================

TEST(AgentBobusangUserDataPlane, RequestProtocolFieldType) {
    static_assert(std::is_same<decltype(BobusangUserRequest{}.protocol),
                               std::uint8_t>::value,
                  "protocol must be uint8_t");
    EXPECT_TRUE(true);
}

TEST(AgentBobusangUserDataPlane, RequestObjectIdFieldType) {
    static_assert(std::is_same<decltype(BobusangUserRequest{}.object_id),
                               std::uint32_t>::value,
                  "object_id must be uint32_t");
    EXPECT_TRUE(true);
}

TEST(AgentBobusangUserDataPlane, RequestUserFoundFieldType) {
    static_assert(std::is_same<decltype(BobusangUserRequest{}.user_found),
                               bool>::value,
                  "user_found must be bool");
    EXPECT_TRUE(true);
}
