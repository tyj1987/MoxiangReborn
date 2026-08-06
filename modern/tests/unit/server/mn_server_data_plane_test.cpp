
// mn_server_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::mn_server (D4.131).
// Augments the legacy 4-test mn_server_test.cpp with deeper coverage of:
//   - MnServerState enum values (Stopped=0, Booting=1, Listening=2,
//     Running=3).
//   - Default port constant MXH_MN_DEFAULT_PORT = 12001.
//   - MNServer state machine (bind / stop sequence).
//   - session_count / channel_count invariants.
//   - MnPlayerInfo wire struct defaults.
//   - Idempotent stop + bind cycles.
//
// 1:1 invariants (locked):
//   - MXH_MN_DEFAULT_PORT = 12001 (legacy MNServer default).
//   - MnServerState::Stopped = 0, Booting = 1, Listening = 2,
//     Running = 3.
//   - Default-constructed MNServer starts in Stopped state.
//   - bind() advances: Stopped -> Booting -> Listening.
//   - stop() transitions to Stopped (different from distribute_server
//     which goes to Stopping - MNServer is a smaller leaf process).
//   - session_count() returns 0 (legacy stub).
//   - channel_count() tracks MurimNetChannellingManager channels.
//   - channels() returns the underlying channelling manager reference.

#pragma once

#include "mxh/server/mn_server.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace {

using mxh::server::MXH_MN_DEFAULT_PORT;
using mxh::server::MNServer;
using mxh::server::MnServerState;
using mxh::server::MnPlayerInfo;

static constexpr std::uint16_t kDefaultMnPort = 12001;

}  // namespace


// ===========================================================================
// MnServerState enum (1:1 with legacy)
// ===========================================================================

TEST(MnServerDataPlane, StateStoppedIsZero) {
    EXPECT_EQ(static_cast<std::uint8_t>(MnServerState::Stopped), 0u);
}

TEST(MnServerDataPlane, StateBootingIsOne) {
    EXPECT_EQ(static_cast<std::uint8_t>(MnServerState::Booting), 1u);
}

TEST(MnServerDataPlane, StateListeningIsTwo) {
    EXPECT_EQ(static_cast<std::uint8_t>(MnServerState::Listening), 2u);
}

TEST(MnServerDataPlane, StateRunningIsThree) {
    EXPECT_EQ(static_cast<std::uint8_t>(MnServerState::Running), 3u);
}


// ===========================================================================
// Constants
// ===========================================================================

TEST(MnServerDataPlane, DefaultMnPortIsTwelveThousandOne) {
    EXPECT_EQ(kDefaultMnPort, 12001u);
    EXPECT_EQ(MXH_MN_DEFAULT_PORT, 12001u);
}


// ===========================================================================
// Default state
// ===========================================================================

TEST(MnServerDataPlane, DefaultServerIsStopped) {
    MNServer s;
    EXPECT_EQ(s.state(), MnServerState::Stopped);
    EXPECT_EQ(s.session_count(), 0u);
    EXPECT_EQ(s.channel_count(), 0u);
}

TEST(MnServerDataPlane, DefaultChannelCountIsZero) {
    MNServer s;
    EXPECT_EQ(s.channel_count(), 0u);
}

TEST(MnServerDataPlane, DefaultSessionCountIsZero) {
    MNServer s;
    EXPECT_EQ(s.session_count(), 0u);
}


// ===========================================================================
// bind() state transitions
// ===========================================================================

TEST(MnServerDataPlane, BindAdvancesToListening) {
    MNServer s;
    EXPECT_EQ(s.state(), MnServerState::Stopped);
    EXPECT_TRUE(s.bind(13001));
    EXPECT_EQ(s.state(), MnServerState::Listening);
}

TEST(MnServerDataPlane, BindDefaultPortReturnsTrue) {
    MNServer s;
    EXPECT_TRUE(s.bind());
    EXPECT_EQ(s.state(), MnServerState::Listening);
}

TEST(MnServerDataPlane, BindReturnsTrue) {
    MNServer s;
    EXPECT_TRUE(s.bind(14000));
    EXPECT_EQ(s.state(), MnServerState::Listening);
}

TEST(MnServerDataPlane, BindTwiceAdvancesAgain) {
    MNServer s;
    EXPECT_TRUE(s.bind(14001));
    EXPECT_EQ(s.state(), MnServerState::Listening);
    EXPECT_TRUE(s.bind(14002));
    EXPECT_EQ(s.state(), MnServerState::Listening);
}


// ===========================================================================
// stop() state transitions
// ===========================================================================

TEST(MnServerDataPlane, StopFromListeningTransitionsToStopped) {
    MNServer s;
    s.bind(14001);
    EXPECT_EQ(s.state(), MnServerState::Listening);
    s.stop();
    EXPECT_EQ(s.state(), MnServerState::Stopped);
}

TEST(MnServerDataPlane, StopFromStoppedTransitionsToStopped) {
    MNServer s;
    EXPECT_EQ(s.state(), MnServerState::Stopped);
    s.stop();
    EXPECT_EQ(s.state(), MnServerState::Stopped);
}

TEST(MnServerDataPlane, StopIsIdempotent) {
    MNServer s;
    s.bind(14001);
    s.stop();
    s.stop();
    s.stop();
    EXPECT_EQ(s.state(), MnServerState::Stopped);
}



// ===========================================================================
// Bind/Stop cycles
// ===========================================================================

TEST(MnServerDataPlane, BindStopCycleIsRepeatable) {
    MNServer s;
    for (std::uint16_t i = 0; i < 3; ++i) {
        EXPECT_TRUE(s.bind(static_cast<std::uint16_t>(15000 + i)));
        EXPECT_EQ(s.state(), MnServerState::Listening);
        s.stop();
        EXPECT_EQ(s.state(), MnServerState::Stopped);
    }
}

TEST(MnServerDataPlane, BindStopBindStopSequence) {
    MNServer s;
    s.bind(16001);
    s.stop();
    s.bind(16002);
    s.stop();
    EXPECT_EQ(s.state(), MnServerState::Stopped);
}


// ===========================================================================
// channels() / channel_count() invariants
// ===========================================================================

TEST(MnServerDataPlane, ChannelsAccessibleBeforeBind) {
    MNServer s;
    EXPECT_EQ(s.channel_count(), 0u);
    auto cid = s.channels().create_channel(100);
    EXPECT_GT(cid, 0u);
    EXPECT_EQ(s.channel_count(), 1u);
}

TEST(MnServerDataPlane, ChannelsAccessibleAfterBind) {
    MNServer s;
    s.bind();
    auto cid = s.channels().create_channel(200);
    EXPECT_GT(cid, 0u);
    EXPECT_EQ(s.channel_count(), 1u);
}

TEST(MnServerDataPlane, ChannelCountTracksMultipleChannels) {
    MNServer s;
    auto c1 = s.channels().create_channel(300);
    auto c2 = s.channels().create_channel(301);
    auto c3 = s.channels().create_channel(302);
    EXPECT_GT(c1, 0u);
    EXPECT_GT(c2, 0u);
    EXPECT_GT(c3, 0u);
    EXPECT_EQ(s.channel_count(), 3u);
}

TEST(MnServerDataPlane, ChannelCountAfterStop) {
    MNServer s;
    s.channels().create_channel(400);
    s.channels().create_channel(401);
    EXPECT_EQ(s.channel_count(), 2u);
    s.bind();
    s.stop();
    EXPECT_EQ(s.channel_count(), 2u);
}

TEST(MnServerDataPlane, SessionCountZeroThroughoutLifecycle) {
    MNServer s;
    EXPECT_EQ(s.session_count(), 0u);
    s.bind();
    EXPECT_EQ(s.session_count(), 0u);
    s.stop();
    EXPECT_EQ(s.session_count(), 0u);
}


// ===========================================================================
// state() observability
// ===========================================================================

TEST(MnServerDataPlane, StateObservableWithoutMutating) {
    MNServer s;
    auto a = s.state();
    auto b = s.state();
    EXPECT_EQ(a, b);
    EXPECT_EQ(a, MnServerState::Stopped);
}

TEST(MnServerDataPlane, StateAdvancesToListeningAtomically) {
    MNServer s;
    s.bind(17001);
    EXPECT_EQ(s.state(), MnServerState::Listening);
}


// ===========================================================================
// MnPlayerInfo wire struct defaults
// ===========================================================================

TEST(MnServerDataPlane, MnPlayerInfoDefaultFields) {
    MnPlayerInfo p{};
    EXPECT_EQ(p.player_id, 0u);
    EXPECT_EQ(p.room_id, 0u);
    EXPECT_EQ(p.last_seen_ms, 0u);
    EXPECT_EQ(p.fighting, 0u);
    EXPECT_EQ(p.reserved0, 0u);
    EXPECT_EQ(p.reserved1, 0u);
}

TEST(MnServerDataPlane, MnPlayerInfoAssignsFields) {
    MnPlayerInfo p{};
    p.player_id = 12345;
    p.room_id = 5;
    p.last_seen_ms = 1000;
    p.fighting = 1;
    EXPECT_EQ(p.player_id, 12345u);
    EXPECT_EQ(p.room_id, 5u);
    EXPECT_EQ(p.last_seen_ms, 1000u);
    EXPECT_EQ(p.fighting, 1u);
}

TEST(MnServerDataPlane, MnPlayerInfoFightingRange) {
    MnPlayerInfo p{};
    p.fighting = 0;
    EXPECT_EQ(p.fighting, 0u);
    p.fighting = 1;
    EXPECT_EQ(p.fighting, 1u);
    p.fighting = 255;
    EXPECT_EQ(p.fighting, 255u);
}

TEST(MnServerDataPlane, MnPlayerInfoReservedFieldsUntouched) {
    MnPlayerInfo p{};
    EXPECT_EQ(p.reserved0, 0u);
    EXPECT_EQ(p.reserved1, 0u);
    p.reserved0 = 0xAB;
    p.reserved1 = 0xCDEF;
    EXPECT_EQ(p.reserved0, 0xABu);
    EXPECT_EQ(p.reserved1, 0xCDEFu);
}

TEST(MnServerDataPlane, MnPlayerInfoRoomIdRange) {
    MnPlayerInfo p{};
    p.room_id = 0;
    EXPECT_EQ(p.room_id, 0u);
    p.room_id = 65535;
    EXPECT_EQ(p.room_id, 65535u);
    p.room_id = 4294967295u;
    EXPECT_EQ(p.room_id, 4294967295u);
}

TEST(MnServerDataPlane, MnPlayerInfoLastSeenRange) {
    MnPlayerInfo p{};
    p.last_seen_ms = 0;
    EXPECT_EQ(p.last_seen_ms, 0u);
    p.last_seen_ms = 4294967295u;
    EXPECT_EQ(p.last_seen_ms, 4294967295u);
}



// ===========================================================================
// State enum distinctness (compile-time guarantee)
// ===========================================================================

TEST(MnServerDataPlane, AllStatesAreDistinct) {
    EXPECT_NE(MnServerState::Stopped, MnServerState::Booting);
    EXPECT_NE(MnServerState::Stopped, MnServerState::Listening);
    EXPECT_NE(MnServerState::Stopped, MnServerState::Running);
    EXPECT_NE(MnServerState::Booting, MnServerState::Listening);
    EXPECT_NE(MnServerState::Booting, MnServerState::Running);
    EXPECT_NE(MnServerState::Listening, MnServerState::Running);
}

TEST(MnServerDataPlane, AllStatesHaveExpectedOrdinals) {
    EXPECT_LT(static_cast<int>(MnServerState::Stopped),
              static_cast<int>(MnServerState::Booting));
    EXPECT_LT(static_cast<int>(MnServerState::Booting),
              static_cast<int>(MnServerState::Listening));
    EXPECT_LT(static_cast<int>(MnServerState::Listening),
              static_cast<int>(MnServerState::Running));
}


// ===========================================================================
// Port parameter passing
// ===========================================================================

TEST(MnServerDataPlane, BindAcceptsMinimumPort) {
    MNServer s;
    EXPECT_TRUE(s.bind(1));
    EXPECT_EQ(s.state(), MnServerState::Listening);
}

TEST(MnServerDataPlane, BindAcceptsMaximumPort) {
    MNServer s;
    EXPECT_TRUE(s.bind(65535));
    EXPECT_EQ(s.state(), MnServerState::Listening);
}

TEST(MnServerDataPlane, BindAcceptsDefaultPort) {
    MNServer s;
    EXPECT_TRUE(s.bind(MXH_MN_DEFAULT_PORT));
    EXPECT_EQ(s.state(), MnServerState::Listening);
}

TEST(MnServerDataPlane, BindDefaultPortEqualsTwelveThousandOne) {
    EXPECT_EQ(MXH_MN_DEFAULT_PORT, 12001u);
}

TEST(MnServerDataPlane, MultipleBindSessions) {
    MNServer s;
    EXPECT_TRUE(s.bind(18001));
    EXPECT_TRUE(s.bind(18002));
    EXPECT_TRUE(s.bind(18003));
    EXPECT_EQ(s.state(), MnServerState::Listening);
}

TEST(MnServerDataPlane, StateFromStoppedThenBindThenStop) {
    MNServer s;
    EXPECT_EQ(s.state(), MnServerState::Stopped);
    s.bind(19000);
    EXPECT_EQ(s.state(), MnServerState::Listening);
    s.stop();
    EXPECT_EQ(s.state(), MnServerState::Stopped);
}


// ===========================================================================
// Distinct instances
// ===========================================================================

TEST(MnServerDataPlane, DistinctInstancesAreIndependent) {
    MNServer a;
    MNServer b;
    a.bind(20001);
    EXPECT_EQ(a.state(), MnServerState::Listening);
    EXPECT_EQ(b.state(), MnServerState::Stopped);
    b.bind(20002);
    EXPECT_EQ(a.state(), MnServerState::Listening);
    EXPECT_EQ(b.state(), MnServerState::Listening);
    a.stop();
    EXPECT_EQ(a.state(), MnServerState::Stopped);
    EXPECT_EQ(b.state(), MnServerState::Listening);
}

TEST(MnServerDataPlane, DistinctInstancesHaveIndependentChannels) {
    MNServer a;
    MNServer b;
    a.channels().create_channel(100);
    a.channels().create_channel(101);
    EXPECT_EQ(a.channel_count(), 2u);
    EXPECT_EQ(b.channel_count(), 0u);
}

TEST(MnServerDataPlane, MultipleStopCallsFromStopped) {
    MNServer s;
    s.stop();
    s.stop();
    s.stop();
    EXPECT_EQ(s.state(), MnServerState::Stopped);
}

TEST(MnServerDataPlane, BindThenMultipleStops) {
    MNServer s;
    s.bind(21000);
    s.stop();
    s.stop();
    s.stop();
    EXPECT_EQ(s.state(), MnServerState::Stopped);
}

TEST(MnServerDataPlane, ChannelsReturnsReference) {
    MNServer s;
    auto& chans = s.channels();
    EXPECT_EQ(&chans, &s.channels());
}
