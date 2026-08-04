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

std::uint32_t cGTRegistDialog::TournamentRegistSyn() {
    // 1:1 with legacy CGTRegistDialog::TournamentRegistSyn.
    // The legacy is:
    //   if (HERO->GetGuildMemberRank() != GUILD_MASTER)
    //     return eGTError_NOGUILDMASTER;
    //   // (commented-out level + member checks)
    //   MSGBASE msg;
    //   msg.Category = MP_GTOURNAMENT;
    //   msg.Protocol = MP_GTOURNAMENT_REGIST_SYN;
    //   msg.dwObjectID = HEROID;
    //   NETWORK->Send(&msg, sizeof(msg));
    //   return eGTError_NOERROR;
    //
    // The modern port:
    //   - The whole method is TODO (3-singleton:
    //     HERO + GUILDMGR + NETWORK not ported,
    //     R-12.x deferred).
    //   - Returns kErrorNoGuildMaster (matching
    //     the legacy early-return path for non-master
    //     — the modern port assumes non-master as
    //     a safe default while the singletons are
    //     unported).
    //   - When ported, the body becomes the legacy
    //     code.
    // TODO: 3-singleton dispatch (R-12.x deferred).
    return kErrorNoGuildMaster;
}

void cGTRegistDialog::SetRegistGuildCount(std::uint32_t count) {
    // 1:1 with legacy CGTRegistDialog::SetRegistGuildCount.
    // The legacy is:
    //   m_pRegistGuild->SetStaticValue(count);
    //   m_pRegistableGuild->SetStaticValue(MAXGUILD_INTOURNAMENT - count);
    //
    // The modern port: the whole method is TODO
    // (cStatic::SetStaticValue not yet ported, R-12.x
    // deferred). Modern port is a no-op (no state
    // change). When ported, the body becomes the
    // legacy code with kMaxGuildInTournament - count.
    (void)count;
    // TODO: cStatic::SetStaticValue not ported (R-12.x
    //       deferred). When ported, replace with:
    //         if (m_pRegistGuild) m_pRegistGuild->SetStaticValue(count);
    //         if (m_pRegistableGuild)
    //           m_pRegistableGuild->SetStaticValue(
    //             kMaxGuildInTournament - count);
}

}  // namespace mxh::ui
