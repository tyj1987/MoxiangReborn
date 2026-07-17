// unionnotedlg.cpp — 1:1 port of 墨香
// CUnionNoteDialog (guild union note sender
// dialog). See unionnotedlg.hpp for the data-model
// rationale + 1:1 quirks.

#include "unionnotedlg.hpp"
#include "ctextarea.hpp"
#include "ceditbox.hpp"

namespace mxh::ui {

cUnionNoteDlg::cUnionNoteDlg() {
    // 1:1 with legacy CUnionNoteDlg ctor:
    //   m_bUse = FALSE;
    //
    // 1:1 quirk: modern bool uses default member
    // init (m_bUse = false in header). ctor body
    // is empty.
}

cUnionNoteDlg::~cUnionNoteDlg() = default;

void cUnionNoteDlg::Linking() {
    // 1:1 with legacy CUnionNoteDlg::Linking. The
    // legacy is:
    //   m_pNoteText = (cTextArea*)GetWindowForID(AN_TEXTREA);
    //   m_pNoteText->SetEnterAllow(FALSE);
    //   m_pNoteText->SetScriptText("");
    m_pNoteText =
        static_cast<cTextArea*>(findWindowById(kIdNoteText));
    if (m_pNoteText) {
        // 1:1 with legacy SetEnterAllow(FALSE).
        m_pNoteText->SetEnterAllow(false);
        m_pNoteText->SetScriptText("");
    }
    // m_pTitleEdit is unused in legacy cpp; modern
    // port doesn't resolve it (preserves 1:1 with
    // legacy unused field).
}

void cUnionNoteDlg::Show(void* pItem) {
    // 1:1 with legacy CUnionNoteDlg::Show. The
    // legacy is:
    //   if (!HERO->GetGuildIdx()) {
    //     CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(35));
    //     return;
    //   }
    //   if (HERO->GetGuildMemberRank() != GUILD_MASTER &&
    //       HERO->GetGuildMemberRank() != GUILD_VICEMASTER) {
    //     CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(1100));
    //     return;
    //   }
    //   if (!HERO->GetGuildUnionIdx()) {
    //     CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(1103));
    //     return;
    //   }
    //   if (pItem == NULL) {
    //     CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(786));
    //     return;
    //   }
    //   if (m_bUse) {
    //     CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(752));
    //     return;
    //   }
    //   m_pItem = pItem;
    //   SetActive(TRUE);
    //
    // The modern port: the 4-singleton dispatch is
    // TODO (R-12.x deferred). Modern port stores
    // pItem + SetActive(true) without the checks.
    // TODO: 1:1 with legacy 4-singleton checks
    //       (HERO + CHATMGR not ported, R-12.x
    //       deferred). When ported, the body
    //       becomes the legacy code.
    m_pItem = pItem;
    SetActive(true);
}

void cUnionNoteDlg::Use() {
    // 1:1 with legacy CUnionNoteDlg::Use. The
    // legacy is:
    //   m_bUse = FALSE;
    //   m_pNoteText->SetScriptText("");
    //   MSG_ITEM_USE_SYN msg;
    //   ...
    //   NETWORK->Send(&msg, sizeof(msg));
    //   ITEMMGR->m_nItemUseCount++;
    //
    // The modern port: clears m_bUse + m_pNoteText +
    // m_pItem. The HERO + NETWORK + ITEMMGR dispatch
    // is TODO.
    m_bUse = false;
    m_pItem = nullptr;
    if (m_pNoteText) {
        m_pNoteText->SetScriptText("");
    }
    // TODO: 1:1 with legacy NETWORK send + ITEMMGR
    //       m_nItemUseCount++ (R-12.x deferred).
}

void cUnionNoteDlg::OnActionEvent(std::int32_t lId, void* p,
                                  std::uint32_t we) {
    // 1:1 with legacy CUnionNoteDlg::OnActionEvnet
    // (typo'd). The legacy is:
    //   if (we & WE_BTNCLICK) {
    //     switch (lId) {
    //     case AN_SENDOKBTN:
    //       MSG_GUILD_SEND_NOTE msg;
    //       ...
    //       NETWORK->Send(&msg, msg.GetMsgLength());
    //       SetActive(FALSE);
    //     case AN_CANCELBTN:
    //       SetActive(FALSE);
    //     }
    //   }
    //
    // The modern port: the whole method is TODO
    // (HERO + NETWORK singletons, R-12.x deferred).
    // The method is a no-op for now; the body
    // becomes the legacy code when CHATMGR + HERO +
    // NETWORK are ported.
    (void)lId;
    (void)p;
    (void)we;
}

}  // namespace mxh::ui
