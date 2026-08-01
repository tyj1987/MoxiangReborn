#include "guildplustimedialog.hpp"

#include "mxh/ui/cListDialog.hpp"
#include "mxh/ui/cStatic.hpp"

#include <cstdio>
#include <cstring>

namespace mxh::ui {

namespace {
constexpr std::size_t kBufferSize = 256;
}

cGuildPlusTimeDialog::cGuildPlusTimeDialog() = default;
cGuildPlusTimeDialog::~cGuildPlusTimeDialog() = default;

void cGuildPlusTimeDialog::SetCallbacks(ChatTextFn chatText,
                                        UseGuildPointFn useGuildPoint,
                                        PlustimeCountFn getCount,
                                        PlustimeEntryFn getEntry,
                                        void* userData) noexcept {
    m_chatTextFn = chatText;
    m_useGuildPointFn = useGuildPoint;
    m_getCountFn = getCount;
    m_getEntryFn = getEntry;
    m_callbackUserData = userData;
}

void cGuildPlusTimeDialog::SetPlustimeEntries(const GuildPlusTimeEntry* entries,
                                              std::size_t count) noexcept {
    m_testEntries = entries;
    m_testCount = count;
}

void cGuildPlusTimeDialog::SetControlsForTest(cStatic* point,
                                              cListDialog* list) noexcept {
    m_pCurrentHavePoint = point;
    m_pPlusItemList = list;
}

const char* cGuildPlusTimeDialog::ResolveChatFormat(std::int32_t messageId) const {
    return m_chatTextFn ? m_chatTextFn(messageId, m_callbackUserData) : nullptr;
}

void cGuildPlusTimeDialog::FormatThousands(char* buffer, std::int32_t value) const {
    if (!buffer) {
        return;
    }
    char temp[32];
    std::snprintf(temp, sizeof(temp), "%d", value);
    std::size_t len = std::strlen(temp);
    std::size_t out = 0;
    for (std::size_t i = 0; i < len; ++i) {
        if (i > 0 && (len - i) % 3 == 0) {
            buffer[out++] = ',';
        }
        buffer[out++] = temp[i];
    }
    buffer[out] = '\0';
}

void cGuildPlusTimeDialog::SetGuildPointText(std::uint32_t guildPoint) {
    if (!m_pCurrentHavePoint) {
        return;
    }
    char buf[64]{};
    FormatThousands(buf, static_cast<std::int32_t>(guildPoint));
    m_pCurrentHavePoint->SetStaticText(buf);
}

void cGuildPlusTimeDialog::Linking() {
    m_pCurrentHavePoint = dynamic_cast<cStatic*>(findWindowById(kPointStaticId));
    m_pPlusItemList = dynamic_cast<cListDialog*>(findWindowById(kPlustimeListId));
    m_CurrentSelectedItem = kNoSelection;

    if (m_pPlusItemList) {
        m_pPlusItemList->SetShowSelect(true);
    }
    LoadPlustimeList();
    SetGuildPointText(kDefaultInitialPoints);
}

void cGuildPlusTimeDialog::SetActive(bool val) noexcept {
    cDialog::SetActive(val);
}

void cGuildPlusTimeDialog::HandleMouseAction(std::int32_t mouseX,
                                             std::int32_t mouseY,
                                             std::uint32_t weFromDialog) {
    if (!m_pPlusItemList) {
        return;
    }
    if (m_pPlusItemList->PtIdxInRow(mouseX, mouseY) == -1) {
        return;
    }
    if ((weFromDialog & kActionBtnClick) == 0) {
        return;
    }
    const int idx = m_pPlusItemList->GetCurSelectedRowIdx();
    if (idx != kNoSelection) {
        m_CurrentSelectedItem = idx;
    }
}

void cGuildPlusTimeDialog::DispatchStartButton() {
    if (m_CurrentSelectedItem == kNoSelection) {
        return;
    }
    if (m_useGuildPointFn) {
        m_useGuildPointFn(kGuildPlusTimeForKind,
                          m_CurrentSelectedItem + 1,
                          m_callbackUserData);
    }
}

void cGuildPlusTimeDialog::OnActionEvent(std::int32_t lId, void* /*p*/,
                                          std::uint32_t we) {
    if ((we & kActionBtnClick) == 0) {
        return;
    }
    switch (lId) {
    case kPlustimeStartButtonId:
        DispatchStartButton();
        break;
    case kCloseButtonId:
        SetActive(false);
        break;
    default:
        break;
    }
}

void cGuildPlusTimeDialog::LoadPlustimeList() {
    if (!m_pPlusItemList) {
        return;
    }
    std::size_t count = 0;
    if (m_testEntries && m_testCount > 0) {
        count = m_testCount;
    } else if (m_getCountFn) {
        count = m_getCountFn(m_callbackUserData);
    }

    for (std::size_t i = 0; i < count; ++i) {
        GuildPlusTimeEntry entry{};
        if (m_testEntries) {
            entry = m_testEntries[i];
        } else if (m_getEntryFn) {
            entry.kind = m_getEntryFn(i, &entry.addData, &entry.needPoint,
                                      m_callbackUserData);
        }
        const char* format = nullptr;
        switch (entry.kind) {
        case GuildPlusTimeKind::Suryun:
            format = ResolveChatFormat(kSuryunMessageId);
            break;
        case GuildPlusTimeKind::Mugong:
            format = ResolveChatFormat(kMugongMessageId);
            break;
        case GuildPlusTimeKind::Exp:
            format = ResolveChatFormat(kExpMessageId);
            break;
        case GuildPlusTimeKind::DamageUp:
            format = ResolveChatFormat(kDamageUpMessageId);
            break;
        case GuildPlusTimeKind::Unknown:
        default:
            format = nullptr;
            break;
        }

        char buf[kBufferSize]{};
        if (format) {
            std::snprintf(buf, sizeof(buf), format, entry.addData,
                          static_cast<unsigned>(entry.needPoint));
        }
        m_pPlusItemList->AddItem(buf, kDefaultTextColor);
    }
}

} // namespace mxh::ui
