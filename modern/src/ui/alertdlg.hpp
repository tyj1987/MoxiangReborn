// alertdlg.hpp — modern port of 墨香
// CAlertDialog (alert dialog: 2 cButton + cbBtnFunc
// callback).
//
// 1:1 port of legacy `CAlertDlg` from
//   `墨香【源码】\[Client]MH\AlertDlg.h` (956 B) and
//   `墨香【源码】\[Client]MH\AlertDlg.cpp`.
//
// What the legacy does:
//   - Ctor: m_pOk=NULL; m_pCancel=NULL.
//   - Dtor: m_pOk=NULL; m_pCancel=NULL.
//   - ActionEvent override: union of
//     cWindow::ActionEvent + cDialog::ActionEventWindow
//     + m_pOk->ActionEvent + m_pCancel->ActionEvent.
//     If m_pOk fires WE_BTNCLICK →
//     cbBtnFunc(m_ID, this, 1).
//     If m_pCancel fires WE_BTNCLICK →
//     cbBtnFunc(m_ID, this, 0).
//     Returns the union we.
//   - Add(cWindow*) override: if window->GetType() ==
//     WT_BUTTON, capture into m_pOk (first) or
//     m_pCancel (second). Always call cDialog::Add
//     (pass-through ownership).
//   - SetcbBtn(void (*cbFunc)(LONG lId, void * p, DWORD we))
//     — sets cbBtnFunc.
//   - SetObj(void* obj) / GetObj() — set/get m_obj.
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: m_pOk / m_pCancel
//     null-init is the default member init).
//   - Dtor: empty (no-op; member cleanup is automatic
//     for raw pointers / std::function).
//   - Linking: REAL — synth 2 cButton (OK + Cancel)
//     and store in m_pOk / m_pCancel. 1:1 quirk:
//     legacy uses Add() side-channel to capture
//     cButton references; modern port synthesizes
//     children in Linking (no resource loader hook
//     in modern, same pattern as cMainDialog).
//   - ActionEvent override: TODO (CMouse not ported,
//     R-12.x deferred). Modern port returns WE_NULL
//     (matches legacy "no event" path).
//   - SetcbBtn / SetObj / GetObj: REAL with
//     std::function (1:1 with legacy function pointer).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 42nd **Tier 2** dialog port (after
// cMPMissionDialog). The dialog has 2 cButton
// (m_pOk + m_pCancel) + 1 callback function pointer
// (cbBtnFunc) + 1 generic object pointer (m_obj).
// CMouse is the only TODO singleton (R-12.x deferred).

#pragma once

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

    // ----- 1:1 with legacy CAlertDlg::Linking -----

    // 1:1 with legacy Linking (synthesized in modern
    // port — the legacy uses Add() side-channel to
    // capture cButton references; modern port creates
    // 2 cButton directly in Linking).
    void Linking();

    // ----- 1:1 with legacy CAlertDlg::ActionEvent override -----

    // 1:1 with legacy ActionEvent. The whole
    // method is TODO (CMouse not ported, R-12.x
    // deferred). Modern port returns WE_NULL
    // (matching the legacy "no event" path).
    std::uint32_t ActionEvent();

    // ----- 1:1 with legacy CAlertDlg::SetcbBtn -----

    // 1:1 with legacy SetcbBtn(void (*cbFunc)).
    // Modern port uses std::function for type
    // safety (vs legacy C function pointer).
    void SetcbBtn(BtnCallback cbFunc) noexcept;

    // ----- 1:1 with legacy CAlertDlg::SetObj / GetObj -----

    // 1:1 with legacy SetObj(void* obj) / GetObj().
    // Stores an opaque user object pointer.
    void SetObj(void* obj) noexcept { m_obj = obj; }
    void* GetObj() const noexcept   { return m_obj; }

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy AB_OKCANCEL / AB_YESNO enum
    // (the legacy uses these as button *types*, not
    // ids). Local 600-601 — distinct from 200-580
    // used by previous Tier 2 dialogs.
    static constexpr std::int32_t kIdOkBtn     = 600;
    static constexpr std::int32_t kIdCancelBtn = 601;

    // 1:1 with legacy AB_OKCANCEL=0 / AB_YESNO=1.
    static constexpr std::int32_t kAbOkCancel = 0;
    static constexpr std::int32_t kAbYesNo    = 1;

private:
    // 1:1 with legacy m_pOk (captured by legacy Add
    // side-channel). Modern port: non-owning raw
    // pointer; the dialog owns the button via its
    // cWindow children list.
    cButton* m_pOk = nullptr;

    // 1:1 with legacy m_pCancel (captured by legacy
    // Add side-channel). Modern port: non-owning
    // raw pointer; the dialog owns the button via
    // its cWindow children list.
    cButton* m_pCancel = nullptr;

    // 1:1 with legacy cbBtnFunc. Modern port uses
    // std::function (1:1 with legacy C function
    // pointer; default = no-op).
    BtnCallback m_cbBtnFunc;

    // 1:1 with legacy m_obj (opaque user object).
    void* m_obj = nullptr;
};

}  // namespace mxh::ui
