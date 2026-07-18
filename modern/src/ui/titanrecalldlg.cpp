// titanrecalldlg.cpp — 1:1 port of 墨香
// CTitanRecallDlg (titan recall progress bar
// dialog). See titanrecalldlg.hpp for the
// data-model rationale + 1:1 quirks.

#include "titanrecalldlg.hpp"
#include "cobjectguagen.hpp"
#include "cstatic.hpp"

namespace mxh::ui {

cTitanRecallDlg::cTitanRecallDlg() {
    // 1:1 with legacy CTitanRecallDlg ctor:
    //   m_bSuccessRecall = FALSE;
    //
    // 1:1 quirk: modern bool uses default member
    // init (m_bSuccessRecall = false in header).
    // ctor body is empty.
}

cTitanRecallDlg::~cTitanRecallDlg() = default;

void cTitanRecallDlg::Linking() {
    // 1:1 with legacy CTitanRecallDlg::Linking. The
    // legacy is:
    //   m_pProgressGuagen = (CObjectGuagen*)GetWindowForID(TITAN_RECALL_GUAGE);
    //   m_pRemaintimeStatic = (cStatic*)GetWindowForID(TITAN_RECALL_TIME);
    //   SetSuccessTime(7000);
    SetProgressGuagen(static_cast<cObjectGuagen*>(
        findWindowById(kIdProgressBarGage)));
    SetRemaintimeStatic(static_cast<cStatic*>(
        findWindowById(kIdRemaintimeTime)));
    SetSuccessTime(kBaseSuccessTime);
}

void cTitanRecallDlg::Render() {
    // 1:1 with legacy CTitanRecallDlg::Render. The
    // legacy is:
    //   if (GetSuccessProgress() == TRUE) {
    //     MSGBASE msg;
    //     msg.Category = MP_TITAN;
    //     msg.Protocol = MP_TITAN_RECALL_SYN;
    //     msg.dwObjectID = HERO->GetID();
    //     NETWORK->Send(&msg, sizeof(msg));
    //     InitProgress();
    //   }
    //   CProgressBarDlg::Render();
    //
    // The modern port: the NETWORK send + InitProgress
    // dispatch is TODO (R-12.x deferred). Modern port
    // calls base Render.
    cProgressBarDlg::Render();
    // TODO: 1:1 with legacy NETWORK send + InitProgress
    //       (R-12.x deferred).
}

bool cTitanRecallDlg::OnActionEvent(std::int32_t lId, void* p,
                                    std::uint32_t we) {
    // 1:1 with legacy CTitanRecallDlg::OnActionEvent.
    // The legacy is:
    //   switch (we) {
    //   case WE_CLOSEWINDOW:
    //     if (HERO->GetState() == eObjectState_Society)
    //       OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Society);
    //     return TRUE;
    //   }
    //   switch (lId) {
    //   case TITAN_RECALL_CANCEL:
    //     MSGBASE msg;
    //     msg.Category = MP_TITAN;
    //     msg.Protocol = MP_TITAN_RECALL_CANCEL_SYN;
    //     msg.dwObjectID = HERO->GetID();
    //     NETWORK->Send(&msg, sizeof(msg));
    //     break;
    //   }
    //   return TRUE;
    //
    // The modern port: the HERO + OBJECTSTATEMGR +
    // NETWORK dispatch is TODO (R-12.x deferred).
    // Modern port returns TRUE (legacy also returns
    // TRUE).
    (void)lId;
    (void)p;
    (void)we;
    // TODO: 1:1 with legacy 2-switch dispatch
    //       (HERO + OBJECTSTATEMGR + NETWORK not
    //       ported, R-12.x deferred).
    return true;
}

}  // namespace mxh::ui
