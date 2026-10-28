// guildmarkdialog.hpp — modern port of 墨香
// CGuildMarkDialog (guild / guild-union mark
// registration dialog: 1 cTextArea + 2 cButton).
//
// 1:1 port of legacy `CGuildMarkDialog` from
//   `墨香【源码】\[Client]MH\GuildMarkDialog.h` (847 B) and
//   `墨香【源码】\[Client]MH\GuildMarkDialog.cpp`.
//
// What the legacy does:
//   - Ctor: m_type = WT_GUILDMARKDLG.
//   - Dtor: empty body.
//   - Linking: resolve 1 cTextArea (m_pInfoText by
//     GDM_INFOTEXT) + 2 cButton (m_pGuildMarkBtn by
//     GDM_REGISTOKBTN, m_pGuildUnionMarkBtn by
//     GUM_REGISTOKBTN); SetScriptText on
//     m_pInfoText (CHATMGR->GetChatMsg(303)).
//   - SetActive override: if val == FALSE →
//     resolve cEditBox by GDM_NAMEEDIT, call
//     SetFocusEdit(false); if HERO == 0 return;
//     if HERO state == eObjectState_Deal &&
//     GAMEIN->GetNpcScriptDialog()->IsActive() ==
//     FALSE, call OBJECTSTATEMGR->EndObjectState
//     (HERO, eObjectState_Deal). Always call base
//     cDialog::SetActive(val).
//   - ShowGuildMark: SetActive(TRUE) +
//     m_pGuildMarkBtn->SetActive(TRUE) +
//     m_pGuildUnionMarkBtn->SetActive(FALSE) +
//     m_pInfoText->SetScriptText(CHATMGR->GetChatMsg(303)).
//   - ShowGuildUnionMark: SetActive(TRUE) +
//     m_pGuildMarkBtn->SetActive(FALSE) +
//     m_pGuildUnionMarkBtn->SetActive(TRUE) +
//     m_pInfoText->SetScriptText(CHATMGR->GetChatMsg(1114)).
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: m_type = WT_GUILDMARKDLG
//     drop, modern cWindow does not have m_type).
//   - Dtor: empty (no-op).
//   - Linking: REAL — resolve 3 children by id,
//     call SetScriptText on the cTextArea (with
//     "GUILD_MARK_INFO_TEXT" placeholder for the
//     CHATMGR->GetChatMsg(303) call, 1:1 with
//     legacy c-string content).
//   - SetActive override: REAL -- the val == FALSE
//     path now honors the legacy OBJECTSTATEMGR
//     deal-state guard through OPTIONAL host
//     callbacks (HERO + GAMEIN-NpcScriptDialog +
//     OBJECTSTATEMGR). The cEditBox SetFocusEdit
//     remains REAL. Always calls base
//     cDialog::SetActive(val) (1:1 with legacy).
//   - ShowGuildMark / ShowGuildUnionMark: 1:1
//     with legacy. The cButton->SetActive is the
//     R-12 fix — modern cButton doesn't have
//     SetActive, so modern port uses
//     cWindow::SetVisible as a 1:1 semantic
//     equivalent.
//   - 1:1 quirk: cButton has no SetActive (inherits
//     cWindow), modern port uses SetVisible.
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 40th **Tier 2** dialog port (after
// cMainDialog). The dialog has 1 cTextArea (m_pInfoText)
// + 2 cButton (m_pGuildMarkBtn, m_pGuildUnionMarkBtn)
// + 1 cEditBox (resolved in SetActive only, m_pMarkName
// not stored). The CHATMGR global is mapped to a
// literal placeholder (kGuildMarkInfoText +
// kGuildUnionMarkInfoText). HERO + GAMEIN + OBJECTSTATEMGR
// are reached through OPTIONAL host-injected callbacks
// (SetCallbacks) rather than the legacy singletons.

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cTextArea;
class cButton;

class cGuildMarkDialog : public cDialog {
public:
    cGuildMarkDialog();
    ~cGuildMarkDialog() override;

    // ----- 1:1 with legacy CGuildMarkDialog::Linking -----

    // 1:1 with legacy Linking. Resolve 1 cTextArea
    // (m_pInfoText by kIdInfoText) + 2 cButton
    // (m_pGuildMarkBtn by kIdRegistOkBtn,
    // m_pGuildUnionMarkBtn by kIdUnionRegistOkBtn).
    // Calls SetScriptText on m_pInfoText (with
    // kGuildMarkInfoText placeholder for the
    // legacy CHATMGR->GetChatMsg(303) call).
    void Linking();

    // ----- 1:1 with legacy CGuildMarkDialog::SetActive override -----

    // 1:1 with legacy SetActive override. The val==FALSE
    // path now honors the legacy OBJECTSTATEMGR deal
    // guard through OPTIONAL host callbacks (HERO +
    // GAMEIN-NpcScriptDialog + OBJECTSTATEMGR). The
    // cEditBox SetFocusEdit stays REAL. Always calls
    // base cDialog::SetActive(val) (1:1 with legacy).
    void SetActive(bool val) noexcept override;

    // ----- 1:1 with legacy CGuildMarkDialog::ShowGuildMark -----

    // 1:1 with legacy ShowGuildMark. SetActive(TRUE)
    // + m_pGuildMarkBtn->SetVisible(TRUE) (R-12
    // fix: cButton 没 SetActive, 用 SetVisible) +
    // m_pGuildUnionMarkBtn->SetVisible(FALSE) +
    // m_pInfoText->SetScriptText(kGuildMarkInfoText).
    void ShowGuildMark();

    // ----- 1:1 with legacy CGuildMarkDialog::ShowGuildUnionMark -----

    // 1:1 with legacy ShowGuildUnionMark.
    // SetActive(TRUE) +
    // m_pGuildMarkBtn->SetVisible(FALSE) +
    // m_pGuildUnionMarkBtn->SetVisible(TRUE) +
    // m_pInfoText->SetScriptText(kGuildUnionMarkInfoText).
    void ShowGuildUnionMark();

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h GDM_INFOTEXT /
    // GDM_REGISTOKBTN / GUM_REGISTOKBTN (550-552).
    // Local 550-552 — distinct from 200-540 used
    // by previous Tier 2 dialogs.
    static constexpr std::int32_t kIdInfoText        = 550;
    static constexpr std::int32_t kIdRegistOkBtn     = 551;
    static constexpr std::int32_t kIdUnionRegistOkBtn = 552;
    // 1:1 with legacy GDM_NAMEEDIT — the cEditBox
    // resolved in SetActive for the SetFocusEdit(false)
    // call. Not stored in modern port (resolved
    // per-call via findWindowById).
    static constexpr std::int32_t kIdNameEdit        = 553;

    // 1:1 with legacy CHATMGR->GetChatMsg(303) for
    // guild mark info text + CHATMGR->GetChatMsg(1114)
    // for guild union mark info text. Modern port
    // uses literal placeholders until CHATMGR is
    // ported.
    static constexpr const char* kGuildMarkInfoText =
        "GUILD_MARK_INFO_TEXT";  // CHATMGR msg 303
    static constexpr const char* kGuildUnionMarkInfoText =
        "GUILD_UNION_MARK_INFO_TEXT";  // CHATMGR msg 1114

    // 1:1 with legacy eObjectState_Deal = 6 (CommonGameDefine.h).
    static constexpr std::int32_t kObjectStateDeal = 6;

    // ----- Host-injected callbacks (legacy: HERO + GAMEIN + OBJECTSTATEMGR) -----

    using GetHeroObjectIdFn = std::uint32_t (*)(void* userData);
    using GetHeroStateFn = std::int32_t (*)(void* userData);
    using IsNpcScriptDialogActiveFn = bool (*)(void* userData);
    using EndObjectStateFn =
        void (*)(std::uint32_t objectId, std::int32_t stateIdx,
                 void* userData);

    // Replaces legacy HERO + OBJECTSTATEMGR + GAMEIN NpcScriptDialog
    // dispatch in SetActive(false). When HERO is missing (HEROID=0),
    // or hero state != kObjectStateDeal, or GAMEIN NpcScriptDialog is
    // active, EndObjectState is skipped (1:1 with legacy guard order).
    void SetCallbacks(GetHeroObjectIdFn getHeroObjectId,
                      GetHeroStateFn getHeroState,
                      IsNpcScriptDialogActiveFn isNpcScriptDialogActive,
                      EndObjectStateFn endObjectState,
                      void* userData = nullptr) noexcept;

private:
    // 1:1 with legacy m_pInfoText (resolved in
    // Linking by GDM_INFOTEXT id).
    cTextArea* m_pInfoText = nullptr;

    // 1:1 with legacy m_pGuildMarkBtn (resolved in
    // Linking by GDM_REGISTOKBTN id).
    cButton* m_pGuildMarkBtn = nullptr;

    // 1:1 with legacy m_pGuildUnionMarkBtn (resolved
    // in Linking by GUM_REGISTOKBTN id).
    cButton* m_pGuildUnionMarkBtn = nullptr;

    GetHeroObjectIdFn m_getHeroObjectIdFn = nullptr;
    GetHeroStateFn m_getHeroStateFn = nullptr;
    IsNpcScriptDialogActiveFn m_isNpcScriptDialogActiveFn = nullptr;
    EndObjectStateFn m_endObjectStateFn = nullptr;
    void* m_callbackUserData = nullptr;
};

}  // namespace mxh::ui
