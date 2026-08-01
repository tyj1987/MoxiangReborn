#include "guildrankdialog.hpp"

#include "mxh/ui/cButton.hpp"
#include "mxh/ui/cComboBox.hpp"
#include "mxh/ui/cTextArea.hpp"

#include <cstdio>

namespace mxh::ui {

cGuildRankDialog::cGuildRankDialog() = default;
cGuildRankDialog::~cGuildRankDialog() = default;

void cGuildRankDialog::Linking() {
    m_pRankMemberName = dynamic_cast<cTextArea*>(findWindowById(kMemberNameId));
    m_pDRankComboBox = dynamic_cast<cComboBox*>(findWindowById(kDanRankComboId));
    m_pRankComboBox = dynamic_cast<cComboBox*>(findWindowById(kGuildRankComboId));
    m_pDOkBtn = dynamic_cast<cButton*>(findWindowById(kDanOkButtonId));
    m_pOkBtn = dynamic_cast<cButton*>(findWindowById(kGuildOkButtonId));

    m_GuildRankCtrlList[static_cast<std::size_t>(GuildRankMode::Dan)] = {
        m_pDRankComboBox, m_pDOkBtn
    };
    m_GuildRankCtrlList[static_cast<std::size_t>(GuildRankMode::Guild)] = {
        m_pRankComboBox, m_pOkBtn
    };
}

void cGuildRankDialog::SetActive(bool val) noexcept {
    if (val) {
        if (m_selection.selectedMemberId == 0
            || m_selection.selectedMemberId == m_selection.heroId) {
            if (isActive()) {
                cDialog::SetActive(false);
            }
            if (m_systemMessageCallback) {
                const char* message = m_chatTextCallback
                    ? m_chatTextCallback(kInvalidSelectionMessageId, m_callbackUserData)
                    : "";
                m_systemMessageCallback(message ? message : "", m_callbackUserData);
            }
            return;
        }
        SetName(m_selection.selectedMemberName.c_str());
    }
    cDialog::SetActive(val);
}

void cGuildRankDialog::ShowGuildRankMode(std::uint8_t guildLevel) {
    const auto showMode = guildLevel == kMaxGuildLevel
        ? GuildRankMode::Guild
        : GuildRankMode::Dan;
    const auto modeValue = static_cast<std::uint8_t>(showMode);
    if (m_CurGuildRankMode == modeValue) {
        return;
    }

    if (m_CurGuildRankMode != kUnsetMode) {
        SetActiveGuildRankMode(m_CurGuildRankMode, false);
    }
    SetActiveGuildRankMode(modeValue, true);
    m_CurGuildRankMode = modeValue;
}

void cGuildRankDialog::SetActiveGuildRankMode(std::int32_t showMode,
                                               bool active) noexcept {
    if (showMode < 0 || static_cast<std::size_t>(showMode) >= kModeCount) {
        return;
    }
    for (cWindow* control : m_GuildRankCtrlList[static_cast<std::size_t>(showMode)]) {
        if (control) {
            control->SetActive(active);
        }
    }
}

void cGuildRankDialog::SetName(const char* name) {
    if (!m_pRankMemberName) {
        return;
    }
    const char* format = m_chatTextCallback
        ? m_chatTextCallback(kMemberNameFormatMessageId, m_callbackUserData)
        : "%s";
    char buffer[64]{};
    std::snprintf(buffer, sizeof(buffer), format ? format : "%s",
                  name ? name : "");
    m_pRankMemberName->SetScriptText(buffer);
}

void cGuildRankDialog::SetChatCallbacks(ChatTextCallback chatText,
                                         SystemMessageCallback systemMessage,
                                         void* userData) noexcept {
    m_chatTextCallback = chatText;
    m_systemMessageCallback = systemMessage;
    m_callbackUserData = userData;
}

void cGuildRankDialog::SetControlsForTest(cTextArea* memberName,
                                               cComboBox* guildRank,
                                               cComboBox* danRank,
                                               cButton* guildOk,
                                               cButton* danOk) noexcept {
    m_pRankMemberName = memberName;
    m_pRankComboBox = guildRank;
    m_pDRankComboBox = danRank;
    m_pOkBtn = guildOk;
    m_pDOkBtn = danOk;
    m_GuildRankCtrlList[static_cast<std::size_t>(GuildRankMode::Dan)] = {
        m_pDRankComboBox, m_pDOkBtn
    };
    m_GuildRankCtrlList[static_cast<std::size_t>(GuildRankMode::Guild)] = {
        m_pRankComboBox, m_pOkBtn
    };
}

} // namespace mxh::ui
