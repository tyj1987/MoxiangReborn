// reinforcedataguidedlg.hpp — modern port of 墨香
// CReinforceDataGuideDlg (reinforce data guide
// dialog: 9 cPushupButton + 9 cDialog page + 1 OK
// button).
//
// 1:1 port of legacy `CReinforceDataGuideDlg` from
//   `墨香【源码】\[Client]MH\ReinforceDataGuideDlg.h` (793 B) and
//   `墨香【源码】\[Client]MH\ReinforceDataGuideDlg.cpp`.
//
// What the legacy does:
//   - Ctor: 9 cPushupButton + 9 cDialog all NULL;
//     m_wCurData = 0.
//   - Dtor: empty body.
//   - Linking: resolve 9 cPushupButton by RFDG_BTN1-9
//     + 9 cDialog by GUIDE_SHEET1-7 (with
//     1:1 quirk: m_pDataDlg[6] = m_pDataDlg[5]
//     and m_pDataDlg[8] = m_pDataDlg[7] — some
//     sheets share the same dialog, so only 7
//     unique sheet dialogs exist).
//   - Show: SetActive(TRUE), for each tab: dialog
//     SetActive(FALSE) + button SetPush(FALSE) +
//     SetDisable(FALSE); then activate the current
//     tab (m_wCurData) with SetPush(TRUE) +
//     SetDisable(TRUE).
//   - Close: SetActive(FALSE) + m_wCurData = 0.
//   - SelectData(WORD index): inline setter for
//     m_wCurData.
//   - OnActionEvent: 2 paths:
//     1) `if (we == WE_PUSHDOWN)`: compute id =
//        lId - RFDG_BTN1; if id in [0, 9), set
//        m_wCurData = id, call Show(), return.
//     2) `if (we & WE_BTNCLICK && lId ==
//        RFDGUIDE_OKBTN)`: call Close().
//   - ActionEvent: override (call cWindow
//     ::ActionEvent + ActionEventWindow +
//     ActionEventComponent; only when active +
//     not disabled).
//   - ActionEventWindow: override (call
//     cDialog::ActionEventWindow; if LButtonDown
//     and !PtInWindow → no-op).
//
// The modern port covers:
//   - Ctor: empty (1:1 with legacy default init).
//   - Dtor: empty (no-op).
//   - Linking: REAL — resolve 9 cPushupButton + 9
//     cDialog (with the 1:1 quirks: m_pDataDlg[6]
//     aliases m_pDataDlg[5], m_pDataDlg[8] aliases
//     m_pDataDlg[7]).
//   - Show: REAL — same as cTipBrowserDlg (which
//     has 4 tabs; this has 9).
//   - Close: REAL — same pattern.
//   - SelectData: REAL inline setter.
//   - OnActionEvent: 1:1 with legacy 2 paths +
//     same 1:1 quirks as cTipBrowserDlg
//     (`we == WE_PUSHDOWN` exact match).
//   - ActionEvent: 1:1 with legacy (call base
//     cWindow::ActionEvent + ActionEventWindow +
//     ActionEventComponent; 1:1 quirk: legacy
//     uses `m_bActive` and `m_bDisable` member
//     fields; modern cWindow has `m_bEnabled`
//     instead — modern port uses m_bEnabled
//     via the same pattern).
//   - ActionEventWindow: 1:1 with legacy (call
//     base cDialog::ActionEventWindow; if
//     LButtonDown + !PtInWindow → no-op; 1:1 quirk:
//     the no-op block is empty in legacy).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 36th **Tier 2** dialog port (after
// cGTRegistDialog). The dialog has no service
// dependency on the modern service interface
// (Phase 13) — and no singleton dependencies
// (the data is static item kind enum tables).

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cPushupButton;

class cReinforceDataGuideDlg : public cDialog {
public:
    cReinforceDataGuideDlg();
    ~cReinforceDataGuideDlg() override;

    // ----- 1:1 with legacy CReinforceDataGuideDlg::Linking -----

    // 1:1 with legacy Linking. Resolve 9
    // cPushupButton by kIdBtnBase + i + 9 cDialog
    // by kIdSheetBase + i (with 1:1 quirks:
    // m_pDataDlg[6] = m_pDataDlg[5],
    // m_pDataDlg[8] = m_pDataDlg[7]).
    void Linking();

    // ----- 1:1 with legacy CReinforceDataGuideDlg::Show -----

    // 1:1 with legacy Show. SetActive(TRUE), then
    // for each tab: dialog SetActive(FALSE) +
    // button SetPush(FALSE) + SetDisable(FALSE);
    // then activate the current tab
    // (m_wCurData) with SetPush(TRUE) +
    // SetDisable(TRUE).
    void Show();

    // ----- 1:1 with legacy CReinforceDataGuideDlg::Close -----

    // 1:1 with legacy Close. SetActive(FALSE) +
    // m_wCurData = 0.
    void Close();

    // ----- 1:1 with legacy CReinforceDataGuideDlg::SelectData -----

    void SelectData(std::uint16_t index) noexcept { m_wCurData = index; }

    // ----- 1:1 with legacy CReinforceDataGuideDlg::OnActionEvent -----

    // 1:1 with legacy OnActionEvent. 2 paths
    // (1:1 quirk: legacy uses `we == WE_PUSHDOWN`
    // exact match, not bit-and).
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // ----- Local id range + constants (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h WINDOW_ID
    // values (RFDG_BTN1-9 + GUIDE_SHEET1-7 +
    // RFDGUIDE_OKBTN). Local 480-486 (9 pushup
    // buttons) + 487-493 (7 unique sheet
    // dialogs) + 494 (OK button) — distinct
    // from 200-472 used by previous Tier 2
    // dialogs.
    static constexpr std::int32_t kIdBtnBase      = 480;  // 9 pushup buttons (480-488)
    static constexpr std::int32_t kIdSheetBase    = 490;  // 7 unique sheet dialogs (490-496)
    static constexpr std::int32_t kIdOkBtn        = 498;
    static constexpr std::size_t   kNumTabs        = 9;
    static constexpr std::size_t   kNumUniqueSheets = 7;

    // Item kind enum (1:1 with legacy eRFDG_ITEM_KIND).
    // The modern port inlines the enum values (no
    // shared header dependency) — the values match
    // the legacy enum order.
    static constexpr std::int32_t kItemWeapon  = 0;
    static constexpr std::int32_t kItemCap     = 1;
    static constexpr std::int32_t kItemClothes = 2;
    static constexpr std::int32_t kItemBoots   = 3;
    static constexpr std::int32_t kItemClove   = 4;
    static constexpr std::int32_t kItemCloak   = 5;
    static constexpr std::int32_t kItemBlet    = 6;
    static constexpr std::int32_t kItemAmulet  = 7;
    static constexpr std::int32_t kItemRing    = 8;

private:
    // 1:1 with legacy m_pItemKindButton[9] (9
    // cPushupButton tab buttons). Modern port
    // stores raw pointer (not owned; cDialog owns
    // the children).
    cPushupButton* m_pItemKindButton[kNumTabs] = {
        nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
    };

    // 1:1 with legacy m_pDataDlg[9] (9 cDialog
    // page children). Modern port stores raw
    // pointer. 1:1 quirk: m_pDataDlg[6] aliases
    // m_pDataDlg[5]; m_pDataDlg[8] aliases
    // m_pDataDlg[7] (some sheets share the same
    // dialog).
    cDialog* m_pDataDlg[kNumTabs] = {
        nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
    };

    // 1:1 with legacy m_wCurData (current tab
    // index).
    std::uint16_t m_wCurData = 0;
};

}  // namespace mxh::ui
