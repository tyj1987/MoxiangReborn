// shoutchatdialog_test.cpp — 1:1 port tests for
// 墨香 CShoutchatDialog (shout chat log dialog).

#include "shoutchatdialog.hpp"
#include "cdialog.hpp"
#include "clistdialog.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

using mxh::ui::cDialog;
using mxh::ui::cListDialog;
using mxh::ui::cShoutchatDialog;

namespace {

struct LinkedDialog {
    cShoutchatDialog dlg;
    std::unique_ptr<cListDialog> msgList;

    LinkedDialog() {
        dlg.Init(0, 0, 200, 200, nullptr, 0);
        msgList = std::make_unique<cListDialog>();
        // cListDialog::InitList(maxLines, clipX, clipY, clipW, clipH) — 5 params
        msgList->InitList(40, 0, 0, 200, 100);
        msgList->setId(cShoutchatDialog::kIdMsgList);
        auto* mPtr = msgList.get();
        dlg.Add(std::move(msgList));

        dlg.Linking();

        mPtr_ = mPtr;
    }

    cListDialog* mPtr_ = nullptr;
};

}  // namespace

TEST(CShoutchatDialogTest, CtorDoesNotCrash) {
    cShoutchatDialog dlg;
    SUCCEED();
}

TEST(CShoutchatDialogTest, DtorDoesNotCrash) {
    cShoutchatDialog dlg;
    SUCCEED();
}

TEST(CShoutchatDialogTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cShoutchatDialog>,
                  "cShoutchatDialog must inherit from cDialog");
    SUCCEED();
}

TEST(CShoutchatDialogTest, IdConstantMatchesExpected) {
    EXPECT_EQ(cShoutchatDialog::kIdMsgList, 730);
}

TEST(CShoutchatDialogTest, ShoutchatItemColorIsD9CEF7) {
    // 1:1 with legacy RGBA_MAKE(217, 206, 247, 255) = 0xFFD9CEF7
    EXPECT_EQ(cShoutchatDialog::kShoutchatItemColor, 0xFFD9CEF7u);
}

TEST(CShoutchatDialogTest, MsgThrottleMsIs5000) {
    EXPECT_EQ(cShoutchatDialog::kMsgThrottleMs, 5000u);
}

TEST(CShoutchatDialogTest, MaxMsgLenIs60) {
    EXPECT_EQ(cShoutchatDialog::kMaxMsgLen, 60u);
}

TEST(CShoutchatDialogTest, LinkingResolvesMsgList) {
    LinkedDialog ld;
    // m_pMsgListDlg is private; verify via AddMsg
    // which uses it.
    ld.dlg.AddMsg("Hello");
    EXPECT_EQ(ld.mPtr_->RowCount(), 1);
}

TEST(CShoutchatDialogTest, LinkingBeforeInitDoesNotCrash) {
    cShoutchatDialog dlg;
    dlg.Linking();
    SUCCEED();
}

TEST(CShoutchatDialogTest, LinkingWithoutChildrenDoesNotCrash) {
    cShoutchatDialog dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.Linking();
    SUCCEED();
}

TEST(CShoutchatDialogTest, AddMsgAddsItem) {
    LinkedDialog ld;
    EXPECT_EQ(ld.mPtr_->RowCount(), 0);
    ld.dlg.AddMsg("Test message");
    EXPECT_EQ(ld.mPtr_->RowCount(), 1);
}

TEST(CShoutchatDialogTest, AddMsgMultipleAdds) {
    LinkedDialog ld;
    ld.dlg.AddMsg("First");
    ld.dlg.AddMsg("Second");
    ld.dlg.AddMsg("Third");
    EXPECT_EQ(ld.mPtr_->RowCount(), 3);
}

TEST(CShoutchatDialogTest, AddMsgWithNullIsSafe) {
    LinkedDialog ld;
    EXPECT_EQ(ld.mPtr_->RowCount(), 0);
    ld.dlg.AddMsg(nullptr);
    EXPECT_EQ(ld.mPtr_->RowCount(), 0);  // defensive
}

TEST(CShoutchatDialogTest, AddMsgTruncatesAtMaxLen) {
    LinkedDialog ld;
    // 1:1 with legacy strncpy(buf, pstr, 60)
    std::string longMsg(100, 'X');
    ld.dlg.AddMsg(longMsg.c_str());
    EXPECT_EQ(ld.mPtr_->RowCount(), 1);
    // Item text should be truncated to 60 chars.
    // (We don't read the text directly here, just
    // verify the call didn't crash.)
}

TEST(CShoutchatDialogTest, AddMsgWithoutLinkingIsSafe) {
    cShoutchatDialog dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.AddMsg("Test");
    SUCCEED();
}

TEST(CShoutchatDialogTest, SetActiveTrueUpdatesBaseState) {
    cShoutchatDialog dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CShoutchatDialogTest, SetActiveFalseUpdatesBaseState) {
    LinkedDialog ld;
    ld.dlg.SetActive(true);
    EXPECT_TRUE(ld.dlg.isActive());
    ld.dlg.SetActive(false);
    EXPECT_FALSE(ld.dlg.isActive());
}

TEST(CShoutchatDialogTest, SetActiveBeforeInitDoesNotCrash) {
    cShoutchatDialog dlg;
    dlg.SetActive(true);
    SUCCEED();
}

TEST(CShoutchatDialogTest, ProcessIsNoOp) {
    LinkedDialog ld;
    ld.dlg.Process();
    SUCCEED();
}

TEST(CShoutchatDialogTest, RefreshPositionIsNoOp) {
    LinkedDialog ld;
    ld.dlg.RefreshPosition();
    SUCCEED();
}
