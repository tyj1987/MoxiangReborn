// gtregistcanceldialog.cpp — 1:1 port of 墨香
// CGTRegistcancelDialog (guild tournament
// registration cancel). See
// gtregistcanceldialog.hpp for the data-model
// rationale + 1:1 quirks.

#include "gtregistcanceldialog.hpp"
#include "cbutton.hpp"

namespace mxh::ui {

cGTRegistcancelDialog::cGTRegistcancelDialog() {
    // 1:1 with legacy CGTRegistcancelDialog ctor:
    //   m_type = WT_GTREGISTCANCEL_DLG;
    //
    // 1:1 quirk: modern cWindow does not have
    // m_type field (removed in Phase 6 when cWindow
    // was modernized). The ctor body is dropped.
}

cGTRegistcancelDialog::~cGTRegistcancelDialog() = default;

void cGTRegistcancelDialog::Linking() {
    // 1:1 with legacy CGTRegistcancelDialog::Linking.
    // The legacy is:
    //   m_pCancelBtn = (cButton*)GetWindowForID(GDT_CANCELBTN);
    m_pCancelBtn = static_cast<cButton*>(findWindowById(kIdCancelBtn));
}

void cGTRegistcancelDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CGTRegistcancelDialog
    // ::SetActive override. The legacy is:
    //   cDialog::SetActive(val);
    //   if (!val) {
    //     if (HERO->GetState() == eObjectState_Deal)
    //       OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Deal);
    //   }
    //
    // The modern port:
    //   - Always calls base SetActive(val) (matches
    //     legacy call order).
    //   - When val == FALSE, the host HERO state check
    //     + host OBJECTSTATEMGR EndObjectState(Deal)
    //     are dispatched via OPTIONAL callbacks
    //     (replacing the legacy singletons).
    cDialog::SetActive(val);
    if (!val) {
        if (m_getHeroStateFn && m_endDealStateFn) {
            const std::int32_t heroState = m_getHeroStateFn(m_callbackUserData);
            if (heroState == kObjectStateDeal) {
                m_endDealStateFn(m_callbackUserData);
            }
        }
    }
}

void cGTRegistcancelDialog::SetCallbacks(
    GetHeroStateFn getHeroState,
    EndDealStateFn endDealState,
    void* userData) noexcept {
    m_getHeroStateFn   = getHeroState;
    m_endDealStateFn   = endDealState;
    m_callbackUserData = userData;
}

void cGTRegistcancelDialog::SetTournamentCallbacks(
    GetHeroObjectIdFn getHeroObjectId,
    SendTournamentCancelFn sendTournamentCancel,
    void* userData) noexcept {
    m_getHeroObjectIdFn = getHeroObjectId;
    m_sendTournamentCancelFn = sendTournamentCancel;
    m_tournamentUserData = userData;
}

void cGTRegistcancelDialog::TournamentRegistCancelSyn() {
    if (!m_getHeroObjectIdFn || !m_sendTournamentCancelFn) {
        return;
    }
    const std::uint32_t objectId =
        m_getHeroObjectIdFn(m_tournamentUserData);
    (void)m_sendTournamentCancelFn(objectId, m_tournamentUserData);
}

}  // namespace mxh::ui
