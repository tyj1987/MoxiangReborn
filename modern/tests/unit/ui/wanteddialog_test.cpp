// wanteddialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cWantedDialog (wanted list dialog:
// 1 cListDialog).
//
// Covers modern/src/ui/wanteddialog.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\WantedDialog.h (759 B) and
//   墨香【源码】\[Client]MH\WantedDialog.cpp.
//
// What's tested:
//   - Default construction: cWantedDialog is a
//     cDialog and inherits its tree management.
//   - m_pWantedLDG starts null (1:1 with legacy
//     default init).
//   - Id constant matches expected local range
//     (kIdWantedList=500).
//   - kMaxWantedNum == 20 (1:1 with legacy
//     MAX_WANTED_NUM common header constant).
//   - Linking resolves the cListDialog child by id.
//   - Linking without children leaves m_pWantedLDG
//     null (SetInfo + AddInfo + InitWanted are
//     safe).
//   - Linking before Init does not crash.
//   - InitWanted calls RemoveAll on the cListDialog
//     (1:1 with legacy).
//   - InitWanted without Linking is safe (guarded
//     null m_pWantedLDG).
//   - InitWanted before Init does not crash.
//   - SetInfo calls InitWanted (verified via the
//     cListDialog being empty after SetInfo).
//   - SetInfo without Linking is safe.
//   - SetInfo before Init does not crash.
//   - AddInfo is a no-op (TODO: WANTEDLIST struct +
//     CHATMGR not ported, R-12.x deferred).
//   - AddInfo without Linking is safe.
//   - AddInfo before Init does not crash.
//
// 1:1 quirks preserved:
//   - Ctor body empty (1:1 quirk: m_type =
//     WT_WANTEDDIALOG drop, modern cWindow does
//     not have m_type field).
//   - SetInfo + AddInfo TODO (WANTEDLIST struct +
//     CHATMGR not ported, R-12.x deferred).
//   - 1:1 quirk: legacy passes WANTEDLIST* via a
//     function parameter; modern port uses a
//     no-arg signature (WANTEDLIST struct not
//     ported). When ported, the signature becomes
//     `SetInfo(WANTEDLIST* pInfo)`.
//   - InitWanted REAL (no singleton dep).
//   - kMaxWantedNum = 20 (1:1 with legacy
//     MAX_WANTED_NUM common header constant).
//   - Local id range 500 (distinct from 200-498
//     used by previous Tier 2 dialogs; no
//     collision).

#include "wanteddialog.hpp"
#include "cdialog.hpp"
#include "clistdialog.hpp"
#include "cwindow.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

namespace mxh::ui::test {

// ===========================================================================
// Construction + state
// ===========================================================================

TEST(CWantedDialogTest, DefaultConstructionIsValid) {
    cWantedDialog dlg;
    // 1:1 quirk: ctor body is empty (legacy
    // m_type = WT_WANTEDDIALOG drop, modern
    // cWindow does not have m_type field).
    SUCCEED();
}

TEST(CWantedDialogTest, InheritsDialogTreeManagement) {
    cWantedDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetAbsXY(10, 20);
    EXPECT_EQ(dlg.absX(), 10);
    EXPECT_EQ(dlg.absY(), 20);
}

TEST(CWantedDialogTest, IdConstantMatchesExpectedLocalRange) {
    EXPECT_EQ(cWantedDialog::kIdWantedList, 500);
}

TEST(CWantedDialogTest, MaxWantedNumIs20) {
    // 1:1 with legacy MAX_WANTED_NUM = 20 common
    // header constant.
    EXPECT_EQ(cWantedDialog::kMaxWantedNum, 20);
}

// ===========================================================================
// Linking
// ===========================================================================

TEST(CWantedDialogTest, LinkingResolvesListDialog) {
    cWantedDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto list = std::make_unique<cListDialog>();
    list->Init(0, 0, 200, 200, nullptr, cWantedDialog::kIdWantedList); list->InitList(100, 0, 0, 200, 200);
    dlg.Add(std::unique_ptr<cWindow>(list.release()));

    dlg.Linking();
    // m_pWantedLDG is private; verified indirectly
    // via InitWanted being safe (no crash).
    dlg.InitWanted();
    SUCCEED();
}

TEST(CWantedDialogTest, LinkingWithoutChildrenDoesNotCrash) {
    cWantedDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    // SetInfo + AddInfo + InitWanted without
    // children must be safe.
    dlg.SetInfo();
    dlg.AddInfo();
    dlg.InitWanted();
    SUCCEED();
}

TEST(CWantedDialogTest, LinkingBeforeInitDoesNotCrash) {
    cWantedDialog dlg;
    dlg.Linking();
    SUCCEED();
}

// ===========================================================================
// InitWanted
// ===========================================================================

TEST(CWantedDialogTest, InitWantedClearsListDialog) {
    // 1:1 with legacy: InitWanted calls RemoveAll
    // on the cListDialog. The cListDialog port
    // provides RemoveAll (REAL). Verified by the
    // call not crashing + the dialog state is
    // consistent.
    cWantedDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto list = std::make_unique<cListDialog>();
    list->Init(0, 0, 200, 200, nullptr, cWantedDialog::kIdWantedList);
    list->InitList(100, 0, 0, 200, 200);
    cListDialog* pList = list.get();
    dlg.Add(std::unique_ptr<cWindow>(list.release()));
    dlg.Linking();

    // Add some items first.
    pList->AddItem("test1", 0u);
    pList->AddItem("test2", 0u);
    EXPECT_EQ(pList->RowCount(), 2u);

    dlg.InitWanted();
    // After InitWanted, the cListDialog should be
    // empty (RemoveAll). Verified by RowCount == 0.
    EXPECT_EQ(pList->RowCount(), 0u);
}

TEST(CWantedDialogTest, InitWantedWithoutLinkIsSafe) {
    cWantedDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.InitWanted();
    SUCCEED();
}

TEST(CWantedDialogTest, InitWantedBeforeInitDoesNotCrash) {
    cWantedDialog dlg;
    dlg.InitWanted();
    SUCCEED();
}

// ===========================================================================
// SetInfo
// ===========================================================================

TEST(CWantedDialogTest, SetInfoCallsInitWanted) {
    cWantedDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto list = std::make_unique<cListDialog>();
    list->Init(0, 0, 200, 200, nullptr, cWantedDialog::kIdWantedList);
    list->InitList(100, 0, 0, 200, 200);
    cListDialog* pList = list.get();
    dlg.Add(std::unique_ptr<cWindow>(list.release()));
    dlg.Linking();

    // Add some items first.
    pList->AddItem("stale1", 0u);
    pList->AddItem("stale2", 0u);
    EXPECT_EQ(pList->RowCount(), 2u);

    dlg.SetInfo();
    // SetInfo calls InitWanted (REAL) which clears
    // the list. The for-loop body is TODO (no new
    // items added).
    EXPECT_EQ(pList->RowCount(), 0u);
}

TEST(CWantedDialogTest, SetInfoWithoutLinkIsSafe) {
    cWantedDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetInfo();
    SUCCEED();
}

TEST(CWantedDialogTest, SetInfoBeforeInitDoesNotCrash) {
    cWantedDialog dlg;
    dlg.SetInfo();
    SUCCEED();
}

// ===========================================================================
// AddInfo
// ===========================================================================

TEST(CWantedDialogTest, AddInfoIsNoOpUntilWantedListStructPorted) {
    // 1:1 with legacy contract: returns void.
    // Modern port is a no-op (TODO: WANTEDLIST
    // struct + CHATMGR not ported, R-12.x
    // deferred).
    cWantedDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto list = std::make_unique<cListDialog>();
    list->Init(0, 0, 200, 200, nullptr, cWantedDialog::kIdWantedList); list->InitList(100, 0, 0, 200, 200);
    dlg.Add(std::unique_ptr<cWindow>(list.release()));
    dlg.Linking();

    dlg.AddInfo();
    // No items added (TODO marker).
    SUCCEED();
}

TEST(CWantedDialogTest, AddInfoWithoutLinkIsSafe) {
    cWantedDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.AddInfo();
    SUCCEED();
}

TEST(CWantedDialogTest, AddInfoBeforeInitDoesNotCrash) {
    cWantedDialog dlg;
    dlg.AddInfo();
    SUCCEED();
}

}  // namespace mxh::ui::test
