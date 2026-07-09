// cdividebox_test.cpp — Phase 6.10 coverage for cDivideBox (split-stack
// dialog, 1:1 port of legacy `cDivideBox`).

#include "cDivideBox.hpp"
#include "cButton.hpp"
#include "cEditBox.hpp"

#include <gtest/gtest.h>

namespace {

// Shared dummy sprite — pointer identity only; the dialog never renders.
void* g_basicImg = reinterpret_cast<void*>(0x1234);

int g_divideCbCount = 0;
int g_cancelCbCount = 0;
std::uint32_t g_lastValue = 0;
void* g_lastVData1     = nullptr;
void* g_lastVData2     = nullptr;
std::int32_t g_lastId  = 0;

void ResetCallbacks() {
    g_divideCbCount = g_cancelCbCount = 0;
    g_lastValue = 0; g_lastVData1 = g_lastVData2 = nullptr; g_lastId = 0;
}

auto MakeDivideCb() {
    return [](std::int32_t idThis, mxh::ui::cDivideBox*,
             std::uint32_t v, void* d1, void* d2) {
        ++g_divideCbCount;
        g_lastValue = v;
        g_lastVData1 = d1; g_lastVData2 = d2;
        g_lastId = idThis;
    };
}
auto MakeCancelCb() {
    return [](std::int32_t, mxh::ui::cDivideBox*,
             std::uint32_t, void*, void*) {
        ++g_cancelCbCount;
    };
}

}  // namespace

TEST(CDivideBox, DefaultConstructionHasNullChildren) {
    mxh::ui::cDivideBox db;
    EXPECT_EQ(db.m_okBtn, nullptr);
    EXPECT_EQ(db.m_cancelBtn, nullptr);
    EXPECT_EQ(db.m_input, nullptr);
}

TEST(CDivideBox, CreateDivideBoxBuildsThreeChildren) {
    mxh::ui::cDivideBox db;
    db.CreateDivideBox(10, 20, 7, nullptr, nullptr, nullptr, nullptr, nullptr);
    EXPECT_NE(db.m_okBtn, nullptr);
    EXPECT_NE(db.m_cancelBtn, nullptr);
    EXPECT_NE(db.m_input, nullptr);
    // The dialog itself is sized 173x40 like the legacy version.
    EXPECT_EQ(static_cast<std::int32_t>(db.width()),  173);
    EXPECT_EQ(static_cast<std::int32_t>(db.height()),  40);
}

TEST(CDivideBox, SetValueAndGetValueRoundtrip) {
    mxh::ui::cDivideBox db;
    db.CreateDivideBox(0, 0, 1, nullptr, nullptr, nullptr, nullptr, nullptr);
    db.SetValue(42);
    EXPECT_EQ(db.GetValue(), 42u);
    db.SetValue(0);
    EXPECT_EQ(db.GetValue(), 0u);
    db.SetValue(123456);
    EXPECT_EQ(db.GetValue(), 123456u);
}

TEST(CDivideBox, MinMaxClamping) {
    mxh::ui::cDivideBox db;
    db.CreateDivideBox(0, 0, 1, nullptr, nullptr, nullptr, nullptr, nullptr);
    db.SetMinValue(10);
    db.SetMaxValue(100);
    db.SetValue(5);
    EXPECT_EQ(db.GetValue(), 10u);  // clamped to min
    db.SetValue(200);
    EXPECT_EQ(db.GetValue(), 100u); // clamped to max
}

TEST(CDivideBox, ExcuteDBFuncReturnsDividesOnEnter) {
    ResetCallbacks();
    mxh::ui::cDivideBox db;
    db.CreateDivideBox(0, 0, 1, MakeDivideCb(), MakeCancelCb(),
                       reinterpret_cast<void*>(0xAA), reinterpret_cast<void*>(0xBB),
                       nullptr);
    db.SetValue(99);
    db.ExcuteDBFunc(static_cast<std::uint32_t>(mxh::ui::cWindow::WindowEvent::KeyDown));
    EXPECT_EQ(g_divideCbCount, 1);
    EXPECT_EQ(g_cancelCbCount, 0);
    EXPECT_EQ(g_lastValue, 99u);
    EXPECT_EQ(g_lastVData1, reinterpret_cast<void*>(0xAA));
    EXPECT_EQ(g_lastVData2, reinterpret_cast<void*>(0xBB));
    EXPECT_EQ(g_lastId, 1);
}

TEST(CDivideBox, ExcuteDBFuncZeroCancels) {
    ResetCallbacks();
    mxh::ui::cDivideBox db;
    db.CreateDivideBox(0, 0, 1, MakeDivideCb(), MakeCancelCb(),
                       nullptr, nullptr, nullptr);
    db.SetValue(7);
    db.ExcuteDBFunc(0);  // legacy "cancel" signal
    EXPECT_EQ(g_divideCbCount, 0);
    EXPECT_EQ(g_cancelCbCount, 1);
    // Close should be requested so the manager can clean up.
    EXPECT_TRUE(db.closeRequested());
}

TEST(CDivideBox, ActionKeyboardEnterTriggersDivide) {
    ResetCallbacks();
    mxh::ui::cDivideBox db;
    db.CreateDivideBox(0, 0, 1, MakeDivideCb(), MakeCancelCb(),
                       nullptr, nullptr, nullptr);
    db.SetActive(true);
    db.SetValue(50);
    db.ActionKeyboardEvent(13 /*VK_RETURN*/, 0);
    EXPECT_EQ(g_divideCbCount, 1);
    EXPECT_EQ(g_lastValue, 50u);
}

TEST(CDivideBox, ActionKeyboardOtherKeyIsNoop) {
    ResetCallbacks();
    mxh::ui::cDivideBox db;
    db.CreateDivideBox(0, 0, 1, MakeDivideCb(), MakeCancelCb(),
                       nullptr, nullptr, nullptr);
    db.SetActive(true);
    db.ActionKeyboardEvent(65 /*'A'*/, 65);
    EXPECT_EQ(g_divideCbCount, 0);
    EXPECT_EQ(g_cancelCbCount, 0);
}

TEST(CDivideBox, ClickOkButtonFiresDivideAndCloses) {
    ResetCallbacks();
    mxh::ui::cDivideBox db;
    db.CreateDivideBox(0, 0, 9, MakeDivideCb(), MakeCancelCb(),
                       nullptr, nullptr, nullptr);
    db.SetActive(true);
    db.SetValue(123);
    // OK button is at absX=0+128=128, absY=0+15=15, w=40, h=20. Center: (148, 25).
    db.ActionEvent(148, 25, mxh::ui::cWindow::MouseFlagLButton);
    EXPECT_EQ(g_divideCbCount, 1);
    EXPECT_EQ(g_lastValue, 123u);
    EXPECT_EQ(g_lastId, 9);
    EXPECT_TRUE(db.closeRequested());
    EXPECT_FALSE(db.isEnabled());
}

TEST(CDivideBox, ClickCancelButtonFiresCancelAndCloses) {
    ResetCallbacks();
    mxh::ui::cDivideBox db;
    db.CreateDivideBox(0, 0, 9, MakeDivideCb(), MakeCancelCb(),
                       nullptr, nullptr, nullptr);
    db.SetActive(true);
    db.SetValue(7);
    // Cancel button at absX=85, absY=15, w=40, h=20. Center: (105, 25).
    db.ActionEvent(105, 25, mxh::ui::cWindow::MouseFlagLButton);
    EXPECT_EQ(g_divideCbCount, 0);
    EXPECT_EQ(g_cancelCbCount, 1);
    EXPECT_TRUE(db.closeRequested());
}

TEST(CDivideBox, ButtonIdsAreDistinct) {
    mxh::ui::cDivideBox db;
    db.CreateDivideBox(0, 0, 1, nullptr, nullptr, nullptr, nullptr, nullptr);
    ASSERT_TRUE(db.m_okBtn);
    ASSERT_TRUE(db.m_cancelBtn);
    ASSERT_TRUE(db.m_input);
    EXPECT_NE(db.m_okBtn->id(), db.m_cancelBtn->id());
    EXPECT_NE(db.m_okBtn->id(), db.m_input->id());
    EXPECT_NE(db.m_cancelBtn->id(), db.m_input->id());
}

TEST(CDivideBox, ClickOutsideButtonsIsNoop) {
    ResetCallbacks();
    mxh::ui::cDivideBox db;
    db.CreateDivideBox(0, 0, 1, MakeDivideCb(), MakeCancelCb(),
                       nullptr, nullptr, nullptr);
    db.SetActive(true);
    db.SetValue(50);
    // Click 5,5 — outside both buttons (OK starts at x=128).
    db.ActionEvent(5, 5, mxh::ui::cWindow::MouseFlagLButton);
    EXPECT_EQ(g_divideCbCount, 0);
    EXPECT_EQ(g_cancelCbCount, 0);
    EXPECT_FALSE(db.closeRequested());
}
