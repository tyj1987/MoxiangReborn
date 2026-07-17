// namechangedialog.cpp — 1:1 port of 墨香
// CNameChangeDialog (name change editor). See
// namechangedialog.hpp for the data-model
// rationale + 1:1 quirks.

#include "namechangedialog.hpp"
#include "ceditbox.hpp"

namespace mxh::ui {

cNameChangeDialog::cNameChangeDialog() {
    // 1:1 with legacy CNameChangeDialog ctor:
    //   m_type = WT_NAMECHANGE_DLG;
    //   m_dwDBIdx = 0;
    //
    // 1:1 quirk: modern cWindow does not have
    // m_type field (removed in Phase 6 when cWindow
    // was modernized). The ctor body is dropped
    // (m_dwDBIdx uses default member init in the
    // header).
}

cNameChangeDialog::~cNameChangeDialog() = default;

void cNameChangeDialog::Linking() {
    // 1:1 with legacy CNameChangeDialog::Linking.
    // The legacy is:
    //   m_pNameBox = (cEditBox*)GetWindowForID(CH_NAME_CHANGE_EDITBOX);
    //   m_pNameBox->SetValidCheck(VCM_CHARNAME);
    m_pNameBox = static_cast<cEditBox*>(findWindowById(kIdNameBox));
    if (m_pNameBox) {
        m_pNameBox->SetValidCheck(kVcmCharname);
    }
}

void cNameChangeDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CNameChangeDialog::SetActive
    // override. The legacy is:
    //   cDialog::SetActive(val);
    //   if (val)
    //     m_pNameBox->SetEditText("");
    cDialog::SetActive(val);
    if (val && m_pNameBox) {
        // 1:1 quirk: modern SetEditText is a no-op
        // unless InitEditbox was called (m_bInitEdit
        // guard). The test caller must call
        // InitEditbox before this method to make
        // the SetEditText take effect.
        m_pNameBox->SetEditText("");
    }
}

void cNameChangeDialog::NameChangeSyn() {
    // 1:1 with legacy CNameChangeDialog::NameChangeSyn.
    // The legacy is:
    //   DWORD len = 0;
    //   char buf[20];
    //   strcpy(buf, m_pNameBox->GetEditText());
    //   len = strlen(buf);
    //   if (len == 0) { CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(11)); return; }
    //   else if (len < 4) { CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(19)); return; }
    //   else if (len > MAX_NAME_LENGTH) return;
    //   if (strcmp(buf, HERO->GetObjectName()) == 0) return;
    //   if (FILTERTABLE->IsInvalidCharInclude(...)) { CHATMGR->AddMsg(...,14); return; }
    //   if (!FILTERTABLE->IsUsableName(buf)) { CHATMGR->AddMsg(...,14); return; }
    //   if (m_dwDBIdx == 0) return;
    //   SEND_CHANGENAMEBASE msg;
    //   msg.Category = MP_ITEM;
    //   msg.Protocol = MP_ITEM_SHOPITEM_NCHANGE_SYN;
    //   msg.dwObjectID = HERO->GetID();
    //   msg.DBIdx = m_dwDBIdx;
    //   strncpy(msg.Name, buf, MAX_NAME_LENGTH+1);
    //   NETWORK->Send(&msg, sizeof(msg));
    //   SetActive(FALSE);
    //
    // The modern port: the whole method is TODO
    // (4-singleton: CHATMGR + FILTERTABLE + HERO +
    // NETWORK not ported, R-12.x deferred). Returns
    // immediately (no-op) while singletons are
    // unported. When ported, the body becomes the
    // legacy code.
    // TODO: 4-singleton dispatch (R-12.x deferred).
}

}  // namespace mxh::ui
