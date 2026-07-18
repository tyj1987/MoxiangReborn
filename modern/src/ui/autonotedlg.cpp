// autonotedlg.cpp — 1:1 port of 墨香
// CAutoNoteDlg (auto note / auto reply dialog).
// See autonotedlg.hpp for the data-model rationale
// + 1:1 quirks.

#include "autonotedlg.hpp"
#include "ctextarea.hpp"
#include "cbutton.hpp"
#include "clistdialog.hpp"

#include <cstdio>
#include <string>

namespace mxh::ui {

cAutoNoteDlg::cAutoNoteDlg() {
    // 1:1 with legacy CAutoNoteDlg ctor:
    //   m_pTextAreaManual = NULL;
    //   m_pBtnAsk = NULL;
    //   m_pListAuto = NULL;
    //
    // 1:1 quirk: modern raw pointers use default
    // member init (= nullptr in header). ctor body
    // is empty.
}

cAutoNoteDlg::~cAutoNoteDlg() = default;

void cAutoNoteDlg::Linking() {
    // 1:1 with legacy CAutoNoteDlg::Linking. The
    // legacy is:
    //   m_pTextAreaManual = (cTextArea*)GetWindowForID(AND_TEXTAREA_MANUAL);
    //   m_pBtnAsk = (cButton*)GetWindowForID(AND_BTN_ASK);
    //   m_pListAuto = (cListDialog*)GetWindowForID(AND_LIST_AUTO);
    //   m_pTextAreaManual->SetScriptText(CHATMGR->GetChatMsg(1721));
    //   m_pTextAreaManual->SetTextColor(RGB_HALF(128, 128, 128));
    m_pTextAreaManual = static_cast<cTextArea*>(
        findWindowById(kIdTextAreaManual));
    m_pBtnAsk = static_cast<cButton*>(
        findWindowById(kIdBtnAsk));
    m_pListAuto = static_cast<cListDialog*>(
        findWindowById(kIdListAuto));
    if (m_pTextAreaManual) {
        // 1:1 with legacy CHATMGR->GetChatMsg(1721).
        // Modern port uses kAutoNoteManualText
        // placeholder until CHATMGR is ported.
        m_pTextAreaManual->SetScriptText(kAutoNoteManualText);
        // 1:1 with legacy RGB_HALF(128, 128, 128)
        // (gray).
        m_pTextAreaManual->SetTextColor(kAutoNoteTextColor);
    }
}

void cAutoNoteDlg::OnActionEvent(std::int32_t lId, void* p,
                                 std::uint32_t we) {
    // 1:1 with legacy CAutoNoteDlg::OnActionEvent.
    // The legacy is:
    //   if (we & WE_BTNCLICK) {
    //     if (lId == AND_BTN_ASK) {
    //       CObject* pObject = OBJECTMGR->GetSelectedObject();
    //       if (pObject == NULL) {
    //         CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(1704));
    //         return;
    //       }
    //       if (pObject->GetObjectKind() != eObjectKind_Player) {
    //         CHATMGR->AddMsg(CTC_SYSMSG, CHATMGR->GetChatMsg(1704));
    //         return;
    //       }
    //       #ifndef _GMTOOL_
    //       if (pObject == HERO) return;
    //       #endif
    //       AUTONOTEMGR->AskToAutoUser(pObject->GetID(), rand()%100);
    //     }
    //   }
    //
    // The modern port: the whole method is TODO
    // (OBJECTMGR + HERO + AUTONOTEMGR + CHATMGR
    // singletons, R-12.x deferred). Modern port is
    // no-op for now.
    (void)lId;
    (void)p;
    (void)we;
    // TODO: OBJECTMGR + HERO + AUTONOTEMGR + CHATMGR
    //       not ported (R-12.x deferred). When
    //       ported, the body becomes the legacy code.
}

void cAutoNoteDlg::AddAutoList(const char* strName, const char* strDate) {
    // 1:1 with legacy CAutoNoteDlg::AddAutoList.
    // The legacy is:
    //   char buf[128];
    //   char day[11];
    //   SafeStrCpy(day, strDate, 11);
    //   sprintf(buf, "%-16s %s", strName, day);
    //   m_pListAuto->AddItem(buf, RGB_HALF(128, 128, 128));
    //
    // The modern port: sprintf "%-16s %s" with
    // defensive null checks (legacy SafeStrCpy
    // assumes non-null). AddItem is REAL (modern
    // cListDialog::AddItem).
    if (!m_pListAuto) return;
    if (!strName || !strDate) return;
    char buf[128];
    std::snprintf(buf, sizeof(buf), "%-16s %s", strName, strDate);
    m_pListAuto->AddItem(buf, kAutoNoteTextColor);
}

void cAutoNoteDlg::SetActiveTestClient() {
    // 1:1 with legacy CAutoNoteDlg::SetActiveTestClient.
    // The legacy is:
    //   SetActive(TRUE);
    //   char buf[128];
    //   for (int i = 0; i < 35; ++i) {
    //     wsprintf(buf, "%d %-16s %s", i, "테스트유저", "2008-05-01 12:00");
    //     m_pListAuto->AddItem(buf, RGB_HALF(0, 0, 0));
    //   }
    SetActive(true);
    if (!m_pListAuto) return;
    char buf[128];
    for (int i = 0; i < kTestClientLoopCount; ++i) {
        std::snprintf(buf, sizeof(buf), "%d %-16s %s", i,
                      "TESTUSER", "2008-05-01 12:00");
        m_pListAuto->AddItem(buf, 0xFF000000u);  // RGB_HALF(0, 0, 0) (black)
    }
}

}  // namespace mxh::ui
