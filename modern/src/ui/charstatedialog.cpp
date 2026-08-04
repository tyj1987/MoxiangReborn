// charstatedialog.cpp — 1:1 port of 墨香 CCharStateDialog
// (character state bar). See charstatedialog.hpp for the
// data-model rationale + 1:1 quirks.

#include "charstatedialog.hpp"
#include "cpushupbutton.hpp"

namespace mxh::ui {

cCharStateDialog::cCharStateDialog() = default;

cCharStateDialog::~cCharStateDialog() = default;

void cCharStateDialog::SetActionCallbacks(PlayMacroCallback playMacro,
                                           TogglePkCallback togglePk,
                                           void* userData) noexcept {
    m_playMacroCb = playMacro;
    m_togglePkCb = togglePk;
    m_actionCallbackUserData = userData;
}

void cCharStateDialog::Linking() {
    // 1:1 with legacy CCharStateDialog::Linking. Resolve
    // 5 cPushupButton children by id and SetPassive(TRUE)
    // on each (so user can't toggle them — code alone
    // flips their state via SetXxxMode). Defensive
    // null-checks: each button is optional (the modern
    // port is more defensive than the legacy, which
    // unconditionally dereferences each pointer).
    auto resolve = [this](std::int32_t id) -> cPushupButton* {
        return static_cast<cPushupButton*>(findWindowById(id));
    };
    m_pBtnPK        = resolve(kBtnPKId);
    m_pBtnMove      = resolve(kBtnMoveId);
    m_pBtnKyungGong = resolve(kBtnKyungGongId);
    m_pBtnPeaceWar  = resolve(kBtnPeaceWarId);
    m_pBtnUngi      = resolve(kBtnUngiId);

    if (m_pBtnPK)        m_pBtnPK->SetPassive(true);
    if (m_pBtnMove)      m_pBtnMove->SetPassive(true);
    if (m_pBtnKyungGong) m_pBtnKyungGong->SetPassive(true);
    if (m_pBtnPeaceWar)  m_pBtnPeaceWar->SetPassive(true);
    if (m_pBtnUngi)      m_pBtnUngi->SetPassive(true);
}

void cCharStateDialog::OnActionEvent(std::int32_t lId, void* p,
                                     std::uint32_t we) {
    (void)p;
    if ((we & kWePushUp) == 0 && (we & kWePushDown) == 0) {
        return;
    }

    switch (lId) {
    case kBtnMoveId:
        if (m_playMacroCb) {
            m_playMacroCb(kMacroToggleMove, m_actionCallbackUserData);
        }
        break;
    case kBtnPeaceWarId:
        if (m_playMacroCb) {
            m_playMacroCb(kMacroTogglePeaceWar, m_actionCallbackUserData);
        }
        break;
    case kBtnPKId:
        if (m_togglePkCb) {
            m_togglePkCb(m_actionCallbackUserData);
        }
        break;
    default:
        break;
    }
}

void cCharStateDialog::SetPKMode(bool bPKMode) noexcept {
    // 1:1 with legacy CCharStateDialog::SetPKMode. Pure
    // widget state — no singleton. Defensive null-check
    // (the legacy unconditionally dereferences).
    if (m_pBtnPK) m_pBtnPK->SetPush(bPKMode);
}

void cCharStateDialog::SetMoveMode(bool bRun) noexcept {
    if (m_pBtnMove) m_pBtnMove->SetPush(bRun);
}

void cCharStateDialog::SetKyungGongMode(bool bKyungGong) noexcept {
    if (m_pBtnKyungGong) m_pBtnKyungGong->SetPush(bKyungGong);
}

void cCharStateDialog::SetPeaceWarMode(bool bPeace) noexcept {
    // 1:1 quirk: the legacy inverts the argument
    // (m_pBtnPeaceWar->SetPush(!bPeace)) because the
    // underlying "peace" flag is the OPPOSITE of what
    // the button displays. The button shows "war mode"
    // (push = war) but the public API is "peace mode"
    // (bPeace = true means peace = button NOT pushed).
    // Modern port mirrors the inversion.
    if (m_pBtnPeaceWar) m_pBtnPeaceWar->SetPush(!bPeace);
}

void cCharStateDialog::SetUngiMode(bool bUngi) noexcept {
    if (m_pBtnUngi) m_pBtnUngi->SetPush(bUngi);
}

void cCharStateDialog::Refresh() {
    // 1:1 with legacy CCharStateDialog::Refresh. Rebuilds
    // tooltips on 3 of the 5 buttons (Move + PeaceWar
    // active, KyungGong + Ungi commented out in legacy)
    // using SCRIPTMGR + RESRCMGR + MACROMGR + GAMEIN
    // singletons. The modern port is a no-op until those
    // singletons are ported.
    //
    // When ported, the implementation will mirror the
    // legacy:
    //   cImage ToolTipImg;
    //   SCRIPTMGR->GetImage(63, &ToolTipImg, PFT_HARDPATH);
    //   sMACRO* pMacro;
    //   char imagePath[128], strMacro[32];
    //   strcpy(imagePath, RESRCMGR->GetMsg(358));
    //   pMacro = MACROMGR->GetCurMacroKey(ME_TOGGLE_MOVEMODE);
    //   GAMEIN->GetMacroDialog()->ConvertMacroToText(strMacro, pMacro);
    //   wsprintf(imagePath, "%s(%s)", imagePath, strMacro);
    //   m_pBtnMove->SetToolTip(imagePath, RGB_HALF(255,255,255), &ToolTipImg);
    //   // (similar for PeaceWar with msg 360)
    //   // (KyungGong msg 359 + Ungi msg 361 commented out)
    //
    // TODO: rebuild tooltips once SCRIPTMGR + RESRCMGR +
    //       MACROMGR + GAMEIN singletons are ported.
}

}  // namespace mxh::ui
