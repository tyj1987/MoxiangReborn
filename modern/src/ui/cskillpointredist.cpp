// cskillpointredist.cpp -- modern implementation
//   of Moxiang CSkillPointRedist (skill-point
//   redistribution dialog).

#include "cskillpointredist.hpp"

namespace mxh::ui {

cSkillPointRedist::cSkillPointRedist() = default;

cSkillPointRedist::~cSkillPointRedist() = default;

void cSkillPointRedist::SetActive(bool val) noexcept {
    // 1:1 with legacy SetActive: forwards to
    // cDialog::SetActive(val).
    cDialog::SetActive(val);

    // 1:1 with legacy on-deactivate hook:
    // SetDisable(FALSE), SuryunDialog->SetDisable,
    // OBJECTSTATEMGR->EndObjectState(HERO,
    // eObjectState_Deal).  All R-12.x deferred --
    // the modern port forwards to the host-
    // injected deactivate callback.
    if (!val && m_deactivateCb) {
        m_deactivateCb(m_deactivateUser);
    }

    // 1:1 with legacy on-activate hook:
    // RefreshAbilityIcons().  R-12.x deferred --
    // the modern port forwards to the host-
    // injected refresh-icons callback.
    if (val && m_refreshIconsCb) {
        m_refreshIconsCb(m_refreshIconsUser);
    }
}

void cSkillPointRedist::Linking() {
    // 1:1 with legacy Linking order: 3 cButton,
    // 3 cStatic, then the 3-tab loop for
    // cPushupButton + cIconGridDialog.
    auto resolve = [this](std::int32_t id) -> void* {
        return m_windowResolverCb
            ? m_windowResolverCb(id, m_windowResolverUser)
            : nullptr;
    };
    m_upBtn    = static_cast<cButton*>(resolve(kIdUpBtn));
    m_downBtn  = static_cast<cButton*>(resolve(kIdDownBtn));
    m_okBtn    = static_cast<cButton*>(resolve(kIdOkBtn));
    m_rePoint  = static_cast<cStatic*>(resolve(kIdRePoint));
    m_usePoint = static_cast<cStatic*>(resolve(kIdUsePoint));
    m_ogPoint  = static_cast<cStatic*>(resolve(kIdOgPoint));
    for (std::size_t i = 0; i < kTabCount; ++i) {
        m_gridButton[i] = static_cast<cPushupButton*>(
            resolve(static_cast<std::int32_t>(kIdTabBtn0) +
                    static_cast<std::int32_t>(i)));
        m_iconGrid[i] = static_cast<cIconGridDialog*>(
            resolve(static_cast<std::int32_t>(kIdIconGrid0) +
                    static_cast<std::int32_t>(i)));
    }
}

void cSkillPointRedist::RefreshAbilityIcons() {
    if (m_refreshIconsCb) m_refreshIconsCb(m_refreshIconsUser);
}

void* cSkillPointRedist::MakeNewAbilityIcon(void* pInfo) {
    if (m_makeIconCb) return m_makeIconCb(pInfo, m_makeIconUser);
    return nullptr;
}

void cSkillPointRedist::SetAbilityToolTip(void* pIcon) {
    if (m_setToolTipCb) m_setToolTipCb(pIcon, m_setToolTipUser);
}

void cSkillPointRedist::SetAbilitySyn(bool bDown) {
    if (m_setAbilitySynCb) m_setAbilitySynCb(bDown, m_setAbilitySynUser);
}

void cSkillPointRedist::SetAbilityExp(std::uint32_t exp) noexcept {
    // 1:1 with legacy SetAbilityExp(DWORD Exp):
    // caches the current ability exp.  The legacy
    // body is the assignment itself.
    m_abilityExp = exp;
}

void cSkillPointRedist::RefreshAbilityPoint() {
    if (m_refreshPointCb) m_refreshPointCb(m_refreshPointUser);
}

const char* cSkillPointRedist::GetCurAbilityName() {
    if (m_curAbilityNameCb) return m_curAbilityNameCb(m_curAbilityNameUser);
    return nullptr;
}

int cSkillPointRedist::GetCurAbilityLevel() {
    if (m_curAbilityLevelCb) return m_curAbilityLevelCb(m_curAbilityLevelUser);
    return 0;
}

std::uint32_t cSkillPointRedist::GetCurItemIdx() const noexcept {
    return m_itemIdx;
}

std::uint32_t cSkillPointRedist::GetCurItemPos() const noexcept {
    return m_itemPos;
}

void cSkillPointRedist::SetCurItem(std::uint32_t idx, std::uint32_t pos) noexcept {
    m_itemIdx = idx;
    m_itemPos = pos;
}

void cSkillPointRedist::SetTabNumber(std::uint32_t dwTab) noexcept {
    if (dwTab < kTabCount) {
        m_curTabNum = dwTab;
    }
}

std::uint32_t cSkillPointRedist::GetTabNumber() const noexcept {
    return m_curTabNum;
}

void* cSkillPointRedist::GetCurAbilityInfo() {
    if (m_curAbilityInfoCb) return m_curAbilityInfoCb(m_curAbilityInfoUser);
    return nullptr;
}

void cSkillPointRedist::SetUpBtnForTest(cButton* b) noexcept { m_upBtn = b; }
void cSkillPointRedist::SetDownBtnForTest(cButton* b) noexcept { m_downBtn = b; }
void cSkillPointRedist::SetOkBtnForTest(cButton* b) noexcept { m_okBtn = b; }
cButton* cSkillPointRedist::GetUpBtnForTest() const noexcept { return m_upBtn; }
cButton* cSkillPointRedist::GetDownBtnForTest() const noexcept { return m_downBtn; }
cButton* cSkillPointRedist::GetOkBtnForTest() const noexcept { return m_okBtn; }

void cSkillPointRedist::SetRePointForTest(cStatic* s) noexcept { m_rePoint = s; }
void cSkillPointRedist::SetUsePointForTest(cStatic* s) noexcept { m_usePoint = s; }
void cSkillPointRedist::SetOgPointForTest(cStatic* s) noexcept { m_ogPoint = s; }
cStatic* cSkillPointRedist::GetRePointForTest() const noexcept { return m_rePoint; }
cStatic* cSkillPointRedist::GetUsePointForTest() const noexcept { return m_usePoint; }
cStatic* cSkillPointRedist::GetOgPointForTest() const noexcept { return m_ogPoint; }

void cSkillPointRedist::SetGridButtonForTest(std::size_t tab, cPushupButton* b) noexcept {
    if (tab < kTabCount) m_gridButton[tab] = b;
}
void cSkillPointRedist::SetIconGridForTest(std::size_t tab, cIconGridDialog* g) noexcept {
    if (tab < kTabCount) m_iconGrid[tab] = g;
}
cPushupButton* cSkillPointRedist::GetGridButtonForTest(std::size_t tab) const noexcept {
    if (tab < kTabCount) return m_gridButton[tab];
    return nullptr;
}
cIconGridDialog* cSkillPointRedist::GetIconGridForTest(std::size_t tab) const noexcept {
    if (tab < kTabCount) return m_iconGrid[tab];
    return nullptr;
}

void cSkillPointRedist::SetWindowResolverForTest(WindowResolver cb, void* user) noexcept {
    m_windowResolverCb = cb; m_windowResolverUser = user;
}
void cSkillPointRedist::SetRefreshIconsCallbackForTest(RefreshIconsCallback cb, void* user) noexcept {
    m_refreshIconsCb = cb; m_refreshIconsUser = user;
}
void cSkillPointRedist::SetMakeIconCallbackForTest(MakeIconCallback cb, void* user) noexcept {
    m_makeIconCb = cb; m_makeIconUser = user;
}
void cSkillPointRedist::SetToolTipCallbackForTest(SetToolTipCallback cb, void* user) noexcept {
    m_setToolTipCb = cb; m_setToolTipUser = user;
}
void cSkillPointRedist::SetAbilitySynCallbackForTest(SetAbilitySynCallback cb, void* user) noexcept {
    m_setAbilitySynCb = cb; m_setAbilitySynUser = user;
}
void cSkillPointRedist::SetRefreshPointCallbackForTest(RefreshPointCallback cb, void* user) noexcept {
    m_refreshPointCb = cb; m_refreshPointUser = user;
}
void cSkillPointRedist::SetCurAbilityNameCallbackForTest(CurAbilityNameCallback cb, void* user) noexcept {
    m_curAbilityNameCb = cb; m_curAbilityNameUser = user;
}
void cSkillPointRedist::SetCurAbilityLevelCallbackForTest(CurAbilityLevelCallback cb, void* user) noexcept {
    m_curAbilityLevelCb = cb; m_curAbilityLevelUser = user;
}
void cSkillPointRedist::SetCurAbilityInfoCallbackForTest(CurAbilityInfoCallback cb, void* user) noexcept {
    m_curAbilityInfoCb = cb; m_curAbilityInfoUser = user;
}
void cSkillPointRedist::SetDeactivateCallbackForTest(DeactivateCallback cb, void* user) noexcept {
    m_deactivateCb = cb; m_deactivateUser = user;
}

}  // namespace mxh::ui
