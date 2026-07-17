// partyinvitedlg.hpp — modern port of 墨香 CPartyInviteDlg
// (party invitation dialog: 2 button + 1 cTextArea + 1 cStatic).
//
// 1:1 port of legacy `CPartyInviteDlg` from
//   `墨香【源码】\[Client]MH\PartyInviteDlg.h` (812 B) and
//   `墨香【源码】\[Client]MH\PartyInviteDlg.cpp`.
//
// What the legacy does:
//   - Ctor: m_type = WT_PARTYINVITEDLG (legacy
//     cWindow type tag).
//   - Dtor: empty body.
//   - Linking: resolve 4 children (m_pDistribute
//     cStatic by PA_INVITEDISTRIBUTE, m_pInviter
//     cTextArea by PA_INVITER, m_pOK + m_pCancel
//     cButton by PA_INVITEOK / PA_INVITECANCEL).
//   - SetMsg(char* pInviter, BYTE Option):
//     * Option == ePartyOpt_Random → SafeStrCpy
//       Opt = CHATMGR->GetChatMsg(640).
//     * Option == ePartyOpt_Damage → SafeStrCpy
//       Opt = CHATMGR->GetChatMsg(641).
//     * sprintf buf = CHATMGR->GetChatMsg(305)
//       with pInviter argument.
//     * m_pDistribute->SetStaticText(Opt).
//     * m_pInviter->SetScriptText(buf).
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: m_type = WT_PARTYINVITEDLG
//     drop, modern cWindow does not have m_type).
//   - Dtor: empty (no-op).
//   - Linking: REAL — resolve 4 children by id.
//   - SetMsg: REAL with placeholder format
//     strings "PARTY_OPT_RANDOM" (kOptRandom) /
//     "PARTY_OPT_DAMAGE" (kOptDamage) for the
//     distribute text, and "PARTY_INVITER_MSG_FORMAT"
//     for the inviter text. When CHATMGR is ported,
//     the body becomes the legacy code with real
//     CHATMGR->GetChatMsg(640/641/305).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 31st **Tier 2** dialog port (after
// cStallKindSelectDlg). The dialog has no service
// dependency on the modern service interface
// (Phase 13) — only CHATMGR singleton (R-12.x
// deferred, but the placeholder pattern works
// around it without the singleton).

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cButton;
class cStatic;
class cTextArea;

class cPartyInviteDlg : public cDialog {
public:
    cPartyInviteDlg();
    ~cPartyInviteDlg() override;

    // ----- 1:1 with legacy CPartyInviteDlg::Linking -----

    // 1:1 with legacy Linking. Resolve 4 children
    // (m_pDistribute cStatic, m_pInviter cTextArea,
    // m_pOK + m_pCancel cButton) by id.
    void Linking();

    // ----- 1:1 with legacy CPartyInviteDlg::SetMsg -----

    // 1:1 with legacy SetMsg(char* pInviter, BYTE
    // Option). The modern port uses std::string for
    // pInviter (1:1 with legacy char* c-string,
    // since the sprintf is just a format
    // placeholder). The Option selects between
    // kOptRandom (= ePartyOpt_Random) and
    // kOptDamage (= ePartyOpt_Damage) branches.
    void SetMsg(const char* pInviter, std::uint8_t option);

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h WINDOW_ID values
    // (PA_INVITEDISTRIBUTE / PA_INVITER /
    // PA_INVITEOK / PA_INVITECANCEL). Local
    // 440-443 — distinct from 200-432 used by
    // previous Tier 2 dialogs.
    static constexpr std::int32_t kIdDistribute = 440;
    static constexpr std::int32_t kIdInviter    = 441;
    static constexpr std::int32_t kIdOk         = 442;
    static constexpr std::int32_t kIdCancel     = 443;

    // 1:1 with legacy ePartyOpt_Random / ePartyOpt_Damage
    // enum. The legacy enum is a global in
    // CommonGameDefine.h. Modern port inlines the
    // values (0 = random, 1 = damage) to avoid
    // pulling in the shared header.
    static constexpr std::uint8_t kOptRandom = 0;
    static constexpr std::uint8_t kOptDamage = 1;

private:
    // 1:1 with legacy m_pDistribute / m_pInviter /
    // m_pOK / m_pCancel (resolved in Linking).
    cStatic*  m_pDistribute = nullptr;
    cTextArea* m_pInviter   = nullptr;
    cButton*  m_pOK         = nullptr;
    cButton*  m_pCancel     = nullptr;
};

}  // namespace mxh::ui
