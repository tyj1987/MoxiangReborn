// guildnotedlg.cpp — 1:1 port of 墨香
// CGuildNoteDialog (guild note sender dialog).
// See guildnotedlg.hpp for the data-model
// rationale + 1:1 quirks.

#include "guildnotedlg.hpp"
#include "ctextarea.hpp"
#include "ceditbox.hpp"

namespace mxh::ui {

cGuildNoteDlg::cGuildNoteDlg() {
    // 1:1 with legacy CGuildNoteDlg ctor:
    //   m_bUse = FALSE;
    //
    // 1:1 quirk: modern bool uses default member
    // init (m_bUse = false in header). ctor body
    // is empty.
}

cGuildNoteDlg::~cGuildNoteDlg() = default;

void cGuildNoteDlg::Linking() {
    // 1:1 with legacy CGuildNoteDlg::Linking.
    m_pNoteText =
        static_cast<cTextArea*>(findWindowById(kIdNoteText));
    if (m_pNoteText) {
        m_pNoteText->SetEnterAllow(false);
        m_pNoteText->SetScriptText("");
    }
}

void cGuildNoteDlg::Show(void* pItem) {
    // 1:1 with legacy CGuildNoteDlg::Show. The
    // legacy is:
    //   if (!HERO->GetGuildIdx()) { CHATMGR->...; return; }
    //   if (pItem == NULL) { CHATMGR->...; return; }
    //   if (m_bUse) { CHATMGR->...; return; }
    //   m_pItem = pItem;
    //   SetActive(TRUE);
    //
    // The modern port: the 3-singleton dispatch is
    // TODO (R-12.x deferred). Modern port stores
    // pItem + SetActive(true) without the checks.
    m_pItem = pItem;
    SetActive(true);
}

void cGuildNoteDlg::Use() {
    // 1:1 with legacy CGuildNoteDlg::Use. The
    // legacy is:
    //   m_bUse = FALSE;
    //   m_pNoteText->SetScriptText("");
    //   MSG_ITEM_USE_SYN msg;
    //   ...
    //   NETWORK->Send(&msg, sizeof(msg));
    //   ITEMMGR->m_nItemUseCount++;
    //
    // The modern port: clears m_bUse + m_pNoteText +
    // m_pItem. The NETWORK + ITEMMGR dispatch is
    // TODO.
    m_bUse = false;
    m_pItem = nullptr;
    if (m_pNoteText) {
        m_pNoteText->SetScriptText("");
    }
}

void cGuildNoteDlg::OnActionEvent(std::int32_t lId, void* p,
                                  std::uint32_t we) {
    // 1:1 with legacy CGuildNoteDlg::OnActionEvnet
    // (typo'd). The whole method is TODO
    // (NETWORK singletons, R-12.x deferred). Modern
    // port is no-op.
    (void)lId;
    (void)p;
    (void)we;
}

}  // namespace mxh::ui
