// wanteddialog.cpp — 1:1 port of 墨香 CWantedDialog
// (wanted list dialog). See wanteddialog.hpp for
// the data-model rationale + 1:1 quirks.

#include "wanteddialog.hpp"
#include "clistdialog.hpp"

namespace mxh::ui {

cWantedDialog::cWantedDialog() {
    // 1:1 with legacy CWantedDialog ctor:
    //   m_type = WT_WANTEDDIALOG;
    //
    // 1:1 quirk: modern cWindow does not have
    // m_type field (removed in Phase 6 when cWindow
    // was modernized). The ctor body is dropped.
}

cWantedDialog::~cWantedDialog() = default;

void cWantedDialog::Linking() {
    // 1:1 with legacy CWantedDialog::Linking. The
    // legacy is:
    //   m_pWantedLDG = (cListDialog*)GetWindowForID(QUE_WANTEDLDLG);
    m_pWantedLDG = static_cast<cListDialog*>(findWindowById(kIdWantedList));
}

void cWantedDialog::SetInfo() {
    // 1:1 with legacy CWantedDialog::SetInfo. The
    // legacy is:
    //   InitWanted();
    //   for (int i = 0; i < MAX_WANTED_NUM; ++i) {
    //     if (pInfo[i].WantedIDX == 0) break;
    //     m_pWantedLDG->AddItem(pInfo[i].RegistDate, 0xffffffff);
    //     sprintf(temp, CHATMGR->GetChatMsg(545), pInfo[i].WantedName);
    //     m_pWantedLDG->AddItem(temp, 0xffffffff);
    //   }
    //   m_pWantedLDG->ResetGuageBarPos();
    //
    // The modern port:
    //   - Calls InitWanted (REAL) — clears the
    //     cListDialog before populating.
    //   - The for-loop is TODO (WANTEDLIST struct
    //     not ported + CHATMGR not ported, R-12.x
    //     deferred). When ported, the body
    //     becomes the legacy code.
    //   - 1:1 quirk: legacy passes the WANTEDLIST*
    //     via a function parameter; modern port
    //     uses a no-arg signature (the WANTEDLIST
    //     struct is not yet ported). When ported,
    //     the signature becomes `SetInfo(WANTEDLIST* pInfo)`.
    InitWanted();
    // TODO: WANTEDLIST struct + CHATMGR not ported
    //       (R-12.x deferred). When ported, the
    //       body becomes the legacy for-loop with
    //       MAX_WANTED_NUM + WANTEDLIST struct deref
    //       + CHATMGR->GetChatMsg(545) + cListDialog
    //       ::AddItem + cListDialog::ResetGuageBarPos.
}

void cWantedDialog::AddInfo() {
    // 1:1 with legacy CWantedDialog::AddInfo. The
    // legacy is:
    //   m_pWantedLDG->AddItem(pInfo->RegistDate, 0xffffffff);
    //   sprintf(temp, CHATMGR->GetChatMsg(545), pInfo->WantedName);
    //   m_pWantedLDG->AddItem(temp, 0xffffffff);
    //
    // The modern port: TODO (WANTEDLIST struct +
    // CHATMGR not ported, R-12.x deferred).
    // TODO: WANTEDLIST struct + CHATMGR not ported
    //       (R-12.x deferred). When ported, the
    //       body becomes the legacy code.
}

void cWantedDialog::InitWanted() {
    // 1:1 with legacy CWantedDialog::InitWanted.
    // The legacy is:
    //   m_pWantedLDG->RemoveAll();
    if (m_pWantedLDG) {
        m_pWantedLDG->RemoveAll();
    }
}

}  // namespace mxh::ui
