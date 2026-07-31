// murimnet_channelling_manager_test.cpp

#include "mxh/server/murimnet_channelling_manager.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::MurimNetChannellingManager;
using mxh::server::MurimNetChat;
using mxh::server::MnChatMessage;
using mxh::server::MnChannelState;
}

TEST(MurimNetChannelling, CreateJoinLeaveDestroy) {
    MurimNetChannellingManager mn;
    auto id = mn.create_channel(10);
    EXPECT_GT(id, 0u);
    EXPECT_EQ(mn.channel_count(), 1u);
    EXPECT_TRUE(mn.join(id, 20));
    EXPECT_EQ(mn.members(id).size(), 2u);
    EXPECT_FALSE(mn.join(id, 20));   // already in
    EXPECT_FALSE(mn.join(id, 0));    // bad player
    EXPECT_TRUE(mn.leave(id, 20));
    EXPECT_FALSE(mn.leave(id, 999));
    EXPECT_TRUE(mn.destroy_channel(id));
    EXPECT_EQ(mn.channel_count(), 0u);
}

TEST(MurimNetChannelling, ChannelCapIsRespected) {
    MurimNetChannellingManager mn;
    auto id = mn.create_channel(1);
    for (std::uint8_t i = 2; i <= mxh::server::MXH_MN_MAX_PLAYERS; ++i) {
        EXPECT_TRUE(mn.join(id, i));
    }
    EXPECT_FALSE(mn.join(id, 99));  // full
}

TEST(MurimNetChannelling, SendMessageAndHistory) {
    MurimNetChannellingManager mn;
    auto id = mn.create_channel(1);
    ASSERT_TRUE(mn.join(id, 2));
    MnChatMessage m1{}; m1.sender_id=1; m1.channel_id=id; m1.send_ms=100;
    std::strncpy(m1.text, "hello", 5);
    MnChatMessage m2{}; m2.sender_id=2; m2.channel_id=id; m2.send_ms=200;
    std::strncpy(m2.text, "world", 5);
    EXPECT_TRUE(mn.send_message(m1));
    EXPECT_TRUE(mn.send_message(m2));
    auto h = mn.history(id);
    ASSERT_EQ(h.size(), 2u);
    EXPECT_STREQ(h[0].text, "hello");
    EXPECT_STREQ(h[1].text, "world");
    EXPECT_EQ(mn.state(id), MnChannelState::Active);
}

TEST(MurimNetChannelling, RejectMessageForUnknownChannel) {
    MurimNetChannellingManager mn;
    MnChatMessage m{}; m.sender_id=1; m.channel_id=999; m.send_ms=1;
    EXPECT_FALSE(mn.send_message(m));
}

TEST(MurimNetChat, SendAndSnapshot) {
    MurimNetChat chat;
    EXPECT_TRUE(chat.send(1, 7, "hello map7", 1000));
    EXPECT_TRUE(chat.send(2, 7, "another on map7", 1100));
    EXPECT_TRUE(chat.send(3, 8, "different map", 1200));
    EXPECT_EQ(chat.size(), 3u);
    auto map7 = chat.snapshot(7);
    EXPECT_EQ(map7.size(), 2u);
    auto map8 = chat.snapshot(8);
    EXPECT_EQ(map8.size(), 1u);
}

TEST(MurimNetChat, ClearExpiredDropsOld) {
    MurimNetChat chat;
    chat.send(1, 7, "fresh", 1000);
    chat.send(2, 7, "stale",  500);
    chat.clear_expired(1500, /*expire_ms*/600);
    auto out = chat.snapshot(7);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].sender_id, 1u);
}

TEST(MurimNetChat, RejectsBadInputs) {
    MurimNetChat chat;
    EXPECT_FALSE(chat.send(0, 7, "no sender", 1));
    EXPECT_FALSE(chat.send(1, 7, "", 1));
}

TEST(MurimNetChannelling, ClosingChannelRejectsJoinAndMessages) {
    MurimNetChannellingManager mn;
    auto id = mn.create_channel(10);
    EXPECT_TRUE(mn.leave(id, 10));
    EXPECT_EQ(mn.state(id), MnChannelState::Closing);
    EXPECT_FALSE(mn.join(id, 20));
    MnChatMessage message{};
    message.sender_id = 20;
    message.channel_id = id;
    EXPECT_FALSE(mn.send_message(message));
    EXPECT_TRUE(mn.destroy_channel(id));
}

TEST(MurimNetChannelling, RejectsMessagesFromNonMembers) {
    MurimNetChannellingManager mn;
    auto id = mn.create_channel(10);
    MnChatMessage message{};
    message.channel_id = id;
    message.sender_id = 20;
    EXPECT_FALSE(mn.send_message(message));
    message.sender_id = 0;
    EXPECT_FALSE(mn.send_message(message));
    EXPECT_TRUE(mn.join(id, 20));
    message.sender_id = 20;
    EXPECT_TRUE(mn.send_message(message));
}

