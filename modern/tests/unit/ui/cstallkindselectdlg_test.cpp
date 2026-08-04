//
// Unit tests for mxh::ui::cStallKindSelectDlg (Phase C dialog port).
//
// Locks down the 1:1 surface:
//   * Constants: kIdSellBtn=430, kIdBuyBtn=431, kIdCancelBtn=432,
//                kWeBtnClick=0x0001
//   * StallKind enum: Null=0, Sell=1, Buy=2
//   * Default construction: 3 button pointers null
//   * Linking resolves 3 cButton children by id
//   * Linking without children leaves pointers null
//   * SetSellBtnForTest / SetBuyBtnForTest / SetCancelBtnForTest
//     store pointers
//   * Show activates dialog + 3 buttons
//   * Show without links is safe
//   * Close deactivates dialog + 3 buttons
//   * Show + Close round-trip
//   * OnActionEvent(SELL, BTNCLICK) calls SetStallKind(SELL)
//     + OpenStreetStall() + Close()
//   * OnActionEvent(BUY, BTNCLICK) calls SetStallKind(BUY)
//     + OpenStreetStall() + Close()
//   * OnActionEvent(CANCEL, BTNCLICK) calls SetStallKind(NULL)
//     + SetOpenMsgBox(TRUE) + Close()
//   * OnActionEvent(unknown id, BTNCLICK) does NOT call
//     Close() (legacy `else return;` quirk)
//   * OnActionEvent(known id, !BTNCLICK) is a no-op
//   * OnActionEvent before Linking does not crash
//   * OnActionEvent without callbacks is safe
//   * NonCopyable
//

#include "mxh/ui/cstallkindselectdlg.hpp"
#include "mxh/ui/cbutton.hpp"

#include <gtest/gtest.h>

#include <type_traits>

using mxh::ui::cButton;
using mxh::ui::cStallKindSelectDlg;
using mxh::ui::StallKind;

namespace {

struct Harness {
    cStallKindSelectDlg dlg;
    cButton sell, buy, cancel;

    Harness() {
        dlg.SetSellBtnForTest(&sell);
        dlg.SetBuyBtnForTest(&buy);
        dlg.SetCancelBtnForTest(&cancel);
    }
};

// Callback state for the 3 STREESTSTALLMGR methods.
StallKind g_lastKind = StallKind::Null;
std::uint32_t g_setStallKindCalls    = 0;
std::uint32_t g_openStreetStallCalls = 0;
std::uint32_t g_setOpenMsgBoxCalls   = 0;
bool g_lastOpenMsgBox = false;

void ResetCbState() {
    g_lastKind            = StallKind::Null;
    g_setStallKindCalls   = 0;
    g_openStreetStallCalls = 0;
    g_setOpenMsgBoxCalls   = 0;
    g_lastOpenMsgBox      = false;
}

void SetStallKindCb(StallKind k, void* /*user*/) {
    g_lastKind = k;
    ++g_setStallKindCalls;
}
void OpenStreetStallCb(void* /*user*/) {
    ++g_openStreetStallCalls;
}
void SetOpenMsgBoxCb(bool open, void* /*user*/) {
    g_lastOpenMsgBox = open;
    ++g_setOpenMsgBoxCalls;
}

}  // namespace


TEST(CStallKindSelectDlg, ConstantsMatchLegacy) {
    EXPECT_EQ(cStallKindSelectDlg::kIdSellBtn,   430);
    EXPECT_EQ(cStallKindSelectDlg::kIdBuyBtn,    431);
    EXPECT_EQ(cStallKindSelectDlg::kIdCancelBtn, 432);
    EXPECT_EQ(cStallKindSelectDlg::kWeBtnClick, mxh::ui::legacy_window_event::kButtonClick);
}

TEST(CStallKindSelectDlg, IdsAreDistinct) {
    EXPECT_NE(cStallKindSelectDlg::kIdSellBtn,
              cStallKindSelectDlg::kIdBuyBtn);
    EXPECT_NE(cStallKindSelectDlg::kIdSellBtn,
              cStallKindSelectDlg::kIdCancelBtn);
    EXPECT_NE(cStallKindSelectDlg::kIdBuyBtn,
              cStallKindSelectDlg::kIdCancelBtn);
}

TEST(CStallKindSelectDlg, StallKindEnumMatchesLegacy) {
    // 1:1 with legacy STALL_KIND enum.
    EXPECT_EQ(static_cast<std::int32_t>(StallKind::Null), 0);
    EXPECT_EQ(static_cast<std::int32_t>(StallKind::Sell), 1);
    EXPECT_EQ(static_cast<std::int32_t>(StallKind::Buy),  2);
}

TEST(CStallKindSelectDlg, DefaultConstructionHasNullButtons) {
    cStallKindSelectDlg d;
    EXPECT_EQ(d.GetSellBtnForTest(),   nullptr);
    EXPECT_EQ(d.GetBuyBtnForTest(),    nullptr);
    EXPECT_EQ(d.GetCancelBtnForTest(), nullptr);
}

TEST(CStallKindSelectDlg, SetButtonsForTestStoresPointers) {
    cStallKindSelectDlg d;
    cButton s, b, c;
    d.SetSellBtnForTest(&s);
    d.SetBuyBtnForTest(&b);
    d.SetCancelBtnForTest(&c);
    EXPECT_EQ(d.GetSellBtnForTest(),   &s);
    EXPECT_EQ(d.GetBuyBtnForTest(),    &b);
    EXPECT_EQ(d.GetCancelBtnForTest(), &c);
}


TEST(CStallKindSelectDlg, LinkingResolvesAllChildren) {
    Harness h;
    h.dlg.Linking();
    EXPECT_EQ(h.dlg.GetSellBtnForTest(),   &h.sell);
    EXPECT_EQ(h.dlg.GetBuyBtnForTest(),    &h.buy);
    EXPECT_EQ(h.dlg.GetCancelBtnForTest(), &h.cancel);
}

TEST(CStallKindSelectDlg, LinkingWithoutChildrenLeavesNulls) {
    cStallKindSelectDlg d;
    d.Init(0, 0, 400, 400, nullptr, 0);
    d.Linking();
    EXPECT_EQ(d.GetSellBtnForTest(),   nullptr);
    EXPECT_EQ(d.GetBuyBtnForTest(),    nullptr);
    EXPECT_EQ(d.GetCancelBtnForTest(), nullptr);
}

TEST(CStallKindSelectDlg, LinkingBeforeInitDoesNotCrash) {
    cStallKindSelectDlg d;
    d.Linking();
    SUCCEED();
}


TEST(CStallKindSelectDlg, ShowActivatesDialogAndAllButtons) {
    Harness h;
    h.dlg.Show();
    EXPECT_TRUE(h.dlg.isActive());
    EXPECT_TRUE(h.sell.isVisible());
    EXPECT_TRUE(h.buy.isVisible());
    EXPECT_TRUE(h.cancel.isVisible());
}

TEST(CStallKindSelectDlg, ShowWithoutLinksIsSafe) {
    cStallKindSelectDlg d;
    d.Show();
    EXPECT_TRUE(d.isActive());
}

TEST(CStallKindSelectDlg, CloseDeactivatesDialogAndAllButtons) {
    Harness h;
    h.dlg.Show();
    h.dlg.Close();
    EXPECT_FALSE(h.dlg.isActive());
    EXPECT_FALSE(h.sell.isVisible());
    EXPECT_FALSE(h.buy.isVisible());
    EXPECT_FALSE(h.cancel.isVisible());
}

TEST(CStallKindSelectDlg, ShowCloseRoundTrip) {
    Harness h;
    h.dlg.Show();
    EXPECT_TRUE(h.dlg.isActive());
    h.dlg.Close();
    EXPECT_FALSE(h.dlg.isActive());
    h.dlg.Show();
    EXPECT_TRUE(h.dlg.isActive());
}


TEST(CStallKindSelectDlg, OnActionEventSellBtnDispatch) {
    Harness h;
    ResetCbState();
    h.dlg.SetStallKindCallbackForTest(&SetStallKindCb, nullptr);
    h.dlg.SetOpenStreetStallCallbackForTest(&OpenStreetStallCb, nullptr);
    h.dlg.SetOpenMsgBoxCallbackForTest(&SetOpenMsgBoxCb, nullptr);
    h.dlg.Show();
    EXPECT_TRUE(h.dlg.isActive());

    h.dlg.OnActionEvent(cStallKindSelectDlg::kIdSellBtn,
                        nullptr, cStallKindSelectDlg::kWeBtnClick);

    // 1:1 with legacy SELL branch:
    //   SetStallKind(eSK_SELL) + OpenStreetStall() + Close()
    EXPECT_EQ(g_setStallKindCalls,    1u);
    EXPECT_EQ(g_openStreetStallCalls, 1u);
    EXPECT_EQ(g_setOpenMsgBoxCalls,   0u);
    EXPECT_EQ(g_lastKind, StallKind::Sell);
    EXPECT_TRUE(h.dlg.IsSellDispatched());
    EXPECT_EQ(h.dlg.LastDispatchKind(), StallKind::Sell);
    // Close() fired.
    EXPECT_FALSE(h.dlg.isActive());
}

TEST(CStallKindSelectDlg, OnActionEventBuyBtnDispatch) {
    Harness h;
    ResetCbState();
    h.dlg.SetStallKindCallbackForTest(&SetStallKindCb, nullptr);
    h.dlg.SetOpenStreetStallCallbackForTest(&OpenStreetStallCb, nullptr);
    h.dlg.SetOpenMsgBoxCallbackForTest(&SetOpenMsgBoxCb, nullptr);
    h.dlg.Show();

    h.dlg.OnActionEvent(cStallKindSelectDlg::kIdBuyBtn,
                        nullptr, cStallKindSelectDlg::kWeBtnClick);

    // 1:1 with legacy BUY branch:
    //   SetStallKind(eSK_BUY) + OpenStreetStall() + Close()
    EXPECT_EQ(g_setStallKindCalls,    1u);
    EXPECT_EQ(g_openStreetStallCalls, 1u);
    EXPECT_EQ(g_setOpenMsgBoxCalls,   0u);
    EXPECT_EQ(g_lastKind, StallKind::Buy);
    EXPECT_TRUE(h.dlg.IsBuyDispatched());
    EXPECT_EQ(h.dlg.LastDispatchKind(), StallKind::Buy);
    EXPECT_FALSE(h.dlg.isActive());
}

TEST(CStallKindSelectDlg, OnActionEventCancelBtnDispatch) {
    Harness h;
    ResetCbState();
    h.dlg.SetStallKindCallbackForTest(&SetStallKindCb, nullptr);
    h.dlg.SetOpenStreetStallCallbackForTest(&OpenStreetStallCb, nullptr);
    h.dlg.SetOpenMsgBoxCallbackForTest(&SetOpenMsgBoxCb, nullptr);
    h.dlg.Show();

    h.dlg.OnActionEvent(cStallKindSelectDlg::kIdCancelBtn,
                        nullptr, cStallKindSelectDlg::kWeBtnClick);

    // 1:1 with legacy CANCEL branch:
    //   SetStallKind(eSK_NULL) + SetOpenMsgBox(TRUE) + Close()
    EXPECT_EQ(g_setStallKindCalls,    1u);
    EXPECT_EQ(g_openStreetStallCalls, 0u);
    EXPECT_EQ(g_setOpenMsgBoxCalls,   1u);
    EXPECT_EQ(g_lastKind, StallKind::Null);
    EXPECT_TRUE(g_lastOpenMsgBox);
    EXPECT_TRUE(h.dlg.IsCancelDispatched());
    EXPECT_EQ(h.dlg.LastDispatchKind(), StallKind::Null);
    EXPECT_FALSE(h.dlg.isActive());
}

TEST(CStallKindSelectDlg, OnActionEventUnknownIdIsNoOp) {
    // 1:1 with legacy `else return;` for unknown ids:
    // Close() is NOT called.
    Harness h;
    ResetCbState();
    h.dlg.SetStallKindCallbackForTest(&SetStallKindCb, nullptr);
    h.dlg.SetOpenStreetStallCallbackForTest(&OpenStreetStallCb, nullptr);
    h.dlg.SetOpenMsgBoxCallbackForTest(&SetOpenMsgBoxCb, nullptr);
    h.dlg.Show();
    EXPECT_TRUE(h.dlg.isActive());

    h.dlg.OnActionEvent(/*lId=*/9999, nullptr,
                        cStallKindSelectDlg::kWeBtnClick);

    // No callbacks fired.
    EXPECT_EQ(g_setStallKindCalls,    0u);
    EXPECT_EQ(g_openStreetStallCalls, 0u);
    EXPECT_EQ(g_setOpenMsgBoxCalls,   0u);
    // Dialog still active (Close() not called).
    EXPECT_TRUE(h.dlg.isActive());
    EXPECT_FALSE(h.dlg.IsSellDispatched());
    EXPECT_FALSE(h.dlg.IsBuyDispatched());
    EXPECT_FALSE(h.dlg.IsCancelDispatched());
}

TEST(CStallKindSelectDlg, OnActionEventNonBtnClickIsNoOp) {
    // 1:1 with legacy `if (we & WE_BTNCLICK && ...)`:
    // legacy's WE_BTNCLICK mask is required for any
    // dispatch to fire.
    Harness h;
    ResetCbState();
    h.dlg.SetStallKindCallbackForTest(&SetStallKindCb, nullptr);
    h.dlg.SetOpenStreetStallCallbackForTest(&OpenStreetStallCb, nullptr);
    h.dlg.SetOpenMsgBoxCallbackForTest(&SetOpenMsgBoxCb, nullptr);
    h.dlg.Show();

    constexpr std::uint32_t NOT_BTNCLICK = 0x0000;
    h.dlg.OnActionEvent(cStallKindSelectDlg::kIdSellBtn,   nullptr, NOT_BTNCLICK);
    h.dlg.OnActionEvent(cStallKindSelectDlg::kIdBuyBtn,    nullptr, NOT_BTNCLICK);
    h.dlg.OnActionEvent(cStallKindSelectDlg::kIdCancelBtn, nullptr, NOT_BTNCLICK);

    EXPECT_EQ(g_setStallKindCalls,    0u);
    EXPECT_EQ(g_openStreetStallCalls, 0u);
    EXPECT_EQ(g_setOpenMsgBoxCalls,   0u);
    EXPECT_TRUE(h.dlg.isActive());
}

TEST(CStallKindSelectDlg, OnActionEventBeforeLinkingDoesNotCrash) {
    cStallKindSelectDlg d;
    d.Init(0, 0, 400, 400, nullptr, 0);
    d.SetStallKindCallbackForTest(&SetStallKindCb, nullptr);
    d.SetOpenStreetStallCallbackForTest(&OpenStreetStallCb, nullptr);
    d.SetOpenMsgBoxCallbackForTest(&SetOpenMsgBoxCb, nullptr);

    d.OnActionEvent(cStallKindSelectDlg::kIdSellBtn,   nullptr,
                    cStallKindSelectDlg::kWeBtnClick);
    d.OnActionEvent(cStallKindSelectDlg::kIdBuyBtn,    nullptr,
                    cStallKindSelectDlg::kWeBtnClick);
    d.OnActionEvent(cStallKindSelectDlg::kIdCancelBtn, nullptr,
                    cStallKindSelectDlg::kWeBtnClick);
    // Callbacks still fire (they don't depend on Linking).
    SUCCEED();
}

TEST(CStallKindSelectDlg, OnActionEventWithoutCallbacksIsSafe) {
    // 1:1 quirk: legacy STREESTSTALLMGR is a guaranteed
    // singleton; the modern port tolerates a host that
    // hasn't wired any callbacks.
    Harness h;
    ResetCbState();
    h.dlg.Show();
    EXPECT_TRUE(h.dlg.isActive());

    h.dlg.OnActionEvent(cStallKindSelectDlg::kIdSellBtn, nullptr,
                        cStallKindSelectDlg::kWeBtnClick);
    // Close() still fires (Close() is part of the 1:1
    // legacy control flow, not the callback chain).
    EXPECT_FALSE(h.dlg.isActive());
}


TEST(CStallKindSelectDlg, NonCopyable) {
    static_assert(!std::is_copy_constructible<cStallKindSelectDlg>::value,
                  "cStallKindSelectDlg must not be copyable");
    static_assert(!std::is_copy_assignable<cStallKindSelectDlg>::value,
                  "cStallKindSelectDlg must not be copy-assignable");
    SUCCEED();
}

TEST(CStallKindSelectDlg, IscDialog) {
    static_assert(std::is_base_of<mxh::ui::cDialog,
                                  cStallKindSelectDlg>::value,
                  "cStallKindSelectDlg must inherit from cDialog");
    SUCCEED();
}
