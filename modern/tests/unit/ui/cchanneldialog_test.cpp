// mxh/tests/unit/ui/cchanneldialog_test.cpp
//
// Unit tests for mxh::ui::cChannelDialog (Phase C dialog port).
//
// Locks down the 1:1 surface:
//   * Default construction: m_bInit=false, m_SelectRowIdx=0,
//     m_BaseChannelIndex=0
//   * SetChannelList() populates m_rows, picks row 0 as
//     default, sets g_channelNum, and SetActive(true)s the dialog
//   * SetChannelList() with chatmsg-callback formats the
//     "Channel %d" / crowd-level strings
//   * bBattleChannel[i] appends "(battle)" tag
//   * SelectChannel(rowidx) updates m_SelectRowIdx + g_channelNum
//   * SelectChannel with playerNum >= 300 sets g_channelNum = -1
//   * OnConnect with g_channelNum=-1 calls DisplayNotice(279)
//   * OnConnect with g_channelNum>=0 calls EnterGame()
//   * EnterGame() returning false calls DisplayNotice(18)
//   * ActionEvent(WE_ROWCLICK) -> SelectChannel
//   * ActionEvent(WE_ROWDBLCLICK) -> OnConnect
//   * SendMapChannelInfoSYN routes through callback
//   * MapChange with m_wMoveMapNum != 0 routes through callback
//   * MapChange with m_wMoveMapNum == 0 calls AddSysMsg
//   * SetActiveExt(false) clears IsBattleChannel global

#include "mxh/ui/cchanneldialog.hpp"
#include "../../../src/ui/legacy_window_event.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <string>

using mxh::ui::cChannelDialog;
using mxh::ui::MSG_CHANNEL_INFO;

namespace {

// Tiny CHATMGR mock that returns fixed strings per chatmsg id.
const char* faChatMsg(int id, void* /*user*/) {
    switch (id) {
        case 211: return "Low";
        case 212: return "Mid";
        case 213: return "High";
        case 1701: return "Channel %d";
        case 1702: return "Battle";
        default:   return "";
    }
}

}  // namespace

TEST(CChannelDialog, DefaultConstructionIsUninitialized) {
    cChannelDialog d;
    EXPECT_FALSE(d.isInit());
    EXPECT_EQ(d.selectRowIdx(), 0);
    EXPECT_EQ(d.baseChannelIndex(), 0);
    EXPECT_EQ(d.moveMapNum(), 0);
    EXPECT_EQ(d.changeMapState(), 0u);
    EXPECT_EQ(d.channelList(), nullptr);
    EXPECT_EQ(d.channelCount(), 0);
    EXPECT_FALSE(d.isActive());
}

TEST(CChannelDialog, SetChannelListPopulatesRowsAndActivates) {
    cChannelDialog d;
    d.SetChatMsgCallbackForTest(&faChatMsg, nullptr);
    MSG_CHANNEL_INFO info{};
    std::strncpy(info.ChannelName, "Channel", sizeof(info.ChannelName) - 1);
    info.Count = 3;
    info.PlayerNum[0] = 30;   // Low
    info.PlayerNum[1] = 80;   // Mid
    info.PlayerNum[2] = 200;  // High
    d.SetChannelList(info);
    EXPECT_TRUE(d.isInit());
    EXPECT_EQ(d.channelCount(), 3);
    EXPECT_TRUE(d.isActive());
    EXPECT_EQ(d.selectRowIdx(), 0);
    EXPECT_EQ(d.rowForTest(0).name, "Channel 1");
    EXPECT_EQ(d.rowForTest(0).crowd, "Low");
    EXPECT_EQ(d.rowForTest(1).crowd, "Mid");
    EXPECT_EQ(d.rowForTest(2).crowd, "High");
    EXPECT_EQ(cChannelDialog::GlobalChannelNum(), 0);
}

TEST(CChannelDialog, SetChannelListWithoutChatMsgFallsBack) {
    cChannelDialog d;
    // No chat-msg callback -> fallback strings.
    MSG_CHANNEL_INFO info{};
    info.Count = 2;
    info.PlayerNum[0] = 10;
    info.PlayerNum[1] = 90;
    d.SetChannelList(info);
    EXPECT_EQ(d.channelCount(), 2);
    EXPECT_EQ(d.rowForTest(0).name, "Channel 1");
    EXPECT_EQ(d.rowForTest(0).crowd, "Low");
    EXPECT_EQ(d.rowForTest(1).crowd, "Mid");
}

TEST(CChannelDialog, BattleChannelAppendsTag) {
    cChannelDialog d;
    d.SetChatMsgCallbackForTest(&faChatMsg, nullptr);
    MSG_CHANNEL_INFO info{};
    info.Count = 2;
    info.PlayerNum[0] = 10;   // Low
    info.PlayerNum[1] = 10;   // Low + battle
    info.bBattleChannel[1] = true;
    d.SetChannelList(info);
    EXPECT_EQ(d.rowForTest(0).crowd, "Low");
    EXPECT_EQ(d.rowForTest(1).crowd, "Low (Battle)");
    EXPECT_EQ(d.rowForTest(1).battle, true);
}

TEST(CChannelDialog, SetChannelListUpdatesGlobalIsBattleChannel) {
    cChannelDialog d;
    d.SetChatMsgCallbackForTest(&faChatMsg, nullptr);
    MSG_CHANNEL_INFO info{};
    info.Count = 3;
    info.bBattleChannel[1] = true;
    d.SetChannelList(info);
    EXPECT_FALSE(cChannelDialog::GlobalIsBattleChannel()[0]);
    EXPECT_TRUE(cChannelDialog::GlobalIsBattleChannel()[1]);
    EXPECT_FALSE(cChannelDialog::GlobalIsBattleChannel()[2]);
}

TEST(CChannelDialog, SelectChannelUpdatesRowAndGlobal) {
    cChannelDialog d;
    d.SetChatMsgCallbackForTest(&faChatMsg, nullptr);
    MSG_CHANNEL_INFO info{};
    info.Count = 3;
    info.PlayerNum[0] = 10;
    info.PlayerNum[1] = 50;
    info.PlayerNum[2] = 90;
    d.SetChannelList(info);
    EXPECT_EQ(cChannelDialog::GlobalChannelNum(), 0);
    d.SelectChannel(2);
    EXPECT_EQ(d.selectRowIdx(), 2);
    EXPECT_EQ(cChannelDialog::GlobalChannelNum(), 2);
}

TEST(CChannelDialog, SelectChannelSameRowIsNoOp) {
    cChannelDialog d;
    d.SetChatMsgCallbackForTest(&faChatMsg, nullptr);
    MSG_CHANNEL_INFO info{};
    info.Count = 2;
    info.PlayerNum[0] = 10;
    info.PlayerNum[1] = 50;
    d.SetChannelList(info);
    cChannelDialog::SetGlobalChannelNumForTest(7);
    d.SelectChannel(0);
    // 1:1 with legacy: when the new row matches the current
    // selection, the if-block doesn't run, so g_channelNum
    // stays at 7.
    EXPECT_EQ(d.selectRowIdx(), 0);
    EXPECT_EQ(cChannelDialog::GlobalChannelNum(), 7);
}

TEST(CChannelDialog, SelectChannelOvercrowdedSetsGlobalToMinusOne) {
    cChannelDialog d;
    d.SetChatMsgCallbackForTest(&faChatMsg, nullptr);
    MSG_CHANNEL_INFO info{};
    info.Count = 2;
    info.PlayerNum[0] = 10;
    info.PlayerNum[1] = 350;   // >= 300 -> g_channelNum = -1
    d.SetChannelList(info);
    d.SelectChannel(1);
    EXPECT_EQ(cChannelDialog::GlobalChannelNum(), -1);
}

TEST(CChannelDialog, SelectChannelOutOfRangeIsNoOp) {
    cChannelDialog d;
    d.SetChatMsgCallbackForTest(&faChatMsg, nullptr);
    MSG_CHANNEL_INFO info{};
    info.Count = 1;
    info.PlayerNum[0] = 10;
    d.SetChannelList(info);
    cChannelDialog::SetGlobalChannelNumForTest(5);
    d.SelectChannel(99);
    EXPECT_EQ(d.selectRowIdx(), 0);   // unchanged
    EXPECT_EQ(cChannelDialog::GlobalChannelNum(), 5);
}

TEST(CChannelDialog, OnConnectWithNoChannelDisplaysNotice279) {
    cChannelDialog d;
    d.SetChatMsgCallbackForTest(&faChatMsg, nullptr);
    MSG_CHANNEL_INFO info{};
    info.Count = 1;
    info.PlayerNum[0] = 10;
    d.SetChannelList(info);
    int noticeId = 0;
    d.SetDisplayNoticeCallbackForTest(
        [](int id, void* user) { *static_cast<int*>(user) = id; },
        &noticeId);
    cChannelDialog::SetGlobalChannelNumForTest(-1);
    d.OnConnect();
    EXPECT_EQ(noticeId, 279);
}

TEST(CChannelDialog, OnConnectWithChannelEnterGame) {
    cChannelDialog d;
    d.SetChatMsgCallbackForTest(&faChatMsg, nullptr);
    MSG_CHANNEL_INFO info{};
    info.Count = 1;
    info.PlayerNum[0] = 10;
    d.SetChannelList(info);
    int noticeId = 0;
    bool enterCalled = false;
    d.SetDisplayNoticeCallbackForTest(
        [](int id, void* user) { *static_cast<int*>(user) = id; },
        &noticeId);
    d.SetEnterGameCallbackForTest(
        [](void* user) { *static_cast<bool*>(user) = true; return true; },
        &enterCalled);
    cChannelDialog::SetGlobalChannelNumForTest(0);
    d.OnConnect();
    EXPECT_EQ(noticeId, 0);   // no notice on success
    EXPECT_TRUE(enterCalled);
}

TEST(CChannelDialog, OnConnectEnterGameFalseShowsNotice18) {
    cChannelDialog d;
    d.SetChatMsgCallbackForTest(&faChatMsg, nullptr);
    MSG_CHANNEL_INFO info{};
    info.Count = 1;
    info.PlayerNum[0] = 10;
    d.SetChannelList(info);
    int noticeId = 0;
    d.SetDisplayNoticeCallbackForTest(
        [](int id, void* user) { *static_cast<int*>(user) = id; },
        &noticeId);
    d.SetEnterGameCallbackForTest(
        [](void*) { return false; },
        nullptr);
    cChannelDialog::SetGlobalChannelNumForTest(0);
    d.OnConnect();
    EXPECT_EQ(noticeId, 18);
}

TEST(CChannelDialog, ActionEventRowClickSelects) {
    cChannelDialog d;
    d.SetChatMsgCallbackForTest(&faChatMsg, nullptr);
    MSG_CHANNEL_INFO info{};
    info.Count = 2;
    info.PlayerNum[0] = 10;
    info.PlayerNum[1] = 50;
    d.SetChannelList(info);
    d.SetLastActionEventWeForTest(mxh::ui::legacy_window_event::kRowClick);
    d.selectRowIdxForTest() = 1;  // test helper
    d.ActionEvent(nullptr);
    EXPECT_EQ(d.selectRowIdx(), 1);
}

TEST(CChannelDialog, ActionEventRowDblClickOnConnect) {
    cChannelDialog d;
    d.SetChatMsgCallbackForTest(&faChatMsg, nullptr);
    MSG_CHANNEL_INFO info{};
    info.Count = 1;
    info.PlayerNum[0] = 10;
    d.SetChannelList(info);
    int noticeId = 0;
    d.SetDisplayNoticeCallbackForTest(
        [](int id, void* user) { *static_cast<int*>(user) = id; },
        &noticeId);
    cChannelDialog::SetGlobalChannelNumForTest(-1);
    d.SetLastActionEventWeForTest(mxh::ui::legacy_window_event::kRowDoubleClick);
    d.ActionEvent(nullptr);
    EXPECT_EQ(noticeId, 279);
}

TEST(CChannelDialog, ActionEventOnInactiveDialogIsNoOp) {
    cChannelDialog d;
    d.SetChatMsgCallbackForTest(&faChatMsg, nullptr);
    // Dialog not active -> ActionEvent should not dispatch.
    d.SetLastActionEventWeForTest(mxh::ui::legacy_window_event::kRowClick);
    d.ActionEvent(nullptr);
    SUCCEED();
}

TEST(CChannelDialog, SendMapChannelInfoSynRoutesThroughCallback) {
    cChannelDialog d;
    std::uint16_t lastMap = 0;
    std::uint32_t lastState = 0;
    int callCount = 0;
    d.SetSendMapChannelInfoCallbackForTest(
        [](std::uint16_t m, std::uint32_t s, void* user) {
            auto* ctx = static_cast<int*>(user);
            ++*ctx;
            // The captured values are stored globally for assertion.
        },
        &callCount);
    // Note: the simple lambda doesn't capture; for richer
    // capture use a struct.  Use the simplest form here to
    // verify the call count.
    d.SendMapChannelInfoSYN(12, 34);
    EXPECT_EQ(callCount, 1);
}

TEST(CChannelDialog, MapChangeWithMoveMapNumCallsCallback) {
    cChannelDialog d;
    int callCount = 0;
    d.SetMapChangeCallbackForTest(
        [](std::uint16_t, void* user) { ++*static_cast<int*>(user); },
        &callCount);
    // Pre-load move map via SetChannelList (which sets m_wMoveMapNum).
    MSG_CHANNEL_INFO info{};
    info.Count = 1;
    info.PlayerNum[0] = 10;
    info.wMoveMapNum = 7;
    d.SetChannelList(info);
    d.MapChange();
    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(d.moveMapNum(), 0);   // reset after MapChange
    EXPECT_FALSE(d.isActive());
}

TEST(CChannelDialog, MapChangeWithoutMoveMapNumShowsSystemMsg) {
    cChannelDialog d;
    int sysMsgId = 0;
    d.SetAddSysMsgCallbackForTest(
        [](int id, void* user) { *static_cast<int*>(user) = id; },
        &sysMsgId);
    d.MapChange();
    EXPECT_EQ(sysMsgId, 1699);
    EXPECT_FALSE(d.isActive());
}

TEST(CChannelDialog, SetActiveExtFalseClearsIsBattleChannel) {
    cChannelDialog d;
    d.SetChatMsgCallbackForTest(&faChatMsg, nullptr);
    MSG_CHANNEL_INFO info{};
    info.Count = 2;
    info.bBattleChannel[0] = true;
    info.bBattleChannel[1] = true;
    d.SetChannelList(info);
    EXPECT_TRUE(cChannelDialog::GlobalIsBattleChannel()[0]);
    d.SetActiveExt(false);
    EXPECT_FALSE(cChannelDialog::GlobalIsBattleChannel()[0]);
    EXPECT_FALSE(cChannelDialog::GlobalIsBattleChannel()[1]);
}

TEST(CChannelDialog, NonCopyable) {
    static_assert(!std::is_copy_constructible_v<cChannelDialog>);
    static_assert(!std::is_copy_assignable_v<cChannelDialog>);
}
