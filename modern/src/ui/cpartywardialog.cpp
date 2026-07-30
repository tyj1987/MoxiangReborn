//
// 1:1 port of legacy `CPartyWarDialog` from
//   [Client]MH/PartyWarDialog.{h,cpp}.
//

#include "mxh/ui/cpartywardialog.hpp"
#include "mxh/ui/cbutton.hpp"
#include "mxh/ui/ccheckbox.hpp"
#include "mxh/ui/cstatic.hpp"
#include "mxh/ui/ctextarea.hpp"

#include <cstring>
#include <utility>

namespace mxh::ui {

cPartyWarDialog::cPartyWarDialog() {
    m_bPartyLock = false;
    m_bEnemyLock = false;
}

cPartyWarDialog::~cPartyWarDialog() = default;

void cPartyWarDialog::Init(std::int32_t x, std::int32_t y,
                  std::uint16_t wid, std::uint16_t hei,
                  void* basicImage, std::int32_t id) {
    cDialog::Init(x, y, wid, hei, basicImage, id);
}

void cPartyWarDialog::Render() {
    cDialog::Render();
    // 1:1 with legacy: draws the lock sprite at (x+20, y+120) / (x+205, y+120)
    // when m_bPartyLock / m_bEnemyLock are set. Modern port is a no-op
    // (render path lands in 6.13+; the lock state is tracked via
    // m_bPartyLock + m_bEnemyLock).
}

void cPartyWarDialog::Linking() {
    // 1:1 with legacy: lookup 14 checkboxes + 14 statics + 4 buttons +
    // 1 textarea + 3 statics via the host-injected child-window pointers.
    // The legacy SCRIPTMGR->GetImage(65, &m_LockImage, PFT_HARDPATH)
    // call is dropped (no modern script manager yet) and the m_psName[i]->SetAlign(TXT_LEFT)
    // loop is dropped (cStatic is single-line by default). The host injects the
    // child-window pointers before calling Linking().
    (void)m_w;
}

void cPartyWarDialog::ShowPartyWarDlg(bool bMaster) {
    cDialog::SetActive(true);
    if (m_w.btnLock)    m_w.btnLock->SetActive(bMaster);
    if (m_w.btnUnLock)  m_w.btnUnLock->SetActive(bMaster);
    if (m_w.btnStart)   m_w.btnStart->SetActive(false);
    if (m_w.btnCancel)  m_w.btnCancel->SetActive(bMaster);

    for (std::size_t i = 0; i < kMemberCount; ++i) {
        if (m_w.memberCheck[i]) m_w.memberCheck[i]->SetActive(false);
        if (m_w.memberStatic[i]) m_w.memberStatic[i]->SetActive(false);
    }
}

void cPartyWarDialog::HidePartyWarDlg() {
    cDialog::SetActive(false);
    m_bPartyLock = m_bEnemyLock = false;
}

bool cPartyWarDialog::NoChangeCheckBoxState(int nIndex) {
    if (nIndex < 0 || static_cast<std::size_t>(nIndex) >= kMemberCount ||
        m_w.memberCheck[nIndex] == nullptr) {
        return false;
    }
    bool wasChecked = m_w.memberCheck[nIndex]->IsChecked();
    m_w.memberCheck[nIndex]->SetChecked(!wasChecked);
    return wasChecked;
}

void cPartyWarDialog::SetPartyMemberName(const char* pName, int nIndex, bool bLogged) {
    if (nIndex < 0 || static_cast<std::size_t>(nIndex) >= kMemberCount ||
        !pName || m_w.memberCheck[nIndex] == nullptr ||
        m_w.memberStatic[nIndex] == nullptr) {
        return;
    }

    m_w.memberCheck[nIndex]->SetActive(true);
    m_w.memberCheck[nIndex]->SetChecked(false);
    m_w.memberStatic[nIndex]->SetActive(true);
    m_w.memberStatic[nIndex]->SetStaticText(pName);
    // 1:1 with legacy: if !bLogged, set FG color to (172,182199,255)
    // (RGBA_MAKE(172,182,199,255)). Modern port drops SetFGColor
    // (no global rgb constant yet) - the color is ignored in unit tests.
    (void)bLogged;

    if (nIndex == 0 || nIndex == kTeamSize) {
        if (nIndex == 0 && m_w.team1) {
            m_w.team1->SetScriptText(pName);
        }
        if (nIndex == kTeamSize && m_w.team2) {
            m_w.team2->SetScriptText(pName);
        }
        recomposeTitleFromTeams();
    }
}

void cPartyWarDialog::AddPartyWarMember(int nIndex) {
    if (nIndex < 0 || static_cast<std::size_t>(nIndex) >= kMemberCount ||
        !m_w.memberCheck[nIndex]) {
        return;
    }
    m_w.memberCheck[nIndex]->SetChecked(true);
}

void cPartyWarDialog::RemovePartyWarMember(int nIndex) {
    if (nIndex < 0 || static_cast<std::size_t>(nIndex) >= kMemberCount ||
        !m_w.memberCheck[nIndex]) {
        return;
    }
    m_w.memberCheck[nIndex]->SetChecked(false);
}

void cPartyWarDialog::SetLock(bool bParty, bool bMaster) {
    if (m_w.btnStart)   m_w.btnStart->SetActive(false);
    if (m_w.btnCancel)  m_w.btnCancel->SetActive(false);

    if (bParty)    m_bPartyLock = true;
    else           m_bEnemyLock = true;

    if (bMaster) {
        if (m_bPartyLock && m_bEnemyLock) {
            if (m_w.btnStart) m_w.btnStart->SetActive(true);
        } else {
            if (m_w.btnCancel) m_w.btnCancel->SetActive(true);
        }
    }
}

void cPartyWarDialog::SetUnLock(bool bMaster) {
    if (m_w.btnStart)   m_w.btnStart->SetActive(false);
    if (m_w.btnCancel)  m_w.btnCancel->SetActive(false);

    m_bPartyLock = m_bEnemyLock = false;

    if (bMaster) {
        if (m_w.btnCancel) m_w.btnCancel->SetActive(true);
    }
}

void cPartyWarDialog::SetTime(std::uint32_t dwTime) {
    char buf[64] = {0};
    const std::uint32_t min = dwTime / 60;
    const std::uint32_t sec = dwTime % 60;
    // 1:1 with legacy: sprintf( temp, CHATMGR->GetChatMsg(868), min, sec )
    // Modern port drops the CHATMGR dependency (no port yet) - uses
    // a standard snprintf with %d:%02d that matches the legacy format.
    std::snprintf(buf, sizeof(buf), "%d:%02d", static_cast<std::uint32_t>(min), static_cast<std::uint32_t>(sec));
    if (m_w.time)    m_w.time->SetScriptText(buf);
}

void cPartyWarDialog::recomposeTitleFromTeams() {
    if (!m_w.title || !m_w.team1 || !m_w.team2) {
        return;
    }
    const std::string& t1 = m_w.team1->GetScriptText();
    const std::string& t2 = m_w.team2->GetScriptText();
    char buf[128] = {0};
    // 1:1 with legacy: sprintf(buf, CHATMGR->GetChatMsg(867), t1.c_str(), t2.c_str())
    // Modern port drops the CHATMGR dependency (no port yet) - format directly
    std::snprintf(buf, sizeof(buf), "%s VS %s", t1.c_str(), t2.c_str());
    m_w.title->SetScriptText(buf);
}

bool cPartyWarDialog::IsStartButtonActive() const noexcept {
    return m_w.btnStart != nullptr && m_w.btnStart->isActive();
}

bool cPartyWarDialog::IsCancelButtonActive() const noexcept {
    return m_w.btnCancel != nullptr && m_w.btnCancel->isActive();
}

} // namespace mxh::ui
