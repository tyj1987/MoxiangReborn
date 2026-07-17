// guildmarkdialog.cpp — 1:1 port of 墨香
// CGuildMarkDialog (guild / guild-union mark
// registration dialog). See guildmarkdialog.hpp for
// the data-model rationale + 1:1 quirks.

#include "guildmarkdialog.hpp"
#include "ctextarea.hpp"
#include "cbutton.hpp"
#include "cwindow.hpp"
#include "ceditbox.hpp"

namespace mxh::ui {

cGuildMarkDialog::cGuildMarkDialog() {
    // 1:1 with legacy CGuildMarkDialog ctor:
    //   m_type = WT_GUILDMARKDLG;
    //
    // 1:1 quirk: modern cWindow does not have
    // m_type field (removed in Phase 6 when cWindow
    // was modernized). The ctor body is dropped.
}

cGuildMarkDialog::~cGuildMarkDialog() = default;

void cGuildMarkDialog::Linking() {
    // 1:1 with legacy CGuildMarkDialog::Linking.
    // The legacy is:
    //   m_pInfoText = (cTextArea*)GetWindowForID(GDM_INFOTEXT);
    //   m_pInfoText->SetScriptText(CHATMGR->GetChatMsg(303));
    //   m_pGuildMarkBtn = (cButton*)GetWindowForID(GDM_REGISTOKBTN);
    //   m_pGuildUnionMarkBtn = (cButton*)GetWindowForID(GUM_REGISTOKBTN);
    m_pInfoText =
        static_cast<cTextArea*>(findWindowById(kIdInfoText));
    if (m_pInfoText) {
        // 1:1 with legacy CHATMGR->GetChatMsg(303)
        // for guild mark info text. Modern port uses
        // kGuildMarkInfoText placeholder until CHATMGR
        // is ported.
        m_pInfoText->SetScriptText(kGuildMarkInfoText);
    }
    m_pGuildMarkBtn =
        static_cast<cButton*>(findWindowById(kIdRegistOkBtn));
    m_pGuildUnionMarkBtn =
        static_cast<cButton*>(findWindowById(kIdUnionRegistOkBtn));
}

void cGuildMarkDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CGuildMarkDialog::SetActive
    // override. The legacy is:
    //   if (val == FALSE) {
    //     cEditBox* pMarkName = (cEditBox*)GetWindowForID(GDM_NAMEEDIT);
    //     pMarkName->SetFocusEdit(FALSE);
    //     if (HERO == 0) return;
    //     if (HERO->GetState() == eObjectState_Deal &&
    //         GAMEIN->GetNpcScriptDialog()->IsActive() == FALSE) {
    //       OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Deal);
    //     }
    //   }
    //   cDialog::SetActive(val);
    //
    // The modern port:
    //   - The val == FALSE path is REAL for
    //     SetFocusEdit(false). The cEditBox
    //     resolution via findWindowById(kIdNameEdit)
    //     is done per-call (legacy GetWindowForID is
    //     a WINDOWMGR->GetWindowForID lookup).
    //   - The HERO + OBJECTSTATEMGR + GAMEIN dispatch
    //     is TODO (R-12.x deferred).
    //   - Always calls base SetActive(val) (matches
    //     legacy call order).
    if (!val) {
        if (auto* pMarkName = static_cast<cEditBox*>(
                findWindowById(kIdNameEdit))) {
            pMarkName->SetFocusEdit(false);
        }
        // TODO: 1:1 with legacy val == FALSE path:
        //   if (HERO == 0) return;
        //   if (HERO->GetState() == eObjectState_Deal &&
        //       GAMEIN->GetNpcScriptDialog()->IsActive() == FALSE) {
        //     OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Deal);
        //   }
        //
        // HERO + OBJECTSTATEMGR + GAMEIN not ported
        // (R-12.x deferred). When ported, the body
        // becomes the legacy code.
    }
    cDialog::SetActive(val);
}

void cGuildMarkDialog::ShowGuildMark() {
    // 1:1 with legacy CGuildMarkDialog::ShowGuildMark.
    // The legacy is:
    //   SetActive(TRUE);
    //   m_pGuildMarkBtn->SetActive(TRUE);
    //   m_pGuildUnionMarkBtn->SetActive(FALSE);
    //   m_pInfoText->SetScriptText(CHATMGR->GetChatMsg(303));
    SetActive(true);
    if (m_pGuildMarkBtn) {
        // 1:1 quirk: modern cButton has no SetActive
        // (inherits cWindow). Modern port uses
        // cWindow::SetVisible as a 1:1 semantic
        // equivalent (R-12 fix).
        m_pGuildMarkBtn->SetVisible(true);
    }
    if (m_pGuildUnionMarkBtn) {
        m_pGuildUnionMarkBtn->SetVisible(false);
    }
    if (m_pInfoText) {
        m_pInfoText->SetScriptText(kGuildMarkInfoText);
    }
}

void cGuildMarkDialog::ShowGuildUnionMark() {
    // 1:1 with legacy CGuildMarkDialog::ShowGuildUnionMark.
    // The legacy is:
    //   SetActive(TRUE);
    //   m_pGuildMarkBtn->SetActive(FALSE);
    //   m_pGuildUnionMarkBtn->SetActive(TRUE);
    //   m_pInfoText->SetScriptText(CHATMGR->GetChatMsg(1114));
    SetActive(true);
    if (m_pGuildMarkBtn) {
        m_pGuildMarkBtn->SetVisible(false);
    }
    if (m_pGuildUnionMarkBtn) {
        m_pGuildUnionMarkBtn->SetVisible(true);
    }
    if (m_pInfoText) {
        m_pInfoText->SetScriptText(kGuildUnionMarkInfoText);
    }
}

}  // namespace mxh::ui
