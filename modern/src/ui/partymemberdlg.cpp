#include "partymemberdlg.hpp"

#include "mxh/ui/cStatic.hpp"
#include "mxh/ui/cobjectguagen.hpp"
#include "mxh/ui/cpushupbutton.hpp"
#include "mxh/ui/partybtndlg.hpp"

#include <cstdio>

namespace mxh::ui {

cPartyMemberDlg::cPartyMemberDlg() = default;
cPartyMemberDlg::~cPartyMemberDlg() = default;

void cPartyMemberDlg::SetActive(bool val) noexcept {
    if (!isEnabled()) {
        return;
    }
    if (!m_bMember) {
        val = false;
    }
    m_bRealActive = val;
    if (m_MemberID == 0) {
        val = false;
    }
    cDialog::SetActive(val);
}

void cPartyMemberDlg::Linking(std::int32_t index) {
    m_pName = dynamic_cast<cPushupButton*>(
        findWindowById(kMemberNameBaseId + index));
    m_pLife = dynamic_cast<cObjectGuagen*>(
        findWindowById(kMemberLifeBaseId + index));
    m_pNaeryuk = dynamic_cast<cObjectGuagen*>(
        findWindowById(kMemberNaeryukBaseId + index));
    m_pLevel = dynamic_cast<cStatic*>(
        findWindowById(kMemberLevelBaseId + index));
    m_nIndex = index;
}

void cPartyMemberDlg::SetMemberData(const PartyMemberData* info) {
    if (!info) {
        m_MemberID = 0;
        SetActive(m_bRealActive);
        return;
    }

    m_MemberID = info->memberId;
    if (info->logged) {
        if (m_pName) {
            m_pName->SetText(info->name, kLoginBasicColor,
                             kLoginOverColor, kLoginPressColor);
        }
        if (m_pLife) {
            m_pLife->SetValue(static_cast<float>(info->lifePercent) * 0.01f, 0);
        }
        if (m_pNaeryuk) {
            m_pNaeryuk->SetValue(
                static_cast<float>(info->naeryukPercent) * 0.01f, 0);
        }
        if (m_pLevel) {
            char levelText[10]{};
            std::snprintf(levelText, sizeof(levelText), "Lv.%u",
                          static_cast<unsigned>(info->level));
            m_pLevel->SetStaticText(levelText);
        }
    } else {
        if (m_pName) {
            m_pName->SetText(info->name, kLogoutBasicColor,
                             kLogoutOverColor, kLogoutPressColor);
        }
        if (m_pLife) {
            m_pLife->SetValue(0.0f, 0);
        }
        if (m_pNaeryuk) {
            m_pNaeryuk->SetValue(0.0f, 0);
        }
        if (m_pLevel) {
            m_pLevel->SetStaticText("");
        }
    }

    SetActive(m_bRealActive);
}

void cPartyMemberDlg::SetNameBtnPushUp(bool val) {
    if (m_pName) {
        m_pName->SetPush(val);
    }
}

std::uint32_t cPartyMemberDlg::ActionEvent(std::int32_t mouseX,
                                            std::int32_t mouseY,
                                            std::uint32_t mouseFlags) {
    if (!isActive()) {
        return static_cast<std::uint32_t>(cWindow::WindowEvent::Null);
    }

    const std::uint32_t event = m_hasActionEventResultForTest
        ? m_actionEventResultForTest
        : cDialog::ActionEvent(mouseX, mouseY, mouseFlags);
    if (event == static_cast<std::uint32_t>(cWindow::WindowEvent::LButtonClick)
        && m_clickedMemberCallback) {
        m_clickedMemberCallback(m_MemberID, m_clickedMemberUserData);
    }
    return event;
}

void cPartyMemberDlg::Render() {
    if (m_pPartyBtnDlg && m_nIndex != -1) {
        const std::int32_t x = m_pPartyBtnDlg->absX();
        const std::int32_t y = m_pPartyBtnDlg->absY();
        const std::int32_t slotOffset = 48 * m_nIndex;
        const std::int32_t optionOffset = m_bOption ? 80 : 0;

        SetAbsXY(x, y + 54 + optionOffset + slotOffset);
        if (m_pName) {
            m_pName->SetAbsXY(x + 9, y + 68 + optionOffset + slotOffset);
        }
        if (m_pLife) {
            m_pLife->SetAbsXY(x + 6, y + 88 + optionOffset + slotOffset);
        }
        if (m_pNaeryuk) {
            m_pNaeryuk->SetAbsXY(x + 6, y + 94 + optionOffset + slotOffset);
        }
        if (m_pLevel) {
            m_pLevel->SetAbsXY(x + 9, y + 55 + optionOffset + slotOffset);
        }
    }

    cDialog::Render();
}

void cPartyMemberDlg::SetClickedMemberCallback(
    ClickedMemberCallback callback, void* userData) noexcept {
    m_clickedMemberCallback = callback;
    m_clickedMemberUserData = userData;
}

void cPartyMemberDlg::SetActionEventResultForTest(std::uint32_t result) noexcept {
    m_hasActionEventResultForTest = true;
    m_actionEventResultForTest = result;
}

void cPartyMemberDlg::ClearActionEventResultForTest() noexcept {
    m_hasActionEventResultForTest = false;
    m_actionEventResultForTest = 0;
}

void cPartyMemberDlg::SetControlsForTest(cPushupButton* name,
                                                cObjectGuagen* life,
                                                cObjectGuagen* naeryuk,
                                                cStatic* level) noexcept {
    m_pName = name;
    m_pLife = life;
    m_pNaeryuk = naeryuk;
    m_pLevel = level;
}

} // namespace mxh::ui
