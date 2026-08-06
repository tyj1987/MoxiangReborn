
// distribute_server_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::distribute_server (D4.130).
// Augments the legacy 3-test distribute_server_test.cpp with deeper coverage of:
//   - DistributeState enum values (Stopped=0, Booting=1, Listening=2, Running=3, Stopping=4).
//   - Default port constant DEFAULT_DISTRIBUTE_PORT = 6001.
//   - DistributeServer state machine (bind / stop / run sequence).
//   - agent_endpoints() empty-default behavior.
//   - LoginUserRecord wire struct layout + default values.
//   - Idempotent stop + bind cycles.
//
// 1:1 invariants (locked):
//   - DEFAULT_DISTRIBUTE_PORT = 6001.
//   - DistributeState::Stopped = 0, Booting = 1, Listening = 2, Running = 3, Stopping = 4.
//   - Default-constructed DistributeServer starts in Stopped state.
//   - bind() advances: Stopped -> Booting -> Listening.
//   - stop() transitions to Stopping.
//   - state() is observable atomically.
//   - session_count() returns 0 (legacy stub).
//   - agent_endpoints() returns empty vector.

#pragma once

#include "mxh/server/distribute_server.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::DEFAULT_DISTRIBUTE_PORT;
using mxh::server::DistributeServer;
using mxh::server::DistributeState;
using mxh::server::LoginUserRecord;

static constexpr std::uint16_t kDefaultDistributePort = 6001;

}  // namespace


// ===========================================================================
// DistributeState enum (1:1 with legacy)
// ===========================================================================

TEST(DistributeServerDataPlane, StateStoppedIsZero) {
    EXPECT_EQ(static_cast<std::uint8_t>(DistributeState::Stopped), 0u);
}

TEST(DistributeServerDataPlane, StateBootingIsOne) {
    EXPECT_EQ(static_cast<std::uint8_t>(DistributeState::Booting), 1u);
}

TEST(DistributeServerDataPlane, StateListeningIsTwo) {
    EXPECT_EQ(static_cast<std::uint8_t>(DistributeState::Listening), 2u);
}

TEST(DistributeServerDataPlane, StateRunningIsThree) {
    EXPECT_EQ(static_cast<std::uint8_t>(DistributeState::Running), 3u);
}

TEST(DistributeServerDataPlane, StateStoppingIsFour) {
    EXPECT_EQ(static_cast<std::uint8_t>(DistributeState::Stopping), 4u);
}


// ===========================================================================
// Constants
// ===========================================================================

TEST(DistributeServerDataPlane, DefaultDistributePortIsSixThousandOne) {
    EXPECT_EQ(kDefaultDistributePort, 6001u);
    EXPECT_EQ(DEFAULT_DISTRIBUTE_PORT, 6001u);
}


// ===========================================================================
// Default state
// ===========================================================================

TEST(DistributeServerDataPlane, DefaultServerIsStopped) {
    DistributeServer s;
    EXPECT_EQ(s.state(), DistributeState::Stopped);
    EXPECT_EQ(s.session_count(), 0u);
}

TEST(DistributeServerDataPlane, DefaultAgentEndpointsAreEmpty) {
    DistributeServer s;
    auto endpoints = s.agent_endpoints();
    EXPECT_TRUE(endpoints.empty());
}


// ===========================================================================
// bind() state transitions
// ===========================================================================

TEST(DistributeServerDataPlane, BindAdvancesToListening) {
    DistributeServer s;
    EXPECT_EQ(s.state(), DistributeState::Stopped);
    EXPECT_TRUE(s.bind(7000));
    EXPECT_EQ(s.state(), DistributeState::Listening);
    EXPECT_EQ(s.session_count(), 0u);
}

TEST(DistributeServerDataPlane, BindDefaultPortReturnsTrue) {
    DistributeServer s;
    EXPECT_TRUE(s.bind());
    EXPECT_EQ(s.state(), DistributeState::Listening);
}

TEST(DistributeServerDataPlane, BindReturnsTrue) {
    DistributeServer s;
    EXPECT_TRUE(s.bind(7777));
    EXPECT_EQ(s.state(), DistributeState::Listening);
}

TEST(DistributeServerDataPlane, BindTwiceAdvancesAgain) {
    DistributeServer s;
    EXPECT_TRUE(s.bind(7001));
    EXPECT_EQ(s.state(), DistributeState::Listening);
    EXPECT_TRUE(s.bind(7002));
    EXPECT_EQ(s.state(), DistributeState::Listening);
}



// ===========================================================================
// stop() state transitions
// ===========================================================================

TEST(DistributeServerDataPlane, StopFromListeningTransitionsToStopping) {
    DistributeServer s;
    s.bind(7001);
    EXPECT_EQ(s.state(), DistributeState::Listening);
    s.stop();
    EXPECT_EQ(s.state(), DistributeState::Stopping);
}

TEST(DistributeServerDataPlane, StopFromStoppedTransitionsToStopping) {
    DistributeServer s;
    EXPECT_EQ(s.state(), DistributeState::Stopped);
    s.stop();
    EXPECT_EQ(s.state(), DistributeState::Stopping);
}

TEST(DistributeServerDataPlane, StopIsIdempotent) {
    DistributeServer s;
    s.bind(7001);
    s.stop();
    s.stop();
    EXPECT_EQ(s.state(), DistributeState::Stopping);
}


// ===========================================================================
// Bind/Stop cycles
// ===========================================================================

TEST(DistributeServerDataPlane, BindStopCycleIsRepeatable) {
    DistributeServer s;
    for (std::uint16_t i = 0; i < 3; ++i) {
        EXPECT_TRUE(s.bind(static_cast<std::uint16_t>(7100 + i)));
        EXPECT_EQ(s.state(), DistributeState::Listening);
        s.stop();
        EXPECT_EQ(s.state(), DistributeState::Stopping);
    }
}


// ===========================================================================
// agent_endpoints() invariants
// ===========================================================================

TEST(DistributeServerDataPlane, AgentEndpointsIsEmptyBeforeBind) {
    DistributeServer s;
    EXPECT_TRUE(s.agent_endpoints().empty());
}

TEST(DistributeServerDataPlane, AgentEndpointsIsEmptyAfterBind) {
    DistributeServer s;
    s.bind();
    EXPECT_TRUE(s.agent_endpoints().empty());
}

TEST(DistributeServerDataPlane, AgentEndpointsIsEmptyAfterStop) {
    DistributeServer s;
    s.bind();
    s.stop();
    EXPECT_TRUE(s.agent_endpoints().empty());
}

TEST(DistributeServerDataPlane, AgentEndpointsReturnsVector) {
    DistributeServer s;
    auto ep = s.agent_endpoints();
    EXPECT_EQ(ep.size(), 0u);
}


// ===========================================================================
// session_count() invariants
// ===========================================================================

TEST(DistributeServerDataPlane, SessionCountIsZeroBeforeBind) {
    DistributeServer s;
    EXPECT_EQ(s.session_count(), 0u);
}

TEST(DistributeServerDataPlane, SessionCountIsZeroAfterBind) {
    DistributeServer s;
    s.bind();
    EXPECT_EQ(s.session_count(), 0u);
}

TEST(DistributeServerDataPlane, SessionCountIsZeroAfterStop) {
    DistributeServer s;
    s.bind();
    s.stop();
    EXPECT_EQ(s.session_count(), 0u);
}


// ===========================================================================
// state() observability
// ===========================================================================

TEST(DistributeServerDataPlane, StateObservableWithoutMutating) {
    DistributeServer s;
    auto a = s.state();
    auto b = s.state();
    EXPECT_EQ(a, b);
    EXPECT_EQ(a, DistributeState::Stopped);
}

TEST(DistributeServerDataPlane, StateAdvancesToListeningAtomically) {
    DistributeServer s;
    s.bind(8001);
    EXPECT_EQ(s.state(), DistributeState::Listening);
}


// ===========================================================================
// LoginUserRecord wire struct defaults
// ===========================================================================

TEST(DistributeServerDataPlane, LoginUserRecordDefaultFields) {
    LoginUserRecord r{};
    EXPECT_EQ(r.session_id, 0u);
    EXPECT_EQ(r.user_id, 0u);
    EXPECT_EQ(r.auth_step, 0u);
    EXPECT_EQ(r.reserved0, 0u);
    EXPECT_EQ(r.reserved1, 0u);
    EXPECT_EQ(r.account_name, "");
    EXPECT_EQ(r.last_active_ms, 0u);
    EXPECT_TRUE(r.character_ids.empty());
}

TEST(DistributeServerDataPlane, LoginUserRecordAssignsFields) {
    LoginUserRecord r{};
    r.session_id = 12345;
    r.user_id = 67890;
    r.auth_step = 2;
    r.account_name = "alice";
    r.last_active_ms = 1000;
    r.character_ids.push_back(100);
    r.character_ids.push_back(101);
    EXPECT_EQ(r.session_id, 12345u);
    EXPECT_EQ(r.user_id, 67890u);
    EXPECT_EQ(r.auth_step, 2u);
    EXPECT_EQ(r.account_name, "alice");
    EXPECT_EQ(r.last_active_ms, 1000u);
    ASSERT_EQ(r.character_ids.size(), 2u);
    EXPECT_EQ(r.character_ids[0], 100u);
    EXPECT_EQ(r.character_ids[1], 101u);
}

TEST(DistributeServerDataPlane, LoginUserRecordAuthStepRange) {
    LoginUserRecord r{};
    r.auth_step = 1;
    EXPECT_EQ(r.auth_step, 1u);
    r.auth_step = 2;
    EXPECT_EQ(r.auth_step, 2u);
    r.auth_step = 3;
    EXPECT_EQ(r.auth_step, 3u);
    r.auth_step = 255;
    EXPECT_EQ(r.auth_step, 255u);
}



TEST(DistributeServerDataPlane, LoginUserRecordReservedFieldsUntouched) {
    LoginUserRecord r{};
    EXPECT_EQ(r.reserved0, 0u);
    EXPECT_EQ(r.reserved1, 0u);
    r.reserved0 = 0xAB;
    r.reserved1 = 0xCDEF;
    EXPECT_EQ(r.reserved0, 0xABu);
    EXPECT_EQ(r.reserved1, 0xCDEFu);
}

TEST(DistributeServerDataPlane, LoginUserRecordAccountNameStoresLong) {
    LoginUserRecord r{};
    r.account_name = "verylongaccountname1234567890";
    EXPECT_EQ(r.account_name, "verylongaccountname1234567890");
}

TEST(DistributeServerDataPlane, LoginUserRecordCharacterIdsCanGrow) {
    LoginUserRecord r{};
    for (std::uint32_t i = 0; i < 8; ++i) {
        r.character_ids.push_back(i * 100);
    }
    EXPECT_EQ(r.character_ids.size(), 8u);
    EXPECT_EQ(r.character_ids[7], 700u);
}


// ===========================================================================
// State enum distinctness (compile-time guarantee)
// ===========================================================================

TEST(DistributeServerDataPlane, AllStatesAreDistinct) {
    EXPECT_NE(DistributeState::Stopped, DistributeState::Booting);
    EXPECT_NE(DistributeState::Stopped, DistributeState::Listening);
    EXPECT_NE(DistributeState::Stopped, DistributeState::Running);
    EXPECT_NE(DistributeState::Stopped, DistributeState::Stopping);
    EXPECT_NE(DistributeState::Booting, DistributeState::Listening);
    EXPECT_NE(DistributeState::Booting, DistributeState::Running);
    EXPECT_NE(DistributeState::Booting, DistributeState::Stopping);
    EXPECT_NE(DistributeState::Listening, DistributeState::Running);
    EXPECT_NE(DistributeState::Listening, DistributeState::Stopping);
    EXPECT_NE(DistributeState::Running, DistributeState::Stopping);
}

TEST(DistributeServerDataPlane, AllStatesHaveExpectedOrdinals) {
    EXPECT_LT(static_cast<int>(DistributeState::Stopped),
              static_cast<int>(DistributeState::Booting));
    EXPECT_LT(static_cast<int>(DistributeState::Booting),
              static_cast<int>(DistributeState::Listening));
    EXPECT_LT(static_cast<int>(DistributeState::Listening),
              static_cast<int>(DistributeState::Running));
    EXPECT_LT(static_cast<int>(DistributeState::Running),
              static_cast<int>(DistributeState::Stopping));
}


// ===========================================================================
// Port parameter passing
// ===========================================================================

TEST(DistributeServerDataPlane, BindAcceptsMinimumPort) {
    DistributeServer s;
    EXPECT_TRUE(s.bind(1));
    EXPECT_EQ(s.state(), DistributeState::Listening);
}

TEST(DistributeServerDataPlane, BindAcceptsMaximumPort) {
    DistributeServer s;
    EXPECT_TRUE(s.bind(65535));
    EXPECT_EQ(s.state(), DistributeState::Listening);
}

TEST(DistributeServerDataPlane, BindAcceptsDefaultPortWhenZeroSpecified) {
    DistributeServer s;
    EXPECT_TRUE(s.bind(0));
    EXPECT_EQ(s.state(), DistributeState::Listening);
}

TEST(DistributeServerDataPlane, BindDefaultPortEquals6001) {
    EXPECT_EQ(DEFAULT_DISTRIBUTE_PORT, 6001u);
}

TEST(DistributeServerDataPlane, MultipleBindSessions) {
    DistributeServer s;
    EXPECT_TRUE(s.bind(8001));
    EXPECT_TRUE(s.bind(8002));
    EXPECT_TRUE(s.bind(8003));
    EXPECT_EQ(s.state(), DistributeState::Listening);
}

TEST(DistributeServerDataPlane, BindStopBindStopSequence) {
    DistributeServer s;
    s.bind(9001);
    s.stop();
    s.bind(9002);
    s.stop();
    EXPECT_EQ(s.state(), DistributeState::Stopping);
}

TEST(DistributeServerDataPlane, StateFromStoppedThenBindThenStop) {
    DistributeServer s;
    EXPECT_EQ(s.state(), DistributeState::Stopped);
    s.bind(10000);
    EXPECT_EQ(s.state(), DistributeState::Listening);
    s.stop();
    EXPECT_EQ(s.state(), DistributeState::Stopping);
}

TEST(DistributeServerDataPlane, SessionCountZeroThroughLifecycle) {
    DistributeServer s;
    EXPECT_EQ(s.session_count(), 0u);
    s.bind();
    EXPECT_EQ(s.session_count(), 0u);
    s.stop();
    EXPECT_EQ(s.session_count(), 0u);
}

TEST(DistributeServerDataPlane, AgentEndpointsEmptyThroughLifecycle) {
    DistributeServer s;
    EXPECT_TRUE(s.agent_endpoints().empty());
    s.bind();
    EXPECT_TRUE(s.agent_endpoints().empty());
    s.stop();
    EXPECT_TRUE(s.agent_endpoints().empty());
}

TEST(DistributeServerDataPlane, DistinctInstancesAreIndependent) {
    DistributeServer a;
    DistributeServer b;
    a.bind(8001);
    EXPECT_EQ(a.state(), DistributeState::Listening);
    EXPECT_EQ(b.state(), DistributeState::Stopped);
}

TEST(DistributeServerDataPlane, MultipleStopCallsFromStopped) {
    DistributeServer s;
    s.stop();
    s.stop();
    s.stop();
    EXPECT_EQ(s.state(), DistributeState::Stopping);
}

TEST(DistributeServerDataPlane, BindThenMultipleStops) {
    DistributeServer s;
    s.bind(7777);
    s.stop();
    s.stop();
    s.stop();
    EXPECT_EQ(s.state(), DistributeState::Stopping);
}
