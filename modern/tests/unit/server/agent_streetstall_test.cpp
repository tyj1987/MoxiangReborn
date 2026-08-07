//
// 1:1 port of MP_STREETSTALLUserMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp lines 5083-5092.

#include <mxh/server/agent_streetstall.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

namespace {
constexpr std::uint32_t kObjectId = 0x12345678u;
constexpr std::uint32_t kOtherId = 0x87654321u;
}

TEST(AgentStreetStallClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(streetstall_category, 29u);
}

TEST(AgentStreetStallClassify, UserNotFoundDrops) {
    StreetStallUserRequest r;
    r.protocol = streetstall_open_syn;
    r.dw_object_id = kObjectId;
    r.dw_user_character_id = kObjectId;
    r.user_found = false;
    auto a = classify_streetstall_user(r);
    EXPECT_EQ(a.kind, StreetStallUserActionKind::drop_no_user);
    EXPECT_EQ(a.reply_protocol, streetstall_open_syn);
    EXPECT_FALSE(a.forward_payload);
}

TEST(AgentStreetStallClassify, ObjectIdMismatchDrops) {
    StreetStallUserRequest r;
    r.protocol = streetstall_buyitem_syn;
    r.dw_object_id = kObjectId;
    r.dw_user_character_id = kOtherId;
    r.user_found = true;
    auto a = classify_streetstall_user(r);
    EXPECT_EQ(a.kind, StreetStallUserActionKind::drop_object_id_mismatch);
    EXPECT_FALSE(a.forward_payload);
}

TEST(AgentStreetStallClassify, MatchedObjectIdForwardsToMapServer) {
    StreetStallUserRequest r;
    r.protocol = streetstall_open_syn;
    r.dw_object_id = kObjectId;
    r.dw_user_character_id = kObjectId;
    r.user_found = true;
    auto a = classify_streetstall_user(r);
    EXPECT_EQ(a.kind, StreetStallUserActionKind::forward_to_map_server);
    EXPECT_EQ(a.reply_protocol, streetstall_open_syn);
    EXPECT_EQ(a.dw_object_id, kObjectId);
    EXPECT_TRUE(a.forward_payload);
}

TEST(AgentStreetStallClassify, MultipleProtocolsForwardCorrectly) {
    for (std::uint8_t p : {streetstall_open_syn, streetstall_close_syn, streetstall_buyitem_syn,
                         streetstall_sellitem_syn, streetstall_guestin_syn, streetstall_guestout_syn,
                         streetstall_edittitle_syn, streetstall_update_syn, streetstall_updateend_syn}) {
        StreetStallUserRequest r;
        r.protocol = p;
        r.dw_object_id = kObjectId;
        r.dw_user_character_id = kObjectId;
        r.user_found = true;
        auto a = classify_streetstall_user(r);
        EXPECT_EQ(a.kind, StreetStallUserActionKind::forward_to_map_server);
        EXPECT_EQ(a.reply_protocol, p);
    }
}

TEST(AgentStreetStallClassify, UnknownProtocolStillForwardsWhenValid) {
    // Legacy: no switch on protocol in the handler. Any sub-protocol byte
    // with matching dwObjectID + found user gets forwarded.
    StreetStallUserRequest r;
    r.protocol = 200u;
    r.dw_object_id = kObjectId;
    r.dw_user_character_id = kObjectId;
    r.user_found = true;
    auto a = classify_streetstall_user(r);
    EXPECT_EQ(a.kind, StreetStallUserActionKind::forward_to_map_server);
    EXPECT_EQ(a.reply_protocol, 200u);
}

TEST(AgentStreetStallClassify, ObjectIdMismatchTakesPrecedenceOverProtocol) {
    StreetStallUserRequest r;
    r.protocol = streetstall_buyitem_syn;
    r.dw_object_id = kObjectId;
    r.dw_user_character_id = kOtherId;
    r.user_found = true;
    auto a = classify_streetstall_user(r);
    EXPECT_EQ(a.kind, StreetStallUserActionKind::drop_object_id_mismatch);
    EXPECT_EQ(a.reply_protocol, streetstall_buyitem_syn);
}

TEST(AgentStreetStallClassify, EchoesObjectIdOnForward) {
    StreetStallUserRequest r;
    r.protocol = streetstall_close_syn;
    r.dw_object_id = kObjectId;
    r.dw_user_character_id = kObjectId;
    r.user_found = true;
    auto a = classify_streetstall_user(r);
    EXPECT_EQ(a.dw_object_id, kObjectId);
}

TEST(AgentStreetStallClassify, RequestAndActionDefaultsAreStable) {
    StreetStallUserRequest r;
    EXPECT_EQ(r.protocol, 0u);
    EXPECT_EQ(r.dw_object_id, 0u);
    EXPECT_TRUE(r.user_found);
    StreetStallUserAction a;
    EXPECT_EQ(a.kind, StreetStallUserActionKind::drop_no_user);
    EXPECT_EQ(a.reply_protocol, 0u);
    EXPECT_FALSE(a.forward_payload);
}
