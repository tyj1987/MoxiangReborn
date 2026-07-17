// tests/unit/ui/ceditbox_test.cpp
// Phase 6.2 unit tests for the modern mxh::ui::cEditBox widget.
#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "cEditBox.hpp"
#include "cWindow.hpp"

using mxh::ui::cEditBox;
using mxh::ui::cWindow;
using WE = cWindow::WindowEvent;
using Key = cEditBox::Key;

namespace {
int g_basicImage = 1;
int g_focusImage = 2;
} // namespace

TEST(CEditBox, InitStoresImagesAndDefaults) {
    cEditBox e;
    e.Init(0, 0, 200, 30, &g_basicImage, &g_focusImage, 11);
    EXPECT_EQ(e.id(), 11);
    EXPECT_EQ(e.basicImage(), &g_basicImage);
    EXPECT_EQ(e.focusImage(), &g_focusImage);
    EXPECT_FALSE(e.HasCaret());
    EXPECT_FALSE(e.IsSecret());
    EXPECT_FALSE(e.IsReadOnly());
    EXPECT_EQ(e.caretPos(), 0u);
}

TEST(CEditBox, InitEditboxConfiguresBuffer) {
    cEditBox e;
    e.Init(0, 0, 100, 30, &g_basicImage, &g_focusImage);
    EXPECT_EQ(e.maxBytes(), 0u);
    e.InitEditbox(0, 32);
    EXPECT_EQ(e.maxBytes(), 32u);
    EXPECT_EQ(e.editText(), "");
}

TEST(CEditBox, InsertCharAppendsAndMovesCaret) {
    cEditBox e;
    e.Init(0, 0, 100, 30, &g_basicImage, &g_focusImage);
    e.InitEditbox(0, 32);
    e.ActionEvent(50, 15, 0);                // focus
    EXPECT_TRUE(e.hasFocus());
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::None), 'a');
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::None), 'b');
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::None), 'c');
    EXPECT_EQ(e.editText(), "abc");
    EXPECT_EQ(e.caretPos(), 3u);
}

TEST(CEditBox, BackspaceDeletesCharBeforeCaret) {
    cEditBox e;
    e.Init(0, 0, 100, 30, &g_basicImage, &g_focusImage);
    e.InitEditbox(0, 32);
    e.ActionEvent(50, 15, 0);
    e.ActionKeyboardEvent(0, 'a');
    e.ActionKeyboardEvent(0, 'b');
    e.ActionKeyboardEvent(0, 'c');
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::Back), 0);
    EXPECT_EQ(e.editText(), "ab");
    EXPECT_EQ(e.caretPos(), 2u);
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::Back), 0);
    EXPECT_EQ(e.editText(), "a");
    EXPECT_EQ(e.caretPos(), 1u);
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::Back), 0);
    EXPECT_EQ(e.editText(), "");
    EXPECT_EQ(e.caretPos(), 0u);
    // Backspace at the very start is a no-op, not a crash.
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::Back), 0);
    EXPECT_EQ(e.editText(), "");
}

TEST(CEditBox, DeleteRemovesCharAtCaret) {
    cEditBox e;
    e.Init(0, 0, 100, 30, &g_basicImage, &g_focusImage);
    e.InitEditbox(0, 32);
    e.ActionEvent(50, 15, 0);
    e.ActionKeyboardEvent(0, 'a');
    e.ActionKeyboardEvent(0, 'b');
    e.ActionKeyboardEvent(0, 'c');
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::Home), 0);
    EXPECT_EQ(e.caretPos(), 0u);
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::Delete), 0);
    EXPECT_EQ(e.editText(), "bc");
    EXPECT_EQ(e.caretPos(), 0u);
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::Delete), 0);
    EXPECT_EQ(e.editText(), "c");
    // Delete at the end is a no-op.
    e.SetCaretPos(1);
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::Delete), 0);
    EXPECT_EQ(e.editText(), "c");
}

TEST(CEditBox, ArrowKeysMoveCaret) {
    cEditBox e;
    e.Init(0, 0, 100, 30, &g_basicImage, &g_focusImage);
    e.InitEditbox(0, 32);
    e.ActionEvent(50, 15, 0);
    e.ActionKeyboardEvent(0, 'a');
    e.ActionKeyboardEvent(0, 'b');
    e.ActionKeyboardEvent(0, 'c');
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::Home), 0);
    EXPECT_EQ(e.caretPos(), 0u);
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::Right), 0);
    EXPECT_EQ(e.caretPos(), 1u);
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::End), 0);
    EXPECT_EQ(e.caretPos(), 3u);
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::Left), 0);
    EXPECT_EQ(e.caretPos(), 2u);
    // Right at end is a no-op; left at start is a no-op.
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::End), 0);
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::Right), 0);
    EXPECT_EQ(e.caretPos(), 3u);
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::Home), 0);
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::Left), 0);
    EXPECT_EQ(e.caretPos(), 0u);
}

TEST(CEditBox, EnterAndEscapeInvokeCallbacks) {
    cEditBox e;
    int enterCount = 0;
    int escCount = 0;
    e.Init(0, 0, 100, 30, &g_basicImage, &g_focusImage);
    e.InitEditbox(0, 32);
    e.ActionEvent(50, 15, 0);
    e.SetEnterFunc([&](cEditBox&, void*) { ++enterCount; });
    e.SetEscapeFunc([&](cEditBox&, void*) { ++escCount; });
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::Enter), 0);
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::Escape), 0);
    EXPECT_EQ(enterCount, 1);
    EXPECT_EQ(escCount, 1);
}

TEST(CEditBox, ReadOnlyRejectsInputButKeepsNavigation) {
    cEditBox e;
    e.Init(0, 0, 100, 30, &g_basicImage, &g_focusImage);
    e.InitEditbox(0, 32);
    e.ActionEvent(50, 15, 0);
    e.ActionKeyboardEvent(0, 'a');
    e.ActionKeyboardEvent(0, 'b');
    e.SetReadOnly(true);
    e.ActionKeyboardEvent(0, 'c');
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::Back), 0);
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::Delete), 0);
    EXPECT_EQ(e.editText(), "ab");
    // Navigation still works.
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::Home), 0);
    EXPECT_EQ(e.caretPos(), 0u);
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::End), 0);
    EXPECT_EQ(e.caretPos(), 2u);
}

TEST(CEditBox, SecretModeHidesDisplayText) {
    cEditBox e;
    e.Init(0, 0, 100, 30, &g_basicImage, &g_focusImage);
    e.InitEditbox(0, 32);
    e.SetEditText("hunter2");
    EXPECT_EQ(e.displayText(), "hunter2");
    e.SetSecret(true);
    EXPECT_EQ(e.displayText(), "*******");
    EXPECT_EQ(e.editText(), "hunter2");     // raw text is unaffected
}

TEST(CEditBox, SetEditTextReplacesContent) {
    cEditBox e;
    e.Init(0, 0, 100, 30, &g_basicImage, &g_focusImage);
    e.InitEditbox(0, 32);
    e.ActionEvent(50, 15, 0);
    e.ActionKeyboardEvent(0, 'X');
    e.SetEditText("hello world");
    EXPECT_EQ(e.editText(), "hello world");
    EXPECT_EQ(e.caretPos(), 11u);
}

TEST(CEditBox, SetEditTextRespectsCapacity) {
    cEditBox e;
    e.Init(0, 0, 100, 30, &g_basicImage, &g_focusImage);
    e.InitEditbox(0, 8);                     // 8 bytes incl. NUL → 7 chars
    e.SetEditText("ABCDEFGHIJ");
    EXPECT_EQ(e.editText().size(), 7u);
    EXPECT_EQ(e.editText(), "ABCDEFG");
}

TEST(CEditBox, TextChangeFiresCallback) {
    cEditBox e;
    int changeCount = 0;
    e.Init(0, 0, 100, 30, &g_basicImage, &g_focusImage);
    e.InitEditbox(0, 32);
    e.ActionEvent(50, 15, 0);
    e.SetEditFunc([&](cEditBox&, void*) { ++changeCount; });
    e.ActionKeyboardEvent(0, 'a');
    e.ActionKeyboardEvent(0, 'b');
    e.ActionKeyboardEvent(static_cast<std::int32_t>(Key::Back), 0);
    EXPECT_EQ(changeCount, 3);
    EXPECT_TRUE(e.textChanged());
    e.clearTextChanged();
    EXPECT_FALSE(e.textChanged());
}

TEST(CEditBox, BufferFullRejectsInsert) {
    cEditBox e;
    e.Init(0, 0, 100, 30, &g_basicImage, &g_focusImage);
    e.InitEditbox(0, 4);                     // 3 chars + NUL
    e.ActionEvent(50, 15, 0);
    e.ActionKeyboardEvent(0, 'a');
    e.ActionKeyboardEvent(0, 'b');
    e.ActionKeyboardEvent(0, 'c');
    e.ActionKeyboardEvent(0, 'd');          // full, must reject
    EXPECT_EQ(e.editText(), "abc");
}

TEST(CEditBox, ValidCheckDigitsOnly) {
    cEditBox e;
    e.Init(0, 0, 100, 30, &g_basicImage, &g_focusImage);
    e.InitEditbox(0, 32);
    e.ActionEvent(50, 15, 0);
    e.SetValidCheck(1);                       // digits only
    e.ActionKeyboardEvent(0, '1');
    e.ActionKeyboardEvent(0, 'a');           // rejected
    e.ActionKeyboardEvent(0, '2');
    e.ActionKeyboardEvent(0, 'B');           // rejected
    e.ActionKeyboardEvent(0, '3');
    EXPECT_EQ(e.editText(), "123");
}

TEST(CEditBox, ValidCheckAlphaOnly) {
    cEditBox e;
    e.Init(0, 0, 100, 30, &g_basicImage, &g_focusImage);
    e.InitEditbox(0, 32);
    e.ActionEvent(50, 15, 0);
    e.SetValidCheck(2);
    e.ActionKeyboardEvent(0, 'a');
    e.ActionKeyboardEvent(0, '1');           // rejected
    e.ActionKeyboardEvent(0, 'B');
    EXPECT_EQ(e.editText(), "aB");
}

TEST(CEditBox, FocusOnClick) {
    cEditBox e;
    e.Init(10, 20, 100, 30, &g_basicImage, &g_focusImage);
    e.InitEditbox(0, 32);
    EXPECT_FALSE(e.hasFocus());
    e.ActionEvent(50, 30, 0);                // click inside (10..110, 20..50)
    EXPECT_TRUE(e.hasFocus());
    e.ActionEvent(200, 200, 0);               // click outside
    EXPECT_FALSE(e.hasFocus());
}

TEST(CEditBox, DisabledIgnoresKeyboard) {
    cEditBox e;
    e.Init(0, 0, 100, 30, &g_basicImage, &g_focusImage);
    e.InitEditbox(0, 32);
    e.ActionEvent(50, 15, 0);
    e.SetDisable(true);
    EXPECT_FALSE(e.isEnabled());
    e.ActionKeyboardEvent(0, 'a');
    EXPECT_EQ(e.editText(), "");
}

TEST(CEditBox, ShowCaretInReadOnlySetterGetter) {
    // 1:1 with legacy cEditBox::ShowCaretInReadOnly: when read-only mode
    // is on, the caret is normally hidden (matches the typical password
    // field UX). The legacy engine flips this independently of
    // SetReadOnly so the application can show a "you can't edit this"
    // caret without making the box editable. Setter existed before this
    // fix but the getter did not (mirrors cButton 87e831a).
    cEditBox e;
    e.Init(0, 0, 100, 30, &g_basicImage, &g_focusImage);
    e.InitEditbox(0, 32);
    EXPECT_FALSE(e.showCaretInReadOnly());
    e.ShowCaretInReadOnly(true);
    EXPECT_TRUE(e.showCaretInReadOnly());
    // Toggle the other way; setter is a re-write, not an add.
    e.ShowCaretInReadOnly(false);
    EXPECT_FALSE(e.showCaretInReadOnly());
    // The flag must survive a re-Init (cEditBox::Init only resets the
    // widget's abs xy + images + id, not the read-only caret flag).
    e.ShowCaretInReadOnly(true);
    e.Init(0, 0, 100, 30, &g_basicImage, &g_focusImage, 99);
    EXPECT_TRUE(e.showCaretInReadOnly());
    EXPECT_EQ(e.id(), 99);
}

TEST(CEditBox, TextColorAndAlignSetters) {
    cEditBox e;
    e.Init(0, 0, 100, 30, &g_basicImage, &g_focusImage);
    e.InitEditbox(0, 32);
    e.SetActiveTextColor(0xFF000000);
    e.SetNonactiveTextColor(0xFF808080);
    e.SetTextOffset(5, 10, 2);
    e.SetAlign(cEditBox::TextAlign::Right);
    EXPECT_EQ(e.activeTextColor(),    0xFF000000u);
    EXPECT_EQ(e.nonactiveTextColor(), 0xFF808080u);
    EXPECT_EQ(e.textLeftOffset(),  5);
    EXPECT_EQ(e.textRightOffset(), 10);
    EXPECT_EQ(e.textTopOffset(),   2);
    EXPECT_EQ(e.textAlign(), cEditBox::TextAlign::Right);
}
