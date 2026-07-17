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
    //   - The val == FALSE HERO + OBJECTSTATEMGR
    //     dispatch is TODO (R-12.x deferred).
    cDialog::SetActive(val);
    if (!val) {
        // TODO: 1:1 with legacy val == FALSE path:
        //   if (HERO->GetState() == eObjectState_Deal)
        //     OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Deal);
        //
        // HERO + OBJECTSTATEMGR not ported (R-12.x
        // deferred). When ported, the body becomes
        // the legacy code.
    }
}

void cGTRegistcancelDialog::TournamentRegistCancelSyn() {
    // 1:1 with legacy CGTRegistcancelDialog
    // ::TournamentRegistCancelSyn. The legacy is:
    //   MSGBASE msg;
    //   msg.Category = MP_GTOURNAMENT;
    //   msg.Protocol = MP_GTOURNAMENT_REGISTCANCEL_SYN;
    //   msg.dwObjectID = HEROID;
    //   NETWORK->Send(&msg, sizeof(msg));
    //
    // The modern port: the whole method is TODO
    // (2-singleton: HERO + NETWORK not ported,
    // R-12.x deferred). Modern port is a no-op
    // (does not call NETWORK->Send) while
    // singletons are unported. When ported, the
    // body becomes the legacy code.
    // TODO: 2-singleton dispatch (R-12.x deferred).
}

}  // namespace mxh::ui
