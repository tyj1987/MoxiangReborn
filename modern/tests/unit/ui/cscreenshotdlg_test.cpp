// mxh/tests/unit/ui/cscreenshotdlg_test.cpp
//
// Unit tests for mxh::ui::cScreenShotDlg (Phase C dialog port).
//
// Locks down the 1:1 surface:
//   * eDelayTime == 2000 (legacy sentinel)
//   * Linking() captures cStatic pointers from the test hook
//   * SetActive(true, heroId) flips m_bShow + stamps m_dwDelayTime
//     = nowMs + 2000 + populates both cStatic texts
//   * SetActive(false) does not touch m_bShow / texts
//   * ViewDatetime formats the server time + play time strings
//   * Render() auto-closes the dialog after the 2-second window
//   * Render() extends the show window if m_dwDelayTime is in the
//     future (the legacy "rapid screenshot" trick)
//   * SetServerTi stores the SYSTEMTIME / std::tm

#include "mxh/ui/cscreenshotdlg.hpp"
#include "mxh/ui/cstatic.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <ctime>
#include <string>

using mxh::ui::cScreenShotDlg;
using mxh::ui::cStatic;
using mxh::ui::kScreenShotDelayMs;

namespace {
struct Harness {
    cScreenShotDlg dlg;
    cStatic stTime;
    cStatic playTime;

    Harness() {
        cScreenShotDlg::ChildWindows w{};
        w.stTime   = &stTime;
        w.playTime = &playTime;
        dlg.SetChildWindowsForTest(w);
    }
};
}  // namespace

TEST(CScreenShotDlg, DelayConstantIsTwoThousand) {
    // 1:1 with legacy eDelayTime = 2000.
    EXPECT_EQ(kScreenShotDelayMs, 2000);
}

TEST(CScreenShotDlg, DefaultConstructionIsHidden) {
    cScreenShotDlg d;
    EXPECT_FALSE(d.isShow());
    EXPECT_EQ(d.delayTime(), 0u);
    EXPECT_EQ(d.stTime(), nullptr);
    EXPECT_EQ(d.playTime(), nullptr);
}

TEST(CScreenShotDlg, LinkingCapturesChildWindows) {
    Harness h;
    h.dlg.Linking();
    EXPECT_EQ(h.dlg.stTime(), &h.stTime);
    EXPECT_EQ(h.dlg.playTime(), &h.playTime);
}

TEST(CScreenShotDlg, SetActiveTrueFlipsShowAndStampsDelay) {
    Harness h;
    h.dlg.Linking();
    h.dlg.SetNowMsForTest(10000u);
    h.dlg.SetActive(true, /*heroId=*/42);
    EXPECT_TRUE(h.dlg.isActive());
    EXPECT_TRUE(h.dlg.isShow());
    EXPECT_EQ(h.dlg.delayTime(), 10000u + 2000u);
}

TEST(CScreenShotDlg, SetActiveFalseDoesNotFlipShow) {
    Harness h;
    h.dlg.Linking();
    h.dlg.SetNowMsForTest(10000u);
    h.dlg.SetActive(true, 1);
    h.dlg.SetActive(false, 1);
    EXPECT_FALSE(h.dlg.isActive());
    // SetActive(false) does not touch m_bShow; that's Render()'s job.
    EXPECT_TRUE(h.dlg.isShow());
}

TEST(CScreenShotDlg, ViewDatetimeFormatsServerTimeString) {
    Harness h;
    h.dlg.Linking();
    h.dlg.SetServerNameForTest("MainSrv");
    std::tm t{};
    t.tm_year = 124;       // 2024
    t.tm_mon  = 6;         // July
    t.tm_mday = 15;
    t.tm_hour = 14;
    t.tm_min  = 30;
    t.tm_sec  = 45;
    h.dlg.SetServerTi(t);
    // m_nowTime is MILLISECONDS within the day (legacy
    // MHTIMEMGR->GetMHTime() returns ms).  3h25m45s = 12345000 ms.
    h.dlg.SetNowForTest(/*date=*/1000, /*time=*/12345000);
    h.dlg.SetClientLoginForTest(/*date=*/1000, /*time=*/0);
    h.dlg.SetNowMsForTest(200000u);
    h.dlg.ViewDatetime(7);
    // 1:1 with legacy: "[server] YYYY-MM-DD HH:MM:SS"
    EXPECT_EQ(h.stTime.GetStaticText(), "[MainSrv] 2024-07-15 14:30:45");
    EXPECT_EQ(h.playTime.GetStaticText(), "[PLAYTIME]  0 Day 03:25:45");
}

TEST(CScreenShotDlg, ViewDatetimeComputesMultiDayPlayTime) {
    Harness h;
    h.dlg.Linking();
    h.dlg.SetServerNameForTest("S");
    std::tm t{};
    t.tm_year = 100; t.tm_mon = 0; t.tm_mday = 1;
    t.tm_hour = 0; t.tm_min = 0; t.tm_sec = 0;
    h.dlg.SetServerTi(t);
    // 2 days 5 hours 6 minutes 7 seconds elapsed
    // = 2*86400 + 5*3600 + 6*60 + 7 = 191167 sec
    // = 191167 * 1000 = 191167000 ms.
    // Legacy arithmetic: totalCurrent = date*MS_PER_DAY + time,
    // where MS_PER_DAY = 1000*60*60*24 and `time` is milliseconds.
    h.dlg.SetNowForTest(/*date=*/1000, /*time=*/191167000);
    h.dlg.SetClientLoginForTest(/*date=*/1000, /*time=*/0);
    h.dlg.SetNowMsForTest(100u);
    h.dlg.ViewDatetime(0);
    EXPECT_EQ(h.playTime.GetStaticText(), "[PLAYTIME]  2 Day 05:06:07");
}

TEST(CScreenShotDlg, RenderExtendsShowWindowWhileDelayInFuture) {
    Harness h;
    h.dlg.Linking();
    h.dlg.SetNowMsForTest(10000u);
    h.dlg.SetActive(true, 1);
    EXPECT_TRUE(h.dlg.isShow());
    // Render() called before delay expires keeps m_bShow true.
    h.dlg.SetNowMsForTest(11500u);   // 1.5s in
    h.dlg.Render();
    EXPECT_TRUE(h.dlg.isShow());
    EXPECT_TRUE(h.dlg.isActive());
}

TEST(CScreenShotDlg, RenderAutoClosesOnDelayExpiry) {
    Harness h;
    h.dlg.Linking();
    h.dlg.SetNowMsForTest(10000u);
    h.dlg.SetActive(true, 1);
    // Advance past m_dwDelayTime = 10000 + 2000.
    h.dlg.SetNowMsForTest(12500u);
    h.dlg.Render();
    EXPECT_FALSE(h.dlg.isShow());
    EXPECT_FALSE(h.dlg.isActive());
    EXPECT_EQ(h.dlg.delayTime(), 0u);
}

TEST(CScreenShotDlg, SetServerTiStoresTm) {
    Harness h;
    std::tm t{};
    t.tm_year = 80;
    t.tm_mon  = 5;
    t.tm_mday = 7;
    t.tm_hour = 11;
    t.tm_min  = 22;
    t.tm_sec  = 33;
    h.dlg.SetServerTi(t);
    EXPECT_EQ(h.dlg.serverTime().tm_year, 80);
    EXPECT_EQ(h.dlg.serverTime().tm_mon,  5);
    EXPECT_EQ(h.dlg.serverTime().tm_mday, 7);
    EXPECT_EQ(h.dlg.serverTime().tm_hour, 11);
    EXPECT_EQ(h.dlg.serverTime().tm_min,  22);
    EXPECT_EQ(h.dlg.serverTime().tm_sec,  33);
}

TEST(CScreenShotDlg, ViewDatetimeWithZeroLoginAndCurrentShowsZeroPlayTime) {
    Harness h;
    h.dlg.Linking();
    h.dlg.SetServerNameForTest("S");
    std::tm t{};
    t.tm_year = 100;
    h.dlg.SetServerTi(t);
    h.dlg.SetNowForTest(0, 0);
    h.dlg.SetClientLoginForTest(0, 0);
    h.dlg.SetNowMsForTest(0);
    h.dlg.ViewDatetime(0);
    EXPECT_EQ(h.playTime.GetStaticText(), "[PLAYTIME]  0 Day 00:00:00");
}

TEST(CScreenShotDlg, ViewDatetimeWithLinkMissingStampsShow) {
    // 1:1 quirk: ViewDatetime should stamp m_bShow + m_dwDelayTime
    // even when the cStatic pointers are missing (the legacy
    // would crash, but the modern port tolerates it).
    cScreenShotDlg d;
    d.Linking();   // no children injected
    d.SetServerNameForTest("S");
    std::tm t{};
    t.tm_year = 100;
    d.SetServerTi(t);
    d.SetNowMsForTest(1000u);
    d.ViewDatetime(0);
    EXPECT_TRUE(d.isShow());
    EXPECT_EQ(d.delayTime(), 1000u + 2000u);
}
