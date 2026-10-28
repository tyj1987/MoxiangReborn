// cmpguagedialog.hpp -- modern port of Moxiang
//   CMPGuageDialog (event-map timer + exp gauge).
//
// 1:1 port of legacy `CMPGuageDialog` from
//   `[Client]MH\MPGuageDialog.{h,cpp}`.
//
// 4 children resolved in Linking: 1 CObjectGuagen
// (m_ExpGuage) + 3 cStatic (m_Time, m_ExpPercent,
// m_pTitle).  The legacy has 3 entry points:
//   - SetExpGuage(float Percent) -- updates the
//     exp gauge + percent text (%4.2f%%).
//   - SetTime(DWORD RemainTime) -- formats the
//     timer text (mm:ss) and turns red below 30s.
//   - SetEventMapTimer(DWORD RemainTime, BYTE bFlag)
//     -- 3-way switch: 0=blue, 1=conditional red
//     (if RemainTime < 30000), 2=blue.
//   - ShowEventMap() -- activates the dialog and
//     sets the title from CHATMGR->GetChatMsg(140).
//
// 1:1 dependencies:
//   * CObjectGuagen (m_ExpGuage) -- the bar
//     primitive.  Legacy calls SetValue(Percent, 0).
//   * 3 cStatic (m_Time / m_ExpPercent / m_pTitle).
//   * CHATMGR->GetChatMsg(140) (event map title).
//
// 1:1 quirks:
//   - 1:1 with legacy `m_type = WT_MPGUAGEDLG;` --
//     modern cWindow has no m_type field; ctor is
//     empty (1:1 quirk note).
//   - 1:1 with legacy SetFGColor via RGB_HALF
//     macros.  Legacy RGB_HALF(255,0,0) = 0xFFFF0000
//     (ARGB).  Modern port uses the same 0xAARRGGBB
//     layout.
//   - 1:1 with legacy sprintf "%4.2f%%" (exp
//     percent text) and "%02d:%02d" (timer text).
//   - 1:1 with legacy SetTime'"'"'s red threshold
//     (< 30000).
//   - 1:1 with legacy SetEventMapTimer 3-way
//     switch -- bFlag 0 sets blue unconditionally,
//     bFlag 1 conditionally sets red below 30000,
//     bFlag 2 sets blue unconditionally.
//   - 1:1 with legacy ShowEventMap'"'"'s
//     SetActive(TRUE) + SetStaticText via CHATMGR.
//   - 1:1 with legacy cguagen::SetValue(Percent, 0):
//     modern port uses a host-injected callback
//     (the CObjectGuagen framework is R-12.x
//     deferred).
//   - 1:1 with legacy CHATMGR->GetChatMsg(140)
//     event map title: modern port uses a
//     host-injected callback.

#pragma once

#include "cDialog.hpp"
#include "mxh/services/IPlayerStatsService.hpp"

#include <cstdint>

namespace mxh::ui {

class cStatic;

class cMPGuageDialog : public cDialog {
public:
    cMPGuageDialog();
    ~cMPGuageDialog() override;

    cMPGuageDialog(const cMPGuageDialog&) = delete;
    cMPGuageDialog& operator=(const cMPGuageDialog&) = delete;

    // 1:1 with legacy CMPGuageDialog::Linking.
    // Resolve 1 CObjectGuagen (m_ExpGuage by
    // MP_GEXPGUAGE) + 3 cStatic by id.
    void Linking();

    // 1:1 with legacy CMPGuageDialog::SetExpGuage.
    // Calls m_ExpGuage callback + sprintf "%4.2f%%"
    // -> m_ExpPercent->SetStaticText.
    void SetExpGuage(float percent);
    void SetPlayerStatsService(const mxh::services::IPlayerStatsService* service) noexcept {
        m_playerStatsService = service;
    }
    const mxh::services::IPlayerStatsService* playerStatsService() const noexcept {
        return m_playerStatsService;
    }
    void RefreshFromPlayerStats();

    // 1:1 with legacy CMPGuageDialog::SetTime.
    // If RemainTime < 30000 -> m_Time->SetFGColor
    // (red); sprintf "%02d:%02d" -> m_Time->SetStaticText.
    void SetTime(std::uint32_t remainTime);

    // 1:1 with legacy CMPGuageDialog::SetEventMapTimer.
    // 3-way switch on bFlag: 0=blue, 1=conditional red
    // (if RemainTime < 30000), 2=blue; sprintf
    // "%02d:%02d" -> m_Time->SetStaticText.
    void SetEventMapTimer(std::uint32_t remainTime, std::uint8_t bFlag);

    // 1:1 with legacy CMPGuageDialog::ShowEventMap.
    // SetActive(TRUE) + m_pTitle->SetStaticText via
    // the host-injected ChatMsg callback.
    void ShowEventMap();

    // ---- 1:1 id constants (legacy WindowIDs.h) ----
    static constexpr std::int32_t kIdExpGuage   = 610;
    static constexpr std::int32_t kIdTime       = 611;
    static constexpr std::int32_t kIdExpPercent = 612;
    static constexpr std::int32_t kIdTitle      = 613;

    // 1:1 with legacy 30000 (DWORD threshold for
    // red text in SetTime / SetEventMapTimer case 1).
    static constexpr std::uint32_t kRedTextThreshold = 30000;

    // 1:1 with legacy bFlag enum (0=ready, 1=active,
    // 2=stopped).
    static constexpr std::uint8_t kFlagReady   = 0;
    static constexpr std::uint8_t kFlagActive  = 1;
    static constexpr std::uint8_t kFlagStopped = 2;

    // Test hooks -- inject the 4 child pointers.
    void SetExpGuageForTest(void* g) noexcept { m_ExpGuage = g; }
    void SetTimeStaticForTest(cStatic* s) noexcept { m_Time = s; }
    void SetExpPercentForTest(cStatic* s) noexcept { m_ExpPercent = s; }
    void SetTitleForTest(cStatic* s) noexcept { m_pTitle = s; }
    void* GetExpGuageForTest() const noexcept { return m_ExpGuage; }
    cStatic* GetTimeStaticForTest() const noexcept { return m_Time; }
    cStatic* GetExpPercentForTest() const noexcept { return m_ExpPercent; }
    cStatic* GetTitleForTest() const noexcept { return m_pTitle; }

    // ---- 1:1 callbacks for legacy singletons ----
    // 1:1 with legacy CObjectGuagen::SetValue(Percent, 0):
    // host calls SetExpGuageCallbackForTest to wire the
    //      SetValue(Percent, 0) hand-off.
    using SetExpGuageCallback = void(*)(float percent, void* user);
    void SetExpGuageCallbackForTest(SetExpGuageCallback cb, void* user) {
        m_setExpGuageCb = cb; m_setExpGuageUser = user;
    }

    // 1:1 with legacy CHATMGR->GetChatMsg(140) (event
    // map title).  Host supplies a callback that returns
    // the localized string (tests inject a literal).
    using ChatMsgCallback = const char*(*)(int chatMsgId, void* user);
    void SetChatMsgCallbackForTest(ChatMsgCallback cb, void* user) {
        m_chatMsgCb = cb; m_chatMsgUser = user;
    }

    // 1:1 with legacy CHATMGR->GetChatMsg(140) id.
    static constexpr int kEventMapTitleChatMsgId = 140;

private:
    void* m_ExpGuage = nullptr;
    cStatic* m_Time       = nullptr;
    cStatic* m_ExpPercent = nullptr;
    cStatic* m_pTitle     = nullptr;

    SetExpGuageCallback m_setExpGuageCb   = nullptr;
    void*               m_setExpGuageUser = nullptr;
    ChatMsgCallback     m_chatMsgCb       = nullptr;
    void*               m_chatMsgUser     = nullptr;
    const mxh::services::IPlayerStatsService* m_playerStatsService = nullptr;
};

} // namespace mxh::ui
