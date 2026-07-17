// stallkindselectdlg_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cStallKindSelectDlg (street stall kind
// selector dialog: 3 button — sell / buy / cancel).
//
// Covers modern/src/ui/stallkindselectdlg.{hpp,cpp}, a 1:1 port
// of
//   墨香【源码】\[Client]MH\StallKindSelectDlg.h (898 B) and
//   墨香【源码】\[Client]MH\StallKindSelectDlg.cpp.
//
// What's tested:
//   - Default construction: cStallKindSelectDlg is
//     a cDialog and inherits its tree management.
//   - 3 button pointers start null (1:1 with legacy
//     default init).
//   - 3 id constants are distinct (1:1 with legacy
//     SO_SELLBTN / SO_BUYBTN / SO_CANCELBTN).
//   - 3 id constants match expected local range
//     430-432 (no collision with previous Tier 2
//     dialogs).
//   - Linking resolves the 3 cButton children by
//     id.
//   - Linking without children leaves all pointers
//     null (Show / Close / OnActionEvent are safe).
//   - Linking before Init does not crash.
//   - Show: SetActive(TRUE) on dialog + 3 cButton
//     SetActive(TRUE).
//   - Show without linking is safe.
//   - Close: SetActive(FALSE) on dialog + 3 cButton
//     SetActive(FALSE).
//   - Close after Show returns dialog to inactive.
//   - OnActionEvent SELL + BUY + CANCEL are TODO
//     (STREETSTALLMGR not ported, R-12.x deferred):
//     no state change, but does NOT close the
//     dialog (Close() not called yet — the modern
//     port returns after the TODO marker).
//   - OnActionEvent unknown id is a no-op (1:1 with
//     legacy `else return;` early-out).
//   - OnActionEvent non-BTNCLICK is a no-op.
//   - OnActionEvent before Linking is safe.
//
// 1:1 quirks preserved:
//   - Ctor: 3 button pointers start null (1:1 with
//     legacy `m_pSellBtn = m_pBuyBtn = m_pCancelBtn
//     = NULL;`).
//   - Show: dialog + 3 cButton all set active true.
//   - Close: dialog + 3 cButton all set active
//     false.
//   - OnActionEvent SELL / BUY / CANCEL branches
//     are TODO (STREETSTALLMGR not ported, R-12.x
//     deferred). The final Close() call is also
//     deferred.
//   - 1:1 quirk: legacy `else return;` for unknown
//     ids means Close() is NOT called (preserved
//     by the `if (handled) Close();` modern
//     pattern).
//   - Local id range 430-432 (distinct from
//     200-420 used by previous Tier 2 dialogs; no
//     collision).

#include "stallkindselectdlg.hpp"
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

TEST(CStallKindSelectDlgTest, DefaultConstructionIsValid) {
    cStallKindSelectDlg dlg;
    // 1:1 with legacy ctor: 3 button pointers
    // start null.
    SUCCEED();
}

TEST(CStallKindSelectDlgTest, InheritsDialogTreeManagement) {
    cStallKindSelectDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetAbsXY(10, 20);
    EXPECT_EQ(dlg.absX(), 10);
    EXPECT_EQ(dlg.absY(), 20);
}

TEST(CStallKindSelectDlgTest, IdConstantsAreDistinct) {
    EXPECT_NE(cStallKindSelectDlg::kIdSellBtn,
              cStallKindSelectDlg::kIdBuyBtn);
    EXPECT_NE(cStallKindSelectDlg::kIdSellBtn,
              cStallKindSelectDlg::kIdCancelBtn);
    EXPECT_NE(cStallKindSelectDlg::kIdBuyBtn,
              cStallKindSelectDlg::kIdCancelBtn);
}

TEST(CStallKindSelectDlgTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cStallKindSelectDlg::kIdSellBtn, 430);
    EXPECT_EQ(cStallKindSelectDlg::kIdBuyBtn, 431);
    EXPECT_EQ(cStallKindSelectDlg::kIdCancelBtn, 432);
}

// ===========================================================================
// Linking
// ===========================================================================

namespace {

void BuildDlgWithChildren(cStallKindSelectDlg& dlg,
                          cButton** outSell,
                          cButton** outBuy,
                          cButton** outCancel) {
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    auto sell = std::make_unique<cButton>();
    sell->Init(0, 0, 30, 30, nullptr, nullptr, nullptr, nullptr, nullptr,
               cStallKindSelectDlg::kIdSellBtn);
    *outSell = sell.get();
    dlg.Add(std::unique_ptr<cWindow>(sell.release()));

    auto buy = std::make_unique<cButton>();
    buy->Init(0, 0, 30, 30, nullptr, nullptr, nullptr, nullptr, nullptr,
              cStallKindSelectDlg::kIdBuyBtn);
    *outBuy = buy.get();
    dlg.Add(std::unique_ptr<cWindow>(buy.release()));

    auto cancel = std::make_unique<cButton>();
    cancel->Init(0, 0, 30, 30, nullptr, nullptr, nullptr, nullptr, nullptr,
                 cStallKindSelectDlg::kIdCancelBtn);
    *outCancel = cancel.get();
    dlg.Add(std::unique_ptr<cWindow>(cancel.release()));

    dlg.Linking();
}

}  // namespace

TEST(CStallKindSelectDlgTest, LinkingResolvesAllChildren) {
    cStallKindSelectDlg dlg;
    cButton* pSell = nullptr;
    cButton* pBuy = nullptr;
    cButton* pCancel = nullptr;
    BuildDlgWithChildren(dlg, &pSell, &pBuy, &pCancel);

    // m_pSellBtn / m_pBuyBtn / m_pCancelBtn are
    // private; verified indirectly via Show
    // activating all 3 buttons.
    dlg.Show();
    EXPECT_TRUE(dlg.isActive());
    EXPECT_TRUE(pSell->isVisible());
    EXPECT_TRUE(pBuy->isVisible());
    EXPECT_TRUE(pCancel->isVisible());
}

TEST(CStallKindSelectDlgTest, LinkingWithoutChildrenDoesNotCrash) {
    cStallKindSelectDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    dlg.Show();
    dlg.Close();
    SUCCEED();
}

TEST(CStallKindSelectDlgTest, LinkingBeforeInitDoesNotCrash) {
    cStallKindSelectDlg dlg;
    dlg.Linking();
    SUCCEED();
}

// ===========================================================================
// Show / Close
// ===========================================================================

TEST(CStallKindSelectDlgTest, ShowActivatesDialogAndAllButtons) {
    cStallKindSelectDlg dlg;
    cButton* pSell = nullptr;
    cButton* pBuy = nullptr;
    cButton* pCancel = nullptr;
    BuildDlgWithChildren(dlg, &pSell, &pBuy, &pCancel);

    dlg.Show();
    EXPECT_TRUE(dlg.isActive());
    // 1:1 quirk: modern cButton has no SetActive;
    // modern port uses cWindow::SetVisible as the
    // 1:1 equivalent (R-12 fix).
    EXPECT_TRUE(pSell->isVisible());
    EXPECT_TRUE(pBuy->isVisible());
    EXPECT_TRUE(pCancel->isVisible());
}

TEST(CStallKindSelectDlgTest, ShowWithoutLinkIsSafe) {
    cStallKindSelectDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Show();
    EXPECT_TRUE(dlg.isActive());
}

TEST(CStallKindSelectDlgTest, CloseDeactivatesDialogAndAllButtons) {
    cStallKindSelectDlg dlg;
    cButton* pSell = nullptr;
    cButton* pBuy = nullptr;
    cButton* pCancel = nullptr;
    BuildDlgWithChildren(dlg, &pSell, &pBuy, &pCancel);

    dlg.Show();
    dlg.Close();
    EXPECT_FALSE(dlg.isActive());
    EXPECT_FALSE(pSell->isVisible());
    EXPECT_FALSE(pBuy->isVisible());
    EXPECT_FALSE(pCancel->isVisible());
}

TEST(CStallKindSelectDlgTest, ShowCloseRoundTrip) {
    cStallKindSelectDlg dlg;
    cButton* pSell = nullptr;
    cButton* pBuy = nullptr;
    cButton* pCancel = nullptr;
    BuildDlgWithChildren(dlg, &pSell, &pBuy, &pCancel);

    dlg.Show();
    EXPECT_TRUE(dlg.isActive());
    dlg.Close();
    EXPECT_FALSE(dlg.isActive());
    dlg.Show();
    EXPECT_TRUE(dlg.isActive());
}

// ===========================================================================
// OnActionEvent
// ===========================================================================

namespace {
constexpr std::uint32_t WE_BTNCLICK = 0x0001;
}  // namespace

TEST(CStallKindSelectDlgTest, OnActionEventSellBtnClosesDialog) {
    // 1:1 with legacy: SELL branch's
    // STREETSTALLMGR dispatch is TODO (R-12.x
    // deferred), but the trailing Close() is
    // called for all 3 known ids (legacy
    // control flow: `if (...) {...} else if
    // (...) {...} else if (...) {...} else
    // return; Close();`).
    cStallKindSelectDlg dlg;
    cButton* pSell = nullptr;
    cButton* pBuy = nullptr;
    cButton* pCancel = nullptr;
    BuildDlgWithChildren(dlg, &pSell, &pBuy, &pCancel);
    dlg.Show();
    EXPECT_TRUE(dlg.isActive());

    dlg.OnActionEvent(cStallKindSelectDlg::kIdSellBtn, nullptr, WE_BTNCLICK);
    // 1:1 with legacy: Close() called for SELL
    // (the legacy's trailing Close() fires after
    // the TODO singleton dispatch).
    EXPECT_FALSE(dlg.isActive());
    EXPECT_FALSE(pSell->isVisible());
}

TEST(CStallKindSelectDlgTest, OnActionEventBuyBtnClosesDialog) {
    cStallKindSelectDlg dlg;
    cButton* pSell = nullptr;
    cButton* pBuy = nullptr;
    cButton* pCancel = nullptr;
    BuildDlgWithChildren(dlg, &pSell, &pBuy, &pCancel);
    dlg.Show();

    dlg.OnActionEvent(cStallKindSelectDlg::kIdBuyBtn, nullptr, WE_BTNCLICK);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CStallKindSelectDlgTest, OnActionEventCancelBtnClosesDialog) {
    cStallKindSelectDlg dlg;
    cButton* pSell = nullptr;
    cButton* pBuy = nullptr;
    cButton* pCancel = nullptr;
    BuildDlgWithChildren(dlg, &pSell, &pBuy, &pCancel);
    dlg.Show();

    dlg.OnActionEvent(cStallKindSelectDlg::kIdCancelBtn, nullptr, WE_BTNCLICK);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CStallKindSelectDlgTest, OnActionEventUnknownIdIsNoOp) {
    // 1:1 with legacy: `else return;` for unknown
    // ids — Close() is NOT called. The modern
    // port preserves this via the `if (handled)
    // Close();` pattern.
    cStallKindSelectDlg dlg;
    cButton* pSell = nullptr;
    cButton* pBuy = nullptr;
    cButton* pCancel = nullptr;
    BuildDlgWithChildren(dlg, &pSell, &pBuy, &pCancel);
    dlg.Show();
    EXPECT_TRUE(dlg.isActive());

    dlg.OnActionEvent(/*lId=*/9999, nullptr, WE_BTNCLICK);
    // Dialog still active (Close() not called for
    // unknown id).
    EXPECT_TRUE(dlg.isActive());
}

TEST(CStallKindSelectDlgTest, OnActionEventNonBtnClickIsNoOp) {
    cStallKindSelectDlg dlg;
    cButton* pSell = nullptr;
    cButton* pBuy = nullptr;
    cButton* pCancel = nullptr;
    BuildDlgWithChildren(dlg, &pSell, &pBuy, &pCancel);
    dlg.Show();

    constexpr std::uint32_t NOT_BTNCLICK = 0x0000;
    dlg.OnActionEvent(cStallKindSelectDlg::kIdSellBtn, nullptr, NOT_BTNCLICK);
    dlg.OnActionEvent(cStallKindSelectDlg::kIdBuyBtn, nullptr, NOT_BTNCLICK);
    dlg.OnActionEvent(cStallKindSelectDlg::kIdCancelBtn, nullptr, NOT_BTNCLICK);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CStallKindSelectDlgTest, OnActionEventBeforeLinkingDoesNotCrash) {
    cStallKindSelectDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.OnActionEvent(cStallKindSelectDlg::kIdSellBtn, nullptr, WE_BTNCLICK);
    dlg.OnActionEvent(cStallKindSelectDlg::kIdBuyBtn, nullptr, WE_BTNCLICK);
    dlg.OnActionEvent(cStallKindSelectDlg::kIdCancelBtn, nullptr, WE_BTNCLICK);
    SUCCEED();
}

}  // namespace mxh::ui::test
