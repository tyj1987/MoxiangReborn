// tests/unit/ui/cdialog_test.cpp
// Phase 6.3 unit tests for the modern mxh::ui::cDialog widget.
#include <gtest/gtest.h>

#include <memory>

#include "cButton.hpp"
#include "cDialog.hpp"
#include "cEditBox.hpp"
#include "cWindow.hpp"

using mxh::ui::cButton;
using mxh::ui::cDialog;
using mxh::ui::cEditBox;
using mxh::ui::cWindow;
using mxh::ui::CaptionRect;

namespace {
int g_basicImg = 1;
} // namespace

TEST(CDialog, DefaultConstruction) {
    cDialog d;
    EXPECT_FALSE(d.IsAutoClose());
    EXPECT_FALSE(d.isActive());
    EXPECT_FALSE(d.hasCaption());
    EXPECT_EQ(d.alpha(), 255);
    EXPECT_EQ(d.componentCount(), 0u);
}

TEST(CDialog, InitStoresDialogState) {
    cDialog d;
    d.Init(10, 20, 400, 300, &g_basicImg, 42);
    EXPECT_EQ(d.id(), 42);
    EXPECT_EQ(d.absX(), 10);
    EXPECT_EQ(d.absY(), 20);
    EXPECT_EQ(d.width(), 400u);
    EXPECT_EQ(d.height(), 300u);
    EXPECT_EQ(d.basicImage(), &g_basicImg);
    EXPECT_FALSE(d.isActive());
    EXPECT_FALSE(d.IsAutoClose());
    EXPECT_FALSE(d.hasCaption());
}

TEST(CDialog, AutoCloseSetter) {
    cDialog d;
    d.Init(0, 0, 100, 100, &g_basicImg);
    d.SetAutoClose(true);
    EXPECT_TRUE(d.IsAutoClose());
    d.SetAutoClose(false);
    EXPECT_FALSE(d.IsAutoClose());
}

TEST(CDialog, CloseRequestLatchesUntilCleared) {
    cDialog d;
    d.Init(0, 0, 100, 100, &g_basicImg);
    EXPECT_FALSE(d.closeRequested());
    d.requestClose();
    EXPECT_TRUE(d.closeRequested());
    EXPECT_TRUE(d.closeRequested());    // latched
    d.clearCloseRequest();
    EXPECT_FALSE(d.closeRequested());
}

TEST(CDialog, CaptionRectAndPtInCaption) {
    cDialog d;
    d.Init(10, 20, 200, 100, &g_basicImg);
    EXPECT_FALSE(d.hasCaption());
    d.SetCaptionRect(10, 20, 210, 40);
    EXPECT_TRUE(d.hasCaption());
    EXPECT_EQ(d.captionLeft(),   10);
    EXPECT_EQ(d.captionTop(),    20);
    EXPECT_EQ(d.captionRight(),  210);
    EXPECT_EQ(d.captionBottom(), 40);
    EXPECT_TRUE(d.PtInCaption(50, 30));
    EXPECT_TRUE(d.PtInCaption(10, 20));   // top-left corner
    EXPECT_TRUE(d.PtInCaption(210, 40));  // bottom-right corner
    EXPECT_FALSE(d.PtInCaption(50, 60));  // below caption
    EXPECT_FALSE(d.PtInCaption(220, 30)); // right of caption
}

TEST(CDialog, CaptionRectStructOverload) {
    cDialog d;
    d.Init(0, 0, 200, 100, &g_basicImg);
    CaptionRect r{0, 0, 200, 30};
    d.SetCaptionRect(r);
    EXPECT_TRUE(d.hasCaption());
    EXPECT_TRUE(d.PtInCaption(100, 15));
    EXPECT_FALSE(d.PtInCaption(100, 60));
}

TEST(CDialog, EmptyCaptionIsIgnored) {
    cDialog d;
    d.Init(0, 0, 100, 100, &g_basicImg);
    d.SetCaptionRect(50, 20, 30, 10);  // left > right — degenerate
    EXPECT_FALSE(d.hasCaption());
    EXPECT_FALSE(d.PtInCaption(40, 15));
}

TEST(CDialog, ActiveSetter) {
    cDialog d;
    d.Init(0, 0, 100, 100, &g_basicImg);
    EXPECT_FALSE(d.isActive());
    d.SetActive(true);
    EXPECT_TRUE(d.isActive());
    d.SetActive(false);
    EXPECT_FALSE(d.isActive());
}

TEST(CDialog, AlphaSetters) {
    cDialog d;
    d.Init(0, 0, 100, 100, &g_basicImg);
    d.SetAlpha(128);
    EXPECT_EQ(d.alpha(), 128);
    d.SetOptionAlpha(0x80808080u);
    EXPECT_EQ(d.optionAlpha(), 0x80808080u);
}

TEST(CDialog, AddChildStoresInTree) {
    cDialog d;
    d.Init(0, 0, 400, 300, &g_basicImg);
    auto btn = std::make_unique<cButton>();
    btn->Init(50, 50, 100, 30, nullptr, nullptr, nullptr, nullptr, nullptr, 1);
    cButton* rawBtn = btn.get();
    d.Add(std::move(btn));
    EXPECT_EQ(d.componentCount(), 1u);
    EXPECT_EQ(d.componentAt(0), rawBtn);
    EXPECT_EQ(rawBtn->parent(), &d);
}

TEST(CDialog, FindWindowByIdFindsDirectChild) {
    cDialog d;
    d.Init(0, 0, 400, 300, &g_basicImg, 100);
    auto btn = std::make_unique<cButton>();
    btn->Init(0, 0, 50, 20, nullptr, nullptr, nullptr, nullptr, nullptr, 5);
    cButton* rawBtn = btn.get();
    d.Add(std::move(btn));
    EXPECT_EQ(d.findWindowById(5), rawBtn);
    EXPECT_EQ(d.findWindowById(100), &d);    // the dialog itself
    EXPECT_EQ(d.findWindowById(999), nullptr);
}

TEST(CDialog, FindWindowByIdRecursesIntoNestedDialog) {
    cDialog outer;
    outer.Init(0, 0, 400, 300, &g_basicImg, 1);
    auto inner = std::make_unique<cDialog>();
    inner->Init(50, 50, 200, 100, &g_basicImg, 2);
    auto btn = std::make_unique<cButton>();
    btn->Init(0, 0, 50, 20, nullptr, nullptr, nullptr, nullptr, nullptr, 7);
    cButton* rawBtn = btn.get();
    inner->Add(std::move(btn));
    cDialog* rawInner = inner.get();
    outer.Add(std::move(inner));
    EXPECT_EQ(outer.findWindowById(7),  rawBtn);
    EXPECT_EQ(outer.findWindowById(2),  rawInner);
    EXPECT_EQ(outer.findWindowById(1),  &outer);
}

TEST(CDialog, SetAbsXYMovesChildren) {
    cDialog d;
    d.Init(0, 0, 400, 300, &g_basicImg);
    auto btn = std::make_unique<cButton>();
    btn->Init(20, 30, 100, 20, nullptr, nullptr, nullptr, nullptr, nullptr, 1);
    auto edit = std::make_unique<cEditBox>();
    edit->Init(50, 60, 150, 25, nullptr, nullptr, 2);
    cButton* rawBtn = btn.get();
    cEditBox* rawEdit = edit.get();
    d.Add(std::move(btn));
    d.Add(std::move(edit));
    d.SetAbsXY(100, 200);
    EXPECT_EQ(d.absX(), 100);
    EXPECT_EQ(d.absY(), 200);
    EXPECT_EQ(rawBtn->absX(),  120);  // 20 + 100
    EXPECT_EQ(rawBtn->absY(),  230);  // 30 + 200
    EXPECT_EQ(rawEdit->absX(), 150);
    EXPECT_EQ(rawEdit->absY(), 260);
}

TEST(CDialog, SetAbsXYAlsoMovesCaption) {
    cDialog d;
    d.Init(10, 20, 200, 100, &g_basicImg);
    d.SetCaptionRect(10, 20, 210, 40);
    d.SetAbsXY(100, 200);
    EXPECT_EQ(d.captionLeft(),   100);
    EXPECT_EQ(d.captionTop(),    200);
    EXPECT_EQ(d.captionRight(),  300);
    EXPECT_EQ(d.captionBottom(), 220);
    EXPECT_TRUE(d.PtInCaption(150, 210));
    EXPECT_FALSE(d.PtInCaption(50, 30));    // old position no longer hits
}

TEST(CDialog, SetDisableCascadesToChildren) {
    cDialog d;
    d.Init(0, 0, 400, 300, &g_basicImg);
    auto btn = std::make_unique<cButton>();
    btn->Init(0, 0, 50, 20, nullptr, nullptr, nullptr, nullptr, nullptr, 1);
    auto edit = std::make_unique<cEditBox>();
    edit->Init(0, 0, 100, 25, nullptr, nullptr, 2);
    cButton* rawBtn = btn.get();
    cEditBox* rawEdit = edit.get();
    d.Add(std::move(btn));
    d.Add(std::move(edit));
    EXPECT_TRUE(rawBtn->isEnabled());
    EXPECT_TRUE(rawEdit->isEnabled());
    d.SetDisable(true);
    EXPECT_FALSE(d.isEnabled());
    EXPECT_FALSE(rawBtn->isEnabled());
    EXPECT_FALSE(rawEdit->isEnabled());
    d.SetDisable(false);
    EXPECT_TRUE(d.isEnabled());
    EXPECT_TRUE(rawBtn->isEnabled());
    EXPECT_TRUE(rawEdit->isEnabled());
}

TEST(CDialog, ActionEventRequiresActive) {
    cDialog d;
    d.Init(0, 0, 100, 100, &g_basicImg);
    auto btn = std::make_unique<cButton>();
    btn->Init(0, 0, 100, 100, nullptr, nullptr, nullptr, nullptr, nullptr, 1);
    d.Add(std::move(btn));
    // Not active → no event consumed even if the click is inside.
    EXPECT_EQ(d.ActionEvent(50, 50, cWindow::MouseFlagLButton),
              static_cast<std::uint32_t>(cWindow::WindowEvent::Null));
    d.SetActive(true);
    // Now active → button event propagates. cButton on LButtonDown returns
    // LButtonDown (=2), not LButtonClick (=4); the click fires on release.
    const std::uint32_t ev = d.ActionEvent(50, 50, cWindow::MouseFlagLButton);
    EXPECT_EQ(ev, static_cast<std::uint32_t>(cWindow::WindowEvent::LButtonDown));
    // Release: cButton fires the click.
    const std::uint32_t click = d.ActionEvent(50, 50, 0);
    EXPECT_EQ(click, static_cast<std::uint32_t>(cWindow::WindowEvent::LButtonClick));
}
