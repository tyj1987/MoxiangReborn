// calertdlg.hpp -- modern port of Moxiang
//   CAlertDlg (alert dialog: OK/Cancel buttons + callback).
//
// 1:1 port of legacy `CAlertDlg` from
//   `[Client]MH\AlertDlg.{h,cpp}`.
//
// Surface (legacy):
//   - Ctor: m_pOk=NULL; m_pCancel=NULL.
//   - Dtor: m_pOk=NULL; m_pCancel=NULL.
//   - ActionEvent(CMouse*) override: union of
//     cWindow::ActionEvent + cDialog::ActionEventWindow
//     + m_pOk->ActionEvent + m_pCancel->ActionEvent.
//     If m_pOk fires WE_BTNCLICK -> cbBtnFunc(m_ID, this, 1).
//     If m_pCancel fires WE_BTNCLICK -> cbBtnFunc(m_ID, this, 0).
//     Returns the unioned event.
//   - Add(cWindow*) override: if WT_BUTTON, capture into
//     m_pOk (first) or m_pCancel (second). Always call
//     cDialog::Add (pass-through ownership).
//   - SetcbBtn: stores cbBtnFunc function pointer.
//   - SetObj/GetObj: store/retrieve m_obj opaque pointer.
//
// Modern port:
//   - Ctor: default (1:1 quirk: m_pOk / m_pCancel null-init
//     is the default member init).
//   - Dtor: default (no-op; member cleanup is automatic).
//   - Linking() synthesizes 2 cButton (OK + Cancel) when
//     not pre-wired via test hooks. 1:1 quirk: legacy uses
//     Add() side-channel to capture cButton references;
//     modern port resolves via findWindowById in Linking
//     (same pattern as cStallKindSelectDlg).
//   - OnActionEvent(lId, p, we): 1:1 with the WE_BTNCLICK
//     branch of legacy ActionEvent. If we & kWeBtnClick and
//     lId is OK -> cbBtnFunc(m_id, this, 1). If Cancel ->
//     cbBtnFunc(m_id, this, 0). The full ActionEvent
//     dispatch is wired via the cWindow::ActionEvent
//     recursion (m_pOk / m_pCancel -> click -> their
//     m_onClick -> host OnActionEvent hook).
//   - SetcbBtn: 1:1 with legacy SetcbBtn (using
//     std::function for type safety; matches 1:1 callback
//     signature).
//   - SetObj / GetObj: 1:1 with legacy.
//
// Test hooks (1:1 with cStallKindSelectDlg pattern):
//   - SetOkBtnForTest / SetCancelBtnForTest: pre-wire the
//     button pointers (overrides Linking's auto-discovery).
//   - GetOkDispatchCount / GetCancelDispatchCount: how many
//     times the OK / Cancel branch fired cbBtnFunc.
//   - GetLastLId / GetLastWe: capture of last OnActionEvent
//     args (useful for diagnostic assertions).

#pragma once

#include "legacy_window_event.hpp"

#include "cdialog.hpp"

#include <cstdint>
#include <functional>

namespace mxh::ui {

class cButton;

class cAlertDlg : public cDialog {
public:
    // 1:1 with legacy callback signature:
    //   void (*cbBtnFunc)(LONG lId, void * p, DWORD we);
    using BtnCallback = std::function<void(std::int32_t, void*, std::uint32_t)>;

    cAlertDlg();
    ~cAlertDlg() override;

    cAlertDlg(const cAlertDlg&) = delete;
    cAlertDlg& operator=(const cAlertDlg&) = delete;

    // ----- 1:1 with legacy CAlertDlg::Linking -----

    // 1:1 with legacy Linking (resolved in modern port
    // via findWindowById; legacy used Add() side-channel
    // which doesn't apply to modern cWindow::Add(unique_ptr)
    // ownership model).
    void Linking();

    // ----- 1:1 with legacy CAlertDlg::ActionEvent WE_BTNCLICK branch -----

    // 1:1 with legacy ActionEvent -- the WE_BTNCLICK
    // dispatch portion. The full mouse event chain is
    // handled by cWindow::ActionEvent recursion in
    // modern (children handle their own state machines).
    // OnActionEvent is the public hook tests / dispatcher
    // use to fire cbBtnFunc when a button is clicked.
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // ----- 1:1 with legacy CAlertDlg::SetcbBtn -----

    // 1:1 with legacy SetcbBtn(void (*cbFunc)).
    // Modern port uses std::function for type safety.
    void SetcbBtn(BtnCallback cbFunc) noexcept;

    // ----- 1:1 with legacy CAlertDlg::SetObj / GetObj -----

    // 1:1 with legacy SetObj(void* obj).
    void SetObj(void* obj) noexcept { m_obj = obj; }

    // 1:1 with legacy GetObj().
    void* GetObj() const noexcept   { return m_obj; }

    // ----- 1:1 with legacy AB_OKCANCEL / AB_YESNO enum -----

    // 1:1 with legacy AB_OKCANCEL=0 / AB_YESNO=1 (button type,
    // not id; AB_OKCANCEL = OK + Cancel pair, AB_YESNO =
    // Yes + No pair).
    static constexpr std::int32_t kAbOkCancel = 0;
    static constexpr std::int32_t kAbYesNo    = 1;

    // ----- 1:1 with legacy WE_BTNCLICK -----

    // 1:1 with legacy WE_BTNCLICK=0x0001.
    static constexpr std::uint32_t kWeBtnClick = legacy_window_event::kButtonClick;

    // ----- 1:1 with legacy child ids (local range) -----

    // Local id range 600-601 (distinct from 200-580 used by
    // previous Tier 2 dialogs).
    static constexpr std::int32_t kIdOkBtn     = 600;
    static constexpr std::int32_t kIdCancelBtn = 601;

    // ----- Test hooks -----

    // 1:1 with legacy m_pOk / m_pCancel capture: tests
    // pre-wire the button pointers (overrides Linking's
    // findWindowById auto-discovery).
    void  SetOkBtnForTest(cButton* b) noexcept     { m_pOk = b; }
    void  SetCancelBtnForTest(cButton* b) noexcept { m_pCancel = b; }
    cButton* GetOkBtnForTest() const noexcept     { return m_pOk; }
    cButton* GetCancelBtnForTest() const noexcept { return m_pCancel; }

    // Dispatch counters: how many times the OK / Cancel
    // branch fired cbBtnFunc.
    int GetOkDispatchCount() const noexcept     { return m_okDispatchCount; }
    int GetCancelDispatchCount() const noexcept { return m_cancelDispatchCount; }

    // Last OnActionEvent args (for diagnostic assertions).
    std::int32_t GetLastLId() const noexcept  { return m_lastLId; }
    std::uint32_t GetLastWe() const noexcept  { return m_lastWe; }

    // Whether cbBtnFunc is currently set.
    bool HasCallback() const noexcept { return static_cast<bool>(m_cbBtnFunc); }

private:
    // 1:1 with legacy m_pOk (captured by legacy Add
    // side-channel). Modern port: non-owning raw
    // pointer; resolved in Linking() via findWindowById
    // (or pre-wired by host via SetOkBtnForTest).
    cButton* m_pOk = nullptr;

    // 1:1 with legacy m_pCancel.
    cButton* m_pCancel = nullptr;

    // 1:1 with legacy cbBtnFunc. Modern port uses
    // std::function (1:1 with legacy C function pointer).
    BtnCallback m_cbBtnFunc;

    // 1:1 with legacy m_obj (opaque user object).
    void* m_obj = nullptr;

    // Test-introspection state.
    int             m_okDispatchCount     = 0;
    int             m_cancelDispatchCount = 0;
    std::int32_t    m_lastLId             = 0;
    std::uint32_t   m_lastWe              = 0;
};

}  // namespace mxh::ui