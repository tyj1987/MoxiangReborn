// cmpguagedialog.cpp -- modern implementation of
//   Moxiang CMPGuageDialog.

#include "cmpguagedialog.hpp"

#include "cstatic.hpp"

#include <cstdio>

namespace mxh::ui {

cMPGuageDialog::cMPGuageDialog() = default;

cMPGuageDialog::~cMPGuageDialog() = default;

void cMPGuageDialog::Linking() {
    if (!m_ExpGuage) {
        m_ExpGuage = findWindowById(kIdExpGuage);
    }
    if (!m_Time) {
        m_Time = static_cast<cStatic*>(findWindowById(kIdTime));
    }
    if (!m_ExpPercent) {
        m_ExpPercent = static_cast<cStatic*>(findWindowById(kIdExpPercent));
    }
    if (!m_pTitle) {
        m_pTitle = static_cast<cStatic*>(findWindowById(kIdTitle));
    }
}

void cMPGuageDialog::SetExpGuage(float percent) {
    if (m_setExpGuageCb) {
        m_setExpGuageCb(percent, m_setExpGuageUser);
    }
    if (m_ExpPercent) {
        char temp[32];
        std::snprintf(temp, sizeof(temp), "%4.2f%%", percent * 100.0f);
        m_ExpPercent->SetStaticText(temp);
    }
}

void cMPGuageDialog::RefreshFromPlayerStats() {
    if (!m_playerStatsService) return;
    const auto needed = m_playerStatsService->getExpForNextLevel();
    const float percent = needed == 0
        ? 0.0f
        : static_cast<float>(m_playerStatsService->getLevelExp()) /
          static_cast<float>(needed);
    SetExpGuage(percent > 1.0f ? 1.0f : percent);
}

void cMPGuageDialog::SetTime(std::uint32_t remainTime) {
    if (m_Time) {
        if (remainTime < kRedTextThreshold) {
            m_Time->SetFGColor(0xFFFF0000u);
        }
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%02u:%02u",
                      remainTime / 60000u,
                      (remainTime % 60000u) / 1000u);
        m_Time->SetStaticText(buf);
    }
}

void cMPGuageDialog::SetEventMapTimer(std::uint32_t remainTime,
                                      std::uint8_t bFlag) {
    if (m_Time) {
        switch (bFlag) {
            case kFlagReady:
                m_Time->SetFGColor(0xFF0000FFu);
                break;
            case kFlagActive:
                if (remainTime < kRedTextThreshold) {
                    m_Time->SetFGColor(0xFFFF0000u);
                }
                break;
            case kFlagStopped:
                m_Time->SetFGColor(0xFF0000FFu);
                break;
            default:
                break;
        }
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%02u:%02u",
                      remainTime / 60000u,
                      (remainTime % 60000u) / 1000u);
        m_Time->SetStaticText(buf);
    }
}

void cMPGuageDialog::ShowEventMap() {
    SetActive(true);
    if (m_pTitle) {
        if (m_chatMsgCb) {
            const char* title = m_chatMsgCb(kEventMapTitleChatMsgId,
                                            m_chatMsgUser);
            if (title) {
                m_pTitle->SetStaticText(title);
            }
        }
    }
}

} // namespace mxh::ui
