// survivalcountdialog.cpp — 1:1 port of 墨香
// CSurvivalCountDialog (survival-mode alive
// counter + winner name dialog). See
// survivalcountdialog.hpp for the data-model
// rationale + 1:1 quirks.

#include "survivalcountdialog.hpp"
#include "cstatic.hpp"

#include <cstdio>

namespace mxh::ui {

cSurvivalCountDialog::cSurvivalCountDialog() {
    // 1:1 with legacy CSurvivalCountDialog ctor:
    //   m_pCounterNum = NULL;
    //   m_pWinnerName = NULL;
    //
    // 1:1 quirk: modern raw pointers use default
    // member init (= nullptr in header). The
    // commented-out m_pCounterNum[2] = NULL lines
    // from legacy are dropped (1:1 with the active
    // 1-cStatic implementation).
}

cSurvivalCountDialog::~cSurvivalCountDialog() = default;

void cSurvivalCountDialog::SetCallbacks(
    IsSurvivalMapFn isSurvivalMap,
    GetChatMessageFn getChatMessage,
    void* userData) noexcept {
    m_isSurvivalMapFn = isSurvivalMap;
    m_getChatMessageFn = getChatMessage;
    m_callbackUserData = userData;
}

void cSurvivalCountDialog::Linking() {
    // 1:1 with legacy CSurvivalCountDialog::Linking.
    // The legacy is:
    //   m_pCounterNum = (cStatic*)GetWindowForID(SVV_ALIVECOUNTER);
    //   m_pWinnerName = (cStatic*)GetWindowForID(SVV_WINNERNAME);
    //   SetCounterNumber(0);
    //   m_pWinnerName->SetStaticText(CHATMGR->GetChatMsg(484));
    m_pCounterNum =
        static_cast<cStatic*>(findWindowById(kIdAliveCounter));
    m_pWinnerName =
        static_cast<cStatic*>(findWindowById(kIdWinnerName));

    SetCounterNumber(0);
    if (m_pWinnerName) {
        const char* defaultName = m_getChatMessageFn
            ? m_getChatMessageFn(kSurvivalDefaultNameMessageId,
                                 m_callbackUserData)
            : nullptr;
        m_pWinnerName->SetStaticText(
            defaultName ? defaultName : kSurvivalDefaultName);
    }
}

void cSurvivalCountDialog::InitSurvivalCountDlg(int mapNum) {
    // 1:1 with legacy CSurvivalCountDialog::InitSurvivalCountDlg.
    // The legacy is:
    //   if (MAP->IsMapKind(eSurvivalMap))
    //     SetActive(TRUE);
    //   else
    //     SetActive(FALSE);
    //
    // Legacy ignores MapNum and queries the current MAP singleton.
    (void)mapNum;
    SetActive(m_isSurvivalMapFn
        ? m_isSurvivalMapFn(m_callbackUserData)
        : false);
}

void cSurvivalCountDialog::SetCounterNumber(std::uint32_t num) {
    // 1:1 with legacy CSurvivalCountDialog::SetCounterNumber.
    // The legacy is:
    //   int c1, c2;
    //   c1 = num%10;
    //   c2 = num/10;
    //   char temp[128] = {0,};
    //   sprintf(temp, "%d%d", c2, c1);
    //   if (m_pCounterNum)
    //     m_pCounterNum->SetStaticText(temp);
    //
    // The legacy has an "if (num > 99) num = 99;" in
    // the commented-out 2-array version. The active
    // 1-cStatic version does NOT clamp (since
    // sprintf with c2/num/10 would still produce a
    // 3-digit string for num > 99). Modern port
    // does NOT clamp (1:1 with active 1-cStatic
    // version) but the kMaxCounterNumber constant
    // is preserved for the legacy 2-array behavior
    // documentation.
    if (!m_pCounterNum) return;
    int c1 = static_cast<int>(num % 10u);
    int c2 = static_cast<int>(num / 10u);
    char temp[128];
    std::snprintf(temp, sizeof(temp), "%d%d", c2, c1);
    m_pCounterNum->SetStaticText(temp);
}

void cSurvivalCountDialog::SetWinnerName(const char* pName) {
    // 1:1 with legacy CSurvivalCountDialog::SetWinnerName.
    // The legacy is:
    //   if (pName)
    //     m_pWinnerName->SetStaticText(pName);
    //   else
    //     m_pWinnerName->SetStaticText(CHATMGR->GetChatMsg(484));
    //
    // The modern port: defensive null check +
    // SetStaticText. Fallback to kSurvivalDefaultName
    // placeholder for CHATMGR msg 484.
    if (!m_pWinnerName) return;
    if (pName) {
        m_pWinnerName->SetStaticText(pName);
    } else {
        const char* defaultName = m_getChatMessageFn
            ? m_getChatMessageFn(kSurvivalDefaultNameMessageId,
                                 m_callbackUserData)
            : nullptr;
        m_pWinnerName->SetStaticText(
            defaultName ? defaultName : kSurvivalDefaultName);
    }
}

}  // namespace mxh::ui
