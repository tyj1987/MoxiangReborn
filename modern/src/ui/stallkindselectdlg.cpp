// stallkindselectdlg.cpp — 1:1 port of 墨香
// CStallKindSelectDlg (street stall kind selector
// dialog). See stallkindselectdlg.hpp for the
// data-model rationale + 1:1 quirks.

#include "stallkindselectdlg.hpp"
#include "cbutton.hpp"

namespace mxh::ui {

cStallKindSelectDlg::cStallKindSelectDlg() {
    // 1:1 with legacy CStallKindSelectDlg ctor:
    //   m_pSellBtn = m_pBuyBtn = m_pCancelBtn = NULL;
    // Modern port uses default member init in the
    // header (m_pSellBtn / m_pBuyBtn / m_pCancelBtn
    // = nullptr).
}

cStallKindSelectDlg::~cStallKindSelectDlg() = default;

void cStallKindSelectDlg::Linking() {
    // 1:1 with legacy CStallKindSelectDlg::Linking.
    // The legacy is:
    //   m_pSellBtn = (cButton*)GetWindowForID(SO_SELLBTN);
    //   m_pBuyBtn = (cButton*)GetWindowForID(SO_BUYBTN);
    //   m_pCancelBtn = (cButton*)GetWindowForID(SO_CANCELBTN);
    m_pSellBtn   = static_cast<cButton*>(findWindowById(kIdSellBtn));
    m_pBuyBtn    = static_cast<cButton*>(findWindowById(kIdBuyBtn));
    m_pCancelBtn = static_cast<cButton*>(findWindowById(kIdCancelBtn));
}

void cStallKindSelectDlg::Show() {
    // 1:1 with legacy CStallKindSelectDlg::Show.
    // The legacy is:
    //   SetActive(TRUE);
    //   m_pSellBtn->SetActive(TRUE);
    //   m_pBuyBtn->SetActive(TRUE);
    //   m_pCancelBtn->SetActive(TRUE);
    //
    // 1:1 quirk: modern cButton does not have
    // SetActive (cButton extends cWindow which has
    // no SetActive). The modern port uses
    // cWindow::SetVisible(true) as the 1:1
    // equivalent (R-12 fix: SetActive is reserved
    // for cDialog-level state).
    SetActive(true);
    if (m_pSellBtn)   m_pSellBtn->SetVisible(true);
    if (m_pBuyBtn)    m_pBuyBtn->SetVisible(true);
    if (m_pCancelBtn) m_pCancelBtn->SetVisible(true);
}

void cStallKindSelectDlg::Close() {
    // 1:1 with legacy CStallKindSelectDlg::Close.
    // The legacy is:
    //   SetActive(FALSE);
    //   m_pSellBtn->SetActive(FALSE);
    //   m_pBuyBtn->SetActive(FALSE);
    //   m_pCancelBtn->SetActive(FALSE);
    SetActive(false);
    if (m_pSellBtn)   m_pSellBtn->SetVisible(false);
    if (m_pBuyBtn)    m_pBuyBtn->SetVisible(false);
    if (m_pCancelBtn) m_pCancelBtn->SetVisible(false);
}

void cStallKindSelectDlg::OnActionEvent(std::int32_t lId, void* p, std::uint32_t we) {
    // 1:1 with legacy CStallKindSelectDlg
    // ::OnActionEvent (legacy typo'd as
    // "OnActionEvnet" — modern port uses correct
    // spelling). The legacy is:
    //   if (we & WE_BTNCLICK && lId == SO_SELLBTN) {
    //     STREETSTALLMGR->SetStallKind(eSK_SELL);
    //     STREETSTALLMGR->OpenStreetStall();
    //   } else if (we & WE_BTNCLICK && lId == SO_BUYBTN) {
    //     STREETSTALLMGR->SetStallKind(eSK_BUY);
    //     STREETSTALLMGR->OpenStreetStall();
    //   } else if (we & WE_BTNCLICK && lId == SO_CANCELBTN) {
    //     STREETSTALLMGR->SetStallKind(eSK_NULL);
    //     STREETSTALLMGR->SetOpenMsgBox(TRUE);
    //   } else
    //     return;  // unknown id — no Close() call
    //   Close();
    //
    // The modern port:
    //   - Uses kIdSellBtn / kIdBuyBtn / kIdCancelBtn
    //     (1:1 with legacy enum).
    //   - All 3 branches are TODO (STREETSTALLMGR
    //     not ported, R-12.x deferred).
    //   - The `else return;` for unknown ids is
    //     preserved (no Close() call for unknown
    //     ids).
    //   - 1:1 quirk: the `else return;` early-out
    //     for unknown ids means the final Close()
    //     is NOT called for unknown ids (1:1 with
    //     legacy control flow).
    (void)p;
    constexpr std::uint32_t WE_BTNCLICK = 0x0001;
    if (!(we & WE_BTNCLICK)) {
        return;
    }
    bool handled = false;
    if (lId == kIdSellBtn) {
        // TODO: 1:1 with legacy SELL branch:
        //         STREETSTALLMGR->SetStallKind(eSK_SELL);
        //         STREETSTALLMGR->OpenStreetStall();
        handled = true;
    } else if (lId == kIdBuyBtn) {
        // TODO: 1:1 with legacy BUY branch:
        //         STREETSTALLMGR->SetStallKind(eSK_BUY);
        //         STREETSTALLMGR->OpenStreetStall();
        handled = true;
    } else if (lId == kIdCancelBtn) {
        // TODO: 1:1 with legacy CANCEL branch:
        //         STREETSTALLMGR->SetStallKind(eSK_NULL);
        //         STREETSTALLMGR->SetOpenMsgBox(TRUE);
        handled = true;
    }
    if (handled) {
        Close();
    }
    // 1:1 quirk: unknown id falls through
    //   (no Close() call).
}

}  // namespace mxh::ui
