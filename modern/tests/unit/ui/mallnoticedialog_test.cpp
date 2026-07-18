// mallnoticedialog_test.cpp — 1:1 port verification tests for cMallNoticeDialog.

#include "mallnoticedialog.hpp"
#include "ctabdialog.hpp"
#include "cpushupbutton.hpp"
#include "cwindow.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>

using mxh::ui::cMallNoticeDialog;
using mxh::ui::cTabDialog;
using mxh::ui::cPushupButton;
using mxh::ui::cWindow;
using mxh::ui::cDialog;
using mxh::ui::kItemMallBtnId;

namespace {

std::unique_ptr<cMallNoticeDialog> MakeDialog() {
    auto d = std::make_unique<cMallNoticeDialog>();
    d->Init(0, 0, 200, 100, nullptr, 0);
    d->InitTab(4);  // 1:1 with legacy: cTabDialog must InitTab
                    // before Add. 4 tabs is enough for our tests.
    return d;
}

std::unique_ptr<cPushupButton> MakeTabBtn(int id) {
    auto b = std::make_unique<cPushupButton>();
    b->Init(0, 0, 30, 20, nullptr, nullptr, nullptr, nullptr, nullptr, id);
    return b;
}

std::unique_ptr<cDialog> MakeTabSheet(int id) {
    auto s = std::make_unique<cDialog>();
    s->Init(0, 0, 100, 80, nullptr, id);
    return s;
}

class CMallNoticeDialogTest : public ::testing::Test {
protected:
    void SetUp() override { cMallNoticeDialog::ClearTestInjections(); }
    void TearDown() override { cMallNoticeDialog::ClearTestInjections(); }
};

}  // namespace

// ---------------------------------------------------------------------------
// Constants + construction
// ---------------------------------------------------------------------------

TEST_F(CMallNoticeDialogTest, IdConstantMatchesLocalRange) {
    EXPECT_EQ(kItemMallBtnId, 2200);
    // 1:1 with legacy ITEM_MALLBTN (from WindowIDEnum.h).
}

TEST_F(CMallNoticeDialogTest, DefaultUrlIsElseBranch) {
    // 1:1 quirk: legacy `#else` branch uses wldhmx.com URL.
    // Modern port: default mall URL is the else-branch.
    EXPECT_EQ(cMallNoticeDialog::mallUrlForTesting(), mxh::ui::mallUrls::kElse);
}

TEST_F(CMallNoticeDialogTest, AllLocaleUrlsAreDefined) {
    // 1:1 with legacy #ifdef branches: TAIWAN/JP/HK/else.
    EXPECT_STREQ(mxh::ui::mallUrls::kTaiwan,
                 "https://secure.tengwu.com.cn/ItemMall/web_product_main.asp");
    EXPECT_STREQ(mxh::ui::mallUrls::kJapan,  "");  // 1:1 quirk: legacy empty
    EXPECT_STREQ(mxh::ui::mallUrls::kHk,     "");  // 1:1 quirk: legacy empty
    EXPECT_STREQ(mxh::ui::mallUrls::kElse,   "http://www.wldhmx.com/webshop.aspx");
}

TEST_F(CMallNoticeDialogTest, DefaultCountersAreZero) {
    auto d = MakeDialog();
    EXPECT_EQ(d->addCallCount(), 0u);
    EXPECT_EQ(d->onActionEventCallCount(), 0u);
    EXPECT_EQ(d->shellExecuteCount(), 0u);
}

// ---------------------------------------------------------------------------
// Add() — 3-way dispatch
// ---------------------------------------------------------------------------

TEST_F(CMallNoticeDialogTest, AddPushupButtonRoutesToTabBtn) {
    // 1:1 with legacy: WT_PUSHUPBUTTON branch → AddTabBtn
    // (curIdx1++). Modern port: dynamic_cast<cPushupButton*>
    // catches the same branch.
    auto d = MakeDialog();
    auto btn = MakeTabBtn(101);
    cPushupButton* rawBtn = btn.get();
    d->Add(btn.release());
    EXPECT_NE(d->GetTabBtn(0), nullptr);
    EXPECT_EQ(d->GetTabBtn(0), rawBtn);
    // curIdx1 incremented from 0 → 1.
    EXPECT_EQ(d->curIdx1(), 1u);
}

TEST_F(CMallNoticeDialogTest, AddDialogRoutesToTabSheet) {
    // 1:1 with legacy: WT_DIALOG branch → AddTabSheet
    // (curIdx2++). Modern port: dynamic_cast<cDialog*>
    // catches the same branch.
    auto d = MakeDialog();
    auto sheet = MakeTabSheet(201);
    cDialog* rawSheet = sheet.get();
    d->Add(sheet.release());
    EXPECT_NE(d->GetTabSheet(0), nullptr);
    EXPECT_EQ(d->GetTabSheet(0), rawSheet);
    // curIdx2 incremented from 0 → 1.
    EXPECT_EQ(d->curIdx2(), 1u);
}

TEST_F(CMallNoticeDialogTest, AddNullIsTolerated) {
    // 1:1 quirk: legacy `if (window->GetType() == ...)`
    // would dereference a null pointer (UB). Modern port:
    // defensive null check (1:1 quirk documented).
    auto d = MakeDialog();
    d->Add(nullptr);
    EXPECT_EQ(d->addCallCount(), 1u);  // count incremented
    SUCCEED();
}

TEST_F(CMallNoticeDialogTest, AddIncrementsCurIdx) {
    // 1:1 with legacy: each Add increments the appropriate
    // counter. Modern port: same behavior.
    auto d = MakeDialog();
    d->Add(MakeTabBtn(101).release());
    d->Add(MakeTabBtn(102).release());
    d->Add(MakeTabSheet(201).release());
    EXPECT_EQ(d->curIdx1(), 2u);
    EXPECT_EQ(d->curIdx2(), 1u);
    EXPECT_NE(d->GetTabBtn(0), nullptr);
    EXPECT_NE(d->GetTabBtn(1), nullptr);
    EXPECT_NE(d->GetTabSheet(0), nullptr);
}

TEST_F(CMallNoticeDialogTest, AddIncrementsAddCallCount) {
    auto d = MakeDialog();
    d->Add(MakeTabBtn(101).release());
    d->Add(MakeTabSheet(201).release());
    EXPECT_EQ(d->addCallCount(), 2u);
}

// ---------------------------------------------------------------------------
// OnActionEvent — ITEM_MALLBTN
// ---------------------------------------------------------------------------

TEST_F(CMallNoticeDialogTest, OnActionEventIgnoresNonClickEvents) {
    // 1:1 quirk: legacy `we & WE_BTNCLICK` (64) gates the
    // body. Modern port: `we == WindowEvent::LButtonClick`
    // (4) per R-12. A non-click event is no-op.
    auto d = MakeDialog();
    d->OnActionEvent(kItemMallBtnId, nullptr,
                     static_cast<std::uint32_t>(cWindow::WindowEvent::MouseMove));
    EXPECT_EQ(d->shellExecuteCount(), 0u);
}

TEST_F(CMallNoticeDialogTest, OnActionEventMallBtnLaunchesShell) {
    // 1:1 with legacy: ITEM_MALLBTN + WE_BTNCLICK → ShellExecute
    // with locale URL. Modern port: ShellExecute stubbed no-op;
    // URL recorded in s_lastShellUrl.
    auto d = MakeDialog();
    d->OnActionEvent(kItemMallBtnId, nullptr,
                     static_cast<std::uint32_t>(cWindow::WindowEvent::LButtonClick));
    EXPECT_EQ(d->shellExecuteCount(), 1u);
    EXPECT_EQ(d->lastShellUrl(), mxh::ui::mallUrls::kElse);
}

TEST_F(CMallNoticeDialogTest, OnActionEventMallBtnWithCustomUrl) {
    // 1:1 with legacy: locale-specific URL via test-injectable.
    auto d = MakeDialog();
    cMallNoticeDialog::SetMallUrlForTesting(mxh::ui::mallUrls::kTaiwan);
    d->OnActionEvent(kItemMallBtnId, nullptr,
                     static_cast<std::uint32_t>(cWindow::WindowEvent::LButtonClick));
    EXPECT_EQ(d->lastShellUrl(), mxh::ui::mallUrls::kTaiwan);
}

TEST_F(CMallNoticeDialogTest, OnActionEventUnknownIdIsNoOp) {
    // 1:1 quirk: legacy inner if has no `else` branch.
    // Modern port: unknown lId → no ShellExecute call.
    auto d = MakeDialog();
    d->OnActionEvent(99999, nullptr,
                     static_cast<std::uint32_t>(cWindow::WindowEvent::LButtonClick));
    EXPECT_EQ(d->shellExecuteCount(), 0u);
}

TEST_F(CMallNoticeDialogTest, OnActionEventIncrementsCallCount) {
    auto d = MakeDialog();
    d->OnActionEvent(kItemMallBtnId, nullptr,
                     static_cast<std::uint32_t>(cWindow::WindowEvent::LButtonClick));
    d->OnActionEvent(99999, nullptr, 0u);
    EXPECT_EQ(d->onActionEventCallCount(), 2u);
}

// ---------------------------------------------------------------------------
// Test-injection cleanup
// ---------------------------------------------------------------------------

TEST_F(CMallNoticeDialogTest, ClearTestInjectionsResetsState) {
    cMallNoticeDialog::SetMallUrlForTesting("http://test.example.com");
    cMallNoticeDialog::ClearTestInjections();
    EXPECT_EQ(cMallNoticeDialog::mallUrlForTesting(), mxh::ui::mallUrls::kElse);
}
