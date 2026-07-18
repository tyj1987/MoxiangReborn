// skillpointnotify.cpp — modern port of 墨香 CSkillPointNotify
//
// 1:1 port body. See legacy `SkillPointNotify.cpp` for the original.

#include "skillpointnotify.hpp"

#include "cButton.hpp"
#include "cTextArea.hpp"

namespace mxh::ui {

// 1:1 with legacy CHATMGR->GetChatMsg(735) + CHATMGR->GetChatMsg(736).
// Legacy localized strings (Korean) are stored in StringLoader; modern
// port uses literal English placeholders + the chat-msg-id constant for
// future CHATMGR port.
constexpr const char* kNotifyText1Default = "Skill points can be reset.";
constexpr const char* kNotifyText2Default = "Press the button to start.";

cSkillPointNotify::cSkillPointNotify() = default;
cSkillPointNotify::~cSkillPointNotify() = default;

void cSkillPointNotify::Linking() {
    // 1:1 quirk: legacy GetWindowForID; modern findWindowById + own
    // the children as unique_ptr members (membership-as-children pattern
    // matching cGuildLevelUpDialog / cAlertDlg).
    {
        auto p = std::make_unique<cTextArea>();
        TextRect rect{0, 0, 200, 32};
        p->InitTextArea(rect, 256, nullptr, 0, nullptr, 0, nullptr, 0);
        p->setId(kIdNotifyText1);
        m_Notifyta1 = std::move(p);
    }
    {
        auto p = std::make_unique<cTextArea>();
        TextRect rect{0, 32, 200, 64};
        p->InitTextArea(rect, 256, nullptr, 0, nullptr, 0, nullptr, 0);
        p->setId(kIdNotifyText2);
        m_Notifyta2 = std::move(p);
    }
    {
        auto p = std::make_unique<cButton>();
        p->Init(0, 64, 80, 24, nullptr, nullptr, nullptr, nullptr, nullptr, kIdStartBtn);
        m_RedistBtn = std::move(p);
    }
}

void cSkillPointNotify::InitTextArea() {
    // 1:1 with legacy: defensive nullptr-check then SetScriptText.
    // Legacy uses CHATMGR->GetChatMsg(735/736) which returns Korean
    // strings; modern port uses literal English placeholders. R-12.x
    // deferred: wire CHATMGR singleton for real localized text.
    if (m_Notifyta1) {
        m_Notifyta1->SetScriptText(kNotifyText1Default);
    }
    if (m_Notifyta2) {
        m_Notifyta2->SetScriptText(kNotifyText2Default);
    }
}

} // namespace mxh::ui
