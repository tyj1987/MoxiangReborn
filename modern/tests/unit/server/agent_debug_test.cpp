// D4.159 AgentDebug data plane tests.
//
// 1:1 port of MP_DebugMsgParser from legacy
// [Server]Agent/AgentNetworkMsgParser.cpp lines 2837-2853.

#include <mxh/server/agent_debug.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AgentDebugClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(debug_category, 40u);
}

TEST(AgentDebugClassify, ClientAssertProtocolIsZero) {
    EXPECT_EQ(debug_clientassert, 0u);
}

TEST(AgentDebugClassify, LegacyAssertLogIsCoffeeToolsWatermark) {
    EXPECT_FALSE(legacy_debug_assert_log.empty());
    EXPECT_NE(legacy_debug_assert_log.find("coffee tools"), std::string_view::npos);
}

TEST(AgentDebugClassify, ClientAssertWithPayloadLogs) {
    AgentDebugRequest r;
    r.protocol = debug_clientassert;
    r.payload_present = true;
    EXPECT_EQ(classify_agent_debug(r), AgentDebugOutcome::Logged);
}

TEST(AgentDebugClassify, ClientAssertWithoutPayloadDrops) {
    AgentDebugRequest r;
    r.protocol = debug_clientassert;
    r.payload_present = false;
    EXPECT_EQ(classify_agent_debug(r), AgentDebugOutcome::Dropped);
}

TEST(AgentDebugClassify, UnknownProtocolAlwaysDrops) {
    for (int p : {1, 2, 17, 200, 255}) {
        AgentDebugRequest r;
        r.protocol = static_cast<std::uint8_t>(p);
        r.payload_present = true;
        EXPECT_EQ(classify_agent_debug(r), AgentDebugOutcome::Dropped) << "p=" << +p;
    }
}

TEST(AgentDebugClassify, ObjectIdIsIgnoredInClassification) {
    AgentDebugRequest a;
    a.protocol = debug_clientassert;
    a.payload_present = true;
    a.object_id = 0xAABBCCDDu;

    AgentDebugRequest b = a;
    b.object_id = 0u;

    EXPECT_EQ(classify_agent_debug(a), classify_agent_debug(b));
}

TEST(AgentDebugClassify, DroppedTakesPrecedenceOverPayload) {
    AgentDebugRequest r;
    r.protocol = 99u;
    r.payload_present = false;
    EXPECT_EQ(classify_agent_debug(r), AgentDebugOutcome::Dropped);
}

TEST(AgentDebugClassify, LogOutcomeIsDeterministic) {
    AgentDebugRequest r;
    r.protocol = debug_clientassert;
    r.payload_present = true;
    EXPECT_EQ(classify_agent_debug(r), AgentDebugOutcome::Logged);
    EXPECT_EQ(classify_agent_debug(r), AgentDebugOutcome::Logged);
    EXPECT_EQ(classify_agent_debug(r), AgentDebugOutcome::Logged);
}

TEST(AgentDebugClassify, ProtocolBoundaryValuesHandled) {
    AgentDebugRequest r;
    r.protocol = 255u;
    EXPECT_EQ(classify_agent_debug(r), AgentDebugOutcome::Dropped);

    AgentDebugRequest r2;
    r2.protocol = 0u;
    EXPECT_EQ(classify_agent_debug(r2), AgentDebugOutcome::Logged);
}
