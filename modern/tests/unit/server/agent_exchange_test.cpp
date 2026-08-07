//
// 1:1 port of MP_EXCHANGEUserMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp lines 5095-5104.

#include <mxh/server/agent_exchange.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

namespace {
constexpr std::uint32_t kObjectId = 0xDEADBEEFu;
constexpr std::uint32_t kOtherId = 0xCAFEBABEu;
}

TEST(AgentExchangeClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(exchange_category, 28u);
}

TEST(AgentExchangeClassify, UserNotFoundDrops) {
    ExchangeUserRequest r;
    r.protocol = exchange_apply_syn;
    r.dw_object_id = kObjectId;
    r.dw_user_character_id = kObjectId;
    r.user_found = false;
    auto a = classify_exchange_user(r);
    EXPECT_EQ(a.kind, ExchangeUserActionKind::drop_no_user);
    EXPECT_EQ(a.reply_protocol, exchange_apply_syn);
    EXPECT_FALSE(a.forward_payload);
}

TEST(AgentExchangeClassify, ObjectIdMismatchDrops) {
    ExchangeUserRequest r;
    r.protocol = exchange_accept_syn;
    r.dw_object_id = kObjectId;
    r.dw_user_character_id = kOtherId;
    r.user_found = true;
    auto a = classify_exchange_user(r);
    EXPECT_EQ(a.kind, ExchangeUserActionKind::drop_object_id_mismatch);
    EXPECT_FALSE(a.forward_payload);
}

TEST(AgentExchangeClassify, MatchedObjectIdForwardsToMapServer) {
    ExchangeUserRequest r;
    r.protocol = exchange_apply_syn;
    r.dw_object_id = kObjectId;
    r.dw_user_character_id = kObjectId;
    r.user_found = true;
    auto a = classify_exchange_user(r);
    EXPECT_EQ(a.kind, ExchangeUserActionKind::forward_to_map_server);
    EXPECT_EQ(a.reply_protocol, exchange_apply_syn);
    EXPECT_EQ(a.dw_object_id, kObjectId);
    EXPECT_TRUE(a.forward_payload);
}

TEST(AgentExchangeClassify, MultipleProtocolsForwardCorrectly) {
    for (std::uint8_t p : {exchange_apply_syn, exchange_accept_syn, exchange_reject_syn,
                         exchange_waiting_cancel_syn, exchange_additem_syn, exchange_delitem_syn,
                         exchange_inputmoney_syn, exchange_lock_syn, exchange_unlock_syn,
                         exchange_exchange_syn, exchange_cancel_syn}) {
        ExchangeUserRequest r;
        r.protocol = p;
        r.dw_object_id = kObjectId;
        r.dw_user_character_id = kObjectId;
        r.user_found = true;
        auto a = classify_exchange_user(r);
        EXPECT_EQ(a.kind, ExchangeUserActionKind::forward_to_map_server);
        EXPECT_EQ(a.reply_protocol, p);
    }
}

TEST(AgentExchangeClassify, UnknownProtocolStillForwardsWhenValid) {
    // Legacy: no switch on protocol in the handler. Any sub-protocol byte
    // with matching dwObjectID + found user gets forwarded.
    ExchangeUserRequest r;
    r.protocol = 200u;
    r.dw_object_id = kObjectId;
    r.dw_user_character_id = kObjectId;
    r.user_found = true;
    auto a = classify_exchange_user(r);
    EXPECT_EQ(a.kind, ExchangeUserActionKind::forward_to_map_server);
    EXPECT_EQ(a.reply_protocol, 200u);
}

TEST(AgentExchangeClassify, ObjectIdMismatchTakesPrecedenceOverProtocol) {
    ExchangeUserRequest r;
    r.protocol = exchange_apply_syn;
    r.dw_object_id = kObjectId;
    r.dw_user_character_id = kOtherId;
    r.user_found = true;
    auto a = classify_exchange_user(r);
    EXPECT_EQ(a.kind, ExchangeUserActionKind::drop_object_id_mismatch);
    EXPECT_EQ(a.reply_protocol, exchange_apply_syn);
}

TEST(AgentExchangeClassify, EchoesObjectIdOnForward) {
    ExchangeUserRequest r;
    r.protocol = exchange_accept_syn;
    r.dw_object_id = kObjectId;
    r.dw_user_character_id = kObjectId;
    r.user_found = true;
    auto a = classify_exchange_user(r);
    EXPECT_EQ(a.dw_object_id, kObjectId);
}

TEST(AgentExchangeClassify, RequestAndActionDefaultsAreStable) {
    ExchangeUserRequest r;
    EXPECT_EQ(r.protocol, 0u);
    EXPECT_EQ(r.dw_object_id, 0u);
    EXPECT_TRUE(r.user_found);
    ExchangeUserAction a;
    EXPECT_EQ(a.kind, ExchangeUserActionKind::drop_no_user);
    EXPECT_EQ(a.reply_protocol, 0u);
    EXPECT_FALSE(a.forward_payload);
}
