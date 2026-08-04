// gtregistdialog.cpp — 1:1 port of 墨香 CGTRegistDialog
// (guild tournament registration). See
// gtregistdialog.hpp for the data-model rationale
// + 1:1 quirks.

#include "gtregistdialog.hpp"
#include "cstatic.hpp"
#include "cbutton.hpp"

namespace mxh::ui {

cGTRegistDialog::cGTRegistDialog() {
    // 1:1 with legacy CGTRegistDialog ctor:
    //   m_type = WT_GTREGIST_DLG;
    //
    // 1:1 quirk: modern cWindow does not have
    // m_type field (removed in Phase 6 when cWindow
    // was modernized). The ctor body is dropped.
}

cGTRegistDialog::~cGTRegistDialog() = default;

void cGTRegistDialog::Linking() {
    // 1:1 with legacy CGTRegistDialog::Linking.
    // The legacy is:
    //   m_pRegistGuild = (cStatic*)GetWindowForID(GDT_ENTRY1);
    //   m_pRegistableGuild = (cStatic*)GetWindowForID(GDT_ENTRY2);
    //   m_pRegistBtn = (cButton*)GetWindowForID(GDT_ENTRYBTN);
    m_pRegistGuild     = static_cast<cStatic*>(findWindowById(kIdRegistGuild));
    m_pRegistableGuild = static_cast<cStatic*>(findWindowById(kIdRegistableGuild));
    m_pRegistBtn       = static_cast<cButton*>(findWindowById(kIdRegistBtn));
}

void cGTRegistDialog::SetActive(bool val) noexcept {
    // 1:1 with legacy CGTRegistDialog::SetActive
    // override. The legacy is:
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
    //     + host OBJECTSTATEMGR EndObjectState(Deal) are
    //     dispatched via OPTIONAL callbacks.
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

void cGTRegistDialog::SetCallbacks(
    GetHeroStateFn getHeroState,
    EndDealStateFn endDealState,
    void* userData) noexcept {
    m_getHeroStateFn   = getHeroState;
    m_endDealStateFn   = endDealState;
    m_callbackUserData = userData;
}

void cGTRegistDialog::SetTournamentCallbacks(
    GetGuildMemberRankFn getGuildMemberRank,
    GetHeroObjectIdFn getHeroObjectId,
    SendTournamentRegistFn sendTournamentRegist,
    void* userData) noexcept {
    m_getGuildMemberRankFn = getGuildMemberRank;
    m_getHeroObjectIdFn = getHeroObjectId;
    m_sendTournamentRegistFn = sendTournamentRegist;
    m_tournamentUserData = userData;
}

std::uint32_t cGTRegistDialog::TournamentRegistSyn() {
    if (!m_getGuildMemberRankFn
        || m_getGuildMemberRankFn(m_tournamentUserData) != kGuildMasterRank) {
        return kErrorNoGuildMaster;
    }
    if (m_getHeroObjectIdFn && m_sendTournamentRegistFn) {
        const std::uint32_t objectId =
            m_getHeroObjectIdFn(m_tournamentUserData);
        (void)m_sendTournamentRegistFn(objectId, m_tournamentUserData);
    }
    return kErrorNoError;
}

void cGTRegistDialog::SetRegistGuildCount(std::uint32_t count) {
    if (m_pRegistGuild) {
        m_pRegistGuild->SetStaticValue(static_cast<std::int32_t>(count));
    }
    if (m_pRegistableGuild) {
        const std::uint32_t registable = kMaxGuildInTournament - count;
        m_pRegistableGuild->SetStaticValue(
            static_cast<std::int32_t>(registable));
    }
}

}  // namespace mxh::ui
