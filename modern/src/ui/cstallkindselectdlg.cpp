// cstallkindselectdlg.cpp -- modern implementation of
//   Moxiang CStallKindSelectDlg.

#include "cstallkindselectdlg.hpp"

#include "cbutton.hpp"

namespace mxh::ui {

cStallKindSelectDlg::cStallKindSelectDlg() = default;

cStallKindSelectDlg::~cStallKindSelectDlg() = default;

void cStallKindSelectDlg::Linking() {
    // 1:1 with legacy CStallKindSelectDlg::Linking.
    //   m_pSellBtn   = (cButton*)GetWindowForID(SO_SELLBTN);
    //   m_pBuyBtn    = (cButton*)GetWindowForID(SO_BUYBTN);
    //   m_pCancelBtn = (cButton*)GetWindowForID(SO_CANCELBTN);
    // Modern port: prefer the host-injected pointers
    // (set via SetSellBtnForTest / SetBuyBtnForTest /
    // SetCancelBtnForTest); fall back to legacy
    // findWindowById when the host hasn't pre-wired
    // the buttons.
    if (!m_pSellBtn) {
        m_pSellBtn = static_cast<cButton*>(findWindowById(kIdSellBtn));
    }
    if (!m_pBuyBtn) {
        m_pBuyBtn = static_cast<cButton*>(findWindowById(kIdBuyBtn));
    }
    if (!m_pCancelBtn) {
        m_pCancelBtn = static_cast<cButton*>(findWindowById(kIdCancelBtn));
    }
}

void cStallKindSelectDlg::Show() {
    // 1:1 with legacy CStallKindSelectDlg::Show.
    //   SetActive(TRUE);
    //   m_pSellBtn->SetActive(TRUE);
    //   m_pBuyBtn->SetActive(TRUE);
    //   m_pCancelBtn->SetActive(TRUE);
    SetActive(true);
    if (m_pSellBtn)   m_pSellBtn->SetVisible(true);
    if (m_pBuyBtn)    m_pBuyBtn->SetVisible(true);
    if (m_pCancelBtn) m_pCancelBtn->SetVisible(true);
}

void cStallKindSelectDlg::Close() {
    // 1:1 with legacy CStallKindSelectDlg::Close.
    //   SetActive(FALSE);
    //   m_pSellBtn->SetActive(FALSE);
    //   m_pBuyBtn->SetActive(FALSE);
    //   m_pCancelBtn->SetActive(FALSE);
    SetActive(false);
    if (m_pSellBtn)   m_pSellBtn->SetVisible(false);
    if (m_pBuyBtn)    m_pBuyBtn->SetVisible(false);
    if (m_pCancelBtn) m_pCancelBtn->SetVisible(false);
}

void cStallKindSelectDlg::OnActionEvent(std::int32_t lId,
                                        void* /*p*/,
                                        std::uint32_t we) {
    // 1:1 with legacy CStallKindSelectDlg::OnActionEvent
    // (legacy typo'd as "OnActionEvnet").  The legacy
    // body is:
    //   if (we & WE_BTNCLICK && lId == SO_SELLBTN) {
    //       STREETSTALLMGR->SetStallKind(eSK_SELL);
    //       STREETSTALLMGR->OpenStreetStall();
    //   } else if (we & WE_BTNCLICK && lId == SO_BUYBTN) {
    //       STREETSTALLMGR->SetStallKind(eSK_BUY);
    //       STREETSTALLMGR->OpenStreetStall();
    //   } else if (we & WE_BTNCLICK && lId == SO_CANCELBTN) {
    //       STREETSTALLMGR->SetStallKind(eSK_NULL);
    //       STREETSTALLMGR->SetOpenMsgBox(TRUE);
    //   } else
    //       return;  // unknown id -- no Close() call
    //   Close();
    //
    // 1:1 quirk: WE_BTNCLICK == 0x0040 (legacy cWindowDef.h).
    // Modern cWindow::WindowEvent does not export this
    // value; the modern port uses a local constant.
    if (!(we & kWeBtnClick)) {
        return;
    }

    bool handled = false;
    if (lId == kIdSellBtn) {
        // 1:1 with legacy SELL branch.
        if (m_setStallKindCb) {
            m_setStallKindCb(StallKind::Sell, m_setStallKindUser);
        }
        if (m_openStreetStallCb) {
            m_openStreetStallCb(m_openStreetStallUser);
        }
        m_sellDispatched = true;
        m_lastKind       = StallKind::Sell;
        handled = true;
    } else if (lId == kIdBuyBtn) {
        // 1:1 with legacy BUY branch.
        if (m_setStallKindCb) {
            m_setStallKindCb(StallKind::Buy, m_setStallKindUser);
        }
        if (m_openStreetStallCb) {
            m_openStreetStallCb(m_openStreetStallUser);
        }
        m_buyDispatched = true;
        m_lastKind      = StallKind::Buy;
        handled = true;
    } else if (lId == kIdCancelBtn) {
        // 1:1 with legacy CANCEL branch.
        if (m_setStallKindCb) {
            m_setStallKindCb(StallKind::Null, m_setStallKindUser);
        }
        if (m_setOpenMsgBoxCb) {
            m_setOpenMsgBoxCb(true, m_setOpenMsgBoxUser);
        }
        m_cancelDispatched = true;
        m_lastKind         = StallKind::Null;
        handled = true;
    }
    // 1:1 quirk: legacy `else return;` for unknown ids
    // means Close() is NOT called for unknown ids.
    // Modern port preserves via `if (handled) Close();`.
    if (handled) {
        Close();
    }
}

} // namespace mxh::ui
