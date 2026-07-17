// shoutdialog.cpp — 1:1 port of 墨香 CShoutDialog
// (shout message sender dialog). See shoutdialog.hpp
// for the data-model rationale + 1:1 quirks.

#include "shoutdialog.hpp"
#include "ceditbox.hpp"

namespace mxh::ui {

cShoutDialog::cShoutDialog() {
    // 1:1 with legacy CShoutDialog ctor:
    //   m_type = WT_SHOUT_DLG;
    //   m_dwItemIdx = m_dwItemPos = 0;
    //
    // 1:1 quirk: modern cWindow does not have
    // m_type field (removed in Phase 6 when cWindow
    // was modernized). The ctor body is dropped
    // (m_dwItemIdx / m_dwItemPos use default
    // member init in the header).
}

cShoutDialog::~cShoutDialog() = default;

void cShoutDialog::Linking() {
    // 1:1 with legacy CShoutDialog::Linking. The
    // legacy is:
    //   m_pMsgBox = (cEditBox*)GetWindowForID(CHA_MSG);
    m_pMsgBox = static_cast<cEditBox*>(findWindowById(kIdMsgBox));
}

void cShoutDialog::SetItemInfo(std::uint32_t itemIdx, std::uint32_t itemPos) noexcept {
    // 1:1 with legacy CShoutDialog::SetItemInfo
    // (inline setter in the header).
    m_dwItemIdx = itemIdx;
    m_dwItemPos = itemPos;
}

bool cShoutDialog::SendShoutMsgSyn() {
    // 1:1 with legacy CShoutDialog::SendShoutMsgSyn.
    // The legacy is:
    //   char buf[MAX_SHOUT_LENGTH+1] = {0,};
    //   strcpy(buf, m_pMsgBox->GetEditText());
    //   if (strlen(buf) == 0) {
    //     CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(903));
    //     return FALSE;
    //   }
    //   m_pMsgBox->SetEditText("");
    //   if (FILTERTABLE->FilterChat(buf)) {
    //     CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(27));
    //     return FALSE;
    //   }
    //   SEND_SHOUTBASE_ITEMINFO msg;
    //   sprintf(msg.Msg, "%s : %s", HERO->GetObjectName(), buf);
    //   msg.Category = MP_ITEM;
    //   msg.Protocol = MP_ITEM_SHOPITEM_SHOUT_SYN;
    //   msg.dwObjectID = HERO->GetID();
    //   msg.ItemIdx = (WORD)m_dwItemIdx;
    //   msg.ItemPos = (WORD)m_dwItemPos;
    //   NETWORK->Send(&msg, sizeof(msg));
    //   SetActive(FALSE);
    //   m_dwItemIdx = m_dwItemPos = 0;
    //   return TRUE;
    //
    // The modern port: the whole method is TODO
    // (4-singleton: CHATMGR + FILTERTABLE + HERO +
    // NETWORK not ported, R-12.x deferred). Returns
    // false (matching the legacy "early return on
    // empty" path as a safe no-op while the
    // singletons are unported). When the singletons
    // are ported, the body becomes the legacy code.
    // TODO: 4-singleton dispatch (R-12.x deferred).
    return false;
}

}  // namespace mxh::ui
