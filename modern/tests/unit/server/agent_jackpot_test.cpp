//
// 1:1 port of MP_JACKPOTUserMsgParser / MP_JACKPOTServerMsgParser from
// legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 4614-4673.

#include <mxh/server/agent_jackpot.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

namespace {
constexpr std::uint32_t kObjectId = 0xABCDEF01u;
constexpr std::uint32_t kConnId = 0x100u;
constexpr std::uint16_t kMapNum = 17u;
}

TEST(AgentJackpotClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(jackpot_category, 61u);
}

TEST(AgentJackpotClassify, ProtocolConstantsAreContiguousFromZero) {
    EXPECT_EQ(jackpot_prize_notify, 0u);
    EXPECT_EQ(jackpot_totalmoney_notify, 1u);
    EXPECT_EQ(jackpot_totalmoney_notify_to_agent, 2u);
    EXPECT_EQ(jackpot_prize_effect, 3u);
    EXPECT_EQ(jackpot_cheat_maptotalmoney, 4u);
}

TEST(AgentJackpotClassify, UserSideWithoutUserDrops) {
    JackpotUserRequest r;
    r.protocol = jackpot_prize_notify;
    r.dw_object_id = kObjectId;
    r.user_found = false;
    auto a = classify_jackpot_user(r);
    EXPECT_EQ(a.kind, JackpotUserActionKind::drop_no_user);
    EXPECT_EQ(a.reply_protocol, jackpot_prize_notify);
    EXPECT_EQ(a.dw_object_id, kObjectId);
}

TEST(AgentJackpotClassify, UserSideAlwaysDropsRegardlessOfProtocol) {
    for (std::uint8_t p = 0; p < 5; ++p) {
        JackpotUserRequest r;
        r.protocol = p;
        r.user_found = true;
        auto a = classify_jackpot_user(r);
        EXPECT_EQ(a.kind, JackpotUserActionKind::drop_no_handler);
        EXPECT_EQ(a.reply_protocol, p);
    }
}

TEST(AgentJackpotClassify, UserSideEchoesProtocolOnDrop) {
    JackpotUserRequest r;
    r.protocol = 200u;
    auto a = classify_jackpot_user(r);
    EXPECT_EQ(a.kind, JackpotUserActionKind::drop_no_handler);
    EXPECT_EQ(a.reply_protocol, 200u);
}

TEST(AgentJackpotClassify, UserSidePreservesObjectIdEvenOnDrop) {
    JackpotUserRequest r;
    r.protocol = jackpot_prize_effect;
    r.dw_object_id = kObjectId;
    auto a = classify_jackpot_user(r);
    EXPECT_EQ(a.dw_object_id, kObjectId);
}

TEST(AgentJackpotClassify, ServerPrizeNotifyBroadcastsAllUsers) {
    JackpotUserSlot slots[3] = {
        {kConnId + 1, 0, true},   // lobby user
        {kConnId + 2, kMapNum, true},  // in-map user
        {kConnId + 3, kMapNum + 5, true},  // in-map user
    };
    JackpotServerRequest r;
    r.protocol = jackpot_prize_notify;
    r.prize.dw_object_id = kObjectId;
    r.prize.dw_rest_total_money = 0xDEADBEEFu;
    r.user_count = 3;
    r.users = slots;
    auto a = classify_jackpot_server(r);
    EXPECT_EQ(a.kind, JackpotServerActionKind::broadcast_all_users);
    EXPECT_EQ(a.reply_protocol, jackpot_prize_notify);
    EXPECT_EQ(a.dw_object_id, kObjectId);
    EXPECT_EQ(a.jackpot_total_money, 0xDEADBEEFu);
    EXPECT_FALSE(a.rewrite_protocol);
    EXPECT_EQ(a.broadcast_count, 3u);
}

TEST(AgentJackpotClassify, ServerPrizeNotifySkipsUsersNotInTable) {
    JackpotUserSlot slots[3] = {
        {kConnId + 1, 0, true},
        {kConnId + 2, kMapNum, false},  // not in table -> skipped
        {kConnId + 3, kMapNum, true},
    };
    JackpotServerRequest r;
    r.protocol = jackpot_prize_notify;
    r.user_count = 3;
    r.users = slots;
    auto a = classify_jackpot_server(r);
    EXPECT_EQ(a.kind, JackpotServerActionKind::broadcast_all_users);
    EXPECT_EQ(a.broadcast_count, 2u);
}

TEST(AgentJackpotClassify, ServerPrizeNotifyWithEmptyUsersReturnsZero) {
    JackpotServerRequest r;
    r.protocol = jackpot_prize_notify;
    r.user_count = 0;
    r.users = nullptr;
    auto a = classify_jackpot_server(r);
    EXPECT_EQ(a.kind, JackpotServerActionKind::broadcast_all_users);
    EXPECT_EQ(a.broadcast_count, 0u);
}

TEST(AgentJackpotClassify, ServerPrizeNotifyDoesNotRewriteProtocol) {
    JackpotUserSlot slots[1] = {{kConnId, 0, true}};
    JackpotServerRequest r;
    r.protocol = jackpot_prize_notify;
    r.user_count = 1;
    r.users = slots;
    auto a = classify_jackpot_server(r);
    EXPECT_FALSE(a.rewrite_protocol);
    EXPECT_EQ(a.rewritten_protocol, 0u);
}

TEST(AgentJackpotClassify, ServerPrizeNotifyWithZeroTotalMoneyStillBroadcasts) {
    JackpotUserSlot slots[1] = {{kConnId, 0, true}};
    JackpotServerRequest r;
    r.protocol = jackpot_prize_notify;
    r.prize.dw_rest_total_money = 0u;
    r.user_count = 1;
    r.users = slots;
    auto a = classify_jackpot_server(r);
    EXPECT_EQ(a.jackpot_total_money, 0u);
    EXPECT_EQ(a.broadcast_count, 1u);
}

TEST(AgentJackpotClassify, ServerTotalMoneyToAgentBroadcastsInMapOnly) {
    JackpotUserSlot slots[3] = {
        {kConnId + 1, 0, true},   // lobby -> skipped
        {kConnId + 2, kMapNum, true},
        {kConnId + 3, kMapNum + 5, true},
    };
    JackpotServerRequest r;
    r.protocol = jackpot_totalmoney_notify_to_agent;
    r.total.dw_object_id = kObjectId;
    r.total.dw_data = 0x12345678u;
    r.user_count = 3;
    r.users = slots;
    auto a = classify_jackpot_server(r);
    EXPECT_EQ(a.kind, JackpotServerActionKind::broadcast_in_map_users);
    EXPECT_EQ(a.reply_protocol, jackpot_totalmoney_notify);  // rewritten
    EXPECT_EQ(a.dw_object_id, kObjectId);
    EXPECT_EQ(a.jackpot_total_money, 0x12345678u);
    EXPECT_TRUE(a.rewrite_protocol);
    EXPECT_EQ(a.rewritten_protocol, jackpot_totalmoney_notify);
    EXPECT_EQ(a.broadcast_count, 2u);  // lobby user excluded
}

TEST(AgentJackpotClassify, ServerTotalMoneyToAgentSkipsUsersNotInTable) {
    JackpotUserSlot slots[3] = {
        {kConnId + 1, 0, true},
        {kConnId + 2, kMapNum, false},  // not in table -> skipped
        {kConnId + 3, kMapNum, true},
    };
    JackpotServerRequest r;
    r.protocol = jackpot_totalmoney_notify_to_agent;
    r.user_count = 3;
    r.users = slots;
    auto a = classify_jackpot_server(r);
    EXPECT_EQ(a.broadcast_count, 1u);
}

TEST(AgentJackpotClassify, ServerTotalMoneyToAgentWithEmptyUsersReturnsZero) {
    JackpotServerRequest r;
    r.protocol = jackpot_totalmoney_notify_to_agent;
    r.user_count = 0;
    r.users = nullptr;
    auto a = classify_jackpot_server(r);
    EXPECT_EQ(a.kind, JackpotServerActionKind::broadcast_in_map_users);
    EXPECT_EQ(a.broadcast_count, 0u);
}

TEST(AgentJackpotClassify, ServerPrizeEffectForwardsDefault) {
    JackpotServerRequest r;
    r.protocol = jackpot_prize_effect;
    auto a = classify_jackpot_server(r);
    EXPECT_EQ(a.kind, JackpotServerActionKind::forward_to_originating_client);
    EXPECT_EQ(a.reply_protocol, jackpot_prize_effect);
    EXPECT_EQ(a.jackpot_total_money, 0u);
    EXPECT_FALSE(a.rewrite_protocol);
    EXPECT_EQ(a.broadcast_count, 0u);
}

TEST(AgentJackpotClassify, ServerCheatMapTotalMoneyForwardsDefault) {
    JackpotServerRequest r;
    r.protocol = jackpot_cheat_maptotalmoney;
    auto a = classify_jackpot_server(r);
    EXPECT_EQ(a.kind, JackpotServerActionKind::forward_to_originating_client);
    EXPECT_EQ(a.reply_protocol, jackpot_cheat_maptotalmoney);
    EXPECT_EQ(a.jackpot_total_money, 0u);
    EXPECT_FALSE(a.rewrite_protocol);
}

TEST(AgentJackpotClassify, ServerTotalMoneyNotifyForwardsDefault) {
    // Legacy commented-out block falls through to default forward.
    JackpotServerRequest r;
    r.protocol = jackpot_totalmoney_notify;
    auto a = classify_jackpot_server(r);
    EXPECT_EQ(a.kind, JackpotServerActionKind::forward_to_originating_client);
    EXPECT_EQ(a.reply_protocol, jackpot_totalmoney_notify);
    EXPECT_EQ(a.jackpot_total_money, 0u);
    EXPECT_FALSE(a.rewrite_protocol);
    EXPECT_EQ(a.broadcast_count, 0u);
}

TEST(AgentJackpotClassify, ServerUnknownProtocolDrops) {
    JackpotServerRequest r;
    r.protocol = 250u;
    auto a = classify_jackpot_server(r);
    EXPECT_EQ(a.kind, JackpotServerActionKind::drop_unknown_protocol);
    EXPECT_EQ(a.reply_protocol, 250u);
    EXPECT_EQ(a.jackpot_total_money, 0u);
    EXPECT_FALSE(a.rewrite_protocol);
    EXPECT_EQ(a.broadcast_count, 0u);
}

TEST(AgentJackpotClassify, ServerUnknownProtocolWithUsersStillDrops) {
    JackpotUserSlot slots[2] = {
        {kConnId + 1, 0, true},
        {kConnId + 2, kMapNum, true},
    };
    JackpotServerRequest r;
    r.protocol = 250u;
    r.user_count = 2;
    r.users = slots;
    auto a = classify_jackpot_server(r);
    EXPECT_EQ(a.kind, JackpotServerActionKind::drop_unknown_protocol);
    EXPECT_EQ(a.broadcast_count, 0u);
}

TEST(AgentJackpotClassify, ProtocolRewriteTargetIsTotalMoneyNotify) {
    // Legacy: pmsg->Protocol is mutated to MP_JACKPOT_TOTALMONEY_NOTIFY
    // before the broadcast loop. Verify the rewrite target is exactly 1.
    JackpotUserSlot slots[1] = {{kConnId, kMapNum, true}};
    JackpotServerRequest r;
    r.protocol = jackpot_totalmoney_notify_to_agent;
    r.user_count = 1;
    r.users = slots;
    auto a = classify_jackpot_server(r);
    EXPECT_EQ(a.reply_protocol, 1u);  // == jackpot_totalmoney_notify
    EXPECT_EQ(a.rewritten_protocol, 1u);
}

TEST(AgentJackpotClassify, ServerPrizeNotifyEchoesObjectIdOnAction) {
    JackpotUserSlot slots[1] = {{kConnId, 0, true}};
    JackpotServerRequest r;
    r.protocol = jackpot_prize_notify;
    r.prize.dw_object_id = kObjectId;
    r.user_count = 1;
    r.users = slots;
    auto a = classify_jackpot_server(r);
    EXPECT_EQ(a.dw_object_id, kObjectId);
}

TEST(AgentJackpotClassify, ServerTotalMoneyToAgentEchoesObjectIdOnAction) {
    JackpotUserSlot slots[1] = {{kConnId, kMapNum, true}};
    JackpotServerRequest r;
    r.protocol = jackpot_totalmoney_notify_to_agent;
    r.total.dw_object_id = kObjectId;
    r.user_count = 1;
    r.users = slots;
    auto a = classify_jackpot_server(r);
    EXPECT_EQ(a.dw_object_id, kObjectId);
}

TEST(AgentJackpotClassify, ServerPrizeNotifyInMapAndLobbyBothBroadcast) {
    JackpotUserSlot slots[4] = {
        {kConnId + 1, 0, true},    // lobby
        {kConnId + 2, 1, true},    // in map 1
        {kConnId + 3, 100, true},  // in map 100
        {kConnId + 4, 0xFFFFu, true},  // in max map
    };
    JackpotServerRequest r;
    r.protocol = jackpot_prize_notify;
    r.user_count = 4;
    r.users = slots;
    auto a = classify_jackpot_server(r);
    EXPECT_EQ(a.broadcast_count, 4u);
}

TEST(AgentJackpotClassify, ServerTotalMoneyToAgentInMapZeroBroadcastsZero) {
    // All users are in lobby -> in_map broadcast excludes everyone.
    JackpotUserSlot slots[3] = {
        {kConnId + 1, 0, true},
        {kConnId + 2, 0, true},
        {kConnId + 3, 0, true},
    };
    JackpotServerRequest r;
    r.protocol = jackpot_totalmoney_notify_to_agent;
    r.user_count = 3;
    r.users = slots;
    auto a = classify_jackpot_server(r);
    EXPECT_EQ(a.broadcast_count, 0u);
}

TEST(AgentJackpotClassify, ServerPrizeNotifySingleUserMatch) {
    JackpotUserSlot slots[1] = {{kConnId, kMapNum, true}};
    JackpotServerRequest r;
    r.protocol = jackpot_prize_notify;
    r.prize.dw_rest_total_money = 1u;
    r.user_count = 1;
    r.users = slots;
    auto a = classify_jackpot_server(r);
    EXPECT_EQ(a.broadcast_count, 1u);
    EXPECT_EQ(a.jackpot_total_money, 1u);
}
