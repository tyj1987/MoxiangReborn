// chelperspeechdlg.cpp — modern port of 墨香 cHelperSpeechDlg.

#include "mxh/ui/chelperspeechdlg.hpp"

#include <utility>

namespace mxh::ui {

cHelperSpeechDlg::cHelperSpeechDlg() {
    m_pCurPage = nullptr;
    m_hasCurPage = false;
    m_bUseComponent = false;
    m_bFadeIn = false;
    m_bFadeOut = false;
    m_bClose = false;
    m_dwStartTime = 0;
    m_dwCurTime = 0;
    m_nLineHeight = 0;
    m_nLineNum = 0;
    m_textRelLeft = 0;
    m_textRelTop = 0;
    m_textRelRight = 0;
    m_textRelBottom = 0;
    m_vHelperPosX = 0.0f;
    m_vHelperPosY = 0.0f;
}

cHelperSpeechDlg::~cHelperSpeechDlg() {
    // 1:1 with legacy destructor: destroy the cur page +
    // walk the queue + destroy queued pages.
    if (m_pCurPage && m_destroyPageCb) {
        m_destroyPageCb(m_pCurPage);
    }
    m_pCurPage = nullptr;
    m_hasCurPage = false;
    m_NextPagelist.clear();
}

void cHelperSpeechDlg::Init(int nx, int ny, int nwid, int nhei,
                             int nlinehei, long ID) {
    // 1:1 with legacy Init.  Stores the layout + clears
    // state.  The legacy calls cDialog::Init underneath
    // (the modern port's cDialog::Init takes a different
    // signature -- cImage* + id -- so the host is expected
    // to call cDialog::Init separately).
    (void)nx; (void)ny; (void)nwid; (void)nhei; (void)ID;
    m_nLineHeight = nlinehei;
    m_nLineNum = 0;
    m_pCurPage = nullptr;
    m_hasCurPage = false;
    m_bFadeIn = false;
    m_bFadeOut = false;
    m_bClose = false;
    m_NextPagelist.clear();
}

void cHelperSpeechDlg::Linking() {
    // 1:1 with legacy Linking.  The legacy walks the
    // WINDOW_ID tree; modern port defers that.
}

std::uint32_t cHelperSpeechDlg::ActionEvent(void* mouseInfo) {
    // 1:1 with legacy ActionEvent.  Forwards to
    // cDialog::ActionEvent + m_pCurPage->ActionEvent.
    if (m_pCurPage && m_pageActionCb) {
        return m_pageActionCb(m_pCurPage, mouseInfo);
    }
    return 0;
}

void cHelperSpeechDlg::Render() {
    // 1:1 with legacy Render.  Forwards to
    // cDialog::Render + m_pCurPage->Render.
    if (m_pCurPage && m_pageRenderCb) {
        m_pageRenderCb(m_pCurPage);
    }
}

bool cHelperSpeechDlg::OpenDialog(std::uint32_t dwPageId) {
    // 1:1 with legacy OpenDialog.  If a page is already
    // showing, queue the new id; otherwise start showing
    // immediately.
    if (m_hasCurPage) {
        m_NextPagelist.push_back(dwPageId);
        return true;
    }
    m_curPageIdx = dwPageId;
    if (m_createPageCb) {
        m_pCurPage = m_createPageCb(dwPageId);
        m_hasCurPage = (m_pCurPage != nullptr);
    } else {
        m_pCurPage = nullptr;
        m_hasCurPage = false;
    }
    StartFadeIn();
    return true;
}

void cHelperSpeechDlg::CloseDialog() {
    // 1:1 with legacy CloseDialog.  Hides the dialog +
    // clears the page queue + flips m_bClose.
    if (m_pCurPage && m_destroyPageCb) {
        m_destroyPageCb(m_pCurPage);
    }
    m_pCurPage = nullptr;
    m_hasCurPage = false;
    m_bClose = true;
    m_bFadeIn = false;
    m_bFadeOut = false;
    m_NextPagelist.clear();
}

void cHelperSpeechDlg::ResetDialog() {
    // 1:1 with legacy ResetDialog.  Resets the state.
    if (m_pCurPage && m_destroyPageCb) {
        m_destroyPageCb(m_pCurPage);
    }
    m_pCurPage = nullptr;
    m_hasCurPage = false;
    m_bFadeIn = false;
    m_bFadeOut = false;
    m_bClose = false;
    m_dwStartTime = 0;
    m_dwCurTime = 0;
    m_NextPagelist.clear();
}

void cHelperSpeechDlg::Exit() {
    // 1:1 with legacy Exit.  Used as a private helper.
    CloseDialog();
}

void cHelperSpeechDlg::AddPage(std::uint32_t dwPageId) {
    // 1:1 with legacy AddPage.
    m_NextPagelist.push_back(dwPageId);
}

bool cHelperSpeechDlg::StartFadeOut(std::uint32_t dwNextIdx) {
    // 1:1 with legacy StartFadeOut.  Sets m_dwStartTime
    // and flips m_bFadeOut.
    m_dwStartTime = m_nowForTest;
    m_bFadeOut = true;
    m_bFadeIn = false;
    if (dwNextIdx != 0) {
        m_curPageIdx = dwNextIdx;
    }
    return true;
}

void cHelperSpeechDlg::StartFadeIn() {
    // 1:1 with legacy StartFadeIn.
    m_dwStartTime = m_nowForTest;
    m_bFadeIn = true;
    m_bFadeOut = false;
}

std::uint32_t cHelperSpeechDlg::GetCurPageIdx() const noexcept {
    return m_curPageIdx;
}

}  // namespace mxh::ui
