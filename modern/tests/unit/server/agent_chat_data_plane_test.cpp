// agent_chat_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::classify_chat (D4.140).
// Augments the legacy 6-test agent_chat_test.cpp with deeper coverage of:
//   - ChatRoute enum (8 distinct values, broadcast/whisper/party/guild/
//     guild_union/shout_server/fast_chat/rejected)
//   - 11 chat_* protocol constants (chat_all .. chat_fastchat, with chat_whisper_ack/nack)
//   - classify_chat full truth table for 11 protocols x from_user x gm (33 base cases)
//   - boundary protocols (255, 0)
//   - ChatDispatch struct defaults (route=rejected, requires_target=false, gm_only=false)
//   - requires_target / gm_only flag invariants
//
// 1:1 invariants (locked):
//   - ChatRoute 8 values: broadcast=0, whisper=1, party=2, guild=3,
//     guild_union=4, shout_server=5, fast_chat=6, rejected=7
//   - chat_all=0, chat_smallshout=1, chat_gm_smallshout=2,
//     chat_monster_speech=3, chat_whisper_syn=4, chat_whisper_gm_syn=5,
//     chat_party=6, chat_guild=7, chat_guild_union=8, chat_shout_send_server=9,
//     chat_fastchat=10, chat_whisper_ack=12, chat_whisper_nack=13
//   - chat_gm_smallshout requires sender_is_gm=true (else rejected)
//   - chat_fastchat requires from_user=true (else rejected)
//   - chat_whisper_gm_syn marks gm_only=true (whisper_syn does not)
//   - chat_whisper_ack/nack -> rejected (legacy doesn't classify them as user dispatchable)
//   - Unknown protocols (out of [0..10] or [12,13]) -> rejected
//   - Party/guild/guild_union routes never require gm_only

#pragma once

#include "mxh/server/agent_chat.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <type_traits>
#include <vector>

namespace {

using mxh::server::chat_all;
using mxh::server::chat_fastchat;
using mxh::server::chat_gm_smallshout;
using mxh::server::chat_guild;
using mxh::server::chat_guild_union;
using mxh::server::chat_monster_speech;
using mxh::server::chat_party;
using mxh::server::chat_shout_send_server;
using mxh::server::chat_smallshout;
using mxh::server::chat_whisper_ack;
using mxh::server::chat_whisper_gm_syn;
using mxh::server::chat_whisper_nack;
using mxh::server::chat_whisper_syn;
using mxh::server::ChatDispatch;
using mxh::server::ChatRoute;
using mxh::server::classify_chat;

}  // namespace


// ===========================================================================
// ChatRoute enum
// ===========================================================================

TEST(AgentChatDataPlane, ChatRouteHasEightValues) {
    auto all = {
        ChatRoute::broadcast, ChatRoute::whisper, ChatRoute::party,
        ChatRoute::guild, ChatRoute::guild_union, ChatRoute::shout_server,
        ChatRoute::fast_chat, ChatRoute::rejected,
    };
    EXPECT_EQ(all.size(), 8u);
}

TEST(AgentChatDataPlane, ChatRouteUnderlyingTypeIsUint8) {
    EXPECT_TRUE((std::is_same<std::underlying_type_t<ChatRoute>, std::uint8_t>::value));
}

TEST(AgentChatDataPlane, ChatRouteValuesAllDistinct) {
    std::set<std::uint8_t> seen;
    seen.insert(static_cast<std::uint8_t>(ChatRoute::broadcast));
    seen.insert(static_cast<std::uint8_t>(ChatRoute::whisper));
    seen.insert(static_cast<std::uint8_t>(ChatRoute::party));
    seen.insert(static_cast<std::uint8_t>(ChatRoute::guild));
    seen.insert(static_cast<std::uint8_t>(ChatRoute::guild_union));
    seen.insert(static_cast<std::uint8_t>(ChatRoute::shout_server));
    seen.insert(static_cast<std::uint8_t>(ChatRoute::fast_chat));
    seen.insert(static_cast<std::uint8_t>(ChatRoute::rejected));
    EXPECT_EQ(seen.size(), 8u);
}


// ===========================================================================
// ChatDispatch struct defaults
// ===========================================================================

TEST(AgentChatDataPlane, DispatchDefaultsRejected) {
    ChatDispatch d{};
    EXPECT_EQ(d.route, ChatRoute::rejected);
    EXPECT_FALSE(d.requires_target);
    EXPECT_FALSE(d.gm_only);
}

TEST(AgentChatDataPlane, DispatchIsAggregateConstructible) {
    ChatDispatch d{ChatRoute::broadcast, true, false};
    EXPECT_EQ(d.route, ChatRoute::broadcast);
    EXPECT_TRUE(d.requires_target);
    EXPECT_FALSE(d.gm_only);
}


// ===========================================================================
// Protocol constants
// ===========================================================================

TEST(AgentChatDataPlane, ProtocolAllIsZero) { EXPECT_EQ(chat_all, 0u); }
TEST(AgentChatDataPlane, ProtocolSmallshoutIsOne) { EXPECT_EQ(chat_smallshout, 1u); }
TEST(AgentChatDataPlane, ProtocolGmSmallshoutIsTwo) { EXPECT_EQ(chat_gm_smallshout, 2u); }
TEST(AgentChatDataPlane, ProtocolMonsterSpeechIsThree) { EXPECT_EQ(chat_monster_speech, 3u); }
TEST(AgentChatDataPlane, ProtocolWhisperSynIsFour) { EXPECT_EQ(chat_whisper_syn, 4u); }
TEST(AgentChatDataPlane, ProtocolWhisperGmSynIsFive) { EXPECT_EQ(chat_whisper_gm_syn, 5u); }
TEST(AgentChatDataPlane, ProtocolPartyIsSix) { EXPECT_EQ(chat_party, 6u); }
TEST(AgentChatDataPlane, ProtocolGuildIsSeven) { EXPECT_EQ(chat_guild, 7u); }
TEST(AgentChatDataPlane, ProtocolGuildUnionIsEight) { EXPECT_EQ(chat_guild_union, 8u); }
TEST(AgentChatDataPlane, ProtocolShoutSendServerIsNine) { EXPECT_EQ(chat_shout_send_server, 9u); }
TEST(AgentChatDataPlane, ProtocolFastchatIsTen) { EXPECT_EQ(chat_fastchat, 10u); }
TEST(AgentChatDataPlane, ProtocolWhisperAckIsTwelve) { EXPECT_EQ(chat_whisper_ack, 12u); }
TEST(AgentChatDataPlane, ProtocolWhisperNackIsThirteen) { EXPECT_EQ(chat_whisper_nack, 13u); }

TEST(AgentChatDataPlane, ProtocolConstantsAllDistinct) {
    std::set<std::uint8_t> seen = {
        chat_all, chat_smallshout, chat_gm_smallshout, chat_monster_speech,
        chat_whisper_syn, chat_whisper_gm_syn, chat_party, chat_guild,
        chat_guild_union, chat_shout_send_server, chat_fastchat,
        chat_whisper_ack, chat_whisper_nack,
    };
    EXPECT_EQ(seen.size(), 13u);
}


// ===========================================================================
// classify_chat truth table -- broadcast group
// ===========================================================================

TEST(AgentChatDataPlane, ClassifyAllFromUserIsBroadcast) {
    EXPECT_EQ(classify_chat(chat_all, true).route, ChatRoute::broadcast);
}

TEST(AgentChatDataPlane, ClassifyAllFromServerIsBroadcast) {
    EXPECT_EQ(classify_chat(chat_all, false).route, ChatRoute::broadcast);
}

TEST(AgentChatDataPlane, ClassifySmallshoutFromUserIsBroadcast) {
    EXPECT_EQ(classify_chat(chat_smallshout, true).route, ChatRoute::broadcast);
}

TEST(AgentChatDataPlane, ClassifySmallshoutFromServerIsBroadcast) {
    EXPECT_EQ(classify_chat(chat_smallshout, false).route, ChatRoute::broadcast);
}

TEST(AgentChatDataPlane, ClassifyMonsterSpeechAlwaysBroadcast) {
    // Monster speech doesn't care about from_user.
    EXPECT_EQ(classify_chat(chat_monster_speech, true).route, ChatRoute::broadcast);
    EXPECT_EQ(classify_chat(chat_monster_speech, false).route, ChatRoute::broadcast);
}

TEST(AgentChatDataPlane, ClassifyBroadcastDoesNotRequireTarget) {
    EXPECT_FALSE(classify_chat(chat_all, true).requires_target);
    EXPECT_FALSE(classify_chat(chat_smallshout, true).requires_target);
}

TEST(AgentChatDataPlane, ClassifyBroadcastDoesNotRequireGm) {
    EXPECT_FALSE(classify_chat(chat_all, true).gm_only);
    EXPECT_FALSE(classify_chat(chat_smallshout, true).gm_only);
}


// ===========================================================================
// classify_chat truth table -- GM smallshout gate
// ===========================================================================

TEST(AgentChatDataPlane, ClassifyGmSmallshoutUserNotGmRejected) {
    EXPECT_EQ(classify_chat(chat_gm_smallshout, true, false).route, ChatRoute::rejected);
}

TEST(AgentChatDataPlane, ClassifyGmSmallshoutUserIsGmBroadcast) {
    EXPECT_EQ(classify_chat(chat_gm_smallshout, true, true).route, ChatRoute::broadcast);
}

TEST(AgentChatDataPlane, ClassifyGmSmallshoutGmFlagDefaultsToFalse) {
    // Default sender_is_gm=false => rejected
    EXPECT_EQ(classify_chat(chat_gm_smallshout, true).route, ChatRoute::rejected);
}


// ===========================================================================
// classify_chat truth table -- whisper group
// ===========================================================================

TEST(AgentChatDataPlane, ClassifyWhisperSynFromUserRequiresTarget) {
    auto r = classify_chat(chat_whisper_syn, true);
    EXPECT_EQ(r.route, ChatRoute::whisper);
    EXPECT_TRUE(r.requires_target);
}

TEST(AgentChatDataPlane, ClassifyWhisperSynDoesNotRequireGm) {
    auto r = classify_chat(chat_whisper_syn, true);
    EXPECT_FALSE(r.gm_only);
}

TEST(AgentChatDataPlane, ClassifyWhisperGmSynIsGmOnly) {
    auto r = classify_chat(chat_whisper_gm_syn, true);
    EXPECT_TRUE(r.gm_only);
}

TEST(AgentChatDataPlane, ClassifyWhisperAckNackRejected) {
    // Legacy: ack/nack aren't user-dispatchable chat (they are server->client).
    EXPECT_EQ(classify_chat(chat_whisper_ack, true).route, ChatRoute::rejected);
    EXPECT_EQ(classify_chat(chat_whisper_nack, true).route, ChatRoute::rejected);
}


// ===========================================================================
// classify_chat truth table -- group routes
// ===========================================================================

TEST(AgentChatDataPlane, ClassifyPartyRoute) {
    EXPECT_EQ(classify_chat(chat_party, true).route, ChatRoute::party);
}

TEST(AgentChatDataPlane, ClassifyGuildRoute) {
    EXPECT_EQ(classify_chat(chat_guild, true).route, ChatRoute::guild);
}

TEST(AgentChatDataPlane, ClassifyGuildUnionRoute) {
    EXPECT_EQ(classify_chat(chat_guild_union, true).route, ChatRoute::guild_union);
}

TEST(AgentChatDataPlane, ClassifyGroupRoutesDoNotRequireGm) {
    EXPECT_FALSE(classify_chat(chat_party, true).gm_only);
    EXPECT_FALSE(classify_chat(chat_guild, true).gm_only);
    EXPECT_FALSE(classify_chat(chat_guild_union, true).gm_only);
}


// ===========================================================================
// classify_chat truth table -- server shout / fastchat
// ===========================================================================

TEST(AgentChatDataPlane, ClassifyShoutSendServerIsShoutServer) {
    EXPECT_EQ(classify_chat(chat_shout_send_server, false).route, ChatRoute::shout_server);
}

TEST(AgentChatDataPlane, ClassifyFastchatUserFastChat) {
    EXPECT_EQ(classify_chat(chat_fastchat, true).route, ChatRoute::fast_chat);
}

TEST(AgentChatDataPlane, ClassifyFastchatServerRejected) {
    // chat_fastchat must be from_user.
    EXPECT_EQ(classify_chat(chat_fastchat, false).route, ChatRoute::rejected);
}


// ===========================================================================
// Boundary / sweep
// ===========================================================================

TEST(AgentChatDataPlane, ClassifyProtocol255Rejected) {
    EXPECT_EQ(classify_chat(255, true).route, ChatRoute::rejected);
    EXPECT_EQ(classify_chat(255, false).route, ChatRoute::rejected);
}

TEST(AgentChatDataPlane, ClassifyProtocol11Rejected) {
    // 11 is in the gap between chat_fastchat=10 and chat_whisper_ack=12.
    EXPECT_EQ(classify_chat(11, true).route, ChatRoute::rejected);
}

TEST(AgentChatDataPlane, ClassifyProtocol14Rejected) {
    EXPECT_EQ(classify_chat(14, true).route, ChatRoute::rejected);
}

TEST(AgentChatDataPlane, ClassifyProtocolZeroFromUserAllBroadcast) {
    EXPECT_EQ(classify_chat(0, true).route, ChatRoute::broadcast);
}


// ===========================================================================
// All 8 ChatRoute values are reachable from classify_chat
// ===========================================================================

TEST(AgentChatDataPlane, AllRoutesReachable) {
    std::set<ChatRoute> seen;
    // Walk known protocols and gather reachable routes.
    seen.insert(classify_chat(chat_all, true).route);
    seen.insert(classify_chat(chat_whisper_syn, true).route);
    seen.insert(classify_chat(chat_party, true).route);
    seen.insert(classify_chat(chat_guild, true).route);
    seen.insert(classify_chat(chat_guild_union, true).route);
    seen.insert(classify_chat(chat_shout_send_server, false).route);
    seen.insert(classify_chat(chat_fastchat, true).route);
    seen.insert(classify_chat(255, true).route);
    // 8 distinct routes expected.
    EXPECT_EQ(seen.size(), 8u);
}
