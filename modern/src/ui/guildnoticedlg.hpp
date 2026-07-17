// guildnoticedlg.hpp — modern port of 墨香 CGuildNoticeDlg
// (guild notice editor dialog: 1 cTextArea + 2 cButton).
//
// 1:1 port of legacy `CGuildNoticeDlg` from
//   `墨香【源码】\[Client]MH\GuildNoticeDlg.h` (310 B) and
//   `墨香【源码】\[Client]MH\GuildNoticeDlg.cpp`.
//
// What the legacy does:
//   - Ctor / dtor: empty bodies (no state init).
//   - Linking: resolve 1 cTextArea child
//     (m_pNoticeText) by id (GNotice_TEXTREA), then
//     call SetEnterAllow(FALSE) + SetScriptText("") on
//     it. The 2 button children (GNotice_SENDOKBTN /
//     GNotice_CANCELBTN) are NOT resolved — they're
//     handled in OnActionEvent by their id (legacy
//     uses GetWindowForID inside the switch case).
//   - OnActionEvent: 2 button ids:
//     * GNotice_SENDOKBTN → call
//       GUILDMGR->SetGuildNotice(notice) where
//       notice = m_pNoticeText->GetScriptText(notice)
//       (fills local char[MAX_GUILD_NOTICE+1] buffer),
//       then SetActive(FALSE).
//     * GNotice_CANCELBTN → SetActive(FALSE) (no
//       notice save).
//     Both fall through a single if(we & WE_BTNCLICK)
//     gate. Unknown ids are silently ignored.
//   - SetActive override: if val == TRUE and
//     GUILDMGR->GetGuildNotice() returns non-null,
//     call m_pNoticeText->SetScriptText(notice) to
//     pre-fill the text area. Then call
//     cDialog::SetActive(val).
//
// The modern port covers:
//   - Ctor / dtor: empty (no-op) — 1:1 with legacy.
//   - Linking: REAL — resolve cTextArea child by id,
//     call SetEnterAllow(FALSE) + SetScriptText("").
//     The 2 button ids are 1:1 preserved as constexpr
//     for OnActionEvent dispatch (legacy uses
//     GetWindowForID inside switch case; modern port
//     uses the constants directly).
//   - OnActionEvent: 1:1 with legacy 2-button branch
//     + the 2 button ids are constants. The
//     GUILDMGR->SetGuildNotice(notice) call is
//     TODO (GUILDMGR not ported, R-12.x deferred).
//     SetActive(FALSE) at the end of the SEND branch
//     is also TODO (would call our own SetActive
//     which dispatches to GUILDMGR). The CANCEL
//     branch's SetActive(FALSE) is also TODO. When
//     GUILDMGR is ported, these become real calls.
//   - SetActive override: 1:1 with legacy — if val ==
//     TRUE and m_pNoticeText is linked, call
//     SetScriptText(GUILDMGR->GetGuildNotice()). The
//     GUILDMGR->GetGuildNotice() call is TODO
//     (GUILDMGR not ported). The base SetActive is
//     always called (matches legacy `cDialog::SetActive(val)`).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 18th **Tier 2** dialog port (after
// cPetWearedExDialog). The dialog has no service
// dependency on the modern service interface
// (Phase 13) — all state lives in GUILDMGR singleton
// (R-12.x deferred).

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cTextArea;

class cGuildNoticeDlg : public cDialog {
public:
    cGuildNoticeDlg();
    ~cGuildNoticeDlg() override;

    // ----- 1:1 with legacy CGuildNoticeDlg::Linking -----

    // 1:1 with legacy Linking. Resolve cTextArea
    // child (m_pNoticeText) by id, then call
    // SetEnterAllow(FALSE) + SetScriptText(""). The
    // 2 button children (SENDOKBTN / CANCELBTN) are
    // not resolved (legacy handles them by id in
    // OnActionEvent via GetWindowForID).
    void Linking();

    // ----- 1:1 with legacy CGuildNoticeDlg::OnActionEvent -----

    // 1:1 with legacy OnActionEvent (note: legacy
    // typo'd as "OnActionEvnet" — the modern port
    // uses the correct spelling "OnActionEvent"). 2
    // button ids: GNotice_SENDOKBTN → TODO
    // GUILDMGR->SetGuildNotice + SetActive(FALSE);
    // GNotice_CANCELBTN → TODO SetActive(FALSE). The
    // gate is `if(we & WE_BTNCLICK)`.
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // ----- 1:1 with legacy CGuildNoticeDlg::SetActive override -----

    // 1:1 with legacy SetActive override. If val ==
    // TRUE and m_pNoticeText is linked, call
    // SetScriptText(GUILDMGR->GetGuildNotice()). The
    // GUILDMGR->GetGuildNotice() call is TODO
    // (GUILDMGR not ported, R-12.x deferred). Then
    // call cDialog::SetActive(val). 1:1 quirk: the
    // notice pre-fill happens BEFORE the base
    // SetActive (matches legacy call order).
    void SetActive(bool val) noexcept override;

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h WINDOW_ID values
    // (GNotice_TEXTREA / GNotice_SENDOKBTN /
    // GNotice_CANCELBTN). The legacy enum starts at a
    // high base (the WINDOW_ID macro increments
    // through all dialogs sequentially), so the
    // actual numeric values are large. The modern port
    // uses local 350/351/352 — distinct from the
    // local ranges 200-321 used by previous Tier 2
    // dialogs (CharMake/GuildJoin/CharState/SOS/
    // WearedEx/MiniFriend/Revive/MPNotice/
    // EventNotify/GuildCreate/GuildUnion/
    // ChaseInput/Chase/Bail). The numeric value is
    // not significant for the 1:1 contract — the
    // legacy resolves the child by enum symbol, and
    // the modern port uses the constants directly.
    static constexpr std::int32_t kIdNoticeText = 350;
    static constexpr std::int32_t kIdSendOkBtn  = 351;
    static constexpr std::int32_t kIdCancelBtn  = 352;

private:
    // 1:1 with legacy m_pNoticeText (resolved in
    // Linking by GNotice_TEXTREA id). Modern port
    // stores raw pointer (not owned; cDialog owns
    // the child).
    cTextArea* m_pNoticeText = nullptr;
};

}  // namespace mxh::ui
