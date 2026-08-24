// cguildinvitedialog.hpp -- modern port of Moxiang CGuildInviteDialog
//   (guild invitation display dialog).
//
// 1:1 port of legacy `CGuildInviteDialog` from
//   `[Client]MH\GuildInviteDialog.{h,cpp}`.
//
// The guild-invite dialog is a single cTextArea that
// shows a one-line invite message: either
//   "Guild <guild>'s master <master> invites you to
//    join" (FlgKind = AsMember / 0)
// or
//   "Master <master> of guild <guild> invites you to
//    join as a student" (FlgKind = AsStudent / 1).
//
// 1:1 dependencies:
//   * 1 cTextArea child (m_pInviteMsg) by GD_IINVITE
//   * CHATMGR->GetChatMsg(45) (AsMember)
//     CHATMGR->GetChatMsg(1370) (AsStudent)
//
// Modern port keeps the legacy surface (Linking +
// SetInfo).  The host wires up the cTextArea pointer
// via SetInviteTextForTest; the host injects a
// ChatMsgCallback so the modern port can format the
// invite line exactly like the legacy sprintf with
// CHATMGR->GetChatMsg(45/1370).  When the callback is
// absent, the modern port falls back to placeholder
// format strings (so the dialog remains safe even
// without a chatmsg table).

#pragma once

#include "cDialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cTextArea;

class cGuildInviteDialog : public cDialog {
public:
    cGuildInviteDialog();
    ~cGuildInviteDialog() override;

    cGuildInviteDialog(const cGuildInviteDialog&) = delete;
    cGuildInviteDialog& operator=(const cGuildInviteDialog&) = delete;

    // 1:1 with legacy Linking.  Resolves m_pInviteMsg
    // via the host-injected cTextArea pointer.
    void Linking();

    // 1:1 with legacy SetInfo(char* GuildName, char*
    // MasterName, int FlgKind).  Calls
    // m_pInviteMsg->SetScriptText(sprintf(...)) using
    // chatmsg 45 (AsMember) or 1370 (AsStudent) as the
    // format string.  Null GuildName / MasterName
    // are guarded (1:1 quirk: the legacy would crash
    // on sprintf with null).
    void SetInfo(const char* guildName, const char* masterName, int flgKind);

    // 1:1 with legacy WindowIDEnum.h GD_IINVITE.
    // Local 420 (distinct from 590 / 30-32 / 70-80 /
    // 410 / 700-703 / 730 used by other recent 1:1
    // ports).
    static constexpr std::int32_t kIdInviteText = 420;

    // FlgKind enum (1:1 with legacy AsMember /
    // AsStudent).
    static constexpr int kFlgMember  = 0;
    static constexpr int kFlgStudent = 1;

    // 1:1 chatmsg ids used by the dialog.
    static constexpr int kChatMsgMember  = 45;
    static constexpr int kChatMsgStudent = 1370;

    // Test hook -- inject the cTextArea pointer
    // (replaces the legacy GetWindowForID(GD_IINVITE)
    // lookup).
    void SetInviteTextForTest(cTextArea* ta) noexcept { m_pInviteMsg = ta; }
    cTextArea* GetInviteTextForTest() const noexcept { return m_pInviteMsg; }

    // Test hook -- inject a "chatmsg lookup" callback
    // (legacy CHATMGR->GetChatMsg).  The default
    // returns placeholder format strings that match
    // the legacy .bin chatmsg table shape (so
    // 1:1 host environments can verify the sprintf
    // end-to-end).
    using ChatMsgCallback = const char*(*)(int chatMsgId, void* user);
    void SetChatMsgCallbackForTest(ChatMsgCallback cb, void* user) {
        m_chatMsgCb = cb; m_chatMsgUser = user;
    }

private:
    cTextArea*      m_pInviteMsg  = nullptr;
    ChatMsgCallback m_chatMsgCb   = nullptr;
    void*           m_chatMsgUser = nullptr;

    static const char* DefaultChatMsg(int chatMsgId, void* user);
};

} // namespace mxh::ui
