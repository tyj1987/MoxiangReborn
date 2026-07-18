// guildnotedlg_test.cpp — 1:1 port tests for
// 墨香 CGuildNoteDialog (guild note sender).

#include "guildnotedlg.hpp"
#include "cdialog.hpp"
#include "ctextarea.hpp"
#include "ceditbox.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

using mxh::ui::cDialog;
using mxh::ui::cEditBox;
using mxh::ui::cGuildNoteDlg;
using mxh::ui::cTextArea;

namespace {

struct LinkedDialog {
    cGuildNoteDlg dlg;
    std::unique_ptr<cTextArea> noteText;

    LinkedDialog() {
        dlg.Init(0, 0, 200, 200, nullptr, 0);
        noteText = std::make_unique<cTextArea>();
        noteText->InitTextArea(mxh::ui::TextRect{0, 0, 100, 100}, 64);
        noteText->setId(cGuildNoteDlg::kIdNoteText);
        auto* notePtr = noteText.get();
        dlg.Add(std::move(noteText));

        dlg.Linking();

        notePtr_ = notePtr;
    }

    cTextArea* notePtr_ = nullptr;
};

}  // namespace

TEST(CGuildNoteDlgTest, CtorDoesNotCrash) {
    cGuildNoteDlg dlg;
    SUCCEED();
}

TEST(CGuildNoteDlgTest, DtorDoesNotCrash) {
    cGuildNoteDlg dlg;
    SUCCEED();
}

TEST(CGuildNoteDlgTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cGuildNoteDlg>,
                  "cGuildNoteDlg must inherit from cDialog");
    SUCCEED();
}

TEST(CGuildNoteDlgTest, DefaultBUseIsFalse) {
    cGuildNoteDlg dlg;
    EXPECT_FALSE(dlg.IsUse());
}

TEST(CGuildNoteDlgTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cGuildNoteDlg::kIdNoteText, 700);
    EXPECT_EQ(cGuildNoteDlg::kIdTitleEdit, 701);
    EXPECT_EQ(cGuildNoteDlg::kIdSendOkBtn, 702);
    EXPECT_EQ(cGuildNoteDlg::kIdCancelBtn, 703);
}

TEST(CGuildNoteDlgTest, IdConstantsAreUnique) {
    EXPECT_NE(cGuildNoteDlg::kIdNoteText, cGuildNoteDlg::kIdTitleEdit);
    EXPECT_NE(cGuildNoteDlg::kIdNoteText, cGuildNoteDlg::kIdSendOkBtn);
    EXPECT_NE(cGuildNoteDlg::kIdNoteText, cGuildNoteDlg::kIdCancelBtn);
    EXPECT_NE(cGuildNoteDlg::kIdTitleEdit, cGuildNoteDlg::kIdSendOkBtn);
    EXPECT_NE(cGuildNoteDlg::kIdTitleEdit, cGuildNoteDlg::kIdCancelBtn);
    EXPECT_NE(cGuildNoteDlg::kIdSendOkBtn, cGuildNoteDlg::kIdCancelBtn);
}

TEST(CGuildNoteDlgTest, LinkingResolvesNoteText) {
    LinkedDialog ld;
    EXPECT_EQ(ld.notePtr_->GetScriptText(), "");
}

TEST(CGuildNoteDlgTest, LinkingDisablesEnterAllow) {
    LinkedDialog ld;
    EXPECT_FALSE(ld.notePtr_->IsEnterAllow());
}

TEST(CGuildNoteDlgTest, LinkingBeforeInitDoesNotCrash) {
    cGuildNoteDlg dlg;
    dlg.Linking();
    SUCCEED();
}

TEST(CGuildNoteDlgTest, LinkingWithoutChildrenDoesNotCrash) {
    cGuildNoteDlg dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.Linking();
    SUCCEED();
}

TEST(CGuildNoteDlgTest, ShowWithNullItemIsSafe) {
    cGuildNoteDlg dlg;
    dlg.Show(nullptr);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGuildNoteDlgTest, ShowWithItemActivatesDialog) {
    cGuildNoteDlg dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    int myItem = 42;
    dlg.Show(&myItem);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGuildNoteDlgTest, ShowBeforeInitDoesNotCrash) {
    cGuildNoteDlg dlg;
    dlg.Show(nullptr);
    SUCCEED();
}

TEST(CGuildNoteDlgTest, UseClearsNoteText) {
    LinkedDialog ld;
    ld.notePtr_->SetScriptText("Hello");
    ld.dlg.Use();
    EXPECT_EQ(ld.notePtr_->GetScriptText(), "");
}

TEST(CGuildNoteDlgTest, UseClearsBUseFlag) {
    LinkedDialog ld;
    ld.dlg.Use();
    EXPECT_FALSE(ld.dlg.IsUse());
}

TEST(CGuildNoteDlgTest, UseWithoutLinkingIsSafe) {
    cGuildNoteDlg dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.Use();
    SUCCEED();
}

TEST(CGuildNoteDlgTest, OnActionEventIsNoOp) {
    cGuildNoteDlg dlg;
    dlg.OnActionEvent(cGuildNoteDlg::kIdSendOkBtn, nullptr, 0);
    dlg.OnActionEvent(0, nullptr, 0);
    SUCCEED();
}

TEST(CGuildNoteDlgTest, OnActionEventBeforeInitDoesNotCrash) {
    cGuildNoteDlg dlg;
    dlg.OnActionEvent(0, nullptr, 0);
    SUCCEED();
}
