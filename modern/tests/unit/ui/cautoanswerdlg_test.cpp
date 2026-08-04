// mxh/tests/unit/ui/cautoanswerdlg_test.cpp
//
// Unit tests for mxh::ui::cAutoAnswerDlg (Phase C dialog port).
//
// Locks down the 1:1 surface:
//   * kAutoAnswerButtonCount = 4 (legacy ASD_BTN_COLOR1..4)
//   * Linking() captures button positions + resets answer state
//   * SetActive(true) stamps 120s timer
//   * SetActiveWithTime stamps a custom timer
//   * OnActionEvent(lId, _, WE_BTNCLICK) records the answer
//     index, advances the answer cursor, fires the answer
//     callback once all 4 are in
//   * SetQuestion() / Retry() reset the answer cursor
//   * Render() auto-closes the dialog when the timer expires
//   * Shuffle() jitters the dialog Y by the supplied delta

#include "mxh/ui/cautoanswerdlg.hpp"
#include "../../../src/ui/legacy_window_event.hpp"
#include "mxh/ui/cstatic.hpp"
#include "mxh/ui/ctextarea.hpp"
#include "mxh/ui/cbutton.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <string>

using mxh::ui::cAutoAnswerDlg;
using mxh::ui::cButton;
using mxh::ui::cStatic;
using mxh::ui::cTextArea;

namespace {

struct Harness {
    cAutoAnswerDlg dlg;
    cTextArea textAreaDesc;
    cStatic   stcQuestion, stcAnswer, stcTime;
    cButton   btnColor[4];

    Harness() {
        cAutoAnswerDlg::ChildWindows w{};
        w.textAreaDesc = &textAreaDesc;
        w.stcQuestion  = &stcQuestion;
        w.stcAnswer    = &stcAnswer;
        w.stcTime      = &stcTime;
        for (std::int32_t i = 0; i < 4; ++i) w.btnColor[i] = &btnColor[i];
        dlg.SetChildWindowsForTest(w);
    }
};

}  // namespace

TEST(CAutoAnswerDlg, ConstantsMatchLegacy) {
    EXPECT_EQ(mxh::ui::kAutoAnswerButtonCount, 4);
    EXPECT_EQ(mxh::ui::kAutoAnswerFirstButtonId, 1695);
    EXPECT_EQ(mxh::ui::kAutoAnswerLastButtonId, 1698);
}

TEST(CAutoAnswerDlg, DefaultConstructionIsIdle) {
    cAutoAnswerDlg d;
    EXPECT_FALSE(d.IsAnswerStart());
    EXPECT_EQ(d.GetAnswerPos(), 0);
    EXPECT_EQ(d.GetEndTime(), 0u);
    for (std::int32_t i = 0; i < 4; ++i) {
        EXPECT_EQ(d.GetAnswer(i), 0u);
    }
}

TEST(CAutoAnswerDlg, LinkingResetsStateAndCapturesButtonPositions) {
    Harness h;
    // Pre-set a non-default state to verify Linking() resets.
    h.dlg.SetActiveWithTime(true, 5);
    h.dlg.Linking();
    EXPECT_FALSE(h.dlg.IsAnswerStart());
    EXPECT_EQ(h.dlg.GetAnswerPos(), 0);
    EXPECT_EQ(h.dlg.GetEndTime(), 0u);
}

TEST(CAutoAnswerDlg, SetActiveTrueStampsHundredAndTwentySecondTimer) {
    Harness h;
    const std::uint64_t now = 100000u;
    h.dlg.SetNowMsForTest(now);
    h.dlg.SetActive(true);
    // 1:1 with legacy: m_dwEndTime = gCurTime + 120 * 1000
    EXPECT_EQ(h.dlg.GetEndTime(), now + 120u * 1000u);
}

TEST(CAutoAnswerDlg, SetActiveWithTimeStampsCustomTimer) {
    Harness h;
    const std::uint64_t now = 5000u;
    h.dlg.SetNowMsForTest(now);
    h.dlg.SetActiveWithTime(true, /*dwTime=*/7);
    EXPECT_EQ(h.dlg.GetEndTime(), now + 7u * 1000u);
}

TEST(CAutoAnswerDlg, OnActionEventRecordsAnswerAndAdvancesCursor) {
    Harness h;
    h.dlg.Linking();
    h.dlg.SetQuestion("Q1");
    EXPECT_TRUE(h.dlg.IsAnswerStart());
    EXPECT_EQ(h.dlg.GetAnswerPos(), 0);
    h.dlg.OnActionEvent(/*lId=*/mxh::ui::kAutoAnswerFirstButtonId, /*p=*/nullptr, /*we=*/mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(h.dlg.GetAnswerPos(), 1);
    EXPECT_EQ(h.dlg.GetAnswer(0), 0u);
    h.dlg.OnActionEvent(mxh::ui::kAutoAnswerFirstButtonId + 2, nullptr, mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(h.dlg.GetAnswer(1), 2u);
    EXPECT_EQ(h.dlg.GetAnswerPos(), 2);
}

TEST(CAutoAnswerDlg, OnActionEventFiresAnswerCallbackAtFourthTap) {
    Harness h;
    h.dlg.Linking();
    h.dlg.SetQuestion("Q");
    int callCount = 0;
    std::uint32_t last0 = 99u, last1 = 99u, last2 = 99u, last3 = 99u;
    h.dlg.SetOnAnswer([&](std::uint32_t a0, std::uint32_t a1,
                            std::uint32_t a2, std::uint32_t a3) {
        ++callCount;
        last0 = a0; last1 = a1; last2 = a2; last3 = a3;
    });
    h.dlg.OnActionEvent(mxh::ui::kAutoAnswerFirstButtonId + 1, nullptr, mxh::ui::legacy_window_event::kButtonClick);
    h.dlg.OnActionEvent(mxh::ui::kAutoAnswerFirstButtonId + 3, nullptr, mxh::ui::legacy_window_event::kButtonClick);
    h.dlg.OnActionEvent(mxh::ui::kAutoAnswerFirstButtonId, nullptr, mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(callCount, 0);  // 3 taps -> not yet
    h.dlg.OnActionEvent(mxh::ui::kAutoAnswerFirstButtonId + 2, nullptr, mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(last0, 1u);
    EXPECT_EQ(last1, 3u);
    EXPECT_EQ(last2, 0u);
    EXPECT_EQ(last3, 2u);
    // 1:1 with legacy: m_bAnswerStart = FALSE after the 4th tap.
    EXPECT_FALSE(h.dlg.IsAnswerStart());
}

TEST(CAutoAnswerDlg, OnActionEventIgnoresNonClickEvents) {
    Harness h;
    h.dlg.Linking();
    h.dlg.SetQuestion("Q");
    h.dlg.OnActionEvent(mxh::ui::kAutoAnswerFirstButtonId, nullptr, mxh::ui::legacy_window_event::kPushUp);
    EXPECT_EQ(h.dlg.GetAnswerPos(), 0);
    EXPECT_TRUE(h.dlg.IsAnswerStart());
}

TEST(CAutoAnswerDlg, OnActionEventIgnoresOutOfRangeButtonId) {
    Harness h;
    h.dlg.Linking();
    h.dlg.SetQuestion("Q");
    h.dlg.OnActionEvent(mxh::ui::kAutoAnswerLastButtonId + 1, nullptr, mxh::ui::legacy_window_event::kButtonClick);   // above ASD_BTN_COLOR4
    h.dlg.OnActionEvent(mxh::ui::kAutoAnswerFirstButtonId - 1, nullptr, mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(h.dlg.GetAnswerPos(), 0);
}

TEST(CAutoAnswerDlg, OnActionEventStopsAtFifthTap) {
    Harness h;
    h.dlg.Linking();
    h.dlg.SetQuestion("Q");
    int callCount = 0;
    h.dlg.SetOnAnswer([&](std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) {
        ++callCount;
    });
    for (std::int32_t i = 0; i < 5; ++i) {
        h.dlg.OnActionEvent(mxh::ui::kAutoAnswerFirstButtonId + i % 4, nullptr, mxh::ui::legacy_window_event::kButtonClick);
    }
    EXPECT_EQ(callCount, 1);   // only fires on the 4th tap
}

TEST(CAutoAnswerDlg, SetQuestionSetsQuestionAndResetsAnswer) {
    Harness h;
    h.dlg.Linking();
    h.dlg.SetQuestion("2 + 2 = ?");
    EXPECT_EQ(h.stcQuestion.GetStaticText(), "2 + 2 = ?");
    EXPECT_TRUE(h.dlg.IsAnswerStart());
    EXPECT_EQ(h.dlg.GetAnswerPos(), 0);
    EXPECT_EQ(h.stcAnswer.GetStaticText(), "");
}

TEST(CAutoAnswerDlg, RetryDoesNotTouchQuestionStatic) {
    Harness h;
    h.dlg.Linking();
    h.dlg.SetQuestion("Q");
    h.stcQuestion.SetStaticText("2 + 2 = ?");
    h.dlg.Retry();
    // Retry must NOT overwrite the question static.
    EXPECT_EQ(h.stcQuestion.GetStaticText(), "2 + 2 = ?");
    EXPECT_TRUE(h.dlg.IsAnswerStart());
    EXPECT_EQ(h.dlg.GetAnswerPos(), 0);
}

TEST(CAutoAnswerDlg, AnswerStaticAccumulatesStars) {
    Harness h;
    h.dlg.Linking();
    h.dlg.SetQuestion("Q");
    h.dlg.OnActionEvent(mxh::ui::kAutoAnswerFirstButtonId, nullptr, mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(h.stcAnswer.GetStaticText(), " * ");
    h.dlg.OnActionEvent(mxh::ui::kAutoAnswerFirstButtonId + 1, nullptr, mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(h.stcAnswer.GetStaticText(), " *  * ");
}

TEST(CAutoAnswerDlg, RenderAutoClosesOnTimerExpiry) {
    Harness h;
    h.dlg.Linking();
    const std::uint64_t now = 10000u;
    h.dlg.SetNowMsForTest(now);
    h.dlg.SetActiveWithTime(true, /*dwTime=*/1);
    EXPECT_TRUE(h.dlg.isActive());
    // Advance the test clock past m_dwEndTime.
    h.dlg.SetNowMsForTest(now + 2000u);
    h.dlg.Render();
    EXPECT_FALSE(h.dlg.isActive());
    EXPECT_EQ(h.dlg.GetEndTime(), 0u);
}

TEST(CAutoAnswerDlg, RenderKeepsDialogOpenWhileTimerNotExpired) {
    Harness h;
    h.dlg.Linking();
    h.dlg.SetNowMsForTest(1000u);
    h.dlg.SetActiveWithTime(true, 60);
    h.dlg.SetNowMsForTest(2000u);   // 1s elapsed of 60s budget
    h.dlg.Render();
    EXPECT_TRUE(h.dlg.isActive());
    EXPECT_NE(h.dlg.GetEndTime(), 0u);
}

TEST(CAutoAnswerDlg, ShuffleJittersAbsY) {
    cAutoAnswerDlg d;
    d.SetAbsXY(100, 200);
    d.Shuffle(/*randY=*/-30);
    EXPECT_EQ(d.absX(), 100);
    EXPECT_EQ(d.absY(), 170);
    d.Shuffle(40);
    EXPECT_EQ(d.absY(), 210);
}

TEST(CAutoAnswerDlg, SaveImageDelegatesToCallback) {
    Harness h;
    int callCount = 0;
    std::uint8_t* lastRaster = nullptr;
    h.dlg.SetOnSaveImage([&](std::uint8_t* raster) {
        ++callCount;
        lastRaster = raster;
    });
    std::uint8_t fakeRaster[128 * 32 * 3] = {};
    h.dlg.SaveImage(fakeRaster);
    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(lastRaster, fakeRaster);
}

TEST(CAutoAnswerDlg, SetActiveTestClientIsNoOp) {
    // 1:1 with legacy: a no-op.  The test just verifies it
    // doesn't crash + doesn't flip any state.
    Harness h;
    h.dlg.SetActiveTestClient();
    EXPECT_FALSE(h.dlg.isActive());
    EXPECT_FALSE(h.dlg.IsAnswerStart());
}
