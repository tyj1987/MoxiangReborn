// agent_guild_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::classify_guild_user +
// classify_guild_server_default (D4.141).
// Augments the legacy 5-test agent_guild_test.cpp with deeper coverage of:
//   - guild_category constant = 63 (MP_GUILD)
//   - protocol constants (create_syn=2, givenickname_syn=20, givenickname_nack=21, create_nack=3)
//   - error_code constants (guild_err_create_name=4, guild_err_nick_filter=1)
//   - GuildActionKind enum (forward, send_nack)
//   - GuildUserRequest struct defaults (object_id=0, usable_name=true,
//     has_invalid_char=false, has_quote_space=false, is_nickname_path=false)
//   - GuildAction struct defaults
//   - classify_guild_user truth table:
//       reject_path: has_invalid_char || !usable_name -> send_nack
//       forward_path: usable_name && !has_invalid_char -> forward
//       protocol+error_code routing by is_nickname_path
//       object_id preservation
//   - classify_guild_server_default: protocol-preserving forward (legacy stub)
//   - has_quote_space is ignored (legacy quirk -- only has_invalid_char gates)
//
// 1:1 invariants (locked):
//   - guild_category = 63
//   - guild_create_syn=2, guild_givenickname_syn=20, guild_givenickname_nack=21, guild_create_nack=3
//   - guild_err_create_name=4, guild_err_nick_filter=1
//   - Reject path: (has_invalid_char OR !usable_name) => send_nack
//   - Reject path: protocol = is_nickname_path ? guild_givenickname_nack : guild_create_nack
//   - Reject path: error_code = is_nickname_path ? guild_err_nick_filter : guild_err_create_name
//   - Forward path: protocol = is_nickname_path ? guild_givenickname_syn : guild_create_syn
//   - Forward path: error_code = 0
//   - classify_guild_server_default always returns forward (legacy default)

#pragma once

#include "mxh/server/agent_guild.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <type_traits>

namespace {

using mxh::server::classify_guild_server_default;
using mxh::server::classify_guild_user;
using mxh::server::guild_category;
using mxh::server::guild_create_nack;
using mxh::server::guild_create_syn;
using mxh::server::guild_err_create_name;
using mxh::server::guild_err_nick_filter;
using mxh::server::guild_givenickname_nack;
using mxh::server::guild_givenickname_syn;
using mxh::server::GuildAction;
using mxh::server::GuildActionKind;
using mxh::server::GuildUserRequest;

}  // namespace


// ===========================================================================
// guild_category constant
// ===========================================================================

TEST(AgentGuildDataPlane, GuildCategoryIsSixtyThree) {
    EXPECT_EQ(guild_category, 63u);
}


// ===========================================================================
// Protocol constants distinct values
// ===========================================================================

TEST(AgentGuildDataPlane, ProtocolCreateSynIsTwo) { EXPECT_EQ(guild_create_syn, 2u); }
TEST(AgentGuildDataPlane, ProtocolGivenicknameSynIsTwenty) { EXPECT_EQ(guild_givenickname_syn, 20u); }
TEST(AgentGuildDataPlane, ProtocolGivenicknameNackIsTwentyOne) { EXPECT_EQ(guild_givenickname_nack, 21u); }
TEST(AgentGuildDataPlane, ProtocolCreateNackIsThree) { EXPECT_EQ(guild_create_nack, 3u); }

TEST(AgentGuildDataPlane, ProtocolConstantsAllDistinct) {
    std::set<std::uint8_t> seen = {
        guild_create_syn, guild_givenickname_syn, guild_givenickname_nack, guild_create_nack,
    };
    EXPECT_EQ(seen.size(), 4u);
}

TEST(AgentGuildDataPlane, ErrorCodeCreateNameIsFour) { EXPECT_EQ(guild_err_create_name, 4u); }
TEST(AgentGuildDataPlane, ErrorCodeNickFilterIsOne) { EXPECT_EQ(guild_err_nick_filter, 1u); }


// ===========================================================================
// GuildActionKind enum
// ===========================================================================

TEST(AgentGuildDataPlane, ActionKindHasTwoValues) {
    auto all = { GuildActionKind::forward, GuildActionKind::send_nack };
    EXPECT_EQ(all.size(), 2u);
}

TEST(AgentGuildDataPlane, ActionKindForwardIsZero) {
    EXPECT_EQ(static_cast<std::uint8_t>(GuildActionKind::forward), 0u);
}

TEST(AgentGuildDataPlane, ActionKindSendNackIsOne) {
    EXPECT_EQ(static_cast<std::uint8_t>(GuildActionKind::send_nack), 1u);
}


// ===========================================================================
// Struct defaults
// ===========================================================================

TEST(AgentGuildDataPlane, UserRequestDefaults) {
    GuildUserRequest r{};
    EXPECT_EQ(r.object_id, 0u);
    EXPECT_TRUE(r.usable_name);
    EXPECT_FALSE(r.has_invalid_char);
    EXPECT_FALSE(r.has_quote_space);
    EXPECT_FALSE(r.is_nickname_path);
}

TEST(AgentGuildDataPlane, ActionDefaults) {
    GuildAction a{};
    EXPECT_EQ(a.kind, GuildActionKind::forward);
    EXPECT_EQ(a.protocol, 0u);
    EXPECT_EQ(a.object_id, 0u);
    EXPECT_EQ(a.error_code, 0u);
}


// ===========================================================================
// classify_guild_user -- forward path
// ===========================================================================

TEST(AgentGuildDataPlane, ClassifyUsableNameIsForward) {
    GuildUserRequest r;
    r.object_id = 7;
    r.usable_name = true;
    auto a = classify_guild_user(r);
    EXPECT_EQ(a.kind, GuildActionKind::forward);
}

TEST(AgentGuildDataPlane, ClassifyUsableNameDefaultCreatePathUsesCreateSyn) {
    GuildUserRequest r;
    r.object_id = 7;
    auto a = classify_guild_user(r);
    EXPECT_EQ(a.kind, GuildActionKind::forward);
    EXPECT_EQ(a.protocol, guild_create_syn);
}

TEST(AgentGuildDataPlane, ClassifyUsableNameNicknamePathUsesGivenicknameSyn) {
    GuildUserRequest r;
    r.object_id = 7;
    r.is_nickname_path = true;
    auto a = classify_guild_user(r);
    EXPECT_EQ(a.kind, GuildActionKind::forward);
    EXPECT_EQ(a.protocol, guild_givenickname_syn);
}

TEST(AgentGuildDataPlane, ClassifyForwardPathZeroErrorCode) {
    GuildUserRequest r;
    r.object_id = 7;
    auto a = classify_guild_user(r);
    EXPECT_EQ(a.error_code, 0u);
}


// ===========================================================================
// classify_guild_user -- reject path
// ===========================================================================

TEST(AgentGuildDataPlane, ClassifyNotUsableNameIsSendNack) {
    GuildUserRequest r;
    r.object_id = 7;
    r.usable_name = false;
    auto a = classify_guild_user(r);
    EXPECT_EQ(a.kind, GuildActionKind::send_nack);
}

TEST(AgentGuildDataPlane, ClassifyHasInvalidCharIsSendNack) {
    GuildUserRequest r;
    r.object_id = 7;
    r.has_invalid_char = true;
    auto a = classify_guild_user(r);
    EXPECT_EQ(a.kind, GuildActionKind::send_nack);
}

TEST(AgentGuildDataPlane, ClassifyNotUsableCreatePathUsesCreateNack) {
    GuildUserRequest r;
    r.object_id = 7;
    r.usable_name = false;
    auto a = classify_guild_user(r);
    EXPECT_EQ(a.protocol, guild_create_nack);
    EXPECT_EQ(a.error_code, guild_err_create_name);
}

TEST(AgentGuildDataPlane, ClassifyNotUsableNicknamePathUsesGivenicknameNack) {
    GuildUserRequest r;
    r.object_id = 7;
    r.usable_name = false;
    r.is_nickname_path = true;
    auto a = classify_guild_user(r);
    EXPECT_EQ(a.protocol, guild_givenickname_nack);
    EXPECT_EQ(a.error_code, guild_err_nick_filter);
}

TEST(AgentGuildDataPlane, ClassifyInvalidCharCreatePathUsesCreateNack) {
    GuildUserRequest r;
    r.object_id = 7;
    r.has_invalid_char = true;
    auto a = classify_guild_user(r);
    EXPECT_EQ(a.protocol, guild_create_nack);
    EXPECT_EQ(a.error_code, guild_err_create_name);
}

TEST(AgentGuildDataPlane, ClassifyInvalidCharNicknamePathUsesGivenicknameNack) {
    GuildUserRequest r;
    r.object_id = 7;
    r.has_invalid_char = true;
    r.is_nickname_path = true;
    auto a = classify_guild_user(r);
    EXPECT_EQ(a.protocol, guild_givenickname_nack);
    EXPECT_EQ(a.error_code, guild_err_nick_filter);
}


// ===========================================================================
// classify_guild_user -- field interactions
// ===========================================================================

TEST(AgentGuildDataPlane, ClassifyHasInvalidCharOverridesUsableName) {
    GuildUserRequest r;
    r.object_id = 7;
    r.usable_name = true;
    r.has_invalid_char = true;
    auto a = classify_guild_user(r);
    EXPECT_EQ(a.kind, GuildActionKind::send_nack);
}

TEST(AgentGuildDataPlane, ClassifyHasQuoteSpaceIgnoredWhenUsable) {
    // has_quote_space is a legacy hint that does NOT gate by itself.
    GuildUserRequest r;
    r.object_id = 7;
    r.has_quote_space = true;
    auto a = classify_guild_user(r);
    EXPECT_EQ(a.kind, GuildActionKind::forward);
}

TEST(AgentGuildDataPlane, ClassifyPreservesObjectIdForward) {
    GuildUserRequest r;
    r.object_id = 0xDEADBEEFu;
    auto a = classify_guild_user(r);
    EXPECT_EQ(a.object_id, 0xDEADBEEFu);
}

TEST(AgentGuildDataPlane, ClassifyPreservesObjectIdNack) {
    GuildUserRequest r;
    r.object_id = 0xDEADBEEFu;
    r.usable_name = false;
    auto a = classify_guild_user(r);
    EXPECT_EQ(a.object_id, 0xDEADBEEFu);
}

TEST(AgentGuildDataPlane, ClassifyObjectIdMaxUint32) {
    GuildUserRequest r;
    r.object_id = 0xFFFFFFFFu;
    auto a = classify_guild_user(r);
    EXPECT_EQ(a.object_id, 0xFFFFFFFFu);
}


// ===========================================================================
// classify_guild_server_default
// ===========================================================================

TEST(AgentGuildDataPlane, ServerDefaultProtocolZeroIsForward) {
    EXPECT_EQ(classify_guild_server_default(0).kind, GuildActionKind::forward);
}

TEST(AgentGuildDataPlane, ServerDefaultProtocolTenIsForward) {
    EXPECT_EQ(classify_guild_server_default(10).kind, GuildActionKind::forward);
}

TEST(AgentGuildDataPlane, ServerDefaultProtocolTwoFiftyFiveIsForward) {
    EXPECT_EQ(classify_guild_server_default(255).kind, GuildActionKind::forward);
}

TEST(AgentGuildDataPlane, ServerDefaultPreservesProtocol) {
    EXPECT_EQ(classify_guild_server_default(42).protocol, 42u);
    EXPECT_EQ(classify_guild_server_default(255).protocol, 255u);
}
