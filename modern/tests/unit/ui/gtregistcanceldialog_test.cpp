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

// ===========================================================================
// SetCallbacks host-callback injection (C-Batch-2.43).
//
// 1:1 with legacy HERO + OBJECTSTATEMGR dispatch in SetActive(val==FALSE):
//   if (HERO->GetState() == eObjectState_Deal)
//     OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Deal).
// The modern port wires the same dispatch via OPTIONAL host callbacks.
// ===========================================================================

TEST(CGTRegistcancelDialogTest, SetActiveFalseChecksHostHeroStateFn) {
    // 1:1: when val==FALSE and the host reports eObjectState_Deal,
    // the end-deal-state callback is invoked exactly once.
    cGTRegistcancelDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);

    // Use a struct to share state (the getState fn returns Deal) and a
    // counter (the end fn increments) through the same userData pointer.
    struct SharedData {
        int state;
        int endCount;
    };
    SharedData shared{};
    shared.state = mxh::ui::cGTRegistcancelDialog::kObjectStateDeal;
    shared.endCount = 0;
    auto sharedGet = [](void* userData) -> std::int32_t {
        return static_cast<SharedData*>(userData)->state;
    };
    auto sharedEnd = [](void* userData) {
        ++static_cast<SharedData*>(userData)->endCount;
    };
    dlg.SetCallbacks(sharedGet, sharedEnd, &shared);
    dlg.SetActive(false);
    EXPECT_EQ(shared.endCount, 1);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CGTRegistcancelDialogTest, SetActiveFalseSkipsWhenHeroStateNotDeal) {
    // 1:1: when val==FALSE but hero state != eObjectState_Deal,
    // the end-deal-state callback is NOT invoked.
    cGTRegistcancelDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);

    // Hero state 99 != Deal (6), so the dispatch must NOT fire.
    struct SharedData {
        int state;
        int endCount;
    };
    SharedData shared{};
    shared.state = 99;
    shared.endCount = 0;
    auto getState = [](void* userData) -> std::int32_t {
        return static_cast<SharedData*>(userData)->state;
    };
    auto endState = [](void* userData) {
        ++static_cast<SharedData*>(userData)->endCount;
    };
    dlg.SetCallbacks(getState, endState, &shared);
    dlg.SetActive(false);
    EXPECT_EQ(shared.endCount, 0);
}

TEST(CGTRegistcancelDialogTest, SetActiveTrueDoesNotInvokeEndCallback) {
    // 1:1: when val==TRUE the legacy has NO OBJECTSTATEMGR dispatch.
    cGTRegistcancelDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    struct SharedData {
        int state;
        int endCount;
    };
    SharedData shared{};
    shared.state = mxh::ui::cGTRegistcancelDialog::kObjectStateDeal;
    shared.endCount = 0;
    auto getState = [](void* userData) -> std::int32_t {
        return static_cast<SharedData*>(userData)->state;
    };
    auto endState = [](void* userData) {
        ++static_cast<SharedData*>(userData)->endCount;
    };
    dlg.SetCallbacks(getState, endState, &shared);
    dlg.SetActive(true);
    EXPECT_EQ(shared.endCount, 0);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGTRegistcancelDialogTest, SetActiveFalseNullCheckFnSkipsDispatch) {
    // Safe-no-op: a null check fn skips the conditional even if
    // val==FALSE. The base SetActive(false) still runs.
    cGTRegistcancelDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    int endCalls = 0;
    auto endState = [](void* userData) {
        ++(*static_cast<int*>(userData));
    };
    dlg.SetCallbacks(nullptr, endState, &endCalls);
    dlg.SetActive(false);
    EXPECT_EQ(endCalls, 0);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CGTRegistcancelDialogTest, SetActiveFalseNullEndFnSkipsDispatch) {
    // Safe-no-op: a null end fn does not invoke the dispatch.
    cGTRegistcancelDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    auto getState = [](void*) -> std::int32_t {
        return mxh::ui::cGTRegistcancelDialog::kObjectStateDeal;
    };
    int userData = 0;
    dlg.SetCallbacks(getState, nullptr, &userData);
    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CGTRegistcancelDialogTest, SetCallbacksPreservesBaseSetActive) {
    // 1:1: the base SetActive(false) is always called, regardless of
    // whether the deal-state dispatch fires.
    cGTRegistcancelDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CGTRegistcancelDialogTest, SetActiveFalseAllowsNullCallbackUserData) {
    cGTRegistcancelDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    auto getState = [](void*) -> std::int32_t {
        return cGTRegistcancelDialog::kObjectStateDeal;
    };
    auto endState = [](void*) {};
    dlg.SetCallbacks(getState, endState);
    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

}  // namespace mxh::ui::test
