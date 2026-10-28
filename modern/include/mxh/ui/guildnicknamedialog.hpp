// guildnicknamedialog.hpp — modern port of 墨香
// CGuildNickNameDialog (guild member nickname editor
// dialog: 1 cTextArea + 1 cEditBox).
//
// 1:1 port of legacy `CGuildNickNameDialog` from
//   `墨香【源码】\[Client]MH\GuildNickNameDialog.h` (821 B) and
//   `墨香【源码】\[Client]MH\GuildNickNameDialog.cpp`.
//
// What the legacy does:
//   - Ctor: m_type = WT_GUILDNICKNAMEDLG (legacy
//     cWindow type tag).
//   - Dtor: empty body.
//   - Linking: resolve 1 cTextArea (m_pNickMsg by
//     GD_NICKTEXTAREA id) + 1 cEditBox (m_pNickName
//     by GD_NICKNAMEEDIT id), then call
//     m_pNickName->SetValidCheck(VCM_SPACE) to
//     reject spaces in the nickname.
//   - SetActive override:
//     * val == TRUE: if GUILDMGR->GetSelectedMemberID()
//       == 0 → cDialog::SetActive(FALSE) + chat msg
//       714 + early return. Else clear edit text +
//       call SetNickMsg(GUILDMGR
//       ->GetSelectedMemberName()).
//     * val == FALSE: m_pNickName->SetFocusEdit(FALSE).
//     Then cDialog::SetActive(val).
//   - SetNickMsg(char* Name): sprintf into local
//     char[128] using CHATMGR->GetChatMsg(704) with
//     the Name argument, then call
//     m_pNickMsg->SetScriptText(text).
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: m_type = WT_GUILDNICKNAMEDLG
//     drop, modern cWindow does not have m_type).
//   - Dtor: empty (no-op).
//   - Linking: REAL (resolve both children by id,
//     call SetValidCheck(0) — VCM_SPACE alias).
//   - SetActive override: 1:1 with legacy. The
//     GUILDMGR + CHATMGR dispatch is TODO (R-12.x
//     deferred). The base SetActive is always
//     called (matches legacy call order).
//   - SetNickMsg: REAL with placeholder sprintf
//     format string "GUILD_NICK_MSG_FORMAT" (when
//     CHATMGR is ported, the body becomes:
//     `char text[128]; sprintf(text,
//     CHATMGR->GetChatMsg(704), Name);
//     m_pNickMsg->SetScriptText(text);`).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 26th **Tier 2** dialog port (after
// cTipBrowserDlg). The dialog has no service
// dependency on the modern service interface
// (Phase 13) — only GUILDMGR + CHATMGR singletons
// (R-12.x deferred, but the placeholder pattern
// works around it without the singletons).

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cTextArea;
class cEditBox;

class cGuildNickNameDialog : public cDialog {
public:
    cGuildNickNameDialog();
    ~cGuildNickNameDialog() override;

    // ----- 1:1 with legacy CGuildNickNameDialog::Linking -----

    // 1:1 with legacy Linking. Resolve cTextArea
    // (m_pNickMsg by kIdNickTextArea) + cEditBox
    // (m_pNickName by kIdNickNameEdit), then call
    // m_pNickName->SetValidCheck(kVcmSpace = 0)
    // to reject spaces in the nickname.
    void Linking();

    // ----- 1:1 with legacy CGuildNickNameDialog::SetActive override -----

    using GetSelectedMemberIdFn = std::uint32_t (*)(void* userData);
    using GetSelectedMemberNameFn = const char* (*)(void* userData);
    using AddSystemMessageFn = void (*)(std::int32_t messageId,
                                        void* userData);
    using GetChatMessageFn = const char* (*)(std::int32_t messageId,
                                             void* userData);

    void SetCallbacks(GetSelectedMemberIdFn getSelectedMemberId,
                      GetSelectedMemberNameFn getSelectedMemberName,
                      AddSystemMessageFn addSystemMessage,
                      GetChatMessageFn getChatMessage,
                      void* userData = nullptr) noexcept;

    // 1:1 with legacy SetActive override. The
    // GUILDMGR + CHATMGR dispatch is TODO (R-12.x
    // deferred). The base SetActive is always
    // called (matches legacy call order).
    //   val == TRUE: TODO GUILDMGR->GetSelectedMemberID()
    //               check (if 0 → chat msg 714 + early
    //               return); else clear edit text +
    //               SetNickMsg(GUILDMGR
    //               ->GetSelectedMemberName()).
    //   val == FALSE: m_pNickName->SetFocusEdit(false).
    //   Then cDialog::SetActive(val).
    void SetActive(bool val) noexcept override;

    // ----- 1:1 with legacy CGuildNickNameDialog::SetNickMsg -----

    // 1:1 with legacy SetNickMsg(char* Name). The
    // modern port uses std::string for the Name
    // argument (1:1 with legacy char* c-string,
    // since the sprintf is just a format
    // placeholder). When CHATMGR is ported, the
    // body becomes the real sprintf with
    // CHATMGR->GetChatMsg(704).
    void SetNickMsg(const char* name);

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h WINDOW_ID values
    // (GD_NICKTEXTAREA / GD_NICKNAMEEDIT). Local
    // 400-401 — distinct from 200-389 used by
    // previous Tier 2 dialogs.
    static constexpr std::int32_t kIdNickTextArea = 400;
    static constexpr std::int32_t kIdNickNameEdit = 401;

    // VCM_SPACE = 0 (1:1 with legacy cEditBox
    // valid-check enum: reject spaces). 1:1 quirk:
    // modern cEditBox::SetValidCheck uses int
    // directly (the legacy's VCM_* enum values are
    // 0/1/2 for SPACE/NUMBER/CHARNAME).
    static constexpr int kVcmSpace = 0;
    static constexpr std::int32_t kNoSelectionMessageId = 714;
    static constexpr std::int32_t kNickPromptMessageId = 704;

private:
    GetSelectedMemberIdFn m_getSelectedMemberId = nullptr;
    GetSelectedMemberNameFn m_getSelectedMemberName = nullptr;
    AddSystemMessageFn m_addSystemMessage = nullptr;
    GetChatMessageFn m_getChatMessage = nullptr;
    void* m_callbackUserData = nullptr;

    // 1:1 with legacy m_pNickMsg (resolved in
    // Linking by GD_NICKTEXTAREA id).
    cTextArea* m_pNickMsg = nullptr;

    // 1:1 with legacy m_pNickName (resolved in
    // Linking by GD_NICKNAMEEDIT id).
    cEditBox* m_pNickName = nullptr;
};

}  // namespace mxh::ui
