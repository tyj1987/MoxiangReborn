// tests/unit/ui/cwindowmanager_test.cpp
// Phase 6.6 unit tests for the modern mxh::ui::cWindowManager.
#include <gtest/gtest.h>

#include <memory>

#include "cButton.hpp"
#include "cDialog.hpp"
#include "cEditBox.hpp"
#include "cWindow.hpp"
#include "cWindowManager.hpp"

using mxh::ui::cButton;
using mxh::ui::cDialog;
using mxh::ui::cEditBox;
using mxh::ui::cWindow;
using mxh::ui::cWindowManager;

namespace {
int g_basicImg = 1;
} // namespace

TEST(CWindowManager, DefaultState) {
    cWindowManager wm;
    EXPECT_EQ(wm.dialogCount(), 0u);
    EXPECT_EQ(wm.topmost(), nullptr);
    EXPECT_EQ(wm.topmostActive(), nullptr);
    EXPECT_EQ(wm.modalDialog(), nullptr);
    EXPECT_FALSE(wm.isModal());
}

TEST(CWindowManager, AddDialogStoresAndSetsTopmost) {
    cWindowManager wm;
    auto a = std::make_unique<cDialog>();
    a->Init(0, 0, 100, 100, &g_basicImg, 1);
    auto b = std::make_unique<cDialog>();
    b->Init(0, 0, 100, 100, &g_basicImg, 2);
    cDialog* rawB = b.get();
    wm.AddDialog(std::move(a));
    wm.AddDialog(std::move(b));
    EXPECT_EQ(wm.dialogCount(), 2u);
    EXPECT_EQ(wm.topmost(), rawB);
}

TEST(CWindowManager, TopmostActiveSkipsInactiveDialogs) {
    cWindowManager wm;
    auto a = std::make_unique<cDialog>();
    a->Init(0, 0, 100, 100, &g_basicImg, 1);
    a->SetActive(true);
    auto b = std::make_unique<cDialog>();
    b->Init(0, 0, 100, 100, &g_basicImg, 2);
    // b is not active.
    cDialog* rawA = a.get();
    wm.AddDialog(std::move(a));
    wm.AddDialog(std::move(b));
    EXPECT_EQ(wm.topmost(), wm.findById(2));    // b is on top
    EXPECT_EQ(wm.topmostActive(), rawA);         // a is the active one
}

TEST(CWindowManager, RemoveDialogByIdDeferDestroys) {
    cWindowManager wm;
    auto d = std::make_unique<cDialog>();
    d->Init(0, 0, 100, 100, &g_basicImg, 42);
    wm.AddDialog(std::move(d));
    EXPECT_EQ(wm.dialogCount(), 1u);
    EXPECT_TRUE(wm.RemoveDialogById(42));
    EXPECT_EQ(wm.dialogCount(), 0u);
    EXPECT_EQ(wm.destroyQueueSize(), 1u);
    EXPECT_TRUE(wm.findById(42) == nullptr);
    wm.ProcessDestroyQueue();
    EXPECT_EQ(wm.destroyQueueSize(), 0u);
}

TEST(CWindowManager, RemoveDialogByIdUnknownIsNoop) {
    cWindowManager wm;
    EXPECT_FALSE(wm.RemoveDialogById(999));
    EXPECT_EQ(wm.destroyQueueSize(), 0u);
}

TEST(CWindowManager, RemoveAllDefersAll) {
    cWindowManager wm;
    for (int i = 0; i < 5; ++i) {
        auto d = std::make_unique<cDialog>();
        d->Init(0, 0, 100, 100, &g_basicImg, i + 1);
        wm.AddDialog(std::move(d));
    }
    EXPECT_EQ(wm.dialogCount(), 5u);
    wm.RemoveAll();
    EXPECT_EQ(wm.dialogCount(), 0u);
    EXPECT_EQ(wm.destroyQueueSize(), 5u);
    wm.ProcessDestroyQueue();
    EXPECT_EQ(wm.destroyQueueSize(), 0u);
}

TEST(CWindowManager, FindByIdRecursesIntoDialogs) {
    cWindowManager wm;
    auto d = std::make_unique<cDialog>();
    d->Init(0, 0, 400, 300, &g_basicImg, 100);
    auto btn = std::make_unique<cButton>();
    btn->Init(0, 0, 50, 20, nullptr, nullptr, nullptr, nullptr, nullptr, 5);
    d->Add(std::move(btn));
    wm.AddDialog(std::move(d));
    EXPECT_EQ(wm.findById(5),   wm.findById(100));
    EXPECT_EQ(wm.findById(100), wm.findById(100));
    EXPECT_EQ(wm.findById(999), nullptr);
}

TEST(CWindowManager, FindByXYReturnsTopmostHit) {
    cWindowManager wm;
    auto a = std::make_unique<cDialog>();
    a->Init(0, 0, 200, 200, &g_basicImg, 1);
    auto b = std::make_unique<cDialog>();
    b->Init(50, 50, 200, 200, &g_basicImg, 2);   // overlaps a
    cDialog* rawB = b.get();
    wm.AddDialog(std::move(a));
    wm.AddDialog(std::move(b));
    EXPECT_EQ(wm.findByXY(100, 100), rawB);  // topmost wins
    EXPECT_EQ(wm.findByXY(10, 10),   wm.findById(1));  // only a
    EXPECT_EQ(wm.findByXY(500, 500), nullptr);
}

TEST(CWindowManager, ModalDialogSteersAllInput) {
    cWindowManager wm;
    auto a = std::make_unique<cDialog>();
    a->Init(0, 0, 200, 200, &g_basicImg, 1);
    a->SetActive(true);
    auto b = std::make_unique<cDialog>();
    b->Init(0, 0, 200, 200, &g_basicImg, 2);
    b->SetActive(true);
    wm.AddDialog(std::move(a));
    wm.AddDialog(std::move(b));
    // Without modal: b (topmost, no children) consumes the LButtonClick
    // (cWindow's default leaf behavior).
    const std::uint32_t noModal = wm.ActionEvent(50, 50, cWindow::MouseFlagLButton);
    EXPECT_EQ(noModal, static_cast<std::uint32_t>(cWindow::WindowEvent::LButtonClick));
    // With modal = a: even though b is on top, a gets the input.
    wm.SetModalDialog(wm.findById(1));
    EXPECT_TRUE(wm.isModal());
    const std::uint32_t withModal = wm.ActionEvent(50, 50, cWindow::MouseFlagLButton);
    EXPECT_EQ(withModal, static_cast<std::uint32_t>(cWindow::WindowEvent::LButtonClick));
    // After clearing modal, the original routing resumes.
    wm.SetModalDialog(nullptr);
    EXPECT_FALSE(wm.isModal());
    const std::uint32_t cleared = wm.ActionEvent(50, 50, cWindow::MouseFlagLButton);
    EXPECT_EQ(cleared, noModal);
}

TEST(CWindowManager, ModalDialogSteersInputToModalChild) {
    // Verifies that a modal dialog with a cButton child routes input to
    // the modal dialog's tree, not the topmost non-modal one.
    cWindowManager wm;
    auto modal = std::make_unique<cDialog>();
    modal->Init(0, 0, 200, 200, &g_basicImg, 1);
    modal->SetActive(true);
    auto btn = std::make_unique<cButton>();
    btn->Init(0, 0, 100, 100, nullptr, nullptr, nullptr, nullptr, nullptr, 5);
    cButton* rawBtn = btn.get();
    modal->Add(std::move(btn));
    auto top = std::make_unique<cDialog>();
    top->Init(0, 0, 200, 200, &g_basicImg, 2);
    top->SetActive(true);
    wm.AddDialog(std::move(modal));
    wm.AddDialog(std::move(top));
    wm.SetModalDialog(wm.findById(1));
    // Press inside the modal's button: should reach cButton and return
    // LButtonDown (first press, not yet click).
    EXPECT_EQ(wm.ActionEvent(50, 50, cWindow::MouseFlagLButton),
              static_cast<std::uint32_t>(cWindow::WindowEvent::LButtonDown));
    // The button is now in Pressed state.
    EXPECT_EQ(rawBtn->state(), cButton::State::Pressed);
}

TEST(CWindowManager, RemovingModalClearsModalPointer) {
    cWindowManager wm;
    auto d = std::make_unique<cDialog>();
    d->Init(0, 0, 100, 100, &g_basicImg, 1);
    d->SetActive(true);
    wm.AddDialog(std::move(d));
    wm.SetModalDialog(wm.findById(1));
    EXPECT_TRUE(wm.isModal());
    wm.RemoveDialogById(1);
    EXPECT_FALSE(wm.isModal());
    EXPECT_EQ(wm.modalDialog(), nullptr);
}

TEST(CWindowManager, KeyboardRoutedToTopmostActive) {
    cWindowManager wm;
    auto d = std::make_unique<cDialog>();
    d->Init(0, 0, 100, 100, &g_basicImg, 1);
    auto edit = std::make_unique<cEditBox>();
    edit->Init(0, 0, 100, 30, nullptr, nullptr, 2);
    edit->InitEditbox(0, 32);
    cEditBox* rawEdit = edit.get();
    d->Add(std::move(edit));
    d->SetActive(true);
    wm.AddDialog(std::move(d));
    // Focus the editbox so it consumes the keystroke.
    rawEdit->ActionEvent(50, 15, 0);
    EXPECT_TRUE(rawEdit->hasFocus());
    // Topmost is the dialog; ActionKeyboardEvent should propagate down
    // to the focused child. Feed 'a' and confirm the buffer accepts it.
    wm.ActionKeyboardEvent(static_cast<std::int32_t>(cEditBox::Key::None), 'a');
    EXPECT_EQ(rawEdit->editText(), "a");
}

TEST(CWindowManager, NoActiveDialogsInputIsNull) {
    cWindowManager wm;
    auto d = std::make_unique<cDialog>();
    d->Init(0, 0, 100, 100, &g_basicImg, 1);
    // Not active.
    wm.AddDialog(std::move(d));
    EXPECT_EQ(wm.ActionEvent(50, 50, cWindow::MouseFlagLButton),
              static_cast<std::uint32_t>(cWindow::WindowEvent::Null));
    EXPECT_EQ(wm.ActionKeyboardEvent(0, 'x'),
              static_cast<std::uint32_t>(cWindow::WindowEvent::Null));
}

TEST(CWindowManager, RenderAllWalksBackToFront) {
    cWindowManager wm;
    auto a = std::make_unique<cDialog>();
    a->Init(0, 0, 100, 100, &g_basicImg, 1);
    auto b = std::make_unique<cDialog>();
    b->Init(0, 0, 100, 100, &g_basicImg, 2);
    wm.AddDialog(std::move(a));
    wm.AddDialog(std::move(b));
    // RenderAll is a placeholder dispatch; it must not crash and must
    // handle the empty list.
    wm.RenderAll();
    wm.RemoveAll();
    wm.ProcessDestroyQueue();
    wm.RenderAll();   // empty
}
