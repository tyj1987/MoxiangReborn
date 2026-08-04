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

void cTitanRecallDlg::SetRecallSendCallbacks(
    GetHeroObjectIdFn getHeroObjectId,
    SendRecallSynFn sendRecallSyn,
    SendRecallCancelSynFn sendRecallCancelSyn,
    void* userData) noexcept {
    m_getHeroObjectIdFn = getHeroObjectId;
    m_sendRecallSynFn = sendRecallSyn;
    m_sendRecallCancelSynFn = sendRecallCancelSyn;
    m_recallUserData = userData;
}

void cTitanRecallDlg::SetCloseWindowCallbacks(
    GetHeroObjectIdFn getHeroObjectId,
    GetHeroStateFn getHeroState,
    EndObjectStateFn endObjectState,
    void* userData) noexcept {
    // reuse the hero-object-id slot since both bundles
    // share the same HERO->GetID() lookup; track two
    // userData slots so each bundle is independent.
    m_getHeroObjectIdFn = getHeroObjectId;
    m_getHeroStateFn = getHeroState;
    m_endObjectStateFn = endObjectState;
    m_closeWindowUserData = userData;
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
    // The modern port: when GetSuccessProgress()=true,
    // the RECALL send callback (SetRecallSendCallbacks)
    // is invoked with HEROID (legacy NETWORK send of
    // MP_TITAN/MP_TITAN_RECALL_SYN) and InitProgress()
    // is called (1:1 with legacy post-send teardown).
    // With no callback registered the send is skipped
    // but InitProgress() still fires (1:1 with legacy
    // always-tear-down behavior). The send result is
    // ignored (1:1 with legacy NETWORK->Send return).
    if (GetSuccessProgress()) {
        if (m_getHeroObjectIdFn && m_sendRecallSynFn) {
            const std::uint32_t objectId =
                m_getHeroObjectIdFn(m_recallUserData);
            (void)m_sendRecallSynFn(objectId, m_recallUserData);
        }
        InitProgress();
    }
    cProgressBarDlg::Render();
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
    // The modern port: WE_CLOSEWINDOW branch reads the
    // hero state through the close-window host bundle
    // and ends eObjectState_Society (kObjectStateSociety=24)
    // when in that state. kIdCancelBtn branch sends the
    // legacy cancel MSGBASE through the recall host
    // bundle. The send result is ignored (1:1 with legacy).
    // Returns TRUE (1:1 with legacy).
    if (we == kWeCloseWindow) {
        if (m_getHeroObjectIdFn && m_getHeroStateFn &&
            m_endObjectStateFn) {
            const std::uint32_t objectId =
                m_getHeroObjectIdFn(m_closeWindowUserData);
            const std::int32_t state =
                m_getHeroStateFn(m_closeWindowUserData);
            if (state == kObjectStateSociety) {
                m_endObjectStateFn(objectId, state,
                                   m_closeWindowUserData);
            }
        }
        return true;
    }
    if (lId == kIdCancelBtn) {
        if (m_getHeroObjectIdFn && m_sendRecallCancelSynFn) {
            const std::uint32_t objectId =
                m_getHeroObjectIdFn(m_recallUserData);
            (void)m_sendRecallCancelSynFn(objectId, m_recallUserData);
        }
        return true;
    }
    (void)p;
    return true;
}

}  // namespace mxh::ui
