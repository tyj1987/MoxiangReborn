// mn_server_test.cpp

#include "mxh/server/mn_server.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::MNServer;
using mxh::server::MnServerState;
}

TEST(MNServerTest, BindAdvancesState) {
    MNServer s;
    EXPECT_EQ(s.state(), MnServerState::Stopped);
    EXPECT_TRUE(s.bind(12001));
    EXPECT_EQ(s.state(), MnServerState::Listening);
}

TEST(MNServerTest, DefaultPort) {
    MNServer s;
    s.bind();
    EXPECT_EQ(s.state(), MnServerState::Listening);
}

TEST(MNServerTest, StopFromListening) {
    MNServer s;
    s.bind(13001);
    s.stop();
    EXPECT_EQ(s.state(), MnServerState::Stopped);
}

TEST(MNServerTest, ChannelsAccessible) {
    MNServer s;
    s.bind();
    EXPECT_EQ(s.channel_count(), 0u);
    auto cid = s.channels().create_channel(100);
    EXPECT_GT(cid, 0u);
    EXPECT_EQ(s.channel_count(), 1u);
    EXPECT_EQ(s.session_count(), 0u);
}
