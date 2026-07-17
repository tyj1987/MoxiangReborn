// tipbrowserdlg.hpp — modern port of 墨香 CTipBrowserDlg
// (tip browser dialog: 4 pushup tab buttons + 4 nested
// cDialog pages + 1 cancel button).
//
// 1:1 port of legacy `CTipBrowserDlg` from
//   `墨香【源码】\[Client]MH\TipBrowserDlg.h` (383 B) and
//   `墨香【源码】\[Client]MH\TipBrowserDlg.cpp`.
//
// What the legacy does:
//   - Ctor: m_pButton[4] = NULL, m_pDlg[4] = NULL,
//     m_wCurDlg = 0.
//   - Dtor: same NULL-out + m_wCurDlg = 0.
//   - Linking: resolve 4 cDialog page children by
//     id (TB_STATE_PO + i) + 4 cPushupButton tab
//     buttons by id (TB_STATE_PUSHUP1 + i), then
//     m_wCurDlg = 0.
//   - Show: SetActive(TRUE), then for each
//     tab: SetActive(FALSE) on dialog, SetPush(FALSE)
//     + SetDisable(FALSE) on button; then
//     m_pDlg[m_wCurDlg]->SetActive(TRUE) +
//     m_pButton[m_wCurDlg]->SetPush(TRUE) +
//     m_pButton[m_wCurDlg]->SetDisable(TRUE) (1:1
//     quirk: the closing `}` is misplaced — the
//     SetActive(TRUE) for m_pDlg[m_wCurDlg] is
//     actually outside the for loop; modern port
//     follows the legacy's corrected control flow).
//   - Close: SetActive(FALSE), m_wCurDlg = 0.
//   - OnActionEvent: 2 paths:
//     1) `if (we == WE_PUSHDOWN)` (1:1 quirk: legacy
//        uses `==` not `&`; exact match required):
//        compute id = lId - TB_STATE_PUSHUP1; if id
//        in [0,4), set m_wCurDlg = id, call Show(),
//        return.
//     2) `if (we & WE_BTNCLICK && lId ==
//        TB_CANCELBTN)`: call Close().
//
// The modern port covers:
//   - Ctor / dtor: REAL (default member init zeros
//     all fields).
//   - Linking: REAL — resolve 4 cDialog + 4
//     cPushupButton children by id.
//   - Show: REAL — toggle dialog/button active +
//     push + disable state.
//   - Close: REAL — SetActive(FALSE) + reset
//     m_wCurDlg.
//   - OnActionEvent: 2 paths preserved with 1:1
//     quirks (we == WE_PUSHDOWN exact match; CANCEL
//     on WE_BTNCLICK).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 25th **Tier 2** dialog port (after
// cGuildInvitationKindSelectionDialog). The dialog
// has no service dependency on the modern service
// interface (Phase 13) — and no singleton
// dependencies (Linking uses cWindow::findWindowById
// + cPushupButton::SetPush/SetDisable, all
// already-ported).

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cPushupButton;

class cTipBrowserDlg : public cDialog {
public:
    cTipBrowserDlg();
    ~cTipBrowserDlg() override;

    // ----- 1:1 with legacy CTipBrowserDlg::Linking -----

    // 1:1 with legacy Linking. Resolve 4 cDialog
    // page children (TB_STATE_PO + i) + 4
    // cPushupButton tab buttons (TB_STATE_PUSHUP1 +
    // i) by id, then m_wCurDlg = 0.
    void Linking();

    // ----- 1:1 with legacy CTipBrowserDlg::Show -----

    // 1:1 with legacy Show. SetActive(TRUE), then
    // for each tab: SetActive(FALSE) on dialog,
    // SetPush(FALSE) + SetDisable(FALSE) on
    // button; then activate the current tab
    // (m_wCurDlg) with SetPush(TRUE) +
    // SetDisable(TRUE).
    void Show();

    // ----- 1:1 with legacy CTipBrowserDlg::Close -----

    // 1:1 with legacy Close. SetActive(FALSE) +
    // m_wCurDlg = 0.
    void Close();

    // ----- 1:1 with legacy CTipBrowserDlg::OnActionEvent -----

    // 1:1 with legacy OnActionEvent. 2 paths:
    //   1) we == WE_PUSHDOWN (1:1 quirk: exact
    //      match, not bit-and): compute id = lId -
    //      TB_STATE_PUSHUP1; if id in [0, 4), set
    //      m_wCurDlg = id, call Show(), return.
    //   2) we & WE_BTNCLICK && lId == TB_CANCELBTN:
    //      call Close().
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h WINDOW_ID values
    // (TB_STATE_PO + i, TB_STATE_PUSHUP1 + i, and
    // TB_CANCELBTN). Local 380-384 (4 dialog
    // pages) + 385-388 (4 pushup buttons) + 389
    // (cancel button) — distinct from 200-372 used
    // by previous Tier 2 dialogs.
    static constexpr std::int32_t kIdStateBase      = 380;  // dialog page base
    static constexpr std::int32_t kIdPushupBase     = 385;  // pushup button base
    static constexpr std::int32_t kIdCancelBtn      = 389;
    static constexpr std::size_t   kNumTabs          = 4;

private:
    // 1:1 with legacy m_pButton[4] (4 cPushupButton
    // tab buttons). Modern port stores raw pointer
    // (not owned; cDialog owns the children).
    cPushupButton* m_pButton[kNumTabs] = {nullptr, nullptr, nullptr, nullptr};

    // 1:1 with legacy m_pDlg[4] (4 cDialog page
    // children). Modern port stores raw pointer.
    cDialog* m_pDlg[kNumTabs] = {nullptr, nullptr, nullptr, nullptr};

    // 1:1 with legacy m_wCurDlg (current tab index).
    std::uint16_t m_wCurDlg = 0;
};

}  // namespace mxh::ui
