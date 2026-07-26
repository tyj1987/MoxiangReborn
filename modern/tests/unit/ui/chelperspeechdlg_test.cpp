// mxh/tests/unit/unit/ui/chelperspeechdlg_test.cpp
//
// Unit tests for mxh::ui::cHelperSpeechDlg (Phase C dialog port).
//
// Locks down the 1:1 surface:
//   * Init stores the line-height + clears state
//   * Linking is a no-op (legacy WINDOW_ID walk deferred)
//   * ActionEvent forwards to the page's ActionEvent callback
//   * Render forwards to the page's render callback
//   * OpenDialog queues when a page is open, otherwise creates
//   * OpenDialog + start fade-in
//   * CloseDialog destroys the page + flips m_bClose
//   * ResetDialog clears all state
//   * AddPage appends to the queue
//   * UseComponent stores the flag
//   * SetHelperPos / SetTextRect store coords
//   * StartFadeOut flips m_bFadeOut + updates cur page idx
//   * StartFadeIn flips m_bFadeIn
//   * IsListEmpty reflects the queue

#include "mxh/ui/chelperspeechdlg.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <vector>

using mxh::ui::cHelperSpeechDlg;

// The dialog uses mxh::ui::cPageBase* in its callback
// signatures.  We derive a concrete subclass for the test
// shims so the std::function conversions work cleanly under
// MSVC 14.40 (which refuses to convert raw function pointers
// whose parameter type is from an anonymous namespace).
namespace test_hsd {

int g_createCount   = 0;
int g_destroyCount  = 0;
int g_actionCount   = 0;
int g_renderCount   = 0;
std::uint32_t g_actionReturn = 0;

mxh::ui::cPageBase g_pageStorage;

mxh::ui::cPageBase* faCreate(std::uint32_t) {
    ++g_createCount;
    return &g_pageStorage;
}
void faDestroy(mxh::ui::cPageBase*) { ++g_destroyCount; }
std::uint32_t faAction(mxh::ui::cPageBase*, void*) {
    ++g_actionCount;
    return g_actionReturn;
}
void faRender(mxh::ui::cPageBase*) { ++g_renderCount; }

}  // namespace test_hsd

TEST(CHelperSpeechDlg, DefaultConstructionIsReset) {
    cHelperSpeechDlg d;
    EXPECT_FALSE(d.isFadeIn());
    EXPECT_FALSE(d.isFadeOut());
    EXPECT_FALSE(d.isClose());
    EXPECT_FALSE(d.isUseComponent());
    EXPECT_FALSE(d.hasCurPage());
    EXPECT_EQ(d.lineHeight(), 0);
    EXPECT_EQ(d.queuedPageCount(), 0);
    EXPECT_TRUE(d.IsListEmpty());
    EXPECT_EQ(d.GetCurPageIdx(), 0u);
}

TEST(CHelperSpeechDlg, InitStoresLineHeight) {
    cHelperSpeechDlg d;
    d.Init(0, 0, 100, 50, 12, 1);
    EXPECT_EQ(d.lineHeight(), 12);
    EXPECT_TRUE(d.IsListEmpty());
}

TEST(CHelperSpeechDlg, UseComponentStoresFlag) {
    cHelperSpeechDlg d;
    d.UseComponent(true);
    EXPECT_TRUE(d.isUseComponent());
    d.UseComponent(false);
    EXPECT_FALSE(d.isUseComponent());
}

TEST(CHelperSpeechDlg, SetHelperPosStoresCoords) {
    cHelperSpeechDlg d;
    d.SetHelperPos(10.5f, 20.25f);
    EXPECT_FLOAT_EQ(d.helperPosX(), 10.5f);
    EXPECT_FLOAT_EQ(d.helperPosY(), 20.25f);
}

TEST(CHelperSpeechDlg, SetTextRectIsSafe) {
    cHelperSpeechDlg d;
    d.SetTextRect(1, 2, 3, 4);
    SUCCEED();
}

TEST(CHelperSpeechDlg, AddPageAppendsToQueue) {
    cHelperSpeechDlg d;
    d.AddPage(100);
    EXPECT_FALSE(d.IsListEmpty());
    EXPECT_EQ(d.queuedPageCount(), 1);
    d.AddPage(200);
    EXPECT_EQ(d.queuedPageCount(), 2);
}

TEST(CHelperSpeechDlg, OpenDialogWithNoCurPageCreatesAndStartsFadeIn) {
    test_hsd::g_createCount = 0;
    cHelperSpeechDlg d;
    cHelperSpeechDlg::CreatePageCallback ccb =
        [](std::uint32_t id) -> mxh::ui::cPageBase* {
            return static_cast<mxh::ui::cPageBase*>(test_hsd::faCreate(id));
        };
    d.SetCreatePageCallbackForTest(ccb);
    d.SetNowForTest(1000u);
    d.OpenDialog(42);
    EXPECT_TRUE(d.hasCurPage());
    EXPECT_TRUE(d.isFadeIn());
    EXPECT_FALSE(d.isFadeOut());
    EXPECT_EQ(d.GetCurPageIdx(), 42u);
    EXPECT_EQ(test_hsd::g_createCount, 1);
}

TEST(CHelperSpeechDlg, OpenDialogWithCurPageQueues) {
    cHelperSpeechDlg d;
    cHelperSpeechDlg::CreatePageCallback ccb =
        [](std::uint32_t id) -> mxh::ui::cPageBase* {
            return static_cast<mxh::ui::cPageBase*>(test_hsd::faCreate(id));
        };
    d.SetCreatePageCallbackForTest(ccb);
    d.OpenDialog(1);
    d.OpenDialog(2);
    EXPECT_EQ(d.queuedPageCount(), 1);
    EXPECT_EQ(d.GetCurPageIdx(), 1u);
}

TEST(CHelperSpeechDlg, OpenDialogWithoutCreateCallbackDoesNotCrash) {
    cHelperSpeechDlg d;
    d.OpenDialog(1);
    EXPECT_TRUE(d.isFadeIn());
    EXPECT_FALSE(d.hasCurPage());
}

TEST(CHelperSpeechDlg, CloseDialogDestroysPageAndFlipsCloseFlag) {
    test_hsd::g_destroyCount = 0;
    cHelperSpeechDlg d;
    cHelperSpeechDlg::CreatePageCallback ccb =
        [](std::uint32_t id) -> mxh::ui::cPageBase* {
            return static_cast<mxh::ui::cPageBase*>(test_hsd::faCreate(id));
        };
    d.SetCreatePageCallbackForTest(ccb);
    cHelperSpeechDlg::DestroyPageCallback dcb =
        [](mxh::ui::cPageBase* p) { test_hsd::faDestroy(p); };
    d.SetDestroyPageCallbackForTest(dcb);
    d.OpenDialog(1);
    d.CloseDialog();
    EXPECT_TRUE(d.isClose());
    EXPECT_FALSE(d.hasCurPage());
    EXPECT_TRUE(d.IsListEmpty());
    EXPECT_EQ(test_hsd::g_destroyCount, 1);
}

TEST(CHelperSpeechDlg, ResetDialogClearsAllState) {
    cHelperSpeechDlg d;
    cHelperSpeechDlg::CreatePageCallback ccb =
        [](std::uint32_t id) -> mxh::ui::cPageBase* {
            return static_cast<mxh::ui::cPageBase*>(test_hsd::faCreate(id));
        };
    d.SetCreatePageCallbackForTest(ccb);
    d.SetNowForTest(1000u);
    d.OpenDialog(1);
    d.AddPage(2);
    d.StartFadeOut(99);
    d.ResetDialog();
    EXPECT_FALSE(d.hasCurPage());
    EXPECT_FALSE(d.isFadeIn());
    EXPECT_FALSE(d.isFadeOut());
    EXPECT_FALSE(d.isClose());
    EXPECT_TRUE(d.IsListEmpty());
    EXPECT_EQ(d.startTime(), 0u);
}

TEST(CHelperSpeechDlg, StartFadeInFlipsFadeInFlag) {
    cHelperSpeechDlg d;
    d.SetNowForTest(12345u);
    d.StartFadeIn();
    EXPECT_TRUE(d.isFadeIn());
    EXPECT_FALSE(d.isFadeOut());
    EXPECT_EQ(d.startTime(), 12345u);
}

TEST(CHelperSpeechDlg, StartFadeOutFlipsFadeOutAndUpdatesPageIdx) {
    cHelperSpeechDlg d;
    d.SetNowForTest(7777u);
    d.StartFadeOut(42);
    EXPECT_TRUE(d.isFadeOut());
    EXPECT_FALSE(d.isFadeIn());
    EXPECT_EQ(d.startTime(), 7777u);
    EXPECT_EQ(d.GetCurPageIdx(), 42u);
}

TEST(CHelperSpeechDlg, StartFadeOutZeroDoesNotUpdatePageIdx) {
    cHelperSpeechDlg d;
    d.StartFadeOut(50);
    d.StartFadeOut(0);
    EXPECT_EQ(d.GetCurPageIdx(), 50u);
}

TEST(CHelperSpeechDlg, ActionEventForwardsToPage) {
    test_hsd::g_actionCount = 0;
    test_hsd::g_actionReturn = 0xDEADu;
    cHelperSpeechDlg d;
    cHelperSpeechDlg::CreatePageCallback ccb =
        [](std::uint32_t id) -> mxh::ui::cPageBase* {
            return static_cast<mxh::ui::cPageBase*>(test_hsd::faCreate(id));
        };
    d.SetCreatePageCallbackForTest(ccb);
    cHelperSpeechDlg::PageActionCallback pacb =
        [](mxh::ui::cPageBase* p, void* m) -> std::uint32_t {
            return test_hsd::faAction(p, m);
        };
    d.SetPageActionCallbackForTest(pacb);
    d.OpenDialog(1);
    std::uint32_t we = d.ActionEvent(nullptr);
    EXPECT_EQ(test_hsd::g_actionCount, 1);
    EXPECT_EQ(we, 0xDEADu);
}

TEST(CHelperSpeechDlg, RenderForwardsToPage) {
    test_hsd::g_renderCount = 0;
    cHelperSpeechDlg d;
    cHelperSpeechDlg::CreatePageCallback ccb =
        [](std::uint32_t id) -> mxh::ui::cPageBase* {
            return static_cast<mxh::ui::cPageBase*>(test_hsd::faCreate(id));
        };
    d.SetCreatePageCallbackForTest(ccb);
    cHelperSpeechDlg::PageRenderCallback prcb =
        [](mxh::ui::cPageBase* p) { test_hsd::faRender(p); };
    d.SetPageRenderCallbackForTest(prcb);
    d.OpenDialog(1);
    d.Render();
    EXPECT_EQ(test_hsd::g_renderCount, 1);
}

TEST(CHelperSpeechDlg, RenderWithoutPageIsSafe) {
    cHelperSpeechDlg d;
    d.Render();
    SUCCEED();
}

TEST(CHelperSpeechDlg, LinkingIsSafe) {
    cHelperSpeechDlg d;
    d.Linking();
    SUCCEED();
}

TEST(CHelperSpeechDlg, NonCopyable) {
    static_assert(!std::is_copy_constructible_v<cHelperSpeechDlg>);
    static_assert(!std::is_copy_assignable_v<cHelperSpeechDlg>);
}
