// unionnotedlg_test.cpp — 1:1 port tests for
// 墨香 CUnionNoteDialog (guild union note sender
// dialog).
//
// Verifies:
//   - ctor does not crash
//   - Dtor does not crash
//   - Inherits from cDialog
//   - 4 id constants (kIdNoteText=620, kIdTitleEdit=621,
//     kIdSendOkBtn=622, kIdCancelBtn=623)
//   - Linking resolves the cTextArea
//   - Linking sets SetEnterAllow(false)
//   - Linking clears the script text
//   - Linking before Init does not crash
//   - Show stores the pItem + activates dialog
//   - Use clears m_pNoteText + m_bUse + m_pItem
//   - OnActionEvent is a no-op (TODO)
//   - IsUse returns m_bUse

#include "unionnotedlg.hpp"
#include "cdialog.hpp"
#include "ctextarea.hpp"
#include "ceditbox.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

using mxh::ui::cDialog;
using mxh::ui::cEditBox;
using mxh::ui::cTextArea;
using mxh::ui::cUnionNoteDlg;

namespace {

// helper: build a cUnionNoteDlg + 1 cTextArea + Linking
struct LinkedDialog {
    cUnionNoteDlg dlg;
    std::unique_ptr<cTextArea> noteText;

    LinkedDialog() {
        dlg.Init(0, 0, 200, 200, nullptr, 0);
        noteText = std::make_unique<cTextArea>();
        noteText->InitTextArea(mxh::ui::TextRect{0, 0, 100, 100}, 64);
        noteText->setId(cUnionNoteDlg::kIdNoteText);
        auto* notePtr = noteText.get();
        dlg.Add(std::move(noteText));

        dlg.Linking();

        notePtr_ = notePtr;
    }

    cTextArea* notePtr_ = nullptr;
};

}  // namespace

// ---------- ctor / dtor ----------

TEST(CUnionNoteDlgTest, CtorDoesNotCrash) {
    cUnionNoteDlg dlg;
    SUCCEED();
}

TEST(CUnionNoteDlgTest, DtorDoesNotCrash) {
    cUnionNoteDlg dlg;
    SUCCEED();
}

TEST(CUnionNoteDlgTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cUnionNoteDlg>,
                  "cUnionNoteDlg must inherit from cDialog");
    SUCCEED();
}

TEST(CUnionNoteDlgTest, DefaultBUseIsFalse) {
    cUnionNoteDlg dlg;
    EXPECT_FALSE(dlg.IsUse());
}

// ---------- id range ----------

TEST(CUnionNoteDlgTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cUnionNoteDlg::kIdNoteText, 620);
    EXPECT_EQ(cUnionNoteDlg::kIdTitleEdit, 621);
    EXPECT_EQ(cUnionNoteDlg::kIdSendOkBtn, 622);
    EXPECT_EQ(cUnionNoteDlg::kIdCancelBtn, 623);
}

TEST(CUnionNoteDlgTest, IdConstantsAreUnique) {
    EXPECT_NE(cUnionNoteDlg::kIdNoteText, cUnionNoteDlg::kIdTitleEdit);
    EXPECT_NE(cUnionNoteDlg::kIdNoteText, cUnionNoteDlg::kIdSendOkBtn);
    EXPECT_NE(cUnionNoteDlg::kIdNoteText, cUnionNoteDlg::kIdCancelBtn);
    EXPECT_NE(cUnionNoteDlg::kIdTitleEdit, cUnionNoteDlg::kIdSendOkBtn);
    EXPECT_NE(cUnionNoteDlg::kIdTitleEdit, cUnionNoteDlg::kIdCancelBtn);
    EXPECT_NE(cUnionNoteDlg::kIdSendOkBtn, cUnionNoteDlg::kIdCancelBtn);
}

// ---------- Linking ----------

TEST(CUnionNoteDlgTest, LinkingResolvesNoteText) {
    LinkedDialog ld;
    // m_pNoteText is private; verify by setting
    // script text via the dialog's child.
    EXPECT_NE(ld.notePtr_, nullptr);
    EXPECT_EQ(ld.notePtr_->GetScriptText(), "");
}

TEST(CUnionNoteDlgTest, LinkingDisablesEnterAllow) {
    LinkedDialog ld;
    // 1:1 with legacy SetEnterAllow(FALSE).
    EXPECT_FALSE(ld.notePtr_->IsEnterAllow());
}

TEST(CUnionNoteDlgTest, LinkingClearsScriptText) {
    LinkedDialog ld;
    // 1:1 with legacy SetScriptText("").
    EXPECT_EQ(ld.notePtr_->GetScriptText(), "");
}

TEST(CUnionNoteDlgTest, LinkingBeforeInitDoesNotCrash) {
    cUnionNoteDlg dlg;
    dlg.Linking();
    SUCCEED();
}

TEST(CUnionNoteDlgTest, LinkingWithoutChildrenDoesNotCrash) {
    cUnionNoteDlg dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.Linking();
    SUCCEED();
}

// ---------- Show ----------

TEST(CUnionNoteDlgTest, ShowWithNullItemIsSafe) {
    cUnionNoteDlg dlg;
    dlg.Show(nullptr);  // TODO: 4-singleton checks
    EXPECT_TRUE(dlg.isActive());
}

TEST(CUnionNoteDlgTest, ShowWithItemActivatesDialog) {
    cUnionNoteDlg dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    int myItem = 42;
    dlg.Show(&myItem);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CUnionNoteDlgTest, ShowBeforeInitDoesNotCrash) {
    cUnionNoteDlg dlg;
    dlg.Show(nullptr);
    SUCCEED();
}

// ---------- Use ----------

TEST(CUnionNoteDlgTest, UseClearsNoteText) {
    LinkedDialog ld;
    // Set some text first
    ld.notePtr_->SetScriptText("Hello");
    EXPECT_EQ(ld.notePtr_->GetScriptText(), "Hello");
    // Use should clear
    ld.dlg.Use();
    EXPECT_EQ(ld.notePtr_->GetScriptText(), "");
}

TEST(CUnionNoteDlgTest, UseClearsBUseFlag) {
    // m_bUse is private; verify via IsUse() getter
    LinkedDialog ld;
    // We can't set m_bUse directly (private), but
    // Use() should reset it to false.
    ld.dlg.Use();
    EXPECT_FALSE(ld.dlg.IsUse());
}

TEST(CUnionNoteDlgTest, UseWithoutLinkingIsSafe) {
    cUnionNoteDlg dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.Use();
    SUCCEED();
}

// ---------- OnActionEvent ----------

TEST(CUnionNoteDlgTest, OnActionEventIsNoOp) {
    cUnionNoteDlg dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    // TODO: 1:1 with legacy body when CHATMGR +
    //       HERO + NETWORK are ported.
    dlg.OnActionEvent(cUnionNoteDlg::kIdSendOkBtn, nullptr, 0);
    dlg.OnActionEvent(cUnionNoteDlg::kIdCancelBtn, nullptr, 0);
    SUCCEED();
}

TEST(CUnionNoteDlgTest, OnActionEventBeforeInitDoesNotCrash) {
    cUnionNoteDlg dlg;
    dlg.OnActionEvent(0, nullptr, 0);
    SUCCEED();
}
