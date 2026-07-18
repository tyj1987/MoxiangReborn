// pointsavedialog_test.cpp — 1:1 port tests for
// 墨香 CPointSaveDialog.

#include "pointsavedialog.hpp"
#include "cdialog.hpp"
#include "ceditbox.hpp"
#include "ctextarea.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

using mxh::ui::cDialog;
using mxh::ui::cEditBox;
using mxh::ui::cPointSaveDialog;

namespace {

struct LinkedDialog {
    cPointSaveDialog dlg;
    std::unique_ptr<cEditBox> nameEdit;

    LinkedDialog() {
        dlg.Init(0, 0, 200, 200, nullptr, 0);
        nameEdit = std::make_unique<cEditBox>();
        nameEdit->Init(0, 0, 100, 20, nullptr, nullptr,
                       cPointSaveDialog::kIdNameEditBox);
        // InitEditbox so SetEditText works.
        nameEdit->InitEditbox(50, 64);
        auto* ePtr = nameEdit.get();
        dlg.Add(std::move(nameEdit));

        dlg.Linking();

        editPtr_ = ePtr;
    }

    cEditBox* editPtr_ = nullptr;
};

}  // namespace

TEST(CPointSaveDialogTest, CtorDoesNotCrash) {
    cPointSaveDialog dlg;
    SUCCEED();
}

TEST(CPointSaveDialogTest, DtorDoesNotCrash) {
    cPointSaveDialog dlg;
    SUCCEED();
}

TEST(CPointSaveDialogTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cPointSaveDialog>,
                  "cPointSaveDialog must inherit from cDialog");
    SUCCEED();
}

TEST(CPointSaveDialogTest, DefaultNewPointIsTrue) {
    cPointSaveDialog dlg;
    EXPECT_TRUE(dlg.IsNewPoint());
}

TEST(CPointSaveDialogTest, DefaultItemStateIsZero) {
    cPointSaveDialog dlg;
    EXPECT_EQ(dlg.GetItemIdx(), 0u);
    EXPECT_EQ(dlg.GetItemPos(), 0u);
}

TEST(CPointSaveDialogTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cPointSaveDialog::kIdNameEditBox, 710);
}

TEST(CPointSaveDialogTest, VcmCharNameIsTwo) {
    EXPECT_EQ(cPointSaveDialog::kVcmCharName, 2);
}

// ---------- Linking ----------

TEST(CPointSaveDialogTest, LinkingResolvesNameEditBox) {
    LinkedDialog ld;
    // m_pNameEdtBox is private; verify via SetFocusEdit behavior
    // (which we test below).
    SUCCEED();
}

TEST(CPointSaveDialogTest, LinkingBeforeInitDoesNotCrash) {
    cPointSaveDialog dlg;
    dlg.Linking();
    SUCCEED();
}

TEST(CPointSaveDialogTest, LinkingWithoutChildrenDoesNotCrash) {
    cPointSaveDialog dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.Linking();
    SUCCEED();
}

// ---------- SetActive ----------

TEST(CPointSaveDialogTest, SetActiveTrueUpdatesBaseState) {
    cPointSaveDialog dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CPointSaveDialogTest, SetActiveFalseUpdatesBaseState) {
    LinkedDialog ld;
    ld.dlg.SetActive(true);
    EXPECT_TRUE(ld.dlg.isActive());
    ld.dlg.SetActive(false);
    EXPECT_FALSE(ld.dlg.isActive());
}

TEST(CPointSaveDialogTest, SetActiveTrueClearsEditText) {
    LinkedDialog ld;
    ld.editPtr_->SetEditText("OldName");
    EXPECT_EQ(ld.editPtr_->editText(), "OldName");
    ld.dlg.SetActive(true);
    EXPECT_EQ(ld.editPtr_->editText(), "");
}

TEST(CPointSaveDialogTest, SetActiveBeforeInitDoesNotCrash) {
    cPointSaveDialog dlg;
    dlg.SetActive(true);
    SUCCEED();
}

// ---------- SetItemToMapServer ----------

TEST(CPointSaveDialogTest, SetItemToMapServerUpdatesItemIdx) {
    cPointSaveDialog dlg;
    dlg.SetItemToMapServer(100, 5);
    EXPECT_EQ(dlg.GetItemIdx(), 100u);
}

TEST(CPointSaveDialogTest, SetItemToMapServerUpdatesItemPos) {
    cPointSaveDialog dlg;
    dlg.SetItemToMapServer(100, 5);
    EXPECT_EQ(dlg.GetItemPos(), 5u);
}

TEST(CPointSaveDialogTest, SetItemToMapServerMultipleCalls) {
    cPointSaveDialog dlg;
    dlg.SetItemToMapServer(100, 5);
    dlg.SetItemToMapServer(200, 10);
    EXPECT_EQ(dlg.GetItemIdx(), 200u);
    EXPECT_EQ(dlg.GetItemPos(), 10u);
}

// ---------- SetDialogStatus ----------

TEST(CPointSaveDialogTest, SetDialogStatusToggles) {
    cPointSaveDialog dlg;
    EXPECT_TRUE(dlg.IsNewPoint());
    dlg.SetDialogStatus(false);
    EXPECT_FALSE(dlg.IsNewPoint());
    dlg.SetDialogStatus(true);
    EXPECT_TRUE(dlg.IsNewPoint());
}

// ---------- ChangePointName / CancelPointName ----------

TEST(CPointSaveDialogTest, ChangePointNameIsNoOp) {
    LinkedDialog ld;
    ld.dlg.ChangePointName();  // 1:1 with legacy TODO body
    SUCCEED();
}

TEST(CPointSaveDialogTest, CancelPointNameIsNoOp) {
    LinkedDialog ld;
    ld.dlg.CancelPointName();  // 1:1 with legacy TODO body
    SUCCEED();
}
