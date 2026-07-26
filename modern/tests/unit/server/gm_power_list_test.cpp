// gm_power_list_test.cpp - Phase 6.3 GMPowerList 1:1 port tests.

#include "mxh/server/gm_power_list.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::GM_CHEAT_CATEGORY;
using mxh::server::GM_LOGIN_ACK_PROTOCOL;
using mxh::server::GM_LOGIN_NACK_PROTOCOL;
using mxh::server::GM_MAX_NAME_LENGTH;
using mxh::server::GmInfo;
using mxh::server::GmPower;
using mxh::server::add_gm_list;
using mxh::server::get_gm_info;
using mxh::server::get_gm_power;
using mxh::server::gm_info_count;
using mxh::server::gm_power_list_init;
using mxh::server::gm_power_list_release;
using mxh::server::is_event_started;
using mxh::server::make_gm_login_fail;
using mxh::server::make_gm_login_success;
using mxh::server::make_gm_power_list;
using mxh::server::remove_gm_list;
using mxh::server::set_event_cheat;

} // namespace

TEST(GmPowerEnum, ValuesMatchLegacy) {
    EXPECT_EQ(static_cast<int>(GmPower::Master), 0);
    EXPECT_EQ(static_cast<int>(GmPower::Monitor), 1);
    EXPECT_EQ(static_cast<int>(GmPower::Patroller), 2);
    EXPECT_EQ(static_cast<int>(GmPower::Auditor), 3);
    EXPECT_EQ(static_cast<int>(GmPower::Eventer), 4);
    EXPECT_EQ(static_cast<int>(GmPower::Max), 5);
}

TEST(GmPowerConstants, LegacyWireValuesMatch) {
    EXPECT_EQ(GM_MAX_NAME_LENGTH, 20u);
    EXPECT_EQ(GM_CHEAT_CATEGORY, 11u);
    EXPECT_EQ(GM_LOGIN_ACK_PROTOCOL, 81u);
    EXPECT_EQ(GM_LOGIN_NACK_PROTOCOL, 82u);
    EXPECT_EQ(sizeof(GmInfo::szGMID), 21u);
}

TEST(GmPowerLifecycle, NewListStartsEmptyAndFlagsFalse) {
    auto list = make_gm_power_list();
    EXPECT_EQ(gm_info_count(list), 0u);
    EXPECT_FALSE(list.m_bMonitorCheat);
    EXPECT_FALSE(is_event_started(list));
}

TEST(GmPowerLifecycle, InitDetectsMonitorFileWithoutClearingExistingFlag) {
    auto list = make_gm_power_list();
    gm_power_list_init(list, true);
    EXPECT_TRUE(list.m_bMonitorCheat);
    gm_power_list_init(list, false);
    EXPECT_TRUE(list.m_bMonitorCheat);
}

TEST(GmPowerAdd, StoresAllFieldsAndCopiesId) {
    auto list = make_gm_power_list();
    add_gm_list(list, 10u, GmPower::Patroller, 42u, "gm-alice");
    const GmInfo* info = get_gm_info(list, 10u);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->dwConnectionIndex, 10u);
    EXPECT_EQ(info->dwGMIndex, 42u);
    EXPECT_EQ(info->nPower, static_cast<int>(GmPower::Patroller));
    EXPECT_STREQ(info->szGMID, "gm-alice");
}

TEST(GmPowerAdd, IdIsTruncatedAndTerminatedAtLegacyLimit) {
    auto list = make_gm_power_list();
    add_gm_list(list, 10u, 0, 42u, "123456789012345678901234");
    const GmInfo* info = get_gm_info(list, 10u);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->szGMID[GM_MAX_NAME_LENGTH], 0);
    EXPECT_EQ(std::string_view(info->szGMID), "12345678901234567890");
}

TEST(GmPowerAdd, DuplicateConnectionsRemainFifoAndLookupReturnsFirst) {
    auto list = make_gm_power_list();
    add_gm_list(list, 10u, GmPower::Master, 1u, "first");
    add_gm_list(list, 10u, GmPower::Auditor, 2u, "second");
    EXPECT_EQ(gm_info_count(list), 2u);
    EXPECT_EQ(get_gm_power(list, 10u), static_cast<int>(GmPower::Master));
    ASSERT_TRUE(remove_gm_list(list, 10u));
    EXPECT_EQ(get_gm_power(list, 10u), static_cast<int>(GmPower::Auditor));
}

TEST(GmPowerAdd, MonitorCheatElevatesOnlyMonitorPower) {
    auto list = make_gm_power_list();
    gm_power_list_init(list, true);
    add_gm_list(list, 1u, GmPower::Monitor, 0u, "monitor");
    add_gm_list(list, 2u, GmPower::Patroller, 0u, "patroller");
    EXPECT_EQ(get_gm_power(list, 1u), static_cast<int>(GmPower::Master));
    EXPECT_EQ(get_gm_power(list, 2u), static_cast<int>(GmPower::Patroller));
}

TEST(GmPowerAdd, MonitorRemainsMonitorWithoutCheatFile) {
    auto list = make_gm_power_list();
    add_gm_list(list, 1u, GmPower::Monitor, 0u, "monitor");
    EXPECT_EQ(get_gm_power(list, 1u), static_cast<int>(GmPower::Monitor));
}

TEST(GmPowerLookup, MissingConnectionUsesLegacySentinels) {
    auto list = make_gm_power_list();
    EXPECT_EQ(get_gm_power(list, 999u), -1);
    EXPECT_EQ(get_gm_info(list, 999u), nullptr);
}

TEST(GmPowerRemove, ExistingRemovesOnlyFirstAndMissingIsFalse) {
    auto list = make_gm_power_list();
    add_gm_list(list, 7u, GmPower::Master, 1u, "first");
    add_gm_list(list, 8u, GmPower::Monitor, 2u, "other");
    EXPECT_TRUE(remove_gm_list(list, 7u));
    EXPECT_EQ(gm_info_count(list), 1u);
    EXPECT_FALSE(remove_gm_list(list, 7u));
    EXPECT_EQ(gm_info_count(list), 1u);
}

TEST(GmPowerRelease, ClearsRecordsButPreservesEventFlags) {
    auto list = make_gm_power_list();
    add_gm_list(list, 1u, GmPower::Master, 0u, "gm");
    gm_power_list_init(list, true);
    set_event_cheat(list, true);
    gm_power_list_release(list);
    EXPECT_EQ(gm_info_count(list), 0u);
    EXPECT_TRUE(list.m_bMonitorCheat);
    EXPECT_TRUE(is_event_started(list));
}

TEST(GmPowerEvent, SetAndGetEventFlagRoundTrip) {
    auto list = make_gm_power_list();
    EXPECT_FALSE(is_event_started(list));
    set_event_cheat(list, true);
    EXPECT_TRUE(is_event_started(list));
    set_event_cheat(list, false);
    EXPECT_FALSE(is_event_started(list));
}

TEST(GmPowerMessages, LoginSuccessUsesAckAndPowerData) {
    const auto message = make_gm_login_success(123u, static_cast<int>(GmPower::Auditor));
    EXPECT_EQ(message.Category, GM_CHEAT_CATEGORY);
    EXPECT_EQ(message.Protocol, GM_LOGIN_ACK_PROTOCOL);
    EXPECT_EQ(message.dwObjectID, 123u);
    EXPECT_EQ(message.dwData, 3u);
    EXPECT_TRUE(message.HasData);
}

TEST(GmPowerMessages, LoginFailUsesNackWithoutPowerData) {
    const auto message = make_gm_login_fail(123u);
    EXPECT_EQ(message.Category, GM_CHEAT_CATEGORY);
    EXPECT_EQ(message.Protocol, GM_LOGIN_NACK_PROTOCOL);
    EXPECT_EQ(message.dwObjectID, 123u);
    EXPECT_EQ(message.dwData, 0u);
    EXPECT_FALSE(message.HasData);
}

TEST(GmPowerInput, EmptyIdIsNulTerminated) {
    auto list = make_gm_power_list();
    add_gm_list(list, 1u, GmPower::Master, 0u, "");
    const GmInfo* info = get_gm_info(list, 1u);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->szGMID[0], 0);
}

TEST(GmPowerRelease, NewRecordsCanBeAddedAfterRelease) {
    auto list = make_gm_power_list();
    add_gm_list(list, 1u, GmPower::Master, 0u, "before");
    gm_power_list_release(list);
    add_gm_list(list, 2u, GmPower::Eventer, 1u, "after");
    EXPECT_EQ(gm_info_count(list), 1u);
    EXPECT_EQ(get_gm_power(list, 2u), static_cast<int>(GmPower::Eventer));
}