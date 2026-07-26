// bobusang_manager_agent_test.cpp - Phase 6.3 BobusangManager_Agent tests.

#include "mxh/server/bobusang_manager_agent.hpp"

#include <gtest/gtest.h>

namespace {
using namespace mxh::server;

BobusangManagerAgent configured() {
    auto manager = make_bobusang_manager_agent();
    bobusang_manager_set_manager(manager, true);
    load_bobusang_channels(manager, {{12u, 0u, 2u}, {13u, 0u, 3u}});
    std::vector<BobusangMapInfo> maps(2);
    maps[0].dataIdx = 0u;
    maps[0].mapNum = 12u;
    maps[1].dataIdx = 1u;
    maps[1].mapNum = 13u;
    configure_bobusang_schedule(manager, 1u, 3u, 2u, 4u, maps);
    return manager;
}

BobusangRandomChoice choice(std::size_t map_index = 0u,
                            std::uint32_t appearance_gap = 2u,
                            std::uint32_t duration = 3u) {
    BobusangRandomChoice result;
    result.map_index = map_index;
    result.appearance_gap_minutes = appearance_gap;
    result.duration_minutes = duration;
    result.position_index = 5u;
    result.selling_list_index = 9u;
    return result;
}

} // namespace

TEST(BobusangAgentConstants, LegacyLimitsAndStatesMatch) {
    EXPECT_EQ(BOBUSANG_POSNUM_MAX, 4u);
    EXPECT_EQ(DEALITEM_BIN_TABNUM, 7u);
    EXPECT_EQ(BOBUSANG_CHECKTIME, 60000u);
    EXPECT_EQ(eBBSAS_NONE, -1);
    EXPECT_EQ(eBBSAS_DISAPPEAR, 0);
    EXPECT_EQ(eBBSAS_APPEAR, 2);
    EXPECT_EQ(eBBSAS_APPEAR_DELAYED, 4);
    EXPECT_EQ(eBBSAS_DISAPPEAR_DELAYED, 8);
    EXPECT_EQ(eBBSAS_TIME_DELAYED, 16);
}

TEST(BobusangAgentConstants, ProtocolValuesMatchLegacy) {
    EXPECT_EQ(BOBUSANG_CATEGORY, 74u);
    EXPECT_EQ(BOBUSANG_INFO_AGENT_TO_MAP, 0u);
    EXPECT_EQ(BOBUSANG_DISAPPEAR_AGENT_TO_MAP, 1u);
    EXPECT_EQ(BOBUSANG_NOTIFY_FOR_DISAPPEARANCE, 12u);
    EXPECT_EQ(CHEAT_BOBUSANG_INFO_ACK, 149u);
    EXPECT_EQ(CHEAT_BOBUSANG_LEAVE_NACK, 153u);
}

TEST(BobusangAgentTime, PackedFieldsRoundTrip) {
    const auto time = bobusang_pack_time(8u, 6u, 12u, 13u, 45u, 30u);
    EXPECT_EQ(bobusang_time_year(time), 8u);
    EXPECT_EQ(bobusang_time_month(time), 6u);
    EXPECT_EQ(bobusang_time_day(time), 12u);
    EXPECT_EQ(bobusang_time_hour(time), 13u);
    EXPECT_EQ(bobusang_time_minute(time), 45u);
    EXPECT_EQ(bobusang_time_second(time), 30u);
}

TEST(BobusangAgentTime, AddMinutesCarriesHourAndDay) {
    const auto time = bobusang_pack_time(24u, 6u, 12u, 23u, 45u, 0u);
    const auto result = bobusang_add_minutes(time, 30u);
    EXPECT_EQ(bobusang_time_day(result), 13u);
    EXPECT_EQ(bobusang_time_hour(result), 0u);
    EXPECT_EQ(bobusang_time_minute(result), 15u);
}

TEST(BobusangAgentLifecycle, StartUsesServerZeroAsManager) {
    auto manager = make_bobusang_manager_agent();
    bobusang_manager_start(manager, 0u);
    EXPECT_TRUE(manager.m_bManager);
    bobusang_manager_start(manager, 1u);
    EXPECT_FALSE(manager.m_bManager);
}

TEST(BobusangAgentLifecycle, ChannelLoadUsesLargestChannelCount) {
    auto manager = make_bobusang_manager_agent();
    ASSERT_TRUE(load_bobusang_channels(manager, {{1u, 0u, 2u}, {2u, 0u, 5u}}));
    EXPECT_EQ(manager.m_nChannelTotalNum, 5u);
    EXPECT_EQ(manager.m_AppearedState.size(), 5u);
    EXPECT_EQ(manager.m_BobusangInfo.size(), 10u);
    EXPECT_EQ(bobusang_channel_state(manager, 0u), eBBSAS_NONE);
}

TEST(BobusangAgentLifecycle, ReleaseClearsAllChannelState) {
    auto manager = configured();
    initialize_bobusang_info(manager, bobusang_pack_time(24u, 1u, 1u, 0u, 0u, 0u), choice());
    bobusang_manager_release(manager);
    EXPECT_EQ(manager.m_nChannelTotalNum, 0u);
    EXPECT_TRUE(manager.m_AppearedState.empty());
    EXPECT_TRUE(manager.m_BobusangInfo.empty());
}

TEST(BobusangAgentSchedule, SetInfoFillsNextRecordAndRanges) {
    auto manager = configured();
    const auto now = bobusang_pack_time(24u, 1u, 1u, 10u, 0u, 0u);
    ASSERT_TRUE(set_bobusang_info(manager, 0u, now, choice(1u, 2u, 3u)));
    const auto* next = bobusang_info(manager, 0u, eBBSIT_NEXT);
    ASSERT_NE(next, nullptr);
    EXPECT_EQ(next->AppearanceChannel, 0u);
    EXPECT_EQ(next->AppearanceMapNum, 13u);
    EXPECT_EQ(next->AppearanceTime, bobusang_add_minutes(now, 2u));
    EXPECT_EQ(next->DisappearanceTime, bobusang_add_minutes(next->AppearanceTime, 3u));
    EXPECT_EQ(next->AppearancePosIdx, 1u);
    EXPECT_EQ(next->SellingListIndex, 2u);
    EXPECT_EQ(bobusang_channel_state(manager, 0u), eBBSAS_DISAPPEAR);
}

TEST(BobusangAgentSchedule, FailedMapConnectionSetsNone) {
    auto manager = configured();
    manager.m_MapInfo[0].MapServerConnected = false;
    EXPECT_FALSE(set_bobusang_info(manager, 0u,
                                   bobusang_pack_time(24u, 1u, 1u, 0u, 0u, 0u), choice()));
    EXPECT_EQ(bobusang_channel_state(manager, 0u), eBBSAS_NONE);
}

TEST(BobusangAgentSchedule, InvalidChannelDoesNotWrite) {
    auto manager = configured();
    EXPECT_FALSE(set_bobusang_info(manager, 99u, 0u, choice()));
    EXPECT_EQ(bobusang_channel_state(manager, 99u), eBBSAS_NONE);
}

TEST(BobusangAgentProcess, NoneChannelSchedulesNextInfoAfterCheckInterval) {
    auto manager = configured();
    const auto now = bobusang_pack_time(24u, 1u, 1u, 0u, 0u, 0u);
    const auto actions = bobusang_process(manager, 60001u, now, choice());
    EXPECT_TRUE(actions.empty());
    EXPECT_NE(bobusang_info(manager, 0u, eBBSIT_NEXT), nullptr);
    EXPECT_EQ(bobusang_channel_state(manager, 0u), eBBSAS_DISAPPEAR);
    EXPECT_EQ(manager.m_dwBobusangCheckTime, 60001u);
}

TEST(BobusangAgentProcess, CheckIntervalIsStrictlyGreaterThanSixtySeconds) {
    auto manager = configured();
    const auto now = bobusang_pack_time(24u, 1u, 1u, 0u, 0u, 0u);
    EXPECT_TRUE(bobusang_process(manager, 60000u, now, choice()).empty());
    EXPECT_EQ(manager.m_dwBobusangCheckTime, 0u);
}

TEST(BobusangAgentProcess, DisappearStateSendsInfoAndCopiesCurrent) {
    auto manager = configured();
    const auto now = bobusang_pack_time(24u, 1u, 1u, 10u, 0u, 0u);
    ASSERT_TRUE(set_bobusang_info(manager, 0u, now, choice(0u, 0u, 3u)));
    const auto appear_time = bobusang_info(manager, 0u, eBBSIT_NEXT)->AppearanceTime;
    const auto actions = bobusang_process(manager, 60001u,
        bobusang_add_minutes(appear_time, 1u), choice());
    ASSERT_EQ(actions.size(), 1u);
    EXPECT_EQ(actions[0].kind, BobusangOutbound::Kind::InfoToMap);
    EXPECT_EQ(actions[0].channel, 0u);
    EXPECT_EQ(bobusang_channel_state(manager, 0u), eBBSAS_APPEAR_DELAYED);
    EXPECT_EQ(bobusang_info(manager, 0u, eBBSIT_CUR)->AppearanceMapNum, 12u);
}

TEST(BobusangAgentProcess, AppearanceStateNotifiesOneMinuteBeforeLeave) {
    auto manager = configured();
    const auto now = bobusang_pack_time(24u, 1u, 1u, 10u, 0u, 0u);
    ASSERT_TRUE(set_bobusang_info(manager, 0u, now, choice(0u, 0u, 3u)));
    *bobusang_info(manager, 0u, eBBSIT_CUR) = *bobusang_info(manager, 0u, eBBSIT_NEXT);
    manager.m_AppearedState[0] = eBBSAS_APPEAR;
    const auto disappearance = bobusang_info(manager, 0u, eBBSIT_CUR)->DisappearanceTime;
    const auto actions = bobusang_process(manager, 60001u,
        bobusang_add_minutes(disappearance, 0u), choice());
    ASSERT_EQ(actions.size(), 1u);
    EXPECT_EQ(actions[0].kind, BobusangOutbound::Kind::NotifyDisappearance);
    EXPECT_EQ(bobusang_channel_state(manager, 0u), eBBSAS_APPEAR);
}

TEST(BobusangAgentProcess, AppearancePastEndSendsLeaveAndDelays) {
    auto manager = configured();
    const auto now = bobusang_pack_time(24u, 1u, 1u, 10u, 0u, 0u);
    ASSERT_TRUE(set_bobusang_info(manager, 0u, now, choice(0u, 0u, 1u)));
    *bobusang_info(manager, 0u, eBBSIT_CUR) = *bobusang_info(manager, 0u, eBBSIT_NEXT);
    manager.m_AppearedState[0] = eBBSAS_APPEAR;
    const auto disappearance = bobusang_info(manager, 0u, eBBSIT_CUR)->DisappearanceTime;
    const auto actions = bobusang_process(manager, 60001u,
        bobusang_add_minutes(disappearance, 2u), choice());
    ASSERT_GE(actions.size(), 1u);
    EXPECT_EQ(actions.back().kind, BobusangOutbound::Kind::LeaveToMap);
    EXPECT_EQ(bobusang_channel_state(manager, 0u), eBBSAS_DISAPPEAR_DELAYED);
}

TEST(BobusangAgentProcess, DelayedStateRefreshesAfterMaximumGap) {
    auto manager = configured();
    const auto now = bobusang_pack_time(24u, 1u, 1u, 10u, 0u, 0u);
    ASSERT_TRUE(set_bobusang_info(manager, 0u, now, choice(0u, 1u, 1u)));
    manager.m_AppearedState[0] = eBBSAS_APPEAR_DELAYED;
    const auto appearance = bobusang_info(manager, 0u, eBBSIT_NEXT)->AppearanceTime;
    bobusang_process(manager, 60001u,
        bobusang_add_minutes(appearance, manager.m_dwAppearTimeMax + 1u), choice());
    EXPECT_EQ(bobusang_channel_state(manager, 0u), eBBSAS_DISAPPEAR);
}

TEST(BobusangAgentProcess, ProcessingOffStopsAtDisappearState) {
    auto manager = configured();
    manager.m_bOnProcessing = false;
    const auto now = bobusang_pack_time(24u, 1u, 1u, 0u, 0u, 0u);
    set_bobusang_info(manager, 0u, now, choice(0u, 0u, 1u));
    const auto actions = bobusang_process(manager, 60001u,
        bobusang_add_minutes(now, 2u), choice());
    EXPECT_TRUE(actions.empty());
    EXPECT_EQ(bobusang_channel_state(manager, 0u), eBBSAS_DISAPPEAR);
}

TEST(BobusangAgentControl, SetChannelStateRequiresManagerAndBounds) {
    auto manager = configured();
    EXPECT_TRUE(set_channel_state(manager, 1u, eBBSAS_APPEAR));
    EXPECT_EQ(bobusang_channel_state(manager, 1u), eBBSAS_APPEAR);
    EXPECT_FALSE(set_channel_state(manager, 99u, eBBSAS_APPEAR));
    manager.m_bManager = false;
    EXPECT_FALSE(set_channel_state(manager, 1u, eBBSAS_NONE));
}

TEST(BobusangAgentControl, ProcessingSetterOnlyReportsChanges) {
    auto manager = configured();
    EXPECT_FALSE(set_bobusang_processing(manager, true));
    EXPECT_TRUE(set_bobusang_processing(manager, false));
    EXPECT_FALSE(set_bobusang_processing(manager, false));
}

TEST(BobusangAgentDeveloper, InfoRequestAckContainsCurAndNext) {
    auto manager = configured();
    initialize_bobusang_info(manager, 0u, choice());
    const auto reply = developer_bobusang_info(manager, 0u);
    EXPECT_EQ(reply.protocol, CHEAT_BOBUSANG_INFO_ACK);
    EXPECT_EQ(reply.appearance_state, eBBSAS_DISAPPEAR);
    EXPECT_EQ(reply.info[0].AppearanceChannel, 0u);
    EXPECT_EQ(reply.info[1].AppearanceChannel, 0u);
}

TEST(BobusangAgentDeveloper, InvalidInfoRequestUsesNack) {
    const auto reply = developer_bobusang_info(configured(), 99u);
    EXPECT_EQ(reply.protocol, CHEAT_BOBUSANG_INFO_NACK);
}

TEST(BobusangAgentDeveloper, LeaveValidStateUsesAckAndDelayedState) {
    auto manager = configured();
    const auto now = bobusang_pack_time(24u, 1u, 1u, 0u, 0u, 0u);
    set_bobusang_info(manager, 0u, now, choice());
    *bobusang_info(manager, 0u, eBBSIT_CUR) = *bobusang_info(manager, 0u, eBBSIT_NEXT);
    manager.m_AppearedState[0] = eBBSAS_APPEAR;
    const auto reply = developer_bobusang_leave(manager, 0u);
    EXPECT_EQ(reply.protocol, CHEAT_BOBUSANG_LEAVE_ACK);
    EXPECT_EQ(bobusang_channel_state(manager, 0u), eBBSAS_DISAPPEAR_DELAYED);
}

TEST(BobusangAgentDeveloper, LeaveInvalidChannelUsesLegacy99Error) {
    auto manager = configured();
    const auto reply = developer_bobusang_leave(manager, 99u);
    EXPECT_EQ(reply.protocol, CHEAT_BOBUSANG_LEAVE_NACK);
    EXPECT_EQ(reply.data, 99u);
}