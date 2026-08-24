// cmpnoticedialog.hpp -- modern port of Moxiang CMPNoticeDialog (MP notice).
//
// 1:1 port of legacy `CMPNoticeDialog` from
//   `[Client]MH\MPNoticeDialog.{h,cpp}`.
//
// The MP notice dialog is a tiny cDialog that shows two
// caution lines when the user joins an MP (multi-player)
// room: a normal caution ("you must agree to MP rules")
// and a red caution ("offenders will be kicked").  The
// dialog is purely informational -- it has no action
// buttons and never sends a network message.
//
// 1:1 dependencies:
//   * 2 cTextArea children (m_pNCaution / m_pNRedCaution)
//   * CHATMGR->GetChatMsg(667 / 668) for the two lines
//     (modern port routes this through a host-injected
//     ChatMsgCallback; the default values match the
//     legacy .bin chatmsg table)
//
// Modern port keeps the legacy surface (Linking) so
// callers can be ported 1:1.  The host wires up the
// cTextArea pointers via SetTextAreasForTest (replaces
// the legacy GetWindowForID(MP_NCAUTION / MP_NREDCAUTION)
// lookups); the host calls Linking() when the dialog
// resource is loaded.

#pragma once

#include "cDialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cTextArea;

class cMPNoticeDialog : public cDialog {
public:
    cMPNoticeDialog();
    ~cMPNoticeDialog() override;

    cMPNoticeDialog(const cMPNoticeDialog&) = delete;
    cMPNoticeDialog& operator=(const cMPNoticeDialog&) = delete;

    // 1:1 with legacy Linking.  Resolves m_pNCaution /
    // m_pNRedCaution via the host-injected pointers and
    // sets each text area to the corresponding chatmsg
    // (667 / 668).  No-op when the text areas are not
    // yet injected.
    void Linking();

    // 1:1 chatmsg ids used by the dialog.
    static constexpr int kChatMsgNCaution    = 667;
    static constexpr int kChatMsgNRedCaution = 668;

    // 1:1 with legacy WindowIDEnum.h MP_NCAUTION /
    // MP_NREDCAUTION.
    static constexpr std::int32_t kIdNCaution    = 590;
    static constexpr std::int32_t kIdNRedCaution = 591;

    // Test hook -- inject the two cTextArea pointers
    // (replaces the legacy GetWindowForID(MP_NCAUTION /
    // MP_NREDCAUTION) lookups).  The pointers must
    // remain valid for the lifetime of the dialog.
    void SetTextAreasForTest(cTextArea* caution, cTextArea* redCaution) noexcept {
        m_pNCaution = caution; m_pNRedCaution = redCaution;
    }
    cTextArea* GetNCautionForTest()    const noexcept { return m_pNCaution; }
    cTextArea* GetNRedCautionForTest() const noexcept { return m_pNRedCaution; }

    // Test hook -- inject a "chatmsg lookup" callback
    // (legacy CHATMGR->GetChatMsg).  Default returns
    // empty string for unknown ids (so Linking can
    // still run without crashing).
    using ChatMsgCallback = const char*(*)(int chatMsgId, void* user);
    void SetChatMsgCallbackForTest(ChatMsgCallback cb, void* user) {
        m_chatMsgCb = cb; m_chatMsgUser = user;
    }

private:
    cTextArea*       m_pNCaution    = nullptr;
    cTextArea*       m_pNRedCaution = nullptr;
    ChatMsgCallback  m_chatMsgCb    = nullptr;
    void*            m_chatMsgUser  = nullptr;

    static const char* DefaultChatMsg(int chatMsgId, void* user);
};

} // namespace mxh::ui
