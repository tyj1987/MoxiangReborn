// reinforcedataguidedlg.cpp — 1:1 port of 墨香
// CReinforceDataGuideDlg (reinforce data guide).
// See reinforcedataguidedlg.hpp for the
// data-model rationale + 1:1 quirks.

#include "reinforcedataguidedlg.hpp"
#include "legacy_window_event.hpp"
#include "cpushupbutton.hpp"

namespace mxh::ui {

cReinforceDataGuideDlg::cReinforceDataGuideDlg() {
    // 1:1 with legacy CReinforceDataGuideDlg ctor:
    //   for (int i = 0; i < 9; i++) {
    //     m_pItemKindButton[i] = NULL;
    //     m_pDataDlg[i] = NULL;
    //   }
    //   m_wCurData = 0;
    // Modern port uses default member init in the
    // header (m_pItemKindButton[i] = nullptr,
    // m_pDataDlg[i] = nullptr, m_wCurData = 0).
}

cReinforceDataGuideDlg::~cReinforceDataGuideDlg() = default;

void cReinforceDataGuideDlg::Linking() {
    // 1:1 with legacy CReinforceDataGuideDlg::Linking.
    // The legacy is:
    //   m_pItemKindButton[0..8] = (cPushupButton*)GetWindowForID(RFDG_BTN1..9);
    //   m_pDataDlg[0..5] = (cDialog*)GetWindowForID(GUIDE_SHEET1..6);
    //   m_pDataDlg[6] = m_pDataDlg[5];  // 1:1 quirk
    //   m_pDataDlg[7] = (cDialog*)GetWindowForID(GUIDE_SHEET7);
    //   m_pDataDlg[8] = m_pDataDlg[7];  // 1:1 quirk
    for (std::size_t i = 0; i < kNumTabs; ++i) {
        m_pItemKindButton[i] = static_cast<cPushupButton*>(
            findWindowById(kIdBtnBase + static_cast<std::int32_t>(i)));
    }
    for (std::size_t i = 0; i < kNumUniqueSheets; ++i) {
        m_pDataDlg[i] = static_cast<cDialog*>(
            findWindowById(kIdSheetBase + static_cast<std::int32_t>(i)));
    }
    // 1:1 quirks: m_pDataDlg[6] aliases
    // m_pDataDlg[5]; m_pDataDlg[8] aliases
    // m_pDataDlg[7].
    m_pDataDlg[6] = m_pDataDlg[5];
    m_pDataDlg[8] = m_pDataDlg[7];
}

void cReinforceDataGuideDlg::Show() {
    // 1:1 with legacy CReinforceDataGuideDlg::Show.
    // The legacy is:
    //   SetActive(TRUE);
    //   for (int i = 0; i < 9; i++) {
    //     m_pDataDlg[i]->SetActive(FALSE);
    //     m_pItemKindButton[i]->SetPush(FALSE);
    //     m_pItemKindButton[i]->SetDisable(FALSE);
    //   }
    //   m_pDataDlg[m_wCurData]->SetActive(TRUE);
    //   m_pItemKindButton[m_wCurData]->SetPush(TRUE);
    //   m_pItemKindButton[m_wCurData]->SetDisable(TRUE);
    //
    // 1:1 quirk: modern cPushupButton doesn't have
    // SetActive; modern port uses cWindow::SetVisible
    // (1:1 with cTipBrowserDlg pattern, R-12 fix).
    SetActive(true);
    for (std::size_t i = 0; i < kNumTabs; ++i) {
        if (m_pDataDlg[i])        m_pDataDlg[i]->SetActive(false);
        if (m_pItemKindButton[i]) m_pItemKindButton[i]->SetPush(false);
        if (m_pItemKindButton[i]) m_pItemKindButton[i]->SetDisable(false);
    }
    if (m_wCurData < kNumTabs) {
        if (m_pDataDlg[m_wCurData])
            m_pDataDlg[m_wCurData]->SetActive(true);
        if (m_pItemKindButton[m_wCurData])
            m_pItemKindButton[m_wCurData]->SetPush(true);
        if (m_pItemKindButton[m_wCurData])
            m_pItemKindButton[m_wCurData]->SetDisable(true);
    }
}

void cReinforceDataGuideDlg::Close() {
    // 1:1 with legacy CReinforceDataGuideDlg::Close.
    // The legacy is:
    //   SetActive(FALSE);
    //   m_wCurData = 0;
    SetActive(false);
    m_wCurData = 0;
}

void cReinforceDataGuideDlg::OnActionEvent(std::int32_t lId, void* p, std::uint32_t we) {
    // 1:1 with legacy CReinforceDataGuideDlg
    // ::OnActionEvent. The legacy is:
    //   if (we == WE_PUSHDOWN) {
    //     WORD id = (WORD)(lId - RFDG_BTN1);
    //     if (id < 9 && (lId - RFDG_BTN1) >= 0) {
    //       SelectData(id);
    //       Show();
    //       return;
    //     }
    //   }
    //   if (we & WE_BTNCLICK && lId == RFDGUIDE_OKBTN) {
    //     Close();
    //   }
    //
    // 1:1 quirk: legacy uses `we == WE_PUSHDOWN`
    // exact match (not bit-and). Modern port
    // preserves the `==`.
    (void)p;
    constexpr std::uint32_t WE_BTNCLICK = legacy_window_event::kButtonClick;
    constexpr std::uint32_t WE_PUSHDOWN  = legacy_window_event::kPushDown;
    if (we == WE_PUSHDOWN) {
        std::int32_t offset = lId - kIdBtnBase;
        if (offset >= 0 && offset < static_cast<std::int32_t>(kNumTabs)) {
            m_wCurData = static_cast<std::uint16_t>(offset);
            Show();
            return;
        }
    }
    if ((we & WE_BTNCLICK) && lId == kIdOkBtn) {
        Close();
    }
}

}  // namespace mxh::ui
