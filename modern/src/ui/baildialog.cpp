// baildialog.cpp - 1:1 port of legacy CBailDialog
// (bail dialog). See baildialog.hpp for the data-model
// rationale + 1:1 quirks.

#include "baildialog.hpp"
#include "ceditbox.hpp"
#include "ctextarea.hpp"

namespace mxh::ui {

cBailDialog::cBailDialog() = default;

cBailDialog::~cBailDialog() = default;

void cBailDialog::SetCallbacks(GetHeroBadFameFn getHeroBadFame,
                              GetHeroMoneyFn getHeroMoney,
                              GetHeroIdFn getHeroId,
                              ParseAmountFn parseAmount,
                              ShowMsgBoxFn showMsgBox,
                              EndDealStateFn endDealState,
                              SendBadFameFn sendBadFame,
                              void* userData) noexcept {
    m_getHeroBadFameFn = getHeroBadFame;
    m_getHeroMoneyFn   = getHeroMoney;
    m_getHeroIdFn      = getHeroId;
    m_parseAmountFn    = parseAmount;
    m_showMsgBoxFn     = showMsgBox;
    m_endDealStateFn   = endDealState;
    m_sendBadFameFn    = sendBadFame;
    m_callbackUserData = userData;
}

void cBailDialog::Linking() {
    m_pBailEdtBox = static_cast<cEditBox*>(findWindowById(kBailEditBoxId));
    m_pBailText   = static_cast<cTextArea*>(findWindowById(kBailTextId));

    if (m_pBailEdtBox) {
        m_pBailEdtBox->SetValidCheck(1);   // VCM_NUMBER = 1
        m_pBailEdtBox->SetAlign(cEditBox::TextAlign::Right);
    }

    if (m_pBailText) {
        m_pBailText->SetScriptText("BAIL_TEXT_PLACEHOLDER");
    }
}
void cBailDialog::Open() {
    // 1:1 with legacy CBailDialog::Open. The legacy is:
    //   if (HERO->GetBadFame() > MIN_BADFAME_FOR_BAIL) {
    //       m_pBailEdtBox->SetEditText("0");
    //       SetActive(TRUE);
    //   } else {
    //       WINDOWMGR->MsgBox(MBI_BAILLOWBADFAME, MBT_OK,
    //                          CHATMGR->GetChatMsg(659),
    //                          AddComma((DWORD)MIN_BADFAME_FOR_BAIL));
    //   }
    //
    // Modern port (1:1):
    //   - GetHeroBadFameFn() replaces HERO->GetBadFame().
    //     If the callback is null, the dialog stays hidden (safe
    //     no-op).
    //   - The else-branch dispatches via ShowMsgBoxFn (which replaces
    //     WINDOWMGR->MsgBox + CHATMGR->GetChatMsg + AddComma). If the
    //     callback is null, the failure branch is silently skipped.
    //   - The m_pBailEdtBox->SetEditText + SetActive calls are direct
    //     (no host callback needed).
    if (!m_getHeroBadFameFn) {
        return;
    }
    if (m_getHeroBadFameFn(m_callbackUserData) > kMinBadFameForBail) {
        if (m_pBailEdtBox) {
            m_pBailEdtBox->SetEditText("0");
        }
        SetActive(true);
        return;
    }
    if (m_showMsgBoxFn) {
        m_showMsgBoxFn(kMbiBailLowBadFame, kMbtOk, kChatMsgLowBadFame,
                        kMinBadFameForBail, 0u, m_callbackUserData);
    }
}
void cBailDialog::Close() {
    // 1:1 with legacy CBailDialog::Close. The legacy is:
    //   SetDisable(FALSE);
    //   SetActive(FALSE);
    //   OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Deal);
    //
    // Modern port (1:1):
    //   - SetDisable + SetActive are direct base calls (no host
    //     callback needed).
    //   - EndDealStateFn replaces OBJECTSTATEMGR->EndObjectState.
    //     If the callback is null, the state-end branch is silently
    //     skipped.
    SetDisable(false);
    SetActive(false);
    if (m_endDealStateFn) {
        m_endDealStateFn(m_callbackUserData);
    }
}
void cBailDialog::SetFame() {
    // 1:1 with legacy CBailDialog::SetFame. The legacy is:
    //   m_BadFame = atoi(RemoveComma(m_pBailEdtBox->GetEditText()));
    //   if (m_BadFame == 0) return;
    //   if (m_BadFame + MIN_BADFAME_FOR_BAIL > HERO->GetBadFame()) {
    //       cMsgBox* pMsgBox = WINDOWMGR->MsgBox(MBI_BAILNOTICEERR, MBT_OK,
    //                                       CHATMGR->GetChatMsg(648),
    //                                       AddComma((DWORD)MIN_BADFAME_FOR_BAIL));
    //       if (pMsgBox) SetDisable(TRUE);
    //       return;
    //   }
    //   if (HERO->GetMoney() < m_BadFame*BAIL_PRICE) {
    //       cMsgBox* pMsgBox = WINDOWMGR->MsgBox(MBI_BAILNOTICEERR, MBT_OK,
    //                                       CHATMGR->GetChatMsg(117));
    //       if (pMsgBox) SetDisable(TRUE);
    //       return;
    //   }
    //   char buf[256] = { 0, };
    //   char badfame[16] = { 0, };
    //   char money[16] = { 0, };
    //   strcpy(badfame, AddComma(m_BadFame));
    //   strcpy(money, AddComma(m_BadFame*BAIL_PRICE));
    //   sprintf(buf, CHATMGR->GetChatMsg(645), money, badfame);
    //   cMsgBox* pMsgBox = WINDOWMGR->MsgBox(MBI_BAILNOTICEMSG, MBT_YESNO, buf);
    //   if (pMsgBox) SetDisable(TRUE);
    //
    // Modern port (1:1):
    //   - ParseAmountFn replaces atoi(RemoveComma(...)). If the
    //     callback is null or returns 0, the method returns early
    //     (legacy: m_BadFame == 0 -> return).
    //   - GetHeroBadFameFn + GetHeroMoneyFn replace HERO->GetBadFame +
    //     HERO->GetMoney. If either is null, the corresponding
    //     branch is silently skipped (the bail flow cannot proceed
    //     safely without the hero data).
    //   - ShowMsgBoxFn replaces WINDOWMGR->MsgBox + CHATMGR->GetChatMsg
    //     + AddComma + the legacy buf formatting. param1 carries the
    //     minimum required bad fame (1st error) or is zero (2nd error)
    //     or the cost (3rd confirmation). param2 carries the bad fame
    //     amount (3rd confirmation) or is zero (1st/2nd errors).
    //   - The legacy pMsgBox truthiness gates SetDisable(TRUE). The
    //     modern port calls SetDisable(TRUE) iff ShowMsgBoxFn returns
    //     true (1:1).
    //   - 1:1 quirk: both arithmetic expressions use DWORD
    //     semantics. Unsigned 32-bit overflow therefore wraps before
    //     the comparisons and before the confirmation cost is sent.
    if (!m_parseAmountFn || !m_pBailEdtBox) {
        return;
    }
    const std::string editText = m_pBailEdtBox->editText();
    m_BadFame = m_parseAmountFn(editText.c_str(), m_callbackUserData);
    if (m_BadFame == 0) {
        return;
    }

    if (m_getHeroBadFameFn
        && m_BadFame + kMinBadFameForBail
            > m_getHeroBadFameFn(m_callbackUserData)) {
        if (m_showMsgBoxFn
            && m_showMsgBoxFn(kMbiBailNoticeErr, kMbtOk,
                              kChatMsgNoticeBadFame,
                              kMinBadFameForBail, 0u,
                              m_callbackUserData)) {
            SetDisable(true);
        }
        return;
    }

    // Money check: 1:1 with legacy
    //   if (HERO->GetMoney() < m_BadFame*BAIL_PRICE)
    // Both operands are std::uint32_t, preserving the legacy DWORD
    // multiplication and its modulo-2^32 wrap-around.
    if (m_getHeroMoneyFn
        && m_getHeroMoneyFn(m_callbackUserData)
            < m_BadFame * kBailPrice) {
        if (m_showMsgBoxFn
            && m_showMsgBoxFn(kMbiBailNoticeErr, kMbtOk,
                              kChatMsgNoticeMoney,
                              0u, 0u, m_callbackUserData)) {
            SetDisable(true);
        }
        return;
    }

    // Confirmation message: 1:1 with legacy
    //   sprintf(buf, CHATMGR->GetChatMsg(645), money, badfame);
    //   WINDOWMGR->MsgBox(MBI_BAILNOTICEMSG, MBT_YESNO, buf);
    // The host ShowMsgBoxFn receives the raw numeric values (cost,
    // badFame) and is responsible for AddComma-formatting them into
    // the chat message template.
    if (m_showMsgBoxFn
        && m_showMsgBoxFn(kMbiBailNoticeMsg, kMbtYesNo,
                          kChatMsgConfirmBail,
                          m_BadFame * kBailPrice,
                          m_BadFame, m_callbackUserData)) {
        SetDisable(true);
    }
}
void cBailDialog::SetBadFrameSync() {
    // 1:1 with legacy CBailDialog::SetBadFrameSync. The legacy is:
    //   MSG_FAME msg;
    //   msg.Category = MP_CHAR;
    //   msg.Protocol = MP_CHAR_BADFAME_SYN;
    //   msg.dwObjectID = HERO->GetID();
    //   if (m_BadFame <= 0) return;
    //   msg.Fame = m_BadFame;
    //   NETWORK->Send(&msg, sizeof(msg));
    //   Close();
    //
    // Modern port (1:1):
    //   - GetHeroIdFn replaces HERO->GetID().
    //   - SendBadFameFn replaces NETWORK->Send. The host serializes
    //     MSG_FAME {Category=Character=3, Protocol=45, dwObjectID,
    //     Fame} and pushes onto the network queue.
    //   - Close() is called directly (no callback needed).
    //   - 1:1 quirk: legacy sets msg.Category + msg.Protocol +
    //     msg.dwObjectID BEFORE the early-return check. The modern
    //     port preserves the early-return semantics (no send when
    //     m_BadFame <= 0).
    //   - 1:1 quirk: legacy does not check the return value of
    //     NETWORK->Send; modern does not propagate the send result.
    if (m_BadFame <= 0) {
        return;
    }
    if (!m_sendBadFameFn || !m_getHeroIdFn) {
        return;
    }
    const std::uint32_t objectId = m_getHeroIdFn(m_callbackUserData);
    (void)m_sendBadFameFn(objectId, m_BadFame, m_callbackUserData);
    Close();
}

}  // namespace mxh::ui
