// cchinaadvicedlg.cpp -- modern implementation of Moxiang
//   CChinaAdviceDlg (China-region advice / T&C dialog).

#include "cchinaadvicedlg.hpp"

#include "ctextarea.hpp"

namespace mxh::ui {

cChinaAdviceDlg::cChinaAdviceDlg() = default;

cChinaAdviceDlg::~cChinaAdviceDlg() = default;

void cChinaAdviceDlg::Linking() {
    // 1:1 with legacy CChinaAdviceDlg::Linking. The
    // legacy body is:
    //   cTextArea* pTextArea =
    //     (cTextArea*)GetWindowForID(CNA_TEXTAREA);
    //   if (pTextArea)
    //     pTextArea->SetScriptText(CHATMGR->GetChatMsg(30));
    //
    // Modern port:
    //   - Resolves m_pTextArea by kIdTextArea (which
    //     mirrors legacy CNA_TEXTAREA). Host-injected
    //     pointer (SetTextAreaForTest) takes priority
    //     over auto-discovery.
    //   - Calls SetScriptText with the host-injected
    //     chat callback result (1:1 with
    //     CHATMGR->GetChatMsg(30)). When the callback
    //     is not set, falls back to kPlaceholderText
    //     (1:1 with the P2-12 stub pattern).
    if (!m_pTextArea) {
        m_pTextArea = static_cast<cTextArea*>(findWindowById(kIdTextArea));
    }
    if (m_pTextArea) {
        const char* text = kPlaceholderText;
        if (m_chatMsgCb) {
            const char* resolved = m_chatMsgCb(kChinaAdviceChatMsgId,
                                               m_chatMsgUser);
            if (resolved) {
                text = resolved;
            }
        }
        m_pTextArea->SetScriptText(text);
        m_lastScriptText = text;
    }
}

void cChinaAdviceDlg::OnActionEvent(std::int32_t /*lId*/,
                                    void* /*p*/,
                                    std::uint32_t /*we*/) {
    // 1:1 with legacy CChinaAdviceDlg::OnActionEvent:
    //   empty body. No button dispatch. The dialog
    //   relies on auto-close / outside-click dismissal.
}

}  // namespace mxh::ui