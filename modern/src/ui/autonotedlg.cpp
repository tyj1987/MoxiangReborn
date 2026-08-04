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

void cAutoNoteDlg::SetCallbacks(
    GetSelectedObjectFn getSelectedObject,
    GetObjectKindFn getObjectKind,
    GetObjectIdFn getObjectId,
    IsHeroObjectFn isHeroObject,
    AddSystemMessageFn addSystemMessage,
    GetRandomPercentFn getRandomPercent,
    AskToAutoUserFn askToAutoUser,
    void* userData) noexcept {
    m_getSelectedObject = getSelectedObject;
    m_getObjectKind = getObjectKind;
    m_getObjectId = getObjectId;
    m_isHeroObject = isHeroObject;
    m_addSystemMessage = addSystemMessage;
    m_getRandomPercent = getRandomPercent;
    m_askToAutoUser = askToAutoUser;
    m_callbackUserData = userData;
}

void cAutoNoteDlg::OnActionEvent(std::int32_t lId, void* p,
                                 std::uint32_t we) {
    (void)p;
    if ((we & kWeBtnClick) == 0u || lId != kIdBtnAsk) return;

    void* selectedObject = m_getSelectedObject
        ? m_getSelectedObject(m_callbackUserData)
        : nullptr;
    const auto rejectSelection = [this]() {
        if (m_addSystemMessage) {
            m_addSystemMessage(kSelectPlayerMessageId, m_callbackUserData);
        }
    };
    if (!selectedObject) {
        rejectSelection();
        return;
    }
    if (!m_getObjectKind ||
        m_getObjectKind(selectedObject, m_callbackUserData) != kPlayerObjectKind) {
        rejectSelection();
        return;
    }
#ifndef _GMTOOL_
    if (m_isHeroObject && m_isHeroObject(selectedObject, m_callbackUserData)) {
        return;
    }
#endif
    if (!m_getObjectId || !m_askToAutoUser) return;

    const auto randomValue = m_getRandomPercent
        ? m_getRandomPercent(m_callbackUserData) % 100u
        : 0u;
    m_askToAutoUser(m_getObjectId(selectedObject, m_callbackUserData),
                    randomValue, m_callbackUserData);
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
