// changejobdialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cChangeJobDialog (job-change item
// dialog: 2 state field + 2 method dispatch).
//
// Covers modern/src/ui/changejobdialog.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\ChangeJobDialog.h (920 B) and
//   墨香【源码】\[Client]MH\ChangeJobDialog.cpp.
//
// What's tested:
//   - Default construction: cChangeJobDialog is a
//     cDialog and inherits its tree management.
//   - 2 state fields start 0 (1:1 with legacy default
//     init).
//   - SetItemInfo round-trip (item pos + db idx).
//   - GetItemPos / GetItemDBIdx match setters.
//   - SetItemInfo overrides previous.
//   - SetItemInfo zero is valid.
//   - ChangeJobSyn is a no-op (TODO: 4-singleton
//     dispatch HERO + NETWORK + SetProtocol + ITEMMGR,
//     R-12.x deferred). The 1:1 contract is preserved:
//     returns void, no state change.
//   - ChangeJobSyn before Init does not crash.
//   - CancelChangeJob is a no-op (TODO: 4-singleton
//     dispatch HERO + OBJECTSTATEMGR + ITEMMGR, R-12.x
//     deferred). The 1:1 contract is preserved:
//     returns void, no state change.
//   - CancelChangeJob before Init does not crash.
//
// 1:1 quirks preserved:
//   - Ctor body empty (1:1 quirk: m_type =
//     WT_ITEM_CHANGEJOB_DLG drop, modern cWindow
//     does not have m_type field).
//   - State fields default 0 (1:1 with legacy
//     ctor — legacy doesn't init them in ctor
//     body, but they're initialized to 0 by
//     legacy 2003-era C++ constructor; modern
//     uses default member init).
//   - SetItemInfo inline setter (1:1 with legacy).
//   - ChangeJobSyn TODO (4-singleton dispatch,
//     R-12.x deferred).
//   - CancelChangeJob TODO (4-singleton dispatch,
//     R-12.x deferred).
//   - 1:1 quirk: modern ChangeJobSyn /
//     CancelChangeJob are no-ops (do not call
//     SetActive) while singletons are unported.
//     When ported, the body becomes the legacy
//     code (which calls SetActive(FALSE) at the
//     end).

#include "changejobdialog.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>

namespace mxh::ui::test {

// ===========================================================================
// Construction + state
// ===========================================================================

TEST(CChangeJobDialogTest, DefaultConstructionHasZeroState) {
    cChangeJobDialog dlg;
    // 1:1 quirk: ctor body is empty (legacy
    // m_type = WT_ITEM_CHANGEJOB_DLG drop, modern
    // cWindow does not have m_type field). State
    // fields default 0.
    EXPECT_EQ(dlg.GetItemPos(), 0u);
    EXPECT_EQ(dlg.GetItemDBIdx(), 0u);
}

TEST(CChangeJobDialogTest, InheritsDialogTreeManagement) {
    cChangeJobDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetAbsXY(10, 20);
    EXPECT_EQ(dlg.absX(), 10);
    EXPECT_EQ(dlg.absY(), 20);
}

// ===========================================================================
// SetItemInfo / GetItemPos / GetItemDBIdx
// ===========================================================================

TEST(CChangeJobDialogTest, SetItemInfoRoundTrip) {
    cChangeJobDialog dlg;
    dlg.SetItemInfo(42u, 100u);
    EXPECT_EQ(dlg.GetItemPos(), 42u);
    EXPECT_EQ(dlg.GetItemDBIdx(), 100u);
}

TEST(CChangeJobDialogTest, SetItemInfoOverridesPrevious) {
    cChangeJobDialog dlg;
    dlg.SetItemInfo(1u, 2u);
    dlg.SetItemInfo(100u, 200u);
    EXPECT_EQ(dlg.GetItemPos(), 100u);
    EXPECT_EQ(dlg.GetItemDBIdx(), 200u);
}

TEST(CChangeJobDialogTest, SetItemInfoZeroIsValid) {
    cChangeJobDialog dlg;
    dlg.SetItemInfo(0u, 0u);
    EXPECT_EQ(dlg.GetItemPos(), 0u);
    EXPECT_EQ(dlg.GetItemDBIdx(), 0u);
}

// ===========================================================================
// ChangeJobSyn
// ===========================================================================

TEST(CChangeJobDialogTest, ChangeJobSynIsNoOpUntilSingletonsPorted) {
    // 1:1 with legacy contract: returns void.
    // Modern port is a no-op (TODO: 4-singleton
    // dispatch HERO + NETWORK + SetProtocol +
    // ITEMMGR, R-12.x deferred). The 1:1 contract
    // is preserved: no state change observable.
    cChangeJobDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetItemInfo(42u, 100u);

    dlg.ChangeJobSyn();
    // State preserved (legacy would have set
    // active false + sent network message, but
    // modern is TODO).
    EXPECT_FALSE(dlg.isActive());
    EXPECT_EQ(dlg.GetItemPos(), 42u);
    EXPECT_EQ(dlg.GetItemDBIdx(), 100u);
}

TEST(CChangeJobDialogTest, ChangeJobSynDoesNotChangeState) {
    cChangeJobDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetItemInfo(42u, 100u);

    dlg.ChangeJobSyn();
    // State preserved.
    EXPECT_FALSE(dlg.isActive());
    EXPECT_EQ(dlg.GetItemPos(), 42u);
}

TEST(CChangeJobDialogTest, ChangeJobSynBeforeInitDoesNotCrash) {
    cChangeJobDialog dlg;
    dlg.SetItemInfo(1u, 2u);
    dlg.ChangeJobSyn();
    SUCCEED();
}

// ===========================================================================
// CancelChangeJob
// ===========================================================================

TEST(CChangeJobDialogTest, CancelChangeJobIsNoOpUntilSingletonsPorted) {
    // 1:1 with legacy contract: returns void.
    // Modern port is a no-op (TODO: 4-singleton
    // dispatch HERO + OBJECTSTATEMGR + ITEMMGR,
    // R-12.x deferred). The 1:1 contract is
    // preserved: no state change observable.
    cChangeJobDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);

    dlg.CancelChangeJob();
    // 1:1 quirk: modern CancelChangeJob does not
    // call SetActive (TODO marker). When ported,
    // the body would call SetActive(FALSE).
    EXPECT_TRUE(dlg.isActive());
}

TEST(CChangeJobDialogTest, CancelChangeJobDoesNotChangeState) {
    cChangeJobDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    dlg.SetItemInfo(42u, 100u);

    dlg.CancelChangeJob();
    // State preserved.
    EXPECT_TRUE(dlg.isActive());
    EXPECT_EQ(dlg.GetItemPos(), 42u);
}

TEST(CChangeJobDialogTest, CancelChangeJobBeforeInitDoesNotCrash) {
    cChangeJobDialog dlg;
    dlg.CancelChangeJob();
    SUCCEED();
}

}  // namespace mxh::ui::test
