//
// Unit tests for mxh::ui::cAlertDlg (Phase C dialog port).
//
// Locks down the 1:1 surface of legacy CAlertDlg (alert
// dialog with 2 cButton + callback function pointer):
//   * Constants: kIdOkBtn=600, kIdCancelBtn=601,
//                kAbOkCancel=0, kAbYesNo=1,
//                kWeBtnClick=0x0001
//   * Default construction: button pointers null
//   * Inherits from cDialog
//   * Linking resolves 2 cButton children by id
//   * Linking without children leaves pointers null
//   * Linking is idempotent
//   * Linking before Init does not crash
//   * SetOkBtnForTest / SetCancelBtnForTest store pointers
//   * GetOkBtnForTest / GetCancelBtnForTest return them
//   * SetObj / GetObj round-trip
//   * SetObj with nullptr clears
//   * SetcbBtn with lambda stores
//   * SetcbBtn with std::function is safe
//   * SetcbBtn with nullptr clears
//   * HasCallback reports current state
//   * OnActionEvent(OK, BTNCLICK) calls cbBtnFunc(m_id, this, 1)
//   * OnActionEvent(Cancel, BTNCLICK) calls cbBtnFunc(m_id, this, 0)
//   * OnActionEvent(OK, BTNCLICK) increments OK dispatch count
//   * OnActionEvent(Cancel, BTNCLICK) increments Cancel dispatch count
//   * OnActionEvent(unknown, BTNCLICK) does NOT call cbBtnFunc
//   * OnActionEvent(OK, !BTNCLICK) does NOT call cbBtnFunc
//   * OnActionEvent(Cancel, !BTNCLICK) does NOT call cbBtnFunc
//   * OnActionEvent without callback is safe
//   * OnActionEvent before Linking does not crash
//   * OnActionEvent(OK) sets GetLastLId = OK id
//   * OnActionEvent(Cancel) sets GetLastLId = Cancel id
//   * OnActionEvent(OK) sets GetLastWe = passed we
//   * Multiple OK clicks accumulate dispatch count
//   * cbBtnFunc receives the dialog pointer as 2nd arg
//   * NonCopyable
//

#include "mxh/ui/calertdlg.hpp"
#include "mxh/ui/cdialog.hpp"
#include "mxh/ui/cbutton.hpp"
#include "mxh/ui/cwindow.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

using mxh::ui::cAlertDlg;
using mxh::ui::cButton;
using mxh::ui::cDialog;
using mxh::ui::cWindow;

namespace {

// Callback state for cbBtnFunc.
std::int32_t  g_lastLId       = -1;
void*         g_lastP         = nullptr;
std::uint32_t g_lastWe        = 0;
std::uint32_t g_cbCallCount   = 0;

void ResetCbState() {
    g_lastLId     = -1;
    g_lastP       = nullptr;
    g_lastWe      = 0;
    g_cbCallCount = 0;
}

void TestCallback(std::int32_t lId, void* p, std::uint32_t we) {
    g_lastLId     = lId;
    g_lastP       = p;
    g_lastWe      = we;
    ++g_cbCallCount;
}

// Harness: dialog with pre-wired OK + Cancel buttons + callback.
struct Harness {
    cAlertDlg dlg;
    cButton   okBtn;
    cButton   cancelBtn;

    Harness() {
        dlg.Init(0, 0, 200, 80, nullptr, 0);
        okBtn.Init(0, 0, 50, 30, nullptr, nullptr, nullptr, nullptr, nullptr,
                   cAlertDlg::kIdOkBtn);
        cancelBtn.Init(0, 30, 50, 30, nullptr, nullptr, nullptr, nullptr, nullptr,
                       cAlertDlg::kIdCancelBtn);
        dlg.SetOkBtnForTest(&okBtn);
        dlg.SetCancelBtnForTest(&cancelBtn);
        dlg.SetcbBtn(TestCallback);
        ResetCbState();
    }
};

}  // namespace

// ---------- Construction / destruction ----------

TEST(CAlertDlgTest, CtorDoesNotCrash) {
    cAlertDlg dlg;
    SUCCEED();
}

TEST(CAlertDlgTest, DtorDoesNotCrash) {
    cAlertDlg dlg;
    SUCCEED();
}

TEST(CAlertDlgTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cAlertDlg>,
                  "cAlertDlg must inherit from cDialog");
    SUCCEED();
}

TEST(CAlertDlgTest, NonCopyable) {
    static_assert(!std::is_copy_constructible_v<cAlertDlg>,
                  "cAlertDlg must be non-copyable");
    static_assert(!std::is_copy_assignable_v<cAlertDlg>,
                  "cAlertDlg must be non-copy-assignable");
    SUCCEED();
}

// ---------- Constants ----------

TEST(CAlertDlgTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cAlertDlg::kIdOkBtn, 600);
    EXPECT_EQ(cAlertDlg::kIdCancelBtn, 601);
}

TEST(CAlertDlgTest, IdConstantsAreUnique) {
    EXPECT_NE(cAlertDlg::kIdOkBtn, cAlertDlg::kIdCancelBtn);
}

TEST(CAlertDlgTest, AbEnumConstantsMatchLegacyValues) {
    EXPECT_EQ(cAlertDlg::kAbOkCancel, 0);
    EXPECT_EQ(cAlertDlg::kAbYesNo, 1);
}

TEST(CAlertDlgTest, WeBtnClickConstantMatchesLegacy) {
    EXPECT_EQ(cAlertDlg::kWeBtnClick, mxh::ui::legacy_window_event::kButtonClick);
}

// ---------- Linking ----------

TEST(CAlertDlgTest, DefaultButtonsAreNull) {
    cAlertDlg dlg;
    EXPECT_EQ(dlg.GetOkBtnForTest(), nullptr);
    EXPECT_EQ(dlg.GetCancelBtnForTest(), nullptr);
}

TEST(CAlertDlgTest, LinkingResolvesFromTree) {
    cAlertDlg dlg;
    dlg.Init(0, 0, 200, 80, nullptr, 0);
    auto ok = std::make_unique<cButton>();
    ok->Init(0, 0, 50, 30, nullptr, nullptr, nullptr, nullptr, nullptr,
             cAlertDlg::kIdOkBtn);
    auto cancel = std::make_unique<cButton>();
    cancel->Init(0, 30, 50, 30, nullptr, nullptr, nullptr, nullptr, nullptr,
                 cAlertDlg::kIdCancelBtn);
    cButton* okPtr = ok.get();
    cButton* cancelPtr = cancel.get();
    dlg.Add(std::move(ok));
    dlg.Add(std::move(cancel));
    // Linking should find them via findWindowById.
    dlg.Linking();
    EXPECT_EQ(dlg.GetOkBtnForTest(), okPtr);
    EXPECT_EQ(dlg.GetCancelBtnForTest(), cancelPtr);
}

TEST(CAlertDlgTest, LinkingWithoutChildrenLeavesNull) {
    cAlertDlg dlg;
    dlg.Init(0, 0, 200, 80, nullptr, 0);
    dlg.Linking();
    EXPECT_EQ(dlg.GetOkBtnForTest(), nullptr);
    EXPECT_EQ(dlg.GetCancelBtnForTest(), nullptr);
}

TEST(CAlertDlgTest, LinkingIsIdempotent) {
    Harness h;
    h.dlg.Linking();
    h.dlg.Linking();
    EXPECT_EQ(h.dlg.GetOkBtnForTest(), &h.okBtn);
    EXPECT_EQ(h.dlg.GetCancelBtnForTest(), &h.cancelBtn);
}

TEST(CAlertDlgTest, LinkingBeforeInitDoesNotCrash) {
    cAlertDlg dlg;
    dlg.Linking();
    SUCCEED();
}

TEST(CAlertDlgTest, HostInjectedTakesPriorityOverLinking) {
    cAlertDlg dlg;
    dlg.Init(0, 0, 200, 80, nullptr, 0);
    cButton pre;
    pre.Init(0, 0, 50, 30, nullptr, nullptr, nullptr, nullptr, nullptr,
             cAlertDlg::kIdOkBtn);
    dlg.SetOkBtnForTest(&pre);
    dlg.Linking();
    // Host-injected takes priority (no tree child to override).
    EXPECT_EQ(dlg.GetOkBtnForTest(), &pre);
}

// ---------- SetObj / GetObj ----------

TEST(CAlertDlgTest, GetObjDefaultIsNull) {
    cAlertDlg dlg;
    EXPECT_EQ(dlg.GetObj(), nullptr);
}

TEST(CAlertDlgTest, SetObjStoresValue) {
    cAlertDlg dlg;
    int myObj = 42;
    dlg.SetObj(&myObj);
    EXPECT_EQ(dlg.GetObj(), &myObj);
}

TEST(CAlertDlgTest, SetObjWithNullClearsObject) {
    cAlertDlg dlg;
    int myObj = 42;
    dlg.SetObj(&myObj);
    EXPECT_EQ(dlg.GetObj(), &myObj);
    dlg.SetObj(nullptr);
    EXPECT_EQ(dlg.GetObj(), nullptr);
}

TEST(CAlertDlgTest, SetObjAcceptsOpaquePointer) {
    cAlertDlg dlg;
    struct MyType { int x; };
    MyType mt{99};
    dlg.SetObj(&mt);
    auto* retrieved = static_cast<MyType*>(dlg.GetObj());
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->x, 99);
}

// ---------- SetcbBtn ----------

TEST(CAlertDlgTest, SetcbBtnAcceptsLambda) {
    cAlertDlg dlg;
    bool called = false;
    dlg.SetcbBtn([&](std::int32_t, void*, std::uint32_t) {
        called = true;
    });
    EXPECT_TRUE(dlg.HasCallback());
    // Not invoked yet -- verify storage.
    EXPECT_FALSE(called);
}

TEST(CAlertDlgTest, SetcbBtnWithStdFunctionIsSafe) {
    cAlertDlg dlg;
    cAlertDlg::BtnCallback cb = [](std::int32_t, void*, std::uint32_t) {};
    dlg.SetcbBtn(cb);
    EXPECT_TRUE(dlg.HasCallback());
}

TEST(CAlertDlgTest, SetcbBtnWithNullptrClearsCallback) {
    cAlertDlg dlg;
    dlg.SetcbBtn([](std::int32_t, void*, std::uint32_t) {});
    EXPECT_TRUE(dlg.HasCallback());
    dlg.SetcbBtn(nullptr);
    EXPECT_FALSE(dlg.HasCallback());
}

TEST(CAlertDlgTest, HasCallbackDefaultFalse) {
    cAlertDlg dlg;
    EXPECT_FALSE(dlg.HasCallback());
}

// ---------- OnActionEvent: OK branch ----------

TEST(CAlertDlgTest, OnActionEventOkBtnFiresCallbackWithOne) {
    Harness h;
    h.dlg.OnActionEvent(cAlertDlg::kIdOkBtn, &h.dlg, cAlertDlg::kWeBtnClick);
    EXPECT_EQ(g_cbCallCount, 1u);
    EXPECT_EQ(g_lastLId, h.dlg.id());
    EXPECT_EQ(g_lastP, &h.dlg);
    // 1:1 with legacy: OK button dispatches cbBtnFunc with
    // (m_ID, this, 1). The 3rd arg is the BUTTON id (1=OK).
    EXPECT_EQ(g_lastWe, 1u);
}

TEST(CAlertDlgTest, OnActionEventOkBtnIncrementsOkDispatchCount) {
    Harness h;
    EXPECT_EQ(h.dlg.GetOkDispatchCount(), 0);
    h.dlg.OnActionEvent(cAlertDlg::kIdOkBtn, &h.dlg, cAlertDlg::kWeBtnClick);
    EXPECT_EQ(h.dlg.GetOkDispatchCount(), 1);
    h.dlg.OnActionEvent(cAlertDlg::kIdOkBtn, &h.dlg, cAlertDlg::kWeBtnClick);
    EXPECT_EQ(h.dlg.GetOkDispatchCount(), 2);
}

TEST(CAlertDlgTest, OnActionEventOkBtnDoesNotIncrementCancelCount) {
    Harness h;
    h.dlg.OnActionEvent(cAlertDlg::kIdOkBtn, &h.dlg, cAlertDlg::kWeBtnClick);
    EXPECT_EQ(h.dlg.GetCancelDispatchCount(), 0);
}

TEST(CAlertDlgTest, OnActionEventOkBtnSetsLastLId) {
    Harness h;
    h.dlg.OnActionEvent(cAlertDlg::kIdOkBtn, &h.dlg, cAlertDlg::kWeBtnClick);
    EXPECT_EQ(h.dlg.GetLastLId(), cAlertDlg::kIdOkBtn);
}

TEST(CAlertDlgTest, OnActionEventOkBtnSetsLastWe) {
    Harness h;
    h.dlg.OnActionEvent(cAlertDlg::kIdOkBtn, &h.dlg, cAlertDlg::kWeBtnClick);
    EXPECT_EQ(h.dlg.GetLastWe(), cAlertDlg::kWeBtnClick);
}

TEST(CAlertDlgTest, OnActionEventOkBtnReceivesDialogAsPArg) {
    Harness h;
    h.dlg.OnActionEvent(cAlertDlg::kIdOkBtn, &h.dlg, cAlertDlg::kWeBtnClick);
    EXPECT_EQ(g_lastP, static_cast<void*>(&h.dlg));
}

// ---------- OnActionEvent: Cancel branch ----------

TEST(CAlertDlgTest, OnActionEventCancelBtnFiresCallbackWithZero) {
    Harness h;
    h.dlg.OnActionEvent(cAlertDlg::kIdCancelBtn, &h.dlg, cAlertDlg::kWeBtnClick);
    EXPECT_EQ(g_cbCallCount, 1u);
    EXPECT_EQ(g_lastLId, h.dlg.id());
    EXPECT_EQ(g_lastP, &h.dlg);
    // 1:1 with legacy: Cancel button dispatches cbBtnFunc with
    // (m_ID, this, 0). The 3rd arg is the BUTTON id (0=Cancel, 1=OK),
    // not the original we.
    EXPECT_EQ(g_lastWe, 0u);
}

TEST(CAlertDlgTest, OnActionEventCancelBtnIncrementsCancelDispatchCount) {
    Harness h;
    EXPECT_EQ(h.dlg.GetCancelDispatchCount(), 0);
    h.dlg.OnActionEvent(cAlertDlg::kIdCancelBtn, &h.dlg, cAlertDlg::kWeBtnClick);
    EXPECT_EQ(h.dlg.GetCancelDispatchCount(), 1);
    h.dlg.OnActionEvent(cAlertDlg::kIdCancelBtn, &h.dlg, cAlertDlg::kWeBtnClick);
    EXPECT_EQ(h.dlg.GetCancelDispatchCount(), 2);
}

TEST(CAlertDlgTest, OnActionEventCancelBtnDoesNotIncrementOkCount) {
    Harness h;
    h.dlg.OnActionEvent(cAlertDlg::kIdCancelBtn, &h.dlg, cAlertDlg::kWeBtnClick);
    EXPECT_EQ(h.dlg.GetOkDispatchCount(), 0);
}

TEST(CAlertDlgTest, OnActionEventCancelBtnSetsLastLId) {
    Harness h;
    h.dlg.OnActionEvent(cAlertDlg::kIdCancelBtn, &h.dlg, cAlertDlg::kWeBtnClick);
    EXPECT_EQ(h.dlg.GetLastLId(), cAlertDlg::kIdCancelBtn);
}

// ---------- OnActionEvent: edge cases ----------

TEST(CAlertDlgTest, OnActionEventUnknownIdIsNoOp) {
    Harness h;
    h.dlg.OnActionEvent(9999, &h.dlg, cAlertDlg::kWeBtnClick);
    EXPECT_EQ(g_cbCallCount, 0u);
    EXPECT_EQ(h.dlg.GetOkDispatchCount(), 0);
    EXPECT_EQ(h.dlg.GetCancelDispatchCount(), 0);
}

TEST(CAlertDlgTest, OnActionEventOkWithoutBtnClickIsNoOp) {
    Harness h;
    h.dlg.OnActionEvent(cAlertDlg::kIdOkBtn, &h.dlg, 0);
    EXPECT_EQ(g_cbCallCount, 0u);
    EXPECT_EQ(h.dlg.GetOkDispatchCount(), 0);
}

TEST(CAlertDlgTest, OnActionEventCancelWithoutBtnClickIsNoOp) {
    Harness h;
    h.dlg.OnActionEvent(cAlertDlg::kIdCancelBtn, &h.dlg, 0);
    EXPECT_EQ(g_cbCallCount, 0u);
    EXPECT_EQ(h.dlg.GetCancelDispatchCount(), 0);
}

TEST(CAlertDlgTest, OnActionEventUnknownFlagsIsNoOp) {
    Harness h;
    h.dlg.OnActionEvent(cAlertDlg::kIdOkBtn, &h.dlg, 0xFFFFu & ~cAlertDlg::kWeBtnClick);
    EXPECT_EQ(g_cbCallCount, 0u);
    EXPECT_EQ(h.dlg.GetOkDispatchCount(), 0);
}

TEST(CAlertDlgTest, OnActionEventWithoutCallbackIsSafe) {
    cAlertDlg dlg;
    dlg.Init(0, 0, 200, 80, nullptr, 0);
    // No callback set -- must not crash.
    dlg.OnActionEvent(cAlertDlg::kIdOkBtn, &dlg, cAlertDlg::kWeBtnClick);
    dlg.OnActionEvent(cAlertDlg::kIdCancelBtn, &dlg, cAlertDlg::kWeBtnClick);
    // Dispatch counters still update (so host can tell which button
    // was clicked even without a callback wired).
    EXPECT_EQ(dlg.GetOkDispatchCount(), 1);
    EXPECT_EQ(dlg.GetCancelDispatchCount(), 1);
}

TEST(CAlertDlgTest, OnActionEventBeforeLinkingDoesNotCrash) {
    ResetCbState();
    cAlertDlg dlg;
    dlg.Init(0, 0, 200, 80, nullptr, 0);
    dlg.SetcbBtn(TestCallback);
    dlg.OnActionEvent(cAlertDlg::kIdOkBtn, &dlg, cAlertDlg::kWeBtnClick);
    // 1:1: dispatch is by id, not by button pointer; works without
    // m_pOk / m_pCancel being resolved.
    EXPECT_EQ(g_cbCallCount, 1u);
    EXPECT_EQ(dlg.GetOkDispatchCount(), 1);
}

TEST(CAlertDlgTest, OnActionEventBeforeInitDoesNotCrash) {
    ResetCbState();
    cAlertDlg dlg;
    dlg.SetcbBtn(TestCallback);
    dlg.OnActionEvent(cAlertDlg::kIdOkBtn, &dlg, cAlertDlg::kWeBtnClick);
    EXPECT_EQ(g_cbCallCount, 1u);
}

TEST(CAlertDlgTest, MultipleOkClicksAccumulateDispatchCount) {
    Harness h;
    for (int i = 0; i < 5; ++i) {
        h.dlg.OnActionEvent(cAlertDlg::kIdOkBtn, &h.dlg, cAlertDlg::kWeBtnClick);
    }
    EXPECT_EQ(h.dlg.GetOkDispatchCount(), 5);
    EXPECT_EQ(g_cbCallCount, 5u);
}