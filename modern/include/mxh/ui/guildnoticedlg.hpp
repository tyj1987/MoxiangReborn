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
//   - OnActionEvent: REAL -- 2 button ids dispatch
//     through OPTIONAL GUILDMGR host callbacks:
//       SENDOKBTN -> read notice buffer (cTextArea::
//       GetScriptTextCString) -> SetGuildNotice(buf,
//       userData) -> SetActive(false).
//       CANCELBTN -> SetActive(false).
//     Unknown ids and non-WE_BTNCLICK are no-ops
//     (1:1 with legacy switch fallthrough + bitmask).
//   - SetActive override: REAL -- if val == TRUE and
//     m_pNoticeText is linked, pre-fill with the
//     notice returned by the OPTIONAL GUILDMGR host
//     callback (legacy GUILDMGR->GetGuildNotice()).
//     Falls back to SetScriptText("") only when no
//     callback is registered or the callback returns
//     null. 1:1 quirk: the notice pre-fill happens
//     BEFORE the base SetActive (matches legacy call
//     order).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 18th **Tier 2** dialog port (after
// cPetWearedExDialog). The dialog has no service
// dependency on the modern service interface
// (Phase 13). GUILDMGR is supplied through OPTIONAL
// host callbacks (SetGuildNoticeCallbacks) rather
// than the legacy singleton, decoupling the dialog
// from GUILDMGR global state.

#pragma once

#include "cdialog.hpp"
#include "legacy_window_event.hpp"

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
    // button ids: SENDOKBTN reads the cTextArea
    // script text into a kMaxGuildNotice-sized buffer
    // and invokes the OPTIONAL GUILDMGR host
    // SetGuildNotice callback (legacy GUILDMGR->
    // SetGuildNotice), then SetActive(false). CANCELBTN
    // just SetActive(false). The gate is
    // `if(we & WE_BTNCLICK)`. Unknown ids are
    // silently ignored (1:1 with legacy fallthrough).
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // ----- 1:1 with legacy CGuildNoticeDlg::SetActive override -----

    // 1:1 with legacy SetActive override. If val ==
    // TRUE and m_pNoticeText is linked, pre-fill
    // with the GUILDMGR host GetGuildNotice return
    // value (legacy GUILDMGR->GetGuildNotice()).
    // Falls back to SetScriptText("") only when the
    // callback is missing or returns null. Then
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

    // 1:1 with legacy MAX_GUILD_NOTICE = 150
    // (CommonGameDefine.h). The local buffer used in
    // the SENDOKBTN branch of OnActionEvent.
    static constexpr std::int32_t kMaxGuildNotice = 150;

    // 1:1 with legacy cWindow::we constants.
    // WE_BTNCLICK = 0x0040 (legacy cWindow::we bitmask; canonical via legacy_window_event::kButtonClick).
    static constexpr std::uint32_t kWeBtnClick = legacy_window_event::kButtonClick;

    // ----- Host-injected callbacks (legacy: GUILDMGR singleton) -----

    using GetGuildNoticeFn = const char* (*)(void* userData);
    using SetGuildNoticeFn = void (*)(const char* notice,
                                      void* userData);

    // Replaces the legacy GUILDMGR->GetGuildNotice
    // (used in SetActive(true)) and GUILDMGR->
    // SetGuildNotice (used in OnActionEvent SENDOKBTN).
    // Both callbacks are OPTIONAL; with no callback
    // registered the dialog stays safe (pre-fill
    // becomes SetScriptText(""), the SEND branch
    // still toggles the dialog off).
    void SetGuildNoticeCallbacks(GetGuildNoticeFn getGuildNotice,
                                 SetGuildNoticeFn setGuildNotice,
                                 void* userData = nullptr) noexcept;

private:
    // 1:1 with legacy m_pNoticeText (resolved in
    // Linking by GNotice_TEXTREA id). Modern port
    // stores raw pointer (not owned; cDialog owns
    // the child).
    cTextArea* m_pNoticeText = nullptr;

    GetGuildNoticeFn m_getGuildNoticeFn = nullptr;
    SetGuildNoticeFn m_setGuildNoticeFn = nullptr;
    void* m_callbackUserData = nullptr;
};

}  // namespace mxh::ui
