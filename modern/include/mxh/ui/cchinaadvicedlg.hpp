// cchinaadvicedlg.hpp -- modern port of Moxiang
//   CChinaAdviceDlg (China-region advice / T&C dialog).
//
// 1:1 port of legacy `CChinaAdviceDlg` from
//   `[Client]MH\ChinaAdviceDlg.{h,cpp}`.
//
// Surface (legacy):
//   - Ctor: empty body, no state init.
//   - Dtor: empty body.
//   - Linking: resolve 1 cTextArea (CNA_TEXTAREA),
//     call SetScriptText(CHATMGR->GetChatMsg(30))
//     -- the China-region T&C text from the chat
//     message table.
//   - OnActionEvent: empty body. No button dispatch.
//     Dialog closes via auto-close / outside-click.
//
// Modern port:
//   - Ctor / Dtor: default (1:1 with empty bodies).
//   - Linking: resolve cTextArea via findWindowById
//     (kIdTextArea), call SetScriptText with the
//     result of the host-injected chat message
//     callback (1:1 with CHATMGR->GetChatMsg(30)).
//     Without an injected callback, falls back to
//     the placeholder "CHINA_ADVICE_TEXT" (same
//     pattern as cMPNoticeDialog's "MP_NCAUTION").
//   - OnActionEvent: empty no-op (1:1 with legacy
//     empty body).
//   - CHATMGR is a global singleton in legacy (R-12.x
//     deferred). Modern port uses a host-injected
//     callback (ChatMsgCallback) so the dialog is
//     drivable in unit tests + from any service
//     adapter that integrates with the modern
//     chat subsystem.
//
// 1:1 quirks:
//   - 1:1 with legacy CNA_BTN_OK (WindowIDs.h): the
//     enum exists but the legacy .cpp does NOT
//     resolve or use the button. The modern port
//     also does not include a kIdBtnOk constant.
//   - 1:1 with legacy `CHATMGR->GetChatMsg(30)`
//     chat message id 30. Modern port exposes
//     kChinaAdviceChatMsgId = 30 for the callback.
//   - 1:1 with legacy empty OnActionEvent: no
//     button dispatch. Dialog relies on auto-close
//     or outside-click dismissal.

#pragma once

#include "legacy_window_event.hpp"

#include "cdialog.hpp"

#include <cstdint>

namespace mxh::ui {

class cTextArea;

class cChinaAdviceDlg : public cDialog {
public:
    cChinaAdviceDlg();
    ~cChinaAdviceDlg() override;

    cChinaAdviceDlg(const cChinaAdviceDlg&) = delete;
    cChinaAdviceDlg& operator=(const cChinaAdviceDlg&) = delete;

    // ----- 1:1 with legacy CChinaAdviceDlg::Linking -----

    // 1:1 with legacy Linking. Resolves m_pTextArea
    // by kIdTextArea (mirrors legacy CNA_TEXTAREA)
    // and calls SetScriptText with the host-injected
    // chat message callback result (1:1 with
    // CHATMGR->GetChatMsg(30)).
    void Linking();

    // ----- 1:1 with legacy CChinaAdviceDlg::OnActionEvent -----

    // 1:1 with legacy: empty body. No button dispatch.
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);

    // ----- 1:1 with legacy CHATMGR->GetChatMsg(30) -----

    // 1:1 with legacy `CHATMGR->GetChatMsg(int)`. The
    // host supplies a callback that returns the
    // localized string. Tests inject a literal.
    using ChatMsgCallback = const char*(*)(int chatMsgId, void* user);
    void SetChatMsgCallbackForTest(ChatMsgCallback cb, void* user) noexcept {
        m_chatMsgCb   = cb;
        m_chatMsgUser = user;
    }

    // ----- 1:1 with legacy WindowID enum CNA_TEXTAREA -----

    // 1:1 with legacy CNA_TEXTAREA = 360.
    static constexpr std::int32_t kIdTextArea = 360;

    // ----- 1:1 with legacy CHATMGR chat message id 30 -----

    // 1:1 with legacy `CHATMGR->GetChatMsg(30)` -- the
    // China-region T&C / advice text id.
    static constexpr int kChinaAdviceChatMsgId = 30;

    // ----- 1:1 with legacy empty-OnActionEvent no-op -----

    // 1:1 with legacy WE_BTNCLICK constant.
    static constexpr std::uint32_t kWeBtnClick = legacy_window_event::kButtonClick;

    // ----- Test hooks -----

    // 1:1 with legacy m_pTextArea: tests can pre-wire
    // the cTextArea pointer (overrides Linking's
    // findWindowById auto-discovery).
    void  SetTextAreaForTest(cTextArea* t) noexcept  { m_pTextArea = t; }
    cTextArea* GetTextAreaForTest() const noexcept   { return m_pTextArea; }

    // Last SetScriptText argument (for diagnostic
    // assertions about what text was set).
    const char* GetLastScriptTextForTest() const noexcept {
        return m_lastScriptText;
    }

    // 1:1 placeholder text (used when no chat callback
    // is set). Matches the P2-12 stub pattern.
    static constexpr const char* kPlaceholderText = "CHINA_ADVICE_TEXT";

private:
    // 1:1 with legacy m_pTextArea (resolved in Linking
    // by CNA_TEXTAREA id). Modern port stores raw
    // pointer (not owned; cDialog owns the child).
    cTextArea* m_pTextArea = nullptr;

    // 1:1 with legacy CHATMGR singleton -- host-injected
    // callback. The default value (nullptr) means the
    // placeholder text is used (R-12.x deferred).
    ChatMsgCallback m_chatMsgCb   = nullptr;
    void*           m_chatMsgUser = nullptr;

    // Last text passed to SetScriptText (for tests).
    const char*     m_lastScriptText = nullptr;
};

}  // namespace mxh::ui