//
// Unit tests for mxh::ui::cMPGuageDialog (Phase C dialog port).
//
// Locks down the 1:1 surface:
//   * kIdExpGuage=610, kIdTime=611, kIdExpPercent=612, kIdTitle=613
//   * kRedTextThreshold=30000
//   * kFlagReady=0, kFlagActive=1, kFlagStopped=2
//   * kEventMapTitleChatMsgId=140
//   * Default construction: 4 child pointers null
//   * Linking preserves injected children
//   * SetExpGuage calls SetValue callback + sprintf "%4.2f%%"
//   * SetExpGuage with 0% / 100% / 50% text formatting
//   * SetTime formats mm:ss
//   * SetTime below 30000 sets red color
//   * SetTime at/above 30000 leaves color alone
//   * SetEventMapTimer(bFlag=0) sets blue
//   * SetEventMapTimer(bFlag=1) below 30000 sets red
//   * SetEventMapTimer(bFlag=1) at/above 30000 leaves color
//   * SetEventMapTimer(bFlag=2) sets blue
//   * SetEventMapTimer unknown bFlag is no-op for color
//   * ShowEventMap activates dialog + sets title via CHATMGR callback
//   * ShowEventMap without CHATMGR callback is safe
//   * SetExpGuage without SetValue callback is safe
//   * NonCopyable
//

#include "mxh/ui/cmpguagedialog.hpp"
#include "mxh/ui/cstatic.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <type_traits>

using mxh::ui::cMPGuageDialog;
using mxh::ui::cStatic;

namespace {

struct Harness {
    cMPGuageDialog dlg;
    cStatic        timeStc;
    cStatic        expPercentStc;
    cStatic        titleStc;

    Harness() {
        dlg.SetTimeStaticForTest(&timeStc);
        dlg.SetExpPercentForTest(&expPercentStc);
        dlg.SetTitleForTest(&titleStc);
    }
};

float g_lastGaugePercent = 0.0f;
std::uint32_t g_setValueCalls = 0;
void SetExpGuageCb(float percent, void* /*user*/) {
    g_lastGaugePercent = percent;
    ++g_setValueCalls;
}

const char* g_chatMsg140 = nullptr;
std::uint32_t g_chatMsgCalls = 0;
const char* ChatMsgCb(int id, void* /*user*/) {
    ++g_chatMsgCalls;
    if (id == 140) return g_chatMsg140;
    return "";
}

void ResetCbState() {
    g_lastGaugePercent = 0.0f;
    g_setValueCalls    = 0;
    g_chatMsg140       = nullptr;
    g_chatMsgCalls     = 0;
}

}  // namespace


TEST(CMPGuageDialog, ConstantsMatchLegacy) {
    EXPECT_EQ(cMPGuageDialog::kIdExpGuage,    610);
    EXPECT_EQ(cMPGuageDialog::kIdTime,        611);
    EXPECT_EQ(cMPGuageDialog::kIdExpPercent,  612);
    EXPECT_EQ(cMPGuageDialog::kIdTitle,       613);
    EXPECT_EQ(cMPGuageDialog::kRedTextThreshold, 30000u);
    EXPECT_EQ(cMPGuageDialog::kFlagReady,   0);
    EXPECT_EQ(cMPGuageDialog::kFlagActive,  1);
    EXPECT_EQ(cMPGuageDialog::kFlagStopped, 2);
    EXPECT_EQ(cMPGuageDialog::kEventMapTitleChatMsgId, 140);
}

TEST(CMPGuageDialog, DefaultConstructionHasNullChildren) {
    cMPGuageDialog d;
    EXPECT_EQ(d.GetExpGuageForTest(),    nullptr);
    EXPECT_EQ(d.GetTimeStaticForTest(),  nullptr);
    EXPECT_EQ(d.GetExpPercentForTest(),  nullptr);
    EXPECT_EQ(d.GetTitleForTest(),       nullptr);
}

TEST(CMPGuageDialog, SetTestHooksStorePointers) {
    cMPGuageDialog d;
    cStatic a, b, c;
    d.SetTimeStaticForTest(&a);
    d.SetExpPercentForTest(&b);
    d.SetTitleForTest(&c);
    EXPECT_EQ(d.GetTimeStaticForTest(),  &a);
    EXPECT_EQ(d.GetExpPercentForTest(),  &b);
    EXPECT_EQ(d.GetTitleForTest(),       &c);
}

TEST(CMPGuageDialog, LinkingPreservesInjectedChildren) {
    Harness h;
    h.dlg.Linking();
    EXPECT_EQ(h.dlg.GetTimeStaticForTest(),  &h.timeStc);
    EXPECT_EQ(h.dlg.GetExpPercentForTest(),  &h.expPercentStc);
    EXPECT_EQ(h.dlg.GetTitleForTest(),       &h.titleStc);
}

TEST(CMPGuageDialog, LinkingBeforeInitDoesNotCrash) {
    cMPGuageDialog d;
    d.Linking();
    SUCCEED();
}


TEST(CMPGuageDialog, SetExpGuageCallsSetValueCallback) {
    Harness h;
    ResetCbState();
    h.dlg.SetExpGuageCallbackForTest(&SetExpGuageCb, nullptr);
    h.dlg.SetExpGuage(0.5f);
    EXPECT_EQ(g_setValueCalls, 1u);
    EXPECT_FLOAT_EQ(g_lastGaugePercent, 0.5f);
    EXPECT_EQ(h.expPercentStc.GetStaticText(), "50.00%");
}

TEST(CMPGuageDialog, SetExpGuageZeroPercent) {
    Harness h;
    ResetCbState();
    h.dlg.SetExpGuageCallbackForTest(&SetExpGuageCb, nullptr);
    h.dlg.SetExpGuage(0.0f);
    EXPECT_EQ(h.expPercentStc.GetStaticText(), "0.00%");
}

TEST(CMPGuageDialog, SetExpGuageFullPercent) {
    Harness h;
    ResetCbState();
    h.dlg.SetExpGuageCallbackForTest(&SetExpGuageCb, nullptr);
    h.dlg.SetExpGuage(1.0f);
    EXPECT_EQ(h.expPercentStc.GetStaticText(), "100.00%");
}

TEST(CMPGuageDialog, SetExpGuageWithoutCallbackIsSafe) {
    Harness h;
    ResetCbState();
    h.dlg.SetExpGuage(0.42f);
    EXPECT_EQ(h.expPercentStc.GetStaticText(), "42.00%");
}

TEST(CMPGuageDialog, SetExpGuageWithoutExpPercentIsSafe) {
    cMPGuageDialog d;
    d.SetExpGuageCallbackForTest(&SetExpGuageCb, nullptr);
    d.SetExpGuage(0.5f);
    EXPECT_EQ(g_setValueCalls, 1u);
}


TEST(CMPGuageDialog, SetTimeFormatsOneMinute) {
    Harness h;
    h.dlg.SetTime(60000);
    EXPECT_EQ(h.timeStc.GetStaticText(), "01:00");
}

TEST(CMPGuageDialog, SetTimeFormatsThirtySeconds) {
    Harness h;
    h.dlg.SetTime(30000);
    EXPECT_EQ(h.timeStc.GetStaticText(), "00:30");
}

TEST(CMPGuageDialog, SetTimeFormatsMixed) {
    Harness h;
    h.dlg.SetTime(125000);
    EXPECT_EQ(h.timeStc.GetStaticText(), "02:05");
}

TEST(CMPGuageDialog, SetTimeBelowThresholdSetsRedColor) {
    Harness h;
    h.dlg.SetTime(29999);
    EXPECT_EQ(h.timeStc.GetStaticText(), "00:29");
    EXPECT_EQ(h.timeStc.GetFGColor(), 0xFFFF0000u);
}

TEST(CMPGuageDialog, SetTimeAtThresholdDoesNotSetRed) {
    Harness h;
    const std::uint32_t beforeColor = h.timeStc.GetFGColor();
    h.dlg.SetTime(30000);
    EXPECT_EQ(h.timeStc.GetStaticText(), "00:30");
    EXPECT_EQ(h.timeStc.GetFGColor(), beforeColor);
}

TEST(CMPGuageDialog, SetTimeAboveThresholdDoesNotSetRed) {
    Harness h;
    const std::uint32_t beforeColor = h.timeStc.GetFGColor();
    h.dlg.SetTime(60000);
    EXPECT_EQ(h.timeStc.GetStaticText(), "01:00");
    EXPECT_EQ(h.timeStc.GetFGColor(), beforeColor);
}

TEST(CMPGuageDialog, SetTimeWithoutLinkingIsSafe) {
    cMPGuageDialog d;
    d.SetTime(60000);
    SUCCEED();
}


TEST(CMPGuageDialog, SetEventMapTimerReadySetsBlue) {
    Harness h;
    h.dlg.SetEventMapTimer(60000, cMPGuageDialog::kFlagReady);
    EXPECT_EQ(h.timeStc.GetStaticText(), "01:00");
    EXPECT_EQ(h.timeStc.GetFGColor(), 0xFF0000FFu);
}

TEST(CMPGuageDialog, SetEventMapTimerStoppedSetsBlue) {
    Harness h;
    h.dlg.SetEventMapTimer(60000, cMPGuageDialog::kFlagStopped);
    EXPECT_EQ(h.timeStc.GetStaticText(), "01:00");
    EXPECT_EQ(h.timeStc.GetFGColor(), 0xFF0000FFu);
}

TEST(CMPGuageDialog, SetEventMapTimerActiveBelowThresholdSetsRed) {
    Harness h;
    h.dlg.SetEventMapTimer(29999, cMPGuageDialog::kFlagActive);
    EXPECT_EQ(h.timeStc.GetStaticText(), "00:29");
    EXPECT_EQ(h.timeStc.GetFGColor(), 0xFFFF0000u);
}

TEST(CMPGuageDialog, SetEventMapTimerActiveAtThresholdKeepsColor) {
    Harness h;
    const std::uint32_t beforeColor = h.timeStc.GetFGColor();
    h.dlg.SetEventMapTimer(30000, cMPGuageDialog::kFlagActive);
    EXPECT_EQ(h.timeStc.GetStaticText(), "00:30");
    EXPECT_EQ(h.timeStc.GetFGColor(), beforeColor);
}

TEST(CMPGuageDialog, SetEventMapTimerActiveAboveThresholdKeepsColor) {
    Harness h;
    const std::uint32_t beforeColor = h.timeStc.GetFGColor();
    h.dlg.SetEventMapTimer(60000, cMPGuageDialog::kFlagActive);
    EXPECT_EQ(h.timeStc.GetStaticText(), "01:00");
    EXPECT_EQ(h.timeStc.GetFGColor(), beforeColor);
}

TEST(CMPGuageDialog, SetEventMapTimerUnknownFlagDoesNotSetColor) {
    Harness h;
    const std::uint32_t beforeColor = h.timeStc.GetFGColor();
    h.dlg.SetEventMapTimer(60000, /*bFlag=*/255);
    EXPECT_EQ(h.timeStc.GetStaticText(), "01:00");
    EXPECT_EQ(h.timeStc.GetFGColor(), beforeColor);
}

TEST(CMPGuageDialog, SetEventMapTimerWithoutLinkingIsSafe) {
    cMPGuageDialog d;
    d.SetEventMapTimer(60000, 0);
    SUCCEED();
}


TEST(CMPGuageDialog, ShowEventMapActivatesAndSetsTitle) {
    Harness h;
    ResetCbState();
    g_chatMsg140 = "EVENT_MAP_TITLE";
    h.dlg.SetChatMsgCallbackForTest(&ChatMsgCb, nullptr);
    h.dlg.ShowEventMap();
    EXPECT_TRUE(h.dlg.isActive());
    EXPECT_EQ(g_chatMsgCalls, 1u);
    EXPECT_EQ(h.titleStc.GetStaticText(), "EVENT_MAP_TITLE");
}

TEST(CMPGuageDialog, ShowEventMapWithoutChatMsgCallbackIsSafe) {
    Harness h;
    h.dlg.ShowEventMap();
    EXPECT_TRUE(h.dlg.isActive());
    EXPECT_EQ(h.titleStc.GetStaticText(), "");
}

TEST(CMPGuageDialog, ShowEventMapWithoutLinkingIsSafe) {
    cMPGuageDialog d;
    d.SetChatMsgCallbackForTest(&ChatMsgCb, nullptr);
    d.ShowEventMap();
    EXPECT_TRUE(d.isActive());
}


TEST(CMPGuageDialog, NonCopyable) {
    static_assert(!std::is_copy_constructible<cMPGuageDialog>::value,
                  "cMPGuageDialog must not be copyable");
    static_assert(!std::is_copy_assignable<cMPGuageDialog>::value,
                  "cMPGuageDialog must not be copy-assignable");
    SUCCEED();
}

TEST(CMPGuageDialog, IscDialog) {
    static_assert(std::is_base_of<mxh::ui::cDialog,
                                  cMPGuageDialog>::value,
                  "cMPGuageDialog must inherit from cDialog");
    SUCCEED();
}
