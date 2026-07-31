#include "monsterguagedlg.hpp"

#include "mxh/ui/cStatic.hpp"
#include "mxh/ui/cobjectguagen.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace mxh::ui {

cMonsterGuageDlg::cMonsterGuageDlg() = default;

cMonsterGuageDlg::~cMonsterGuageDlg() {
    m_pCurMonster = nullptr;
    for (auto& controls : m_modeControls) {
        controls.clear();
    }
}

void cMonsterGuageDlg::SetChatMsgCallback(ChatMsgCallback callback, void* user) noexcept {
    m_chatMsgCallback = callback;
    m_chatMsgUser = user;
}

void cMonsterGuageDlg::SetControlsForTest(
    cStatic* name, cStatic* lifeText, cObjectGuagen* lifeGauge,
    cStatic* shieldText, cObjectGuagen* shieldGauge, cStatic* guildName,
    cStatic* guildUnionName, cStatic* npcName, cStatic* lifeBase) noexcept {
    m_pName = name;
    m_pLifeText = lifeText;
    m_pLifeGuage = lifeGauge;
    m_pShieldText = shieldText;
    m_pShieldGuage = shieldGauge;
    m_pGuildName = guildName;
    m_pGuildUnionName = guildUnionName;
    m_pNpcName = npcName;

    for (auto& controls : m_modeControls) {
        controls.clear();
    }
    m_modeControls[static_cast<std::size_t>(MonsterGuageMode::Monster)] = {
        m_pLifeGuage, m_pShieldGuage, m_pLifeText, lifeBase};
    m_modeControls[static_cast<std::size_t>(MonsterGuageMode::Character)] = {
        m_pGuildName, m_pGuildUnionName};
    m_modeControls[static_cast<std::size_t>(MonsterGuageMode::Npc)] = {
        m_pNpcName};
}

void cMonsterGuageDlg::Linking() {
    m_pName = dynamic_cast<cStatic*>(findWindowById(kNameId));
    m_pLifeText = dynamic_cast<cStatic*>(findWindowById(kLifeTextId));
    m_pLifeGuage = dynamic_cast<cObjectGuagen*>(findWindowById(kLifeGaugeId));
    m_pShieldText = nullptr;
    m_pShieldGuage = dynamic_cast<cObjectGuagen*>(findWindowById(kShieldGaugeId));
    m_pGuildName = dynamic_cast<cStatic*>(findWindowById(kGuildNameId));
    m_pGuildUnionName = dynamic_cast<cStatic*>(findWindowById(kGuildUnionNameId));
    m_pNpcName = dynamic_cast<cStatic*>(findWindowById(kNpcNameId));
    auto* lifeBase = dynamic_cast<cStatic*>(findWindowById(kLifeBaseId));

    for (auto& controls : m_modeControls) {
        controls.clear();
    }
    m_modeControls[static_cast<std::size_t>(MonsterGuageMode::Monster)] = {
        m_pLifeGuage, m_pShieldGuage, m_pLifeText, lifeBase};
    m_modeControls[static_cast<std::size_t>(MonsterGuageMode::Character)] = {
        m_pGuildName, m_pGuildUnionName};
    m_modeControls[static_cast<std::size_t>(MonsterGuageMode::Npc)] = {
        m_pNpcName};
}

void cMonsterGuageDlg::SetMonsterName(const char* name) {
    const char* safeName = name != nullptr ? name : "";
    if (m_pName != nullptr) {
        m_pName->SetStaticText(safeName);
    }
    m_pCurMonster = nullptr;
    if (m_pLifeGuage != nullptr) m_pLifeGuage->SetValue(0.0f, 0);
    if (m_pShieldGuage != nullptr) m_pShieldGuage->SetValue(0.0f, 0);
}

void cMonsterGuageDlg::SetNpcName(const char* name) {
    if (m_pNpcName != nullptr) {
        m_pNpcName->SetStaticText(name != nullptr ? name : "");
    }
}

void cMonsterGuageDlg::SetMonsterNameColor(std::uint32_t color) {
    if (m_pName != nullptr) {
        m_pName->SetFGColor(color);
    }
}

void cMonsterGuageDlg::SetMonsterLife(std::uint32_t current,
                                           std::uint32_t maximum,
                                           std::int32_t type) {
    if (maximum == 0) maximum = 1;
    current = std::min(current, maximum);

    if (m_cheatEnabled && m_pLifeText != nullptr) {
        char text[64] = {};
        std::snprintf(text, sizeof(text), "%u / %u", current, maximum);
        m_pLifeText->SetStaticText(text);
    }
    if (m_pLifeGuage != nullptr) {
        const float rate = static_cast<float>(current) /
            static_cast<float>(maximum);
        const std::uint32_t effect = type == 0
            ? 0u
            : (1500u / maximum) * current;
        m_pLifeGuage->SetValue(rate, effect);
    }
}

void cMonsterGuageDlg::SetMonsterShield(std::uint32_t current,
                                             std::uint32_t maximum,
                                             std::int32_t type) {
    if (maximum == 0) maximum = 1;
    current = std::min(current, maximum);

    if (m_cheatEnabled && m_pShieldText != nullptr) {
        char text[64] = {};
        std::snprintf(text, sizeof(text), "%u / %u", current, maximum);
        m_pShieldText->SetStaticText(text);
    }
    if (m_pShieldGuage != nullptr) {
        const float rate = static_cast<float>(current) /
            static_cast<float>(maximum);
        const std::uint32_t effect = type == 0
            ? 0u
            : (1500u / maximum) * current;
        m_pShieldGuage->SetValue(rate, effect);
    }
}

void cMonsterGuageDlg::SetMonsterLife(const MonsterGaugeInfo& info,
                                           std::int32_t type) {
    SetMonsterLife(info.life, info.maxLife, type);
}

void cMonsterGuageDlg::SetMonsterShield(const MonsterGaugeInfo& info,
                                             std::int32_t type) {
    SetMonsterShield(info.shield, info.maxShield, type);
}

void cMonsterGuageDlg::FormatName(std::string& output, int messageId,
                                   const char* fallback,
                                   const char* value) const {
    const char* safeValue = value != nullptr ? value : "";
    if (*safeValue == 0) {
        output.clear();
        return;
    }
    const char* format = fallback;
    if (m_chatMsgCallback != nullptr) {
        const char* localized = m_chatMsgCallback(messageId, m_chatMsgUser);
        if (localized != nullptr && *localized != 0) format = localized;
    }
    char text[128] = {};
    std::snprintf(text, sizeof(text), format, safeValue);
    output = text;
}

void cMonsterGuageDlg::SetGuildUnionName(const char* guildName,
                                             const char* unionName) {
    std::string formatted;
    if (m_pGuildName != nullptr) {
        FormatName(formatted, kGuildNameChatMsg, "%s", guildName);
        m_pGuildName->SetStaticText(formatted);
    }
    if (m_pGuildUnionName != nullptr) {
        FormatName(formatted, kGuildUnionNameChatMsg, "%s", unionName);
        m_pGuildUnionName->SetStaticText(formatted);
    }
}

bool cMonsterGuageDlg::IsValidMode(std::int32_t mode) const noexcept {
    return mode >= 0 && mode < static_cast<std::int32_t>(MonsterGuageMode::Max);
}

void cMonsterGuageDlg::SetActiveMonGuageMode(std::int32_t mode, bool active) {
    if (!IsValidMode(mode)) return;
    for (cWindow* control : m_modeControls[static_cast<std::size_t>(mode)]) {
        if (control != nullptr) control->SetActive(active);
    }
}

void cMonsterGuageDlg::ShowMonsterGuageMode(std::int32_t mode) {
    if (!IsValidMode(mode)) return;
    if (m_CurMode == mode) return;
    if (m_CurMode != -1) SetActiveMonGuageMode(m_CurMode, false);
    m_CurMode = mode;
    if (mode == static_cast<std::int32_t>(MonsterGuageMode::Pet)) return;
    SetActiveMonGuageMode(mode, true);
}

std::size_t cMonsterGuageDlg::modeControlCount(std::int32_t mode) const noexcept {
    if (!IsValidMode(mode)) return 0;
    return m_modeControls[static_cast<std::size_t>(mode)].size();
}

void cMonsterGuageDlg::Render() {
    if (m_CurMode == static_cast<std::int32_t>(MonsterGuageMode::Character)) {
        if (m_pGuildName != nullptr && m_pGuildUnionName != nullptr) {
            const std::int32_t y = m_pGuildUnionName->GetStaticText().empty() ? 26 : 18;
            m_pGuildName->SetRelXY(0, y);
            SetAbsXY(absX(), absY());
        }
    }
    cDialog::Render();
}

}  // namespace mxh::ui
