// guildinvitedialog.hpp — modern port of 墨香
// CGuildInviteDialog (guild invitation display dialog:
// 1 cTextArea).
//
// 1:1 port of legacy `CGuildInviteDialog` from
//   `墨香【源码】\[Client]MH\GuildInviteDialog.h` (837 B) and
//   `墨香【源码】\[Client]MH\GuildInviteDialog.cpp`.
//
// What the legacy does:
//   - Ctor: m_type = WT_GUILDINVITEDLG (legacy
//     cWindow type tag).
//   - Dtor: empty body.
//   - Linking: resolve 1 cTextArea (m_pInviteMsg
//     by GD_IINVITE id).
//   - SetInfo(char* GuildName, char* MasterName,
//     int FlgKind): sprintf into local char[128]
//     using CHATMGR->GetChatMsg(45) for AsMember
//     or CHATMGR->GetChatMsg(1370) for AsStudent,
//     then call m_pInviteMsg->SetScriptText(text).
//
// The modern port covers:
//   - Ctor: empty (1:1 quirk: m_type =
//     WT_GUILDINVITEDLG drop, modern cWindow
//     does not have m_type).
//   - Dtor: empty (no-op).
//   - Linking: REAL — resolve cTextArea child by
//     id.
//   - SetInfo: REAL with placeholder sprintf
//     format strings "GUILD_INVITE_MSG_MEMBER" /
//     "GUILD_INVITE_MSG_STUDENT" + std::snprintf.
//     When CHATMGR is ported, the body becomes the
//     real sprintf with CHATMGR->GetChatMsg(45) /
//     GetChatMsg(1370).
//
// Per P2-12 roadmap (docs/P2-12_DIALOGS_ROADMAP.md),
// this is the 28th **Tier 2** dialog port (after
// cShoutDialog). The dialog has no service
// dependency on the modern service interface
// (Phase 13) — only CHATMGR singleton (R-12.x
// deferred, but the placeholder pattern works
// around it without the singleton).

#pragma once

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cTextArea;

class cGuildInviteDialog : public cDialog {
public:
    cGuildInviteDialog();
    ~cGuildInviteDialog() override;

    // ----- 1:1 with legacy CGuildInviteDialog::Linking -----

    // 1:1 with legacy Linking. Resolve cTextArea
    // child (m_pInviteMsg by kIdInviteText) by id.
    void Linking();

    // ----- 1:1 with legacy CGuildInviteDialog::SetInfo -----

    // 1:1 with legacy SetInfo(char* GuildName, char*
    // MasterName, int FlgKind). The modern port
    // uses std::string for GuildName + MasterName
    // (1:1 with legacy char* c-strings, since the
    // sprintf is just a format placeholder). The
    // FlgKind selects between AsMember (kFlgMember)
    // and AsStudent (kFlgStudent) branches.
    void SetInfo(const char* guildName, const char* masterName, int flgKind);

    // ----- Local id range (avoids collision with existing Tier 2 dialogs) -----

    // 1:1 with legacy WindowIDs.h WINDOW_ID
    // (GD_IINVITE). Local 420 — distinct from
    // 200-410 used by previous Tier 2 dialogs.
    static constexpr std::int32_t kIdInviteText = 420;

    // FlgKind enum (1:1 with legacy AsMember /
    // AsStudent). 0 = member, 1 = student.
    static constexpr int kFlgMember = 0;
    static constexpr int kFlgStudent = 1;

private:
    // 1:1 with legacy m_pInviteMsg (resolved in
    // Linking by GD_IINVITE id).
    cTextArea* m_pInviteMsg = nullptr;
};

}  // namespace mxh::ui
