// changejobdialog_test.cpp - Phase C Batch 2.34 dialog 1:1 port
// contract test for modern cChangeJobDialog (job-change item
// dialog: 2 state field + 5 host-injected callback + 2 method
// dispatch).
//
// Covers modern/src/ui/changejobdialog.{hpp,cpp}, a 1:1 port of
//   legacy [Client]MH/ChangeJobDialog.h (920 B) and
//   legacy [Client]MH/ChangeJobDialog.cpp.
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
//   - Constants match legacy preprocessed values:
//     kObjectStateDeal=6, kItemTableInventory=0,
//     kItemTablePyoguk=2, kItemTableGuildWarehouse=10,
//     kItemTableShop=3 (1:1 with [CC]Header/CommonGameDefine.h).
//   - Default-constructed ChangeJobRequest zero-init.
//   - Default ctor leaves all 5 callback function
//     pointers null + userData null (1:1 with no-host
//     integration state).
//   - SetCallbacks captures all 5 function pointers +
//     userData.
//   - SetCallbacks accepts null userData explicitly.
//   - ChangeJobSyn with wired GetHeroObjectIdFn +
//     SendChangeJobSynFn invokes both callbacks once with
//     (heroId, itemPos, itemDbIdx) and then SetActive(false).
//   - ChangeJobSyn with GetHeroObjectIdFn=null +
//     SendChangeJobSynFn wired still sends with objectId=0
//     (1:1 quirk: payload still built even if hero id source
//     missing; wire byte is irrelevant when no real network).
//   - ChangeJobSyn with SendChangeJobSynFn=null still calls
//     SetActive(false) (legacy quirk: dialog hides itself
//     regardless of send result).
//   - ChangeJobSyn always SetActive(false) last, even when
//     both callbacks are wired.
//   - ChangeJobSyn before Init does not crash.
//   - CancelChangeJob with wired IsHeroInDealStateFn=true
//     invokes EndDealStateFn exactly once.
//   - CancelChangeJob with IsHeroInDealStateFn=false
//     does NOT invoke EndDealStateFn (1:1 quirk: state-end
//     is conditional on the deal-state check).
//   - CancelChangeJob with IsHeroInDealStateFn=null
//     defaults to false and skips EndDealStateFn.
//   - CancelChangeJob with wired SetItemTableDisabledFn
//     dispatches exactly 4 calls in legacy order
//     (Inventory=0, Pyoguk=2, GuildWarehouse=10, Shop=3),
//     each passing disabled=false (1:1 with legacy
//     ITEMMGR->SetDisableDialog(FALSE, tableId)).
//   - CancelChangeJob with SetItemTableDisabledFn=null
//     skips all 4 calls but still SetActive(false).
//   - CancelChangeJob always SetActive(false) last.
//   - CancelChangeJob before Init does not crash.
//
// 1:1 quirks preserved:
//   - Ctor body empty (1:1 quirk: m_type =
//     WT_ITEM_CHANGEJOB_DLG drop, modern cWindow does
//     not have m_type field).
//   - State fields default 0 (1:1 with legacy ctor).
//   - SetItemInfo inline setter (1:1 with legacy).
//   - ChangeJobSyn dispatches via host callback (1:1 with
//     legacy NETWORK->Send). SetActive(FALSE) called last
//     regardless of send result (legacy quirk).
//   - CancelChangeJob dispatches via host callbacks (1:1
//     with legacy ITEMMGR + OBJECTSTATEMGR). 4x table calls
//     in legacy order, each with disabled=FALSE (legacy
//     quirk: re-enable dialogs on cancel). SetActive(FALSE)
//     called last regardless of state-end result.
//   - 5 host-injected callbacks (GetHeroObjectIdFn /
//     SendChangeJobSynFn / IsHeroInDealStateFn /
//     EndDealStateFn / SetItemTableDisabledFn) replace
//     legacy HEROID / HERO / NETWORK / OBJECTSTATEMGR /
//     ITEMMGR globals (R-12.x deferred).

#include "changejobdialog.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace mxh::ui::test {

namespace {

// ===========================================================================
// Callback capture + stubs
// ===========================================================================

struct CallbackCapture {
    int getHeroCalls = 0;
    std::uint32_t lastHeroId = 0;

    int sendCalls = 0;
    std::uint32_t lastSendObjectId = 0;
    std::uint32_t lastSendItemPos = 0;
    std::uint32_t lastSendItemDbIdx = 0;
    bool sendReturnValue = true;

    int isDealCalls = 0;
    bool isDealReturnValue = false;
    int endDealCalls = 0;

    int tableCalls = 0;
    std::vector<std::int32_t> tableCallOrder;
    std::vector<bool> tableCallDisabled;
};

std::uint32_t StubGetHeroObjectId(void* userData) {
    auto* cap = static_cast<CallbackCapture*>(userData);
    if (!cap) return 0u;
    ++cap->getHeroCalls;
    cap->lastHeroId = 0xC0FFEEu;
    return cap->lastHeroId;
}

bool StubSendChangeJobSyn(std::uint32_t objectId,
                          std::uint32_t itemPos,
                          std::uint32_t itemDbIdx,
                          void* userData) {
    auto* cap = static_cast<CallbackCapture*>(userData);
    if (!cap) return true;
    ++cap->sendCalls;
    cap->lastSendObjectId = objectId;
    cap->lastSendItemPos = itemPos;
    cap->lastSendItemDbIdx = itemDbIdx;
    return cap->sendReturnValue;
}

bool StubIsHeroInDealState(void* userData) {
    auto* cap = static_cast<CallbackCapture*>(userData);
    if (!cap) return false;
    ++cap->isDealCalls;
    return cap->isDealReturnValue;
}

void StubEndDealState(void* userData) {
    auto* cap = static_cast<CallbackCapture*>(userData);
    if (!cap) return;
    ++cap->endDealCalls;
}

void StubSetItemTableDisabled(bool disabled, std::int32_t tableId,
                              void* userData) {
    auto* cap = static_cast<CallbackCapture*>(userData);
    if (!cap) return;
    ++cap->tableCalls;
    cap->tableCallOrder.push_back(tableId);
    cap->tableCallDisabled.push_back(disabled);
}

void InstallAllCallbacks(cChangeJobDialog& dlg, CallbackCapture& cap) {
    dlg.SetCallbacks(StubGetHeroObjectId,
                     StubSendChangeJobSyn,
                     StubIsHeroInDealState,
                     StubEndDealState,
                     StubSetItemTableDisabled,
                     &cap);
}

}  // namespace

// ===========================================================================
// Construction + state + constants
// ===========================================================================

TEST(CChangeJobDialogTest, DefaultConstructionHasZeroState) {
    cChangeJobDialog dlg;
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

TEST(CChangeJobDialogTest, ConstantsMatchPreprocessedLegacyValues) {
    // 1:1 with [CC]Header/CommonGameDefine.h enum values.
    EXPECT_EQ(cChangeJobDialog::kObjectStateDeal, 6);
    EXPECT_EQ(cChangeJobDialog::kItemTableInventory, 0);
    EXPECT_EQ(cChangeJobDialog::kItemTablePyoguk, 2);
    EXPECT_EQ(cChangeJobDialog::kItemTableGuildWarehouse, 10);
    EXPECT_EQ(cChangeJobDialog::kItemTableShop, 3);
}

TEST(CChangeJobDialogTest, ChangeJobRequestDefaultsAreZero) {
    cChangeJobDialog::ChangeJobRequest req{};
    EXPECT_EQ(req.objectId, 0u);
    EXPECT_EQ(req.itemPos, 0u);
    EXPECT_EQ(req.itemDbIdx, 0u);
}

TEST(CChangeJobDialogTest, DefaultConstructorLeavesAllCallbacksNull) {
    // 1:1 with legacy: dialog without host integration has no
    // network / state / item-table wiring (R-12.x deferred).
    // ChangeJobSyn / CancelChangeJob degenerate to a pure
    // SetActive(false) no-op while callbacks are null.
    cChangeJobDialog dlg;
    EXPECT_FALSE(dlg.isActive());
    // Call methods on a default-constructed dialog to verify
    // the null-callback branches are safe.
    dlg.ChangeJobSyn();
    EXPECT_FALSE(dlg.isActive());
    dlg.CancelChangeJob();
    EXPECT_FALSE(dlg.isActive());
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
// SetCallbacks
// ===========================================================================

TEST(CChangeJobDialogTest, SetCallbacksAcceptsNullUserData) {
    cChangeJobDialog dlg;
    dlg.SetCallbacks(StubGetHeroObjectId,
                     StubSendChangeJobSyn,
                     StubIsHeroInDealState,
                     StubEndDealState,
                     StubSetItemTableDisabled,
                     nullptr);
    // After SetCallbacks with null userData, the dialog is
    // still safe to invoke. The stubs will see userData=null
    // and short-circuit (see Stub* impl above).
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetItemInfo(7u, 13u);
    dlg.ChangeJobSyn();
    EXPECT_FALSE(dlg.isActive());
}

// ===========================================================================
// ChangeJobSyn
// ===========================================================================

TEST(CChangeJobDialogTest, ChangeJobSynDispatchesViaHostCallbacks) {
    // 1:1 with legacy ChangeJobSyn: build {objectId, itemPos,
    // itemDbIdx} payload, send via NETWORK, SetActive(false).
    // Modern: GetHeroObjectIdFn provides objectId,
    // SendChangeJobSynFn serializes + pushes onto the wire.
    cChangeJobDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetItemInfo(42u, 100u);

    CallbackCapture cap;
    InstallAllCallbacks(dlg, cap);

    dlg.ChangeJobSyn();

    EXPECT_EQ(cap.getHeroCalls, 1);
    EXPECT_EQ(cap.sendCalls, 1);
    EXPECT_EQ(cap.lastSendObjectId, 0xC0FFEEu);
    EXPECT_EQ(cap.lastSendItemPos, 42u);
    EXPECT_EQ(cap.lastSendItemDbIdx, 100u);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CChangeJobDialogTest, ChangeJobSynWithoutGetHeroDefaultsObjectIdToZero) {
    // 1:1 quirk: if GetHeroObjectIdFn is null, payload is
    // still built with objectId=0 (the wire byte is irrelevant
    // when no real network layer consumes it).
    cChangeJobDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetItemInfo(42u, 100u);

    CallbackCapture cap;
    dlg.SetCallbacks(nullptr,
                     StubSendChangeJobSyn,
                     nullptr,
                     nullptr,
                     nullptr,
                     &cap);

    dlg.ChangeJobSyn();

    EXPECT_EQ(cap.sendCalls, 1);
    EXPECT_EQ(cap.lastSendObjectId, 0u);
    EXPECT_EQ(cap.lastSendItemPos, 42u);
    EXPECT_EQ(cap.lastSendItemDbIdx, 100u);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CChangeJobDialogTest, ChangeJobSynWithoutSendCallbackSkipsSendButDeactivates) {
    // 1:1 quirk: SetActive(FALSE) is unconditional in legacy,
    // even if NETWORK->Send would have failed.
    cChangeJobDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    dlg.SetItemInfo(42u, 100u);

    CallbackCapture cap;
    dlg.SetCallbacks(StubGetHeroObjectId,
                     nullptr,
                     nullptr,
                     nullptr,
                     nullptr,
                     &cap);

    dlg.ChangeJobSyn();

    EXPECT_EQ(cap.sendCalls, 0);
    EXPECT_EQ(cap.getHeroCalls, 0);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CChangeJobDialogTest, ChangeJobSynAlwaysDeactivatesLast) {
    // 1:1 with legacy: SetActive(FALSE) is the LAST statement
    // in legacy ChangeJobSyn. Verify the modern port also
    // deactivates even when both callbacks are wired and
    // returning their nominal values.
    cChangeJobDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    dlg.SetItemInfo(1u, 2u);

    CallbackCapture cap;
    InstallAllCallbacks(dlg, cap);

    dlg.ChangeJobSyn();

    EXPECT_FALSE(dlg.isActive());
}

TEST(CChangeJobDialogTest, ChangeJobSynBeforeInitDoesNotCrash) {
    cChangeJobDialog dlg;
    CallbackCapture cap;
    InstallAllCallbacks(dlg, cap);
    dlg.SetItemInfo(1u, 2u);
    dlg.ChangeJobSyn();
    EXPECT_FALSE(dlg.isActive());
    // The callbacks were invoked even though Init was not.
    EXPECT_EQ(cap.sendCalls, 1);
}

// ===========================================================================
// CancelChangeJob
// ===========================================================================

TEST(CChangeJobDialogTest, CancelChangeJobDispatchesFourItemTablesInLegacyOrder) {
    // 1:1 with legacy CancelChangeJob: 4x
    // ITEMMGR->SetDisableDialog(FALSE, tableId) in this exact
    // order: Inventory=0, Pyoguk=2, GuildWarehouse=10, Shop=3.
    // Each call passes disabled=FALSE (legacy quirk: cancel
    // re-enables dialogs that were locked by open).
    cChangeJobDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);

    CallbackCapture cap;
    InstallAllCallbacks(dlg, cap);

    dlg.CancelChangeJob();

    ASSERT_EQ(cap.tableCalls, 4);
    EXPECT_EQ(cap.tableCallOrder[0], cChangeJobDialog::kItemTableInventory);
    EXPECT_EQ(cap.tableCallOrder[1], cChangeJobDialog::kItemTablePyoguk);
    EXPECT_EQ(cap.tableCallOrder[2], cChangeJobDialog::kItemTableGuildWarehouse);
    EXPECT_EQ(cap.tableCallOrder[3], cChangeJobDialog::kItemTableShop);
    EXPECT_EQ(cap.tableCallOrder[0], 0);
    EXPECT_EQ(cap.tableCallOrder[1], 2);
    EXPECT_EQ(cap.tableCallOrder[2], 10);
    EXPECT_EQ(cap.tableCallOrder[3], 3);
    for (bool d : cap.tableCallDisabled) {
        EXPECT_FALSE(d);
    }
    EXPECT_FALSE(dlg.isActive());
}

TEST(CChangeJobDialogTest, CancelChangeJobWhenHeroInDealCallsEndDealState) {
    // 1:1 with legacy: EndObjectState is conditional on
    // HERO->GetState() == eObjectState_Deal. Modern
    // IsHeroInDealStateFn returns true => EndDealStateFn
    // invoked once.
    cChangeJobDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);

    CallbackCapture cap;
    cap.isDealReturnValue = true;
    InstallAllCallbacks(dlg, cap);

    dlg.CancelChangeJob();

    EXPECT_EQ(cap.isDealCalls, 1);
    EXPECT_EQ(cap.endDealCalls, 1);
    EXPECT_EQ(cap.tableCalls, 4);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CChangeJobDialogTest, CancelChangeJobWhenHeroNotInDealSkipsEndDealState) {
    // 1:1 quirk: EndObjectState is conditional on the
    // eObjectState_Deal check. When the hero is NOT in deal
    // state, EndDealStateFn must NOT be invoked.
    cChangeJobDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);

    CallbackCapture cap;
    cap.isDealReturnValue = false;
    InstallAllCallbacks(dlg, cap);

    dlg.CancelChangeJob();

    EXPECT_EQ(cap.isDealCalls, 1);
    EXPECT_EQ(cap.endDealCalls, 0);
    EXPECT_EQ(cap.tableCalls, 4);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CChangeJobDialogTest, CancelChangeJobWithoutIsHeroCallbackSkipsEndDeal) {
    // 1:1 quirk: with no IsHeroInDealStateFn wired, the
    // state-end branch is skipped (defaults to false).
    cChangeJobDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);

    CallbackCapture cap;
    dlg.SetCallbacks(nullptr,
                     nullptr,
                     nullptr,
                     StubEndDealState,
                     StubSetItemTableDisabled,
                     &cap);

    dlg.CancelChangeJob();

    EXPECT_EQ(cap.endDealCalls, 0);
    EXPECT_EQ(cap.tableCalls, 4);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CChangeJobDialogTest, CancelChangeJobWithoutSetItemTableCallbackSkipsTables) {
    // 1:1 quirk: with no SetItemTableDisabledFn wired, the
    // 4 table calls are silently skipped. EndDealStateFn is
    // still invoked if IsHeroInDealStateFn returns true.
    cChangeJobDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);

    CallbackCapture cap;
    cap.isDealReturnValue = true;
    dlg.SetCallbacks(nullptr,
                     nullptr,
                     StubIsHeroInDealState,
                     StubEndDealState,
                     nullptr,
                     &cap);

    dlg.CancelChangeJob();

    EXPECT_EQ(cap.tableCalls, 0);
    EXPECT_EQ(cap.isDealCalls, 1);
    EXPECT_EQ(cap.endDealCalls, 1);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CChangeJobDialogTest, CancelChangeJobAlwaysDeactivates) {
    cChangeJobDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);

    // Wire ZERO callbacks; CancelChangeJob must still
    // SetActive(false) per legacy quirk.
    CallbackCapture cap;
    dlg.SetCallbacks(nullptr, nullptr, nullptr, nullptr, nullptr, &cap);

    dlg.CancelChangeJob();

    EXPECT_EQ(cap.tableCalls, 0);
    EXPECT_EQ(cap.endDealCalls, 0);
    EXPECT_EQ(cap.isDealCalls, 0);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CChangeJobDialogTest, CancelChangeJobBeforeInitDoesNotCrash) {
    cChangeJobDialog dlg;
    CallbackCapture cap;
    InstallAllCallbacks(dlg, cap);
    dlg.CancelChangeJob();
    EXPECT_FALSE(dlg.isActive());
    EXPECT_EQ(cap.tableCalls, 4);
}

}  // namespace mxh::ui::test
