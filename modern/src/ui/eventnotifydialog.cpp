// eventnotifydialog.cpp — 1:1 port of 墨香 CEventNotifyDialog
// (GM event notification dialog). See eventnotifydialog.hpp
// for the data-model rationale + 1:1 quirks.

#include "eventnotifydialog.hpp"
#include "cstatic.hpp"
#include "ctextarea.hpp"

namespace mxh::ui {

cEventNotifyDialog::cEventNotifyDialog() = default;

cEventNotifyDialog::~cEventNotifyDialog() = default;

void cEventNotifyDialog::Linking() {
    // 1:1 with legacy CEventNotifyDialog::Linking. REAL
    // — resolve 2 children by id. The SetToolTip call
    // from the legacy is commented out (1:1 quirk:
    // legacy's tool tip requires SCRIPTMGR + CHATMGR
    // singletons which are not ported). Modern port
    // documents it as TODO.
    m_pStcTitle  = static_cast<cStatic*>(findWindowById(kStcTitleId));
    m_pTAContext = static_cast<cTextArea*>(findWindowById(kTAContextId));
    // TODO: call m_pTAContext->SetToolTip(CHATMGR->GetChatMsg(782),
    //       RGB_HALF(255,255,255), &ToolTipImg) once SCRIPTMGR +
    //       CHATMGR singletons are ported. The ToolTipImg is
    //       fetched from SCRIPTMGR->GetImage(63, ..., PFT_HARDPATH).
}

void cEventNotifyDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CEventNotifyDialog::SetActive. The
    // legacy is:
    //   cDialog::SetActive(val);
    //   if (m_pTAContext && !val)
    //       m_pTAContext->SetScriptText("");
    //
    // Modern port: calls base SetActive first, then
    // clears the context text on deactivation.
    cDialog::SetActive(val);
    if (m_pTAContext && !val) {
        m_pTAContext->SetScriptText("");
    }
    // TODO: dispatch to NOTIFYMGR->SetNotifyActive(val)
    //       once NOTIFYMGR is ported.
}

std::uint32_t cEventNotifyDialog::ActionEvent(std::int32_t mouseX,
                                              std::int32_t mouseY,
                                              std::uint32_t mouseFlags) {
    // 1:1 with legacy CEventNotifyDialog::ActionEvent.
    // The legacy is:
    //   DWORD we = cDialog::ActionEvent(mouseInfo);
    //   // (click-to-close logic commented out)
    //   return we;
    //
    // Modern port: calls base ActionEvent. The
    // click-to-close logic is documented as TODO
    // (1:1 quirk: legacy never activated the
    // click-to-close path; the comment block is
    // preserved in the TODO).
    return cDialog::ActionEvent(mouseX, mouseY, mouseFlags);
    // TODO: click-to-close (if (LButtonDown &&
    //       m_pTAContext->PtInWindow(...)) SetActive(false);
    //       once PtInWindow is ported on cTextArea).
}

void cEventNotifyDialog::SetTitle(const char* title) {
    // 1:1 with legacy CEventNotifyDialog::SetTitle.
    // Defensive null-check (the legacy unconditionally
    // dereferences m_pStcTitle).
    if (m_pStcTitle) m_pStcTitle->SetStaticText(title);
}

void cEventNotifyDialog::SetContext(const char* context) {
    // 1:1 with legacy CEventNotifyDialog::SetContext.
    // Defensive null-check (the legacy unconditionally
    // dereferences m_pTAContext).
    if (m_pTAContext) m_pTAContext->SetScriptText(context);
}

void cEventNotifyDialog::SetEventCount(bool /*bAdd*/) noexcept {
    // 1:1 quirk: legacy SetEventCount(bool bAdd) was
    // a state-machine method (probably used to update
    // the unread count / update the event count
    // display). The modern port keeps the method
    // signature for API compatibility but the body
    // is a no-op until the underlying state (event
    // count tracker) is ported.
    // TODO: implement event count tracking.
}

}  // namespace mxh::ui
