// chinaadvicedlg.cpp — 1:1 port of 墨香 CChinaAdviceDlg
// (China-region advice / T&C dialog). See
// chinaadvicedlg.hpp for the data-model rationale +
// 1:1 quirks.

#include "chinaadvicedlg.hpp"
#include "ctextarea.hpp"

namespace mxh::ui {

cChinaAdviceDlg::cChinaAdviceDlg() {
    // 1:1 with legacy CChinaAdviceDlg::CChinaAdviceDlg:
    //   empty body, no state init.
}

cChinaAdviceDlg::~cChinaAdviceDlg() = default;

void cChinaAdviceDlg::Linking() {
    // 1:1 with legacy CChinaAdviceDlg::Linking. The
    // legacy is:
    //   cTextArea* pTextArea =
    //     (cTextArea*)GetWindowForID(CNA_TEXTAREA);
    //   if (pTextArea)
    //     pTextArea->SetScriptText(CHATMGR->GetChatMsg(30));
    //
    // The modern port:
    //   - Resolves m_pTextArea by kIdTextArea (which
    //     mirrors the legacy CNA_TEXTAREA).
    //   - Calls SetScriptText("CHINA_ADVICE_TEXT")
    //     (placeholder for CHATMGR->GetChatMsg(30)
    //     — same pattern as cMPNoticeDialog's
    //     "MP_NCAUTION" placeholder). When CHATMGR is
    //     ported, the body becomes:
    //       if (pTextArea)
    //         pTextArea->SetScriptText(CHATMGR->GetChatMsg(30));
    cTextArea* pText = static_cast<cTextArea*>(findWindowById(kIdTextArea));
    m_pTextArea = pText;
    if (pText) {
        pText->SetScriptText("CHINA_ADVICE_TEXT");
    }
}

void cChinaAdviceDlg::OnActionEvent(std::int32_t /*lId*/, void* /*p*/, std::uint32_t /*we*/) {
    // 1:1 with legacy CChinaAdviceDlg::OnActionEvent:
    //   empty body. No button dispatch. The dialog
    //   relies on auto-close / outside-click dismissal.
}

}  // namespace mxh::ui
