// mpnoticedialog.cpp — 1:1 port of 墨香 CMPNoticeDialog
// (MP notice dialog). See mpnoticedialog.hpp for the
// data-model rationale + 1:1 quirks.

#include "mpnoticedialog.hpp"
#include "ctextarea.hpp"

namespace mxh::ui {

cMPNoticeDialog::cMPNoticeDialog() = default;

cMPNoticeDialog::~cMPNoticeDialog() = default;

void cMPNoticeDialog::Linking() {
    // 1:1 with legacy CMPNoticeDialog::Linking. REAL —
    // resolves 2 cTextArea children and calls
    // SetScriptText on each. The legacy uses
    // CHATMGR->GetChatMsg(667) and
    // CHATMGR->GetChatMsg(668) for the text; the modern
    // port uses placeholder text until CHATMGR is
    // ported (the SetScriptText signature is exercised
    // end-to-end either way).
    m_pNCaution   = static_cast<cTextArea*>(findWindowById(kNCautionId));
    m_pNRedCaution = static_cast<cTextArea*>(findWindowById(kNRedCautionId));

    // 1:1 quirk: the legacy calls
    //   m_pNCaution->SetScriptText(CHATMGR->GetChatMsg(667))
    //   m_pNRedCaution->SetScriptText(CHATMGR->GetChatMsg(668))
    // Modern port uses placeholder text "MP_NCAUTION" +
    // "MP_NREDCAUTION" to keep the cTextArea API
    // exercised without depending on CHATMGR. When
    // CHATMGR is ported, replace these with
    // CHATMGR->GetChatMsg(667) / (668).
    if (m_pNCaution)   m_pNCaution->SetScriptText("MP_NCAUTION");
    if (m_pNRedCaution) m_pNRedCaution->SetScriptText("MP_NREDCAUTION");
}

}  // namespace mxh::ui
