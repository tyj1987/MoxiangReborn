// distribute_server_test.cpp - lifecycle + state tests.

#include "mxh/server/distribute_server.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::DistributeServer;
using mxh::server::DistributeState;
}

TEST(DistributeServerTest, BindAdvancesState) {
    DistributeServer s;
    EXPECT_EQ(s.state(), DistributeState::Stopped);
    EXPECT_TRUE(s.bind(7000));
    EXPECT_EQ(s.state(), DistributeState::Listening);
    EXPECT_EQ(s.session_count(), 0u);
}

TEST(DistributeServerTest, StopWorks) {
    DistributeServer s;
    s.bind(7001);
    s.stop();
    EXPECT_EQ(s.state(), DistributeState::Stopping);
}

TEST(DistributeServerTest, PortDefaultsAt6001) {
    DistributeServer s;
    s.bind();
    EXPECT_EQ(s.state(), DistributeState::Listening);
}
