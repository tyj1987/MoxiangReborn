// partyinvitedlg.cpp — 1:1 port of 墨香 CPartyInviteDlg
// (party invitation dialog). See partyinvitedlg.hpp
// for the data-model rationale + 1:1 quirks.

#include "partyinvitedlg.hpp"
#include "cbutton.hpp"
#include "cstatic.hpp"
#include "ctextarea.hpp"

#include <cstdio>
#include <cstring>
#include <string>

namespace mxh::ui {

cPartyInviteDlg::cPartyInviteDlg() {
    // 1:1 with legacy CPartyInviteDlg ctor:
    //   m_type = WT_PARTYINVITEDLG;
    //
    // 1:1 quirk: modern cWindow does not have
    // m_type field (removed in Phase 6 when cWindow
    // was modernized). The ctor body is dropped.
}

cPartyInviteDlg::~cPartyInviteDlg() = default;

void cPartyInviteDlg::Linking() {
    // 1:1 with legacy CPartyInviteDlg::Linking.
    // The legacy is:
    //   m_pDistribute = (cStatic*)GetWindowForID(PA_INVITEDISTRIBUTE);
    //   m_pInviter = (cTextArea*)GetWindowForID(PA_INVITER);
    //   m_pOK = (cButton*)GetWindowForID(PA_INVITEOK);
    //   m_pCancel = (cButton*)GetWindowForID(PA_INVITECANCEL);
    m_pDistribute = static_cast<cStatic*>(findWindowById(kIdDistribute));
    m_pInviter    = static_cast<cTextArea*>(findWindowById(kIdInviter));
    m_pOK         = static_cast<cButton*>(findWindowById(kIdOk));
    m_pCancel     = static_cast<cButton*>(findWindowById(kIdCancel));
}

void cPartyInviteDlg::SetMsg(const char* pInviter, std::uint8_t option) {
    // 1:1 with legacy CPartyInviteDlg::SetMsg. The
    // legacy is:
    //   char buf[256] = {0,};
    //   char Opt[256] = {0,};
    //   if (Option == ePartyOpt_Random)
    //     SafeStrCpy(Opt, CHATMGR->GetChatMsg(640), 64);
    //   else if (Option == ePartyOpt_Damage)
    //     SafeStrCpy(Opt, CHATMGR->GetChatMsg(641), 32);
    //   sprintf(buf, CHATMGR->GetChatMsg(305), pInviter);
    //   m_pDistribute->SetStaticText(Opt);
    //   m_pInviter->SetScriptText(buf);
    //
    // The modern port:
    //   - Uses placeholder format strings
    //     "PARTY_OPT_RANDOM" / "PARTY_OPT_DAMAGE"
    //     for the distribute text (1:1 with legacy
    //     CHATMGR->GetChatMsg(640/641) — when
    //     CHATMGR ported, body becomes real SafeStrCpy).
    //   - Uses placeholder format string
    //     "PARTY_INVITER_MSG_FORMAT" for the
    //     inviter text (1:1 with legacy
    //     CHATMGR->GetChatMsg(305) with pInviter
    //     argument).
    //   - 1:1 quirk: modern port guards null
    //     pInviter (legacy would crash on sprintf
    //     with null).
    if (m_pDistribute) {
        if (option == kOptRandom) {
            m_pDistribute->SetStaticText("PARTY_OPT_RANDOM");
        } else if (option == kOptDamage) {
            m_pDistribute->SetStaticText("PARTY_OPT_DAMAGE");
        }
    }
    if (m_pInviter) {
        char buf[256] = {0};
        if (pInviter) {
            std::snprintf(buf, sizeof(buf),
                          "PARTY_INVITER_MSG_FORMAT %s",
                          pInviter);
        }
        m_pInviter->SetScriptText(buf);
    }
}

}  // namespace mxh::ui
