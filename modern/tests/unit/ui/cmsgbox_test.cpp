// tests/unit/ui/cmsgbox_test.cpp
// Phase 6.9 unit tests for the modern mxh::ui::cMsgBox.
#include <gtest/gtest.h>

#include <memory>

#include "cButton.hpp"
#include "cDialog.hpp"
#include "cMsgBox.hpp"
#include "cWindow.hpp"
#include "cWindowManager.hpp"

using mxh::ui::cButton;
using mxh::ui::cDialog;
using mxh::ui::cMsgBox;
using mxh::ui::cWindow;
using mxh::ui::cWindowManager;

namespace {
int g_basicImg = 1;
} // namespace

TEST(CMsgBox, InitMsgBoxOnceNoOp) {
    cMsgBox::InitMsgBox();
    EXPECT_TRUE(cMsgBox::IsInitialized());
    cMsgBox::InitMsgBox();   // idempotent
    EXPECT_TRUE(cMsgBox::IsInitialized());
}

TEST(CMsgBox, DefaultConstruction) {
    cMsgBox m;
    EXPECT_FALSE(m.isClosed());
    EXPECT_EQ(m.type(), cMsgBox::MBType::NoBtn);
    EXPECT_EQ(m.message(), "");
    EXPECT_EQ(m.defaultBtn(), cMsgBox::MBResult::Ok);
}

TEST(CMsgBox, MsgBoxOkBuildsOneButton) {
    cMsgBox m;
    m.Init(50, 50, 250, 120, &g_basicImg, 100);
    m.MsgBox(100, cMsgBox::MBType::Ok, "Saved.", nullptr);
    EXPECT_EQ(m.type(), cMsgBox::MBType::Ok);
    EXPECT_EQ(m.message(), "Saved.");
    EXPECT_EQ(m.childCount(), 1u);
    // The single child is a cButton with id = kBtnIdOk.
    cWindow* c = m.childAt(0);
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->id(), 1001);
    EXPECT_EQ(m.defaultBtn(), cMsgBox::MBResult::Ok);
}

TEST(CMsgBox, MsgBoxYesNoBuildsTwoButtons) {
    cMsgBox m;
    m.Init(50, 50, 250, 120, &g_basicImg, 100);
    m.MsgBox(100, cMsgBox::MBType::YesNo, "Are you sure?");
    EXPECT_EQ(m.childCount(), 2u);
    EXPECT_EQ(m.childAt(0)->id(), 1002);  // Yes
    EXPECT_EQ(m.childAt(1)->id(), 1003);  // No
    EXPECT_EQ(m.defaultBtn(), cMsgBox::MBResult::Yes);
}

TEST(CMsgBox, MsgBoxCancelBuildsOneButton) {
    cMsgBox m;
    m.Init(50, 50, 250, 120, &g_basicImg, 100);
    m.MsgBox(100, cMsgBox::MBType::Cancel, "Continue?");
    EXPECT_EQ(m.childCount(), 1u);
    EXPECT_EQ(m.childAt(0)->id(), 1004);
    EXPECT_EQ(m.defaultBtn(), cMsgBox::MBResult::Cancel);
}

TEST(CMsgBox, MsgBoxNoBtnBuildsZeroButtons) {
    cMsgBox m;
    m.Init(50, 50, 250, 120, &g_basicImg, 100);
    m.MsgBox(100, cMsgBox::MBType::NoBtn, "Info");
    EXPECT_EQ(m.childCount(), 0u);
}

TEST(CMsgBox, ClickOkButtonFiresOkCallback) {
    cMsgBox m;
    m.Init(50, 50, 250, 120, &g_basicImg, 100);
    int callCount = 0;
    cMsgBox::MBResult got = cMsgBox::MBResult::Count;
    m.MsgBox(100, cMsgBox::MBType::Ok, "OK?",
             [&](cMsgBox&, cMsgBox::MBResult r, void*) {
                 ++callCount; got = r;
             });
    // Click in the middle of the OK button (the layout centers the
    // button at x = 50 + (250-70)/2 = 140, y = 50 + 120 - 24 - 8 = 138).
    m.SetActive(true);
    m.ActionEvent(140 + 35, 138 + 12, cWindow::MouseFlagLButton);
    m.ActionEvent(140 + 35, 138 + 12, 0);
    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(got, cMsgBox::MBResult::Ok);
    EXPECT_TRUE(m.isClosed());
    EXPECT_TRUE(m.closeRequested());
}

TEST(CMsgBox, ClickNoButtonFiresNoCallback) {
    cMsgBox m;
    m.Init(50, 50, 250, 120, &g_basicImg, 100);
    cMsgBox::MBResult got = cMsgBox::MBResult::Count;
    m.MsgBox(100, cMsgBox::MBType::YesNo, "Sure?",
             [&](cMsgBox&, cMsgBox::MBResult r, void*) { got = r; });
    m.SetActive(true);
    // Click "No" — second button, id 1003.
    cButton* no = static_cast<cButton*>(m.childAt(1));
    const std::int32_t cx = no->absX() + no->width() / 2;
    const std::int32_t cy = no->absY() + no->height() / 2;
    m.ActionEvent(cx, cy, cWindow::MouseFlagLButton);
    m.ActionEvent(cx, cy, 0);
    EXPECT_EQ(got, cMsgBox::MBResult::No);
    EXPECT_TRUE(m.isClosed());
}

TEST(CMsgBox, EnterTriggersDefaultButton) {
    cMsgBox m;
    m.Init(50, 50, 250, 120, &g_basicImg, 100);
    cMsgBox::MBResult got = cMsgBox::MBResult::Count;
    m.MsgBox(100, cMsgBox::MBType::YesNo, "Yes?",
             [&](cMsgBox&, cMsgBox::MBResult r, void*) { got = r; });
    m.SetActive(true);
    m.ActionKeyboardEvent(13 /*VK_RETURN*/, 0);
    EXPECT_EQ(got, cMsgBox::MBResult::Yes);
    EXPECT_TRUE(m.isClosed());
}

TEST(CMsgBox, EscTriggersCancelOrNo) {
    // YesNo box: Esc maps to No (matches the legacy contract).
    {
        cMsgBox m;
        m.Init(50, 50, 250, 120, &g_basicImg, 100);
        cMsgBox::MBResult got = cMsgBox::MBResult::Count;
        m.MsgBox(100, cMsgBox::MBType::YesNo, "?",
                 [&](cMsgBox&, cMsgBox::MBResult r, void*) { got = r; });
        m.SetActive(true);
        m.ActionKeyboardEvent(27 /*VK_ESCAPE*/, 0);
        EXPECT_EQ(got, cMsgBox::MBResult::No);
    }
    // OK box: Esc maps to Ok.
    {
        cMsgBox m;
        m.Init(50, 50, 250, 120, &g_basicImg, 100);
        cMsgBox::MBResult got = cMsgBox::MBResult::Count;
        m.MsgBox(100, cMsgBox::MBType::Ok, "?",
                 [&](cMsgBox&, cMsgBox::MBResult r, void*) { got = r; });
        m.SetActive(true);
        m.ActionKeyboardEvent(27, 0);
        EXPECT_EQ(got, cMsgBox::MBResult::Ok);
    }
    // Cancel box: Esc maps to Cancel.
    {
        cMsgBox m;
        m.Init(50, 50, 250, 120, &g_basicImg, 100);
        cMsgBox::MBResult got = cMsgBox::MBResult::Count;
        m.MsgBox(100, cMsgBox::MBType::Cancel, "?",
                 [&](cMsgBox&, cMsgBox::MBResult r, void*) { got = r; });
        m.SetActive(true);
        m.ActionKeyboardEvent(27, 0);
        EXPECT_EQ(got, cMsgBox::MBResult::Cancel);
    }
}

TEST(CMsgBox, ForcePressButtonFiresCallback) {
    cMsgBox m;
    m.Init(50, 50, 250, 120, &g_basicImg, 100);
    cMsgBox::MBResult got = cMsgBox::MBResult::Count;
    m.MsgBox(100, cMsgBox::MBType::YesNo, "?",
             [&](cMsgBox&, cMsgBox::MBResult r, void*) { got = r; });
    m.SetActive(true);
    EXPECT_TRUE(m.ForcePressButton(cMsgBox::MBResult::Yes));
    EXPECT_EQ(got, cMsgBox::MBResult::Yes);
    EXPECT_TRUE(m.isClosed());
}

TEST(CMsgBox, ForcePressUnknownButtonReturnsFalse) {
    cMsgBox m;
    m.Init(50, 50, 250, 120, &g_basicImg, 100);
    m.MsgBox(100, cMsgBox::MBType::Ok, "?");
    // MBResult::Cancel is not in the box; ForcePressButton must report
    // failure, not crash.
    EXPECT_FALSE(m.ForcePressButton(cMsgBox::MBResult::Cancel));
}

TEST(CMsgBox, ForceCloseSkipsCallback) {
    cMsgBox m;
    m.Init(50, 50, 250, 120, &g_basicImg, 100);
    int callCount = 0;
    m.MsgBox(100, cMsgBox::MBType::Ok, "?",
             [&](cMsgBox&, cMsgBox::MBResult, void*) { ++callCount; });
    m.SetActive(true);
    m.ForceClose();
    EXPECT_TRUE(m.isClosed());
    EXPECT_EQ(callCount, 0);
}

TEST(CMsgBox, CallbackFiresOnceEvenIfClickedTwice) {
    cMsgBox m;
    m.Init(50, 50, 250, 120, &g_basicImg, 100);
    int callCount = 0;
    m.MsgBox(100, cMsgBox::MBType::Ok, "?",
             [&](cMsgBox&, cMsgBox::MBResult, void*) { ++callCount; });
    m.SetActive(true);
    cButton* btn = static_cast<cButton*>(m.childAt(0));
    const std::int32_t cx = btn->absX() + btn->width() / 2;
    const std::int32_t cy = btn->absY() + btn->height() / 2;
    m.ActionEvent(cx, cy, cWindow::MouseFlagLButton);
    m.ActionEvent(cx, cy, 0);
    m.ActionEvent(cx, cy, cWindow::MouseFlagLButton);
    m.ActionEvent(cx, cy, 0);
    EXPECT_EQ(callCount, 1);
}

TEST(CMsgBox, DispatcherIntegrationModalRouting) {
    // Plug a cMsgBox into cWindowManager, mark it modal, dispatch input.
    // The dialog should consume the event regardless of z-order.
    cWindowManager wm;
    auto top = std::make_unique<cDialog>();
    top->Init(0, 0, 200, 200, &g_basicImg, 1);
    top->SetActive(true);
    wm.AddDialog(std::move(top));
    auto mb = std::make_unique<cMsgBox>();
    mb->Init(50, 50, 250, 120, &g_basicImg, 2);
    cMsgBox* rawMb = mb.get();
    int callCount = 0;
    mb->MsgBox(2, cMsgBox::MBType::Ok, "Hi",
               [&](cMsgBox&, cMsgBox::MBResult, void*) { ++callCount; });
    wm.AddDialog(std::move(mb));
    wm.SetModalDialog(wm.findById(2));
    // The msgbox is on top of the topmost-active dialog and modal.
    // Click its OK button via the dispatcher.
    cButton* btn = static_cast<cButton*>(rawMb->childAt(0));
    const std::int32_t cx = btn->absX() + btn->width() / 2;
    const std::int32_t cy = btn->absY() + btn->height() / 2;
    wm.ActionEvent(cx, cy, cWindow::MouseFlagLButton);
    wm.ActionEvent(cx, cy, 0);
    EXPECT_EQ(callCount, 1);
    EXPECT_TRUE(rawMb->isClosed());
    EXPECT_TRUE(rawMb->closeRequested());
    wm.RemoveAll();
    wm.ProcessDestroyQueue();
    EXPECT_EQ(wm.dialogCount(), 0u);
}
