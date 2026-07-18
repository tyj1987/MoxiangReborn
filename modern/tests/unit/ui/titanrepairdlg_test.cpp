// titanrepairdlg_test.cpp — 1:1 port verification tests for cTitanRepairDlg.

#include "titanrepairdlg.hpp"
#include "cdialog.hpp"
#include "cwindow.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

using mxh::ui::cTitanRepairDlg;
using mxh::ui::cDialog;
using mxh::ui::cWindow;
using mxh::ui::ECursorState;
using mxh::ui::kIdTitanRepairPart;
using mxh::ui::kIdTitanRepairAll;
using mxh::ui::kWeCloseWindow;
using mxh::ui::kChatMsgNoItemsToRepair;
using mxh::ui::kChatMsgRepairConfirm;

namespace {

std::unique_ptr<cTitanRepairDlg> MakeDialog() {
    auto d = std::make_unique<cTitanRepairDlg>();
    d->Init(0, 0, 200, 100, nullptr, 0);
    return d;
}

class CTitanRepairDlgTest : public ::testing::Test {
protected:
    void SetUp() override { cTitanRepairDlg::ClearTestInjections(); }
    void TearDown() override { cTitanRepairDlg::ClearTestInjections(); }
};

}  // namespace

// ---------------------------------------------------------------------------
// Constants + construction
// ---------------------------------------------------------------------------

TEST_F(CTitanRepairDlgTest, IdConstantsMatchLocalRange) {
    EXPECT_EQ(kIdTitanRepairPart, 2100);
    EXPECT_EQ(kIdTitanRepairAll,  2101);
}

TEST_F(CTitanRepairDlgTest, DefaultCursorIsDefault) {
    auto d = MakeDialog();
    EXPECT_EQ(d->cursorState(), ECursorState::Default);
}

TEST_F(CTitanRepairDlgTest, DefaultObjectStateDealIsNotEnded) {
    auto d = MakeDialog();
    EXPECT_FALSE(d->objectStateDealEnded());
}

TEST_F(CTitanRepairDlgTest, DefaultCountersAreZero) {
    auto d = MakeDialog();
    EXPECT_EQ(d->inventoryDialogCloseCount(), 0u);
    EXPECT_EQ(d->titanInventoryDialogCloseCount(), 0u);
    EXPECT_EQ(d->chatMsgNoItemsToRepairCount(), 0u);
    EXPECT_EQ(d->chatMsgRepairConfirmCount(), 0u);
    EXPECT_EQ(d->windowMgrMsgBoxCount(), 0u);
    EXPECT_EQ(d->titanMgrRepairCallCount(), 0u);
}

TEST_F(CTitanRepairDlgTest, ChatMsgConstantsMatchLegacyIds) {
    // 1:1 with legacy ChatManager.h message ids.
    EXPECT_EQ(kChatMsgNoItemsToRepair, 1582);
    EXPECT_EQ(kChatMsgRepairConfirm,   1543);
}

TEST_F(CTitanRepairDlgTest, DefaultTitanRepairCostIsZero) {
    auto d = MakeDialog();
    EXPECT_EQ(cTitanRepairDlg::titanRepairCostForTesting(), 0u);
}

// ---------------------------------------------------------------------------
// Linking
// ---------------------------------------------------------------------------

TEST_F(CTitanRepairDlgTest, LinkingIsNoOp) {
    // 1:1 quirk: legacy Linking() body is empty. Modern port
    // preserves the empty body — no children, no state changes.
    auto d = MakeDialog();
    d->Linking();
    EXPECT_EQ(d->cursorState(), ECursorState::Default);
    EXPECT_FALSE(d->objectStateDealEnded());
    EXPECT_EQ(d->inventoryDialogCloseCount(), 0u);
}

// ---------------------------------------------------------------------------
// SetActive override
// ---------------------------------------------------------------------------

TEST_F(CTitanRepairDlgTest, SetActiveTrueDelegatesToBase) {
    auto d = MakeDialog();
    d->SetActive(true);
    EXPECT_TRUE(d->isActive());
    // 1:1 quirk: legacy SetActive(TRUE) does NOT call
    // OBJECTSTATEMGR->EndObjectState / CURSOR->SetCursor —
    // only val==FALSE triggers the cascade. Modern port
    // preserves this: SetActive(true) leaves s_objectStateDealEnded
    // and s_cursor alone.
    EXPECT_FALSE(d->objectStateDealEnded());
}

TEST_F(CTitanRepairDlgTest, SetActiveFalseEndsObjectStateDeal) {
    // 1:1 with legacy: SetActive(FALSE) ends eObjectState_Deal
    // if active. Modern port: conservative "always end" for
    // test inspection (s_objectStateDealEnded = true).
    auto d = MakeDialog();
    d->SetActive(true);
    d->SetActive(false);
    EXPECT_FALSE(d->isActive());
    EXPECT_TRUE(d->objectStateDealEnded());
}

TEST_F(CTitanRepairDlgTest, SetActiveFalseResetsTitanRepairCursor) {
    // 1:1 with legacy: if cursor is eCURSOR_TITANREPAIR, reset
    // to eCURSOR_DEFAULT on val==FALSE.
    auto d = MakeDialog();
    // Simulate the user having set the cursor via TITAN_REPAIR_PART.
    d->OnActionEvent(kIdTitanRepairPart, nullptr, 0u);
    EXPECT_EQ(d->cursorState(), ECursorState::TitanRepair);
    d->SetActive(false);
    EXPECT_EQ(d->cursorState(), ECursorState::Default);
}

TEST_F(CTitanRepairDlgTest, SetActiveFalseLeavesDefaultCursorAlone) {
    // 1:1 quirk: legacy `if(CURSOR->GetCursor() == eCURSOR_TITANREPAIR)`
    // guards the cursor reset. Modern port preserves the guard:
    // if cursor is already DEFAULT, no reset.
    auto d = MakeDialog();
    d->SetActive(false);
    EXPECT_EQ(d->cursorState(), ECursorState::Default);
}

// ---------------------------------------------------------------------------
// OnActionEvent — WE_CLOSEWINDOW
// ---------------------------------------------------------------------------

TEST_F(CTitanRepairDlgTest, OnActionEventCloseWindowEndsObjectStateDeal) {
    // 1:1 with legacy: WE_CLOSEWINDOW ends eObjectState_Deal.
    auto d = MakeDialog();
    d->OnActionEvent(0, nullptr, kWeCloseWindow);
    EXPECT_TRUE(d->objectStateDealEnded());
}

TEST_F(CTitanRepairDlgTest, OnActionEventCloseWindowClosesInventoryDialogs) {
    // 1:1 with legacy: WE_CLOSEWINDOW closes both the
    // inventory and titan-inventory dialogs via GAMEIN.
    auto d = MakeDialog();
    d->OnActionEvent(0, nullptr, kWeCloseWindow);
    EXPECT_EQ(d->inventoryDialogCloseCount(), 1u);
    EXPECT_EQ(d->titanInventoryDialogCloseCount(), 1u);
}

TEST_F(CTitanRepairDlgTest, OnActionEventCloseWindowReturnsTrue) {
    // 1:1 quirk: legacy WE_CLOSEWINDOW branch returns TRUE.
    auto d = MakeDialog();
    EXPECT_TRUE(d->OnActionEvent(0, nullptr, kWeCloseWindow));
}

TEST_F(CTitanRepairDlgTest, OnActionEventCloseWindowDoesNotSelfClose) {
    // 1:1 quirk: legacy self-close
    //   //GAMEIN->GetTitanRepairDlg()->SetActive(FALSE);
    // is commented out (would infinite-loop). Modern port:
    // dialog is still active after the close-window event.
    auto d = MakeDialog();
    d->SetActive(true);
    d->OnActionEvent(0, nullptr, kWeCloseWindow);
    EXPECT_TRUE(d->isActive());
}

// ---------------------------------------------------------------------------
// OnActionEvent — TITAN_REPAIR_PART (cursor toggle + fall-through)
// ---------------------------------------------------------------------------

TEST_F(CTitanRepairDlgTest, OnActionEventPartTogglesCursorToTitanRepair) {
    // 1:1 with legacy: cursor toggles from DEFAULT to
    // TITANREPAIR on PART click.
    auto d = MakeDialog();
    EXPECT_EQ(d->cursorState(), ECursorState::Default);
    d->OnActionEvent(kIdTitanRepairPart, nullptr, 0u);
    EXPECT_EQ(d->cursorState(), ECursorState::TitanRepair);
}

TEST_F(CTitanRepairDlgTest, OnActionEventPartTogglesCursorBackToDefault) {
    // 1:1 with legacy: cursor toggles from TITANREPAIR back
    // to DEFAULT on second PART click.
    auto d = MakeDialog();
    d->OnActionEvent(kIdTitanRepairPart, nullptr, 0u);
    EXPECT_EQ(d->cursorState(), ECursorState::TitanRepair);
    d->OnActionEvent(kIdTitanRepairPart, nullptr, 0u);
    EXPECT_EQ(d->cursorState(), ECursorState::Default);
}

TEST_F(CTitanRepairDlgTest, OnActionEventPartFallsThroughToRepairAll) {
    // 1:1 quirk: legacy TITAN_REPAIR_PART switch has no break,
    // so the TITAN_REPAIR_ALL block executes too. Modern port
    // preserves this fall-through: PART click also calls
    // TITANMGR->GetTitanEnduranceTotalInfo (s_titanMgrRepairCallCount
    // increments).
    auto d = MakeDialog();
    d->OnActionEvent(kIdTitanRepairPart, nullptr, 0u);
    EXPECT_EQ(d->titanMgrRepairCallCount(), 1u);
}

TEST_F(CTitanRepairDlgTest, OnActionEventPartFallsThroughNoItems) {
    // 1:1 quirk: PART click falls through to TITAN_REPAIR_ALL,
    // which uses s_titanRepairCost. Default 0 → "no items to
    // repair" branch (CHATMGR msg 1582 stub).
    auto d = MakeDialog();
    d->OnActionEvent(kIdTitanRepairPart, nullptr, 0u);
    EXPECT_EQ(d->chatMsgNoItemsToRepairCount(), 1u);
    EXPECT_EQ(d->chatMsgRepairConfirmCount(), 0u);
    EXPECT_EQ(d->windowMgrMsgBoxCount(), 0u);
}

TEST_F(CTitanRepairDlgTest, OnActionEventPartFallsThroughWithItems) {
    // 1:1 quirk: PART click with non-zero cost → confirm msgbox
    // path (CHATMGR msg 1543 + WINDOWMGR MsgBox).
    auto d = MakeDialog();
    cTitanRepairDlg::SetTitanRepairCostForTesting(500u);
    d->OnActionEvent(kIdTitanRepairPart, nullptr, 0u);
    EXPECT_EQ(d->chatMsgRepairConfirmCount(), 1u);
    EXPECT_EQ(d->windowMgrMsgBoxCount(), 1u);
    EXPECT_EQ(d->chatMsgNoItemsToRepairCount(), 0u);
}

// ---------------------------------------------------------------------------
// OnActionEvent — TITAN_REPAIR_ALL
// ---------------------------------------------------------------------------

TEST_F(CTitanRepairDlgTest, OnActionEventAllCallsTitanMgr) {
    auto d = MakeDialog();
    d->OnActionEvent(kIdTitanRepairAll, nullptr, 0u);
    EXPECT_EQ(d->titanMgrRepairCallCount(), 1u);
}

TEST_F(CTitanRepairDlgTest, OnActionEventAllNoItemsTellsChat) {
    // 1:1 with legacy: dwMoney == 0 → CHATMGR msg 1582
    // (no items to repair). Modern port: s_chatMsgNoItemsCount++.
    auto d = MakeDialog();
    d->OnActionEvent(kIdTitanRepairAll, nullptr, 0u);
    EXPECT_EQ(d->chatMsgNoItemsToRepairCount(), 1u);
    EXPECT_EQ(d->windowMgrMsgBoxCount(), 0u);
}

TEST_F(CTitanRepairDlgTest, OnActionEventAllWithItemsShowsMsgBox) {
    // 1:1 with legacy: dwMoney != 0 → WINDOWMGR MsgBox with
    // CHATMGR msg 1543 + dwMoney. Modern port: s_windowMgrMsgBoxCount
    // + s_chatMsgRepairConfirmCount++.
    auto d = MakeDialog();
    cTitanRepairDlg::SetTitanRepairCostForTesting(1000u);
    d->OnActionEvent(kIdTitanRepairAll, nullptr, 0u);
    EXPECT_EQ(d->chatMsgRepairConfirmCount(), 1u);
    EXPECT_EQ(d->windowMgrMsgBoxCount(), 1u);
    EXPECT_EQ(d->chatMsgNoItemsToRepairCount(), 0u);
}

TEST_F(CTitanRepairDlgTest, OnActionEventAllReturnsTrue) {
    // 1:1 quirk: legacy returns TRUE at the end (after the
    // lId switch).
    auto d = MakeDialog();
    EXPECT_TRUE(d->OnActionEvent(kIdTitanRepairAll, nullptr, 0u));
}

TEST_F(CTitanRepairDlgTest, OnActionEventAllDoesNotToggleCursor) {
    // 1:1 quirk: legacy TITAN_REPAIR_ALL block has its own
    // body (no cursor toggle). Modern port: cursor stays
    // DEFAULT after ALL click.
    auto d = MakeDialog();
    d->OnActionEvent(kIdTitanRepairAll, nullptr, 0u);
    EXPECT_EQ(d->cursorState(), ECursorState::Default);
}

// ---------------------------------------------------------------------------
// OnActionEvent — unknown id / unknown event
// ---------------------------------------------------------------------------

TEST_F(CTitanRepairDlgTest, OnActionEventUnknownIdIsNoOp) {
    // 1:1 quirk: legacy second switch has no default branch.
    // Modern port: unknown lId → no cursor toggle, no TITANMGR
    // call, no chat/msgbox.
    auto d = MakeDialog();
    d->OnActionEvent(99999, nullptr, 0u);
    EXPECT_EQ(d->cursorState(), ECursorState::Default);
    EXPECT_EQ(d->titanMgrRepairCallCount(), 0u);
    EXPECT_EQ(d->chatMsgNoItemsToRepairCount(), 0u);
    EXPECT_EQ(d->chatMsgRepairConfirmCount(), 0u);
}

TEST_F(CTitanRepairDlgTest, OnActionEventUnknownEventIsNoOp) {
    // 1:1 quirk: legacy first switch has no default branch.
    // Modern port: unknown we → no close-window cascade, no
    // second switch execution.
    auto d = MakeDialog();
    d->OnActionEvent(0, nullptr,
        static_cast<std::uint32_t>(cWindow::WindowEvent::LButtonClick));
    EXPECT_FALSE(d->objectStateDealEnded());
    EXPECT_EQ(d->inventoryDialogCloseCount(), 0u);
    EXPECT_EQ(d->titanInventoryDialogCloseCount(), 0u);
}
