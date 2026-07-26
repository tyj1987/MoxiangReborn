#include "mxh/server/server_system.hpp"
#include <gtest/gtest.h>

namespace {
using namespace mxh::server;

ConnectionUserState user() { return ConnectionUserState{42u, 7u, 0u, false, false, true}; }

TEST(ServerSystemConnection, BillTickSendsSpeedCheckWithLegacyPacket) {
    auto actions = compute_connection_actions(user(), 100u, true);
    ASSERT_EQ(actions.size(), 1u);
    EXPECT_EQ(actions[0].kind, ConnectionActionKind::send_speed_hack_check);
    EXPECT_EQ(actions[0].category, mp_hackcheck);
    EXPECT_EQ(actions[0].protocol, mp_hackcheck_speedhack);
    EXPECT_EQ(actions[0].data, 100u);
}
TEST(ServerSystemConnection, NonBillTickDoesNotSpeedCheck) {
    EXPECT_TRUE(compute_connection_actions(user(), 100u, false).empty());
}
TEST(ServerSystemConnection, TenMinutesSendsConnectionProbe) {
    auto u = user(); u.last_connection_check_time = 0;
    auto actions = compute_connection_actions(u, 600001u, false);
    ASSERT_EQ(actions.size(), 1u);
    EXPECT_EQ(actions[0].kind, ConnectionActionKind::send_connection_check);
    EXPECT_EQ(actions[0].category, mp_userconn);
    EXPECT_EQ(actions[0].protocol, mp_userconn_connection_check);
}
TEST(ServerSystemConnection, FailedProbeDisconnects) {
    auto u = user(); u.connection_check_failed = true;
    auto actions = compute_connection_actions(u, 600001u, false);
    ASSERT_EQ(actions.size(), 1u);
    EXPECT_EQ(actions[0].kind, ConnectionActionKind::mark_disconnect);
}
TEST(ServerSystemConnection, DeadConnectionRemovedAfterTwoMinutes) {
    auto u = user(); u.connection_index = 0;
    auto actions = compute_connection_actions(u, 120001u, false);
    ASSERT_EQ(actions.size(), 1u);
    EXPECT_EQ(actions[0].kind, ConnectionActionKind::remove_user);
}
TEST(ServerSystemConnection, ProgrammerSkipsProbe) {
    auto u = user(); u.programmer = true; u.last_connection_check_time = 0;
    EXPECT_TRUE(compute_connection_actions(u, 600001u, false).empty());
}
TEST(ServerSystemTick, HooksRunInRegistrationOrder) {
    ServerSystemTick tick; std::vector<int> order;
    tick.hooks.emplace_back([&](std::uint32_t) { order.push_back(1); });
    tick.hooks.emplace_back([&](std::uint32_t) { order.push_back(2); });
    tick.process(5u);
    EXPECT_EQ(order, (std::vector<int>{1, 2}));
}
TEST(ServerSystemEvents, StringsUseLegacyCapacity) {
    EventNotifyState state;
    set_event_notify_strings(state, std::string(40, 'T'), std::string(140, 'C'));
    EXPECT_EQ(state.title.size(), 31u); EXPECT_EQ(state.context.size(), 127u);
}
TEST(ServerSystemEvents, EnableAndReset) {
    EventNotifyState state; set_applied_event(state, 3u); set_event_notify_enabled(state, true);
    EXPECT_TRUE(state.enabled); EXPECT_TRUE(state.applied[3]);
    reset_applied_events(state); EXPECT_FALSE(state.applied[3]);
}
TEST(ServerSystemEvents, InvalidEventRejected) {
    EventNotifyState state; EXPECT_FALSE(set_applied_event(state, event_max));
}
TEST(ServerSystemAuth, KeysAreMonotonicAndNonzero) {
    ServerSystemState state; state.next_auth_key = 0u;
    const auto first = make_auth_key(state); const auto second = make_auth_key(state);
    EXPECT_NE(first, 0u); EXPECT_EQ(second, first + 1u);
}
TEST(ServerSystemMapChange, LookupReturnsFirstMatchingKind) {
    ServerSystemState state; state.map_changes.push_back(MapChangeInfo{7u});
    state.map_changes.push_back(MapChangeInfo{8u});
    ASSERT_NE(get_map_change_info(state, 7u), nullptr);
    EXPECT_EQ(get_map_change_info(state, 7u)->kind, 7u);
    EXPECT_EQ(get_map_change_info(state, 99u), nullptr);
}
}