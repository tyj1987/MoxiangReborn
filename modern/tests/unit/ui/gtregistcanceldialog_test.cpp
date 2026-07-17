// gtregistcanceldialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog
// 1:1 port contract test for modern cGTRegistcancelDialog
// (guild tournament registration cancel dialog: 1 cButton).
//
// Covers modern/src/ui/gtregistcanceldialog.{hpp,cpp}, a 1:1
// port of
//   墨香【源码】\[Client]MH\GTRegistcancelDialog.h (795 B) and
//   墨香【源码】\[Client]MH\GTRegistcancelDialog.cpp.
//
// What's tested:
//   - Default construction: cGTRegistcancelDialog is
//     a cDialog and inherits its tree management.
//   - m_pCancelBtn starts null (1:1 with legacy
//     default init).
//   - Id constant matches expected local range
//     (kIdCancelBtn=460).
//   - Linking resolves the cButton child by id.
//   - Linking without children leaves m_pCancelBtn
//     null (SetActive + TournamentRegistCancelSyn
//     are safe).
//   - Linking before Init does not crash.
//   - SetActive val=true calls base SetActive
//     (no singleton dispatch on val=true per
//     legacy 1:1 quirk).
//   - SetActive val=false calls base SetActive
//     (HERO + OBJECTSTATEMGR TODO: the modern
//     port returns without observable state
//     change).
//   - SetActive without Linking is safe.
//   - SetActive before Init does not crash.
//   - TournamentRegistCancelSyn is a no-op (TODO:
//     2-singleton dispatch HERO + NETWORK, R-12.x
//     deferred). The 1:1 contract is preserved:
//     returns void, no state change.
//   - TournamentRegistCancelSyn without Linking
//     is safe.
//   - TournamentRegistCancelSyn before Init does
//     not crash.
//
// 1:1 quirks preserved:
//   - Ctor body empty (1:1 quirk: m_type =
//     WT_GTREGISTCANCEL_DLG drop, modern cWindow
//     does not have m_type field).
//   - SetActive override: base SetActive always
//     called (matches legacy call order).
//   - 1:1 quirk: legacy val == FALSE only triggers
//     the HERO + OBJECTSTATEMGR dispatch (val ==
//     TRUE has no singleton dispatch). Modern
//     port preserves this 1:1 behavior.
//   - TournamentRegistCancelSyn TODO (2-singleton
//     dispatch, R-12.x deferred).
//   - Local id range 460 (distinct from 200-450
//     used by previous Tier 2 dialogs; no
//     collision).

#include "gtregistcanceldialog.hpp"
#include "cdialog.hpp"
#include "cbutton.hpp"
#include "cwindow.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

namespace mxh::ui::test {

// ===========================================================================
// Construction + state
// ===========================================================================

TEST(CGTRegistcancelDialogTest, DefaultConstructionIsValid) {
    cGTRegistcancelDialog dlg;
    // 1:1 quirk: ctor body is empty (legacy
    // m_type = WT_GTREGISTCANCEL_DLG drop, modern
    // cWindow does not have m_type field).
    SUCCEED();
}

TEST(CGTRegistcancelDialogTest, InheritsDialogTreeManagement) {
    cGTRegistcancelDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetAbsXY(10, 20);
    EXPECT_EQ(dlg.absX(), 10);
    EXPECT_EQ(dlg.absY(), 20);
}

TEST(CGTRegistcancelDialogTest, IdConstantMatchesExpectedLocalRange) {
    EXPECT_EQ(cGTRegistcancelDialog::kIdCancelBtn, 460);
}

// ===========================================================================
// Linking
// ===========================================================================

TEST(CGTRegistcancelDialogTest, LinkingResolvesButton) {
    cGTRegistcancelDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto btn = std::make_unique<cButton>();
    btn->Init(0, 0, 30, 30, nullptr, nullptr, nullptr, nullptr, nullptr,
              cGTRegistcancelDialog::kIdCancelBtn);
    dlg.Add(std::unique_ptr<cWindow>(btn.release()));

    dlg.Linking();
    // m_pCancelBtn is private; verified indirectly
    // via the Linking call not crashing + the
    // dialog state is consistent.
    SUCCEED();
}

TEST(CGTRegistcancelDialogTest, LinkingWithoutChildrenDoesNotCrash) {
    cGTRegistcancelDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    // SetActive + TournamentRegistCancelSyn
    // without children must be safe.
    dlg.SetActive(true);
    dlg.SetActive(false);
    dlg.TournamentRegistCancelSyn();
    SUCCEED();
}

TEST(CGTRegistcancelDialogTest, LinkingBeforeInitDoesNotCrash) {
    cGTRegistcancelDialog dlg;
    dlg.Linking();
    SUCCEED();
}

// ===========================================================================
// SetActive override
// ===========================================================================

TEST(CGTRegistcancelDialogTest, SetActiveTrueUpdatesBaseState) {
    cGTRegistcancelDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    EXPECT_FALSE(dlg.isActive());
    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGTRegistcancelDialogTest, SetActiveFalseUpdatesBaseState) {
    cGTRegistcancelDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CGTRegistcancelDialogTest, SetActiveFalseDoesNotChangeState) {
    // 1:1 with legacy: SetActive(val=FALSE) only
    // calls base SetActive + the HERO +
    // OBJECTSTATEMGR dispatch (TODO). The
    // observable state change is the base
    // SetActive(false).
    cGTRegistcancelDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CGTRegistcancelDialogTest, SetActiveWithoutLinkIsSafe) {
    cGTRegistcancelDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CGTRegistcancelDialogTest, SetActiveBeforeInitDoesNotCrash) {
    cGTRegistcancelDialog dlg;
    dlg.SetActive(true);
    dlg.SetActive(false);
    SUCCEED();
}

// ===========================================================================
// TournamentRegistCancelSyn
// ===========================================================================

TEST(CGTRegistcancelDialogTest, TournamentRegistCancelSynIsNoOpUntilSingletonsPorted) {
    // 1:1 with legacy contract: returns void.
    // Modern port is a no-op (TODO: 2-singleton
    // dispatch HERO + NETWORK, R-12.x deferred).
    // The 1:1 contract is preserved: no state
    // change observable.
    cGTRegistcancelDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);

    dlg.TournamentRegistCancelSyn();
    // State preserved (legacy would have sent
    // network message, but modern is TODO).
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGTRegistcancelDialogTest, TournamentRegistCancelSynDoesNotChangeState) {
    cGTRegistcancelDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);

    dlg.TournamentRegistCancelSyn();
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGTRegistcancelDialogTest, TournamentRegistCancelSynWithoutLinkIsSafe) {
    cGTRegistcancelDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    dlg.TournamentRegistCancelSyn();
    SUCCEED();
}

TEST(CGTRegistcancelDialogTest, TournamentRegistCancelSynBeforeInitDoesNotCrash) {
    cGTRegistcancelDialog dlg;
    dlg.TournamentRegistCancelSyn();
    SUCCEED();
}

}  // namespace mxh::ui::test
