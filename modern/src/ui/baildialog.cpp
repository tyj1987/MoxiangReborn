// baildialog.cpp — 1:1 port of 墨香 CBailDialog (bail dialog).
// See baildialog.hpp for the data-model rationale + 1:1
// quirks.

#include "baildialog.hpp"
#include "ceditbox.hpp"
#include "ctextarea.hpp"

namespace mxh::ui {

cBailDialog::cBailDialog() = default;

cBailDialog::~cBailDialog() = default;

void cBailDialog::Linking() {
    // 1:1 with legacy CBailDialog::Linking. REAL —
    // resolve 2 children + SetValidCheck + SetAlign +
    // SetScriptText. Defensive null-checks.
    m_pBailEdtBox = static_cast<cEditBox*>(findWindowById(kBailEditBoxId));
    m_pBailText   = static_cast<cTextArea*>(findWindowById(kBailTextId));

    if (m_pBailEdtBox) {
        // 1:1 quirk: legacy calls
        //   m_pBailEdtBox->SetValidCheck(VCM_NUMBER)
        //   m_pBailEdtBox->SetAlign(TXT_RIGHT)
        // The modern cEditBox supports the same surface
        // (SetValidCheck(1) = digits only, TextAlign::Right
        // = 2).
        m_pBailEdtBox->SetValidCheck(1);   // VCM_NUMBER = 1
        m_pBailEdtBox->SetAlign(cEditBox::TextAlign::Right);
    }

    if (m_pBailText) {
        // 1:1 quirk: legacy calls
        //   char buf[256];
        //   strcpy(strBfame, AddComma((DWORD)MIN_BADFAME_FOR_BAIL));
        //   strcpy(strBprice, AddComma(BAIL_PRICE));
        //   wsprintf(buf, CHATMGR->GetChatMsg(644), strBfame, strBprice);
        //   m_pBailText->SetScriptText(buf);
        // Modern port uses placeholder text
        // "BAIL_TEXT_PLACEHOLDER" (the formatted string
        // would need CHATMGR + AddComma + the bail
        // constants to be ported).
        m_pBailText->SetScriptText("BAIL_TEXT_PLACEHOLDER");
    }
}

void cBailDialog::Open() {
    // 1:1 with legacy CBailDialog::Open. The legacy
    // is:
    //   if (HERO->GetBadFame() > MIN_BADFAME_FOR_BAIL) {
    //       m_pBailEdtBox->SetEditText("0");
    //       SetActive(TRUE);
    //   } else {
    //       WINDOWMGR->MsgBox(MBI_BAILLOWBADFAME, MBT_OK,
    //                          CHATMGR->GetChatMsg(659),
    //                          AddComma((DWORD)MIN_BADFAME_FOR_BAIL));
    //   }
    //
    // Modern port: TODO. The singleton dispatch
    // (HERO + WINDOWMGR + CHATMGR) is deferred.
    (void)0;
    // TODO: implement the conditional Open based on
    //       HERO->GetBadFame() vs MIN_BADFAME_FOR_BAIL +
    //       WINDOWMGR->MsgBox for the failure case.
}

void cBailDialog::Close() {
    // 1:1 with legacy CBailDialog::Close. The legacy
    // is:
    //   SetDisable(FALSE);
    //   SetActive(FALSE);
    //   OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Deal);
    //
    // Modern port: TODO. The SetDisable + SetActive +
    // OBJECTSTATEMGR calls are deferred.
    (void)0;
    // TODO: implement SetDisable(FALSE) + SetActive(FALSE) +
    //       OBJECTSTATEMGR->EndObjectState(HERO, eObjectState_Deal).
}

void cBailDialog::SetFame() {
    // 1:1 with legacy CBailDialog::SetFame. The legacy
    // is:
    //   m_BadFame = atoi(RemoveComma(m_pBailEdtBox->GetEditText()));
    //   if (m_BadFame == 0) return;
    //   if (m_BadFame + MIN_BADFAME_FOR_BAIL > HERO->GetBadFame()) {
    //       WINDOWMGR->MsgBox(MBI_BAILNOTICEERR, MBT_OK,
    //                          CHATMGR->GetChatMsg(648), ...);
    //       SetDisable(TRUE);
    //       return;
    //   }
    //   if (HERO->GetMoney() < m_BadFame * BAIL_PRICE) {
    //       WINDOWMGR->MsgBox(MBI_BAILNOTICEERR, MBT_OK, ...);
    //       SetDisable(TRUE);
    //       return;
    //   }
    //   WINDOWMGR->MsgBox(MBI_BAILNOTICEMSG, MBT_YESNO, ...);
    //   SetDisable(TRUE);
    //
    // Modern port: TODO. The 4-singleton dispatch
    // (HERO + WINDOWMGR + CHATMGR + the bail constants)
    // is deferred.
    (void)0;
    // TODO: implement the bail amount validation +
    //       hero money check + WINDOWMGR msg box flows.
}

void cBailDialog::SetBadFrameSync() {
    // 1:1 with legacy CBailDialog::SetBadFrameSync. The
    // legacy is:
    //   MSG_FAME msg;
    //   msg.Category = MP_CHAR;
    //   msg.Protocol = MP_CHAR_BADFAME_SYN;
    //   msg.dwObjectID = HERO->GetID();
    //   if (m_BadFame <= 0) return;
    //   msg.Fame = m_BadFame;
    //   NETWORK->Send(&msg, sizeof(msg));
    //   Close();
    //
    // Modern port: TODO. The 3-singleton dispatch
    // (HERO + NETWORK + Close) is deferred.
    (void)0;
    // TODO: implement the MSG_FAME network message +
    //       Close() call.
}

}  // namespace mxh::ui
