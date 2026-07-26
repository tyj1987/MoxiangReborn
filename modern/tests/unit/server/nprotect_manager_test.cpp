// nprotect_manager_test.cpp - Phase 6.3 NProtectManager tests.

#include "mxh/server/nprotect_manager.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::NPLOG_DEBUG;
using mxh::server::NPLOG_ERROR;
using mxh::server::NPROTECT_CATEGORY;
using mxh::server::NPROTECT_GM_LEVEL;
using mxh::server::NPROTECT_INITIAL_TIMEOUT_MS;
using mxh::server::NPROTECT_SECOND_ANSWER_OFFSET_MS;
using mxh::server::NPROTECT_STEADY_INTERVAL_MS;
using mxh::server::NPROTECT_UPDATE_INTERVAL_MS;
using mxh::server::NProtectActionKind;
using mxh::server::NProtectAuthData;
using mxh::server::NProtectPacket;
using mxh::server::NProtectProtocol;
using mxh::server::NProtectUserState;
using mxh::server::make_nprotect_manager;
using mxh::server::mark_hack_tool_user;
using mxh::server::nprotect_check_tick;
using mxh::server::nprotect_init;
using mxh::server::nprotect_release;
using mxh::server::nprotect_should_log;
using mxh::server::nprotect_update;
using mxh::server::parse_nprotect_answer;
using mxh::server::send_auth_query;

NProtectAuthData query(std::uint32_t index,
                       std::uint32_t value1,
                       std::uint32_t value2,
                       std::uint32_t value3) {
    NProtectAuthData data;
    data.dwIndex = index;
    data.dwValue1 = value1;
    data.dwValue2 = value2;
    data.dwValue3 = value3;
    return data;
}

NProtectPacket answer(std::uint32_t object_id,
                      const NProtectAuthData& data) {
    NProtectPacket packet;
    packet.Protocol = NProtectProtocol::Answer;
    packet.dwObjectID = object_id;
    packet.dwData1 = data.dwIndex;
    packet.dwData2 = data.dwValue1;
    packet.dwData3 = data.dwValue2;
    packet.dwData4 = data.dwValue3;
    return packet;
}

NProtectUserState user_at_level(std::uint8_t level = NPROTECT_GM_LEVEL) {
    NProtectUserState user;
    user.dwConnectionIndex = 17u;
    user.dwUserID = 99u;
    user.dwCharacterID = 123u;
    user.UserLevel = level;
    return user;
}

} // namespace

TEST(NProtectConstants, ProtocolCategoryAndValuesMatchLegacy) {
    EXPECT_EQ(NPROTECT_CATEGORY, 69u);
    EXPECT_EQ(static_cast<int>(NProtectProtocol::Query), 0);
    EXPECT_EQ(static_cast<int>(NProtectProtocol::Answer), 1);
    EXPECT_EQ(static_cast<int>(NProtectProtocol::Disconnect), 2);
    EXPECT_EQ(static_cast<int>(NProtectProtocol::UserDisconnect), 3);
    EXPECT_EQ(static_cast<int>(NProtectProtocol::HackToolUser), 4);
}

TEST(NProtectConstants, TimingConstantsMatchLegacy) {
    EXPECT_EQ(NPROTECT_INITIAL_TIMEOUT_MS, 60000u);
    EXPECT_EQ(NPROTECT_STEADY_INTERVAL_MS, 180000u);
    EXPECT_EQ(NPROTECT_SECOND_ANSWER_OFFSET_MS, 120000u);
    EXPECT_EQ(NPROTECT_UPDATE_INTERVAL_MS, 300000u);
}

TEST(NProtectLifecycle, InitStoresMapAndVendorResult) {
    auto manager = make_nprotect_manager();
    EXPECT_FALSE(nprotect_init(manager, 7u, false));
    EXPECT_EQ(manager.m_MapNum, 7u);
    EXPECT_FALSE(manager.m_Initialized);
    EXPECT_TRUE(nprotect_init(manager, 8u, true));
    EXPECT_EQ(manager.m_MapNum, 8u);
    EXPECT_TRUE(manager.m_Initialized);
    EXPECT_TRUE(manager.m_InitialUpdateRequested);
}

TEST(NProtectLifecycle, ReleaseMarksSdkUnavailable) {
    auto manager = make_nprotect_manager();
    nprotect_init(manager, 1u, true);
    nprotect_release(manager);
    EXPECT_FALSE(manager.m_Initialized);
    EXPECT_EQ(manager.m_MapNum, 1u);
}

TEST(NProtectUpdate, FirstCallStartsClockWithoutUpdating) {
    auto manager = make_nprotect_manager();
    EXPECT_FALSE(nprotect_update(manager, 1000u));
    EXPECT_TRUE(manager.m_UpdateClockStarted);
    EXPECT_EQ(manager.m_dwUpdateCheckTime, 1000u);
}

TEST(NProtectUpdate, UpdateRunsOnlyAtFiveMinutes) {
    auto manager = make_nprotect_manager();
    ASSERT_FALSE(nprotect_update(manager, 1000u));
    EXPECT_FALSE(nprotect_update(manager, 1000u + NPROTECT_UPDATE_INTERVAL_MS - 1u));
    EXPECT_TRUE(nprotect_update(manager, 1000u + NPROTECT_UPDATE_INTERVAL_MS));
    EXPECT_EQ(manager.m_dwUpdateCheckTime,
              1000u + NPROTECT_UPDATE_INTERVAL_MS);
}

TEST(NProtectLogging, DebugOrErrorModesAreLogged) {
    EXPECT_FALSE(nprotect_should_log(0));
    EXPECT_TRUE(nprotect_should_log(NPLOG_DEBUG));
    EXPECT_TRUE(nprotect_should_log(NPLOG_ERROR));
    EXPECT_TRUE(nprotect_should_log(NPLOG_DEBUG | NPLOG_ERROR));
}

TEST(NProtectQuery, StoresAuthAndBuildsWireArithmetic) {
    auto user = user_at_level();
    const auto data = query(10u, 20u, 30u, 40u);
    const auto action = send_auth_query(user, data, true, 500u);
    ASSERT_EQ(action.Kind, NProtectActionKind::SendQuery);
    EXPECT_EQ(action.Packet.Category, NPROTECT_CATEGORY);
    EXPECT_EQ(action.Packet.Protocol, NProtectProtocol::Query);
    EXPECT_EQ(action.Packet.dwData1, 10u);
    EXPECT_EQ(action.Packet.dwData2, 20u);
    EXPECT_EQ(action.Packet.dwData3, 30u);
    EXPECT_EQ(action.Packet.dwData4, 40u);
    EXPECT_EQ(action.Packet.dwObjectID, 10u + 20u + 60u);
    EXPECT_EQ(user.m_dwHUC, 5u + 10u + 30u + 80u);
    EXPECT_TRUE(user.m_bCSA);
    EXPECT_EQ(user.dwLastNProtectCheck, 500u);
}

TEST(NProtectQuery, VendorFailureDoesNotSetPendingState) {
    auto user = user_at_level();
    const auto action = send_auth_query(user, query(1u, 2u, 3u, 4u), false, 500u);
    EXPECT_EQ(action.Kind, NProtectActionKind::None);
    EXPECT_FALSE(user.m_bCSA);
    EXPECT_EQ(user.dwLastNProtectCheck, 0u);
}

TEST(NProtectQuery, OutstandingQuerySendsDisconnectAndOnlyGmDisconnects) {
    auto user = user_at_level(NPROTECT_GM_LEVEL);
    user.m_bCSA = true;
    auto action = send_auth_query(user, query(1u, 2u, 3u, 4u), true, 0u);
    EXPECT_EQ(action.Kind, NProtectActionKind::SendDisconnect);
    EXPECT_EQ(action.Packet.Protocol, NProtectProtocol::Disconnect);
    EXPECT_TRUE(action.ShouldDisconnectUser);

    user = user_at_level(NPROTECT_GM_LEVEL - 1u);
    user.m_bCSA = true;
    action = send_auth_query(user, query(1u, 2u, 3u, 4u), true, 0u);
    EXPECT_FALSE(action.ShouldDisconnectUser);
}

TEST(NProtectAnswer, InvalidAnswerDisconnectsAndBlocks) {
    auto user = user_at_level();
    user.m_bCSA = true;
    const auto action = parse_nprotect_answer(
        &user, answer(99u, query(1u, 2u, 3u, 4u)), 0x55u);
    EXPECT_EQ(action.Kind, NProtectActionKind::DisconnectAndBlock);
    EXPECT_EQ(action.Packet.Protocol, NProtectProtocol::Disconnect);
    EXPECT_TRUE(action.ShouldDisconnectUser);
    EXPECT_TRUE(action.ShouldBlock);
    EXPECT_EQ(action.BlockType, 0x55u);
    EXPECT_TRUE(user.m_bCSA);
    EXPECT_EQ(user.m_AuthAnswer.dwValue2, 3u);
}

TEST(NProtectAnswer, FirstValidAnswerStartsSecondQueryAndMovesToStateTwo) {
    auto user = user_at_level();
    user.m_nCSAInit = 1;
    user.m_bCSA = true;
    const auto next = query(5u, 6u, 7u, 8u);
    const auto action = parse_nprotect_answer(
        &user, answer(0u, query(1u, 2u, 3u, 4u)), 0u, next, true, 1000u);
    EXPECT_EQ(action.Kind, NProtectActionKind::SendQuery);
    EXPECT_EQ(action.Packet.dwData1, 5u);
    EXPECT_EQ(action.Packet.dwObjectID, 5u + 6u + 14u);
    EXPECT_EQ(user.m_nCSAInit, 2);
    EXPECT_TRUE(user.m_bCSA);
    EXPECT_EQ(user.dwLastNProtectCheck, 1000u);
}

TEST(NProtectAnswer, SecondValidAnswerEntersSteadyStateWithOffset) {
    auto user = user_at_level();
    user.m_nCSAInit = 2;
    user.m_bCSA = true;
    user.dwLastNProtectCheck = 3000u;
    const auto action = parse_nprotect_answer(
        &user, answer(0u, query(1u, 2u, 3u, 4u)), 0u);
    EXPECT_EQ(action.Kind, NProtectActionKind::None);
    EXPECT_EQ(user.m_nCSAInit, 3);
    EXPECT_FALSE(user.m_bCSA);
    EXPECT_EQ(user.dwLastNProtectCheck, 3000u + NPROTECT_SECOND_ANSWER_OFFSET_MS);
}

TEST(NProtectAnswer, SteadyStateHucMismatchLeavesQueryPending) {
    auto user = user_at_level();
    user.m_nCSAInit = 3;
    user.m_bCSA = true;
    user.m_dwHUC = 777u;
    const auto action = parse_nprotect_answer(
        &user, answer(778u, query(1u, 2u, 3u, 4u)), 0u);
    EXPECT_EQ(action.Kind, NProtectActionKind::None);
    EXPECT_TRUE(user.m_bCSA);
}

TEST(NProtectAnswer, QueryCreationFailureAfterFirstAnswerLeavesStateTwo) {
    auto user = user_at_level();
    user.m_nCSAInit = 1;
    user.m_bCSA = true;
    const auto action = parse_nprotect_answer(
        &user, answer(0u, query(1u, 2u, 3u, 4u)), 0u, {}, false, 100u);
    EXPECT_EQ(action.Kind, NProtectActionKind::None);
    EXPECT_EQ(user.m_nCSAInit, 2);
    EXPECT_FALSE(user.m_bCSA);
}

TEST(NProtectAnswer, MissingUserIsIgnored) {
    const auto action = parse_nprotect_answer(
        nullptr, answer(0u, query(1u, 2u, 3u, 4u)), 1u);
    EXPECT_EQ(action.Kind, NProtectActionKind::None);
}

TEST(NProtectTick, InitialAuthTimesOutAfterOneMinute) {
    auto user = user_at_level();
    user.m_nCSAInit = 1;
    user.dwLastNProtectCheck = 1000u;
    const auto action = nprotect_check_tick(user, 1000u + NPROTECT_INITIAL_TIMEOUT_MS);
    EXPECT_EQ(action.Kind, NProtectActionKind::SendDisconnect);
    EXPECT_EQ(user.m_nCSAInit, 4);
    EXPECT_TRUE(action.ShouldDisconnectUser);
}

TEST(NProtectTick, InitialAuthBelowGmSendsButDoesNotForceDisconnect) {
    auto user = user_at_level(NPROTECT_GM_LEVEL - 1u);
    user.m_nCSAInit = 2;
    user.dwLastNProtectCheck = 1000u;
    const auto action = nprotect_check_tick(user, 1000u + NPROTECT_INITIAL_TIMEOUT_MS);
    EXPECT_EQ(action.Kind, NProtectActionKind::SendDisconnect);
    EXPECT_FALSE(action.ShouldDisconnectUser);
}

TEST(NProtectTick, InitialAuthBeforeTimeoutIsNoOp) {
    auto user = user_at_level();
    user.m_nCSAInit = 2;
    user.dwLastNProtectCheck = 1000u;
    const auto action = nprotect_check_tick(
        user, 1000u + NPROTECT_INITIAL_TIMEOUT_MS - 1u);
    EXPECT_EQ(action.Kind, NProtectActionKind::None);
    EXPECT_EQ(user.m_nCSAInit, 2);
}

TEST(NProtectTick, SteadyStateRequestsAfterThreeMinutes) {
    auto user = user_at_level();
    user.m_nCSAInit = 3;
    user.dwLastNProtectCheck = 1000u;
    const auto action = nprotect_check_tick(
        user, 1000u + NPROTECT_STEADY_INTERVAL_MS, query(7u, 8u, 9u, 10u));
    EXPECT_EQ(action.Kind, NProtectActionKind::SendQuery);
    EXPECT_TRUE(user.m_bCSA);
    EXPECT_EQ(user.dwLastNProtectCheck, 1000u + NPROTECT_STEADY_INTERVAL_MS);
}

TEST(NProtectTick, SteadyStateBeforeIntervalIsNoOp) {
    auto user = user_at_level();
    user.m_nCSAInit = 3;
    user.dwLastNProtectCheck = 1000u;
    const auto action = nprotect_check_tick(
        user, 1000u + NPROTECT_STEADY_INTERVAL_MS - 1u);
    EXPECT_EQ(action.Kind, NProtectActionKind::None);
}

TEST(NProtectTick, PendingSteadyStateQueryTriggersDisconnect) {
    auto user = user_at_level();
    user.m_nCSAInit = 3;
    user.m_bCSA = true;
    user.dwLastNProtectCheck = 1000u;
    const auto action = nprotect_check_tick(
        user, 1000u + NPROTECT_STEADY_INTERVAL_MS);
    EXPECT_EQ(action.Kind, NProtectActionKind::SendDisconnect);
}

TEST(NProtectHackTool, MarkingUserSetsLegacyFlag) {
    auto user = user_at_level();
    EXPECT_FALSE(user.bHackToolUser);
    mark_hack_tool_user(&user);
    EXPECT_TRUE(user.bHackToolUser);
    mark_hack_tool_user(nullptr);
}

TEST(NProtectArithmetic, DWORDOverflowIsPreserved) {
    auto user = user_at_level();
    const auto data = query(0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu);
    const auto action = send_auth_query(user, data, true, 0u);
    ASSERT_EQ(action.Kind, NProtectActionKind::SendQuery);
    EXPECT_EQ(action.Packet.dwObjectID, 0xfffffffcU);
    EXPECT_EQ(user.m_dwHUC, 0xfffffffbU);
}