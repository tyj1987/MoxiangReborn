// guildnoticedlg.cpp — 1:1 port of 墨香 CGuildNoticeDlg
// (guild notice editor dialog). See guildnoticedlg.hpp
// for the data-model rationale + 1:1 quirks.

#include "guildnoticedlg.hpp"
#include "ctextarea.hpp"

namespace mxh::ui {

cGuildNoticeDlg::cGuildNoticeDlg() {
    // 1:1 with legacy CGuildNoticeDlg::CGuildNoticeDlg:
    //   empty body, no state init.
}

cGuildNoticeDlg::~cGuildNoticeDlg() = default;

void cGuildNoticeDlg::Linking() {
    // 1:1 with legacy CGuildNoticeDlg::Linking. The
    // legacy is:
    //   m_pNoticeText = (cTextArea*)GetWindowForID(GNotice_TEXTREA);
    //   m_pNoticeText->SetEnterAllow(FALSE);
    //   m_pNoticeText->SetScriptText("");
    //
    // The modern port:
    //   - Resolves m_pNoticeText by kIdNoticeText
    //     (which mirrors the legacy GNotice_TEXTREA).
    //   - Calls SetEnterAllow(FALSE) (cTextArea port
    //     has SetEnterAllow — same 1:1 contract).
    //   - Calls SetScriptText("") to clear any
    //     previous content.
    //
    // 1:1 quirk: legacy only resolves the cTextArea;
    // the 2 button children (SENDOKBTN / CANCELBTN)
    // are NOT resolved here. They're handled in
    // OnActionEvent by their id (legacy uses
    // GetWindowForID inside the switch case; modern
    // port uses the constants directly).
    cTextArea* pText = static_cast<cTextArea*>(findWindowById(kIdNoticeText));
    m_pNoticeText = pText;
    if (pText) {
        pText->SetEnterAllow(false);
        pText->SetScriptText("");
    }
}

void cGuildNoticeDlg::OnActionEvent(std::int32_t lId, void* p, std::uint32_t we) {
    // 1:1 with legacy CGuildNoticeDlg::OnActionEvent
    // (the legacy typo'd the name as "OnActionEvnet"
    // — modern port uses correct spelling). The
    // legacy is:
    //   if (we & WE_BTNCLICK) {
    //       switch (lId) {
    //       case GNotice_SENDOKBTN: {
    //           char notice[MAX_GUILD_NOTICE+1] = {0,};
    //           m_pNoticeText->GetScriptText(notice);
    //           GUILDMGR->SetGuildNotice(notice);
    //           SetActive(FALSE);
    //       } break;
    //       case GNotice_CANCELBTN: {
    //           SetActive(FALSE);
    //       } break;
    //       }
    //   }
    //
    // The modern port:
    //   - Uses kIdSendOkBtn / kIdCancelBtn (1:1 with
    //     legacy GNotice_SENDOKBTN / GNotice_CANCELBTN).
    //   - WE_BTNCLICK is the click event flag (Phase 6
    //     cDialog::we constants). The bit mask check
    //     `if (we & WE_BTNCLICK)` is 1:1 with legacy.
    //   - The SEND branch's GUILDMGR->SetGuildNotice
    //     is TODO (GUILDMGR not ported, R-12.x
    //     deferred). GetScriptText + SetActive(FALSE)
    //     would also dispatch through GUILDMGR, so
    //     the whole branch is documented as TODO.
    //   - The CANCEL branch's SetActive(FALSE) is
    //     also TODO (would dispatch through GUILDMGR
    //     via the SetActive override).
    //   - Unknown ids are silently ignored (1:1 with
    //     legacy switch fallthrough).
    (void)p;
    constexpr std::uint32_t WE_BTNCLICK = 0x0001;  // legacy cWindow::we
    if (!(we & WE_BTNCLICK)) {
        return;
    }
    switch (lId) {
        case kIdSendOkBtn: {
            // TODO: 1:1 with legacy GNotice_SENDOKBTN
            //       branch:
            //         char notice[MAX_GUILD_NOTICE+1] = {0,};
            //         if (m_pNoticeText)
            //             m_pNoticeText->GetScriptText(notice);
            //         GUILDMGR->SetGuildNotice(notice);
            //         SetActive(FALSE);
            //
            // GUILDMGR not ported (R-12.x deferred).
            // When ported, the body becomes:
            //   if (m_pNoticeText) {
            //     char notice[MAX_GUILD_NOTICE+1] = {0,};
            //     m_pNoticeText->GetScriptText(notice);
            //     GUILDMGR->SetGuildNotice(notice);
            //   }
            //   SetActive(false);
            break;
        }
        case kIdCancelBtn: {
            // TODO: 1:1 with legacy GNotice_CANCELBTN
            //       branch:
            //         SetActive(FALSE);
            //
            // The SetActive(FALSE) call is the only
            // operation in this branch. Since it would
            // dispatch through the override (which
            // calls GUILDMGR->GetGuildNotice), the
            // whole branch is documented as TODO.
            // When GUILDMGR is ported, the body
            // becomes:
            //   SetActive(false);
            break;
        }
        default:
            // Unknown id — silently ignored (1:1 with
            // legacy switch fallthrough).
            break;
    }
}

void cGuildNoticeDlg::SetActive(bool val) noexcept {
    // 1:1 with legacy CGuildNoticeDlg::SetActive
    // override. The legacy is:
    //   void CGuildNoticeDlg::SetActive(BOOL val) {
    //       if (val == TRUE) {
    //           if (GUILDMGR->GetGuildNotice())
    //               m_pNoticeText->SetScriptText(GUILDMGR->GetGuildNotice());
    //       }
    //       cDialog::SetActive(val);
    //   }
    //
    // The modern port:
    //   - If val == true and m_pNoticeText is linked,
    //     call m_pNoticeText->SetScriptText(GUILDMGR
    //     ->GetGuildNotice()). The GUILDMGR call is
    //     TODO (GUILDMGR not ported, R-12.x
    //     deferred). The cTextArea->SetScriptText("")
    //     is a safe no-op (it would just clear
    //     pre-existing text) so the modern port
    //     performs the SetScriptText("") unconditionally
    //     when val == true. When GUILDMGR is ported,
    //     this becomes:
    //       if (m_pNoticeText && GUILDMGR->GetGuildNotice())
    //         m_pNoticeText->SetScriptText(GUILDMGR->GetGuildNotice());
    //   - Always calls base SetActive(val) (matches
    //     legacy call order).
    //
    // 1:1 quirk: the notice pre-fill happens BEFORE
    // the base SetActive (matches legacy call order).
    if (val && m_pNoticeText) {
        // TODO: GUILDMGR not ported. When ported,
        //       replace "" with
        //       GUILDMGR->GetGuildNotice() and
        //       guard with non-null check.
        m_pNoticeText->SetScriptText("");
    }
    cDialog::SetActive(val);
}

}  // namespace mxh::ui
