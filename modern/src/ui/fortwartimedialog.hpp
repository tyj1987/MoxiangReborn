// fortwartimedialog.hpp — modern port of 墨香 FortWarDialog.h (subset)
//
// 1:1 port of TWO classes from legacy
//   `墨香【源码】\[Client]MH\FortWarDialog.h`:
//     - CFWEngraveDialog (engrave-in-progress bar)
//     - CFWTimeDialog    (siege-war timer + character name)
//
// The third class in the legacy header (CFWWareHouseDialog) extends
// cTabDialog and depends on cIconGridDialog + cItem + cMsgBox + cDivideBox —
// NOT ported here. R-12.x deferred.
//
// Both ported classes follow the cObjectGuagen pattern: Linking + 1-2
// method 1:1 with legacy body.  ActionEvent now has the legacy
// per-second refresh body, gated on an OPTIONAL host clock provider
// (replaces the gCurTime global) and an OPTIONAL host chat message
// function (replaces the CHATMGR->GetChatMsg(1043) call in
// CFWEngraveDialog).  OnActionEvent (FW_ENGRAVECANCEL -> NETWORK->Send)
// remains a TODO because NETWORK + HERO + MP_FORTWAR are not ported.

#pragma once

#include "cDialog.hpp"

#include <cstdint>
#include <memory>

namespace mxh::ui {

class cStatic;
class cObjectGuagen;

// Shared clock provider signature (replaces legacy `gCurTime` global).
// Returns DWORD milliseconds in legacy monotonic time.
using FwClockFn = std::uint32_t (*)(void* userData);

// Host callback that yields the localized chat template for the
// engrave time-remaining line.  Legacy calls CHATMGR->GetChatMsg(1043);
// the modern port routes through this host callback.  A null provider
// falls back to the literal `kEngraveRemainFmt`.
using FwChatMsgFn = const char* (*)(int msgId, void* userData);

// ---------------------------------------------------------------------------
// CFWEngraveDialog — engrave-in-progress bar
// ---------------------------------------------------------------------------
class cFWEngraveDialog : public cDialog {
public:
    // Local id range 1:1 with legacy FW_* enum (rebased to 780..781).
    static constexpr int kIdEngraveGuage  = 780;
    static constexpr int kIdRemaintimeText = 781;

    cFWEngraveDialog();
    ~cFWEngraveDialog() override;

    void Linking();
    std::uint32_t ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                              std::uint32_t mouseFlags) override;
    void OnActionEvent(std::int32_t lId, void* p, std::uint32_t we);
    void SetActiveWithTime(bool val, std::uint32_t dwTime);

    // Replace the legacy gCurTime read for SetActiveWithTime/ActionEvent.
    // A null provider preserves the safe zero-clock fallback.
    void SetCurrentTimeProvider(FwClockFn getCurrentTime,
                                void* userData = nullptr) noexcept;

    // Replace the legacy CHATMGR->GetChatMsg(1043) call in
    // ActionEvent.  A null provider falls back to `kEngraveRemainFmt`.
    void SetChatMessageFn(FwChatMsgFn getChatMsg,
                          void* userData = nullptr) noexcept;

    // Test accessors.
    const cObjectGuagen* GetEngraveGuage() const noexcept { return m_pEngraveGuage.get(); }
    const cStatic* GetRemaintimeStatic() const noexcept    { return m_pRemaintimeStatic.get(); }
    std::uint32_t GetProcessTime() const noexcept          { return m_dwProcessTime; }
    float GetBasicTime() const noexcept                    { return m_fBasicTime; }

private:
    std::unique_ptr<cObjectGuagen> m_pEngraveGuage;
    std::unique_ptr<cStatic>       m_pRemaintimeStatic;

    std::uint32_t m_dwProcessTime = 0;
    float         m_fBasicTime    = 1.0f;
    int           m_lastTick      = -1;

    FwClockFn   m_getCurrentTimeFn = nullptr;
    void*       m_clockUserData    = nullptr;
    FwChatMsgFn m_getChatMsgFn     = nullptr;
    void*       m_chatUserData     = nullptr;
};

// ---------------------------------------------------------------------------
// CFWTimeDialog — siege-war timer + character name
// ---------------------------------------------------------------------------
class cFWTimeDialog : public cDialog {
public:
    // Local id range 1:1 with legacy FW_TIME* enum (rebased to 782..783).
    static constexpr int kIdTimeStatic      = 782;
    static constexpr int kIdCharacterName   = 783;

    cFWTimeDialog();
    ~cFWTimeDialog() override;

    void Linking();
    std::uint32_t ActionEvent(std::int32_t mouseX, std::int32_t mouseY,
                              std::uint32_t mouseFlags) override;
    void SetActiveWithTimeName(bool val, std::uint32_t dwTime, const char* pName);
    void SetCharacterName(const char* pName);

    // Replace the legacy gCurTime read for SetActiveWithTimeName and
    // ActionEvent.  A null provider preserves the safe zero-clock fallback.
    void SetCurrentTimeProvider(FwClockFn getCurrentTime,
                                void* userData = nullptr) noexcept;

    // Test accessors.
    const cStatic* GetTimeStatic() const noexcept    { return m_pTimeStatic.get(); }
    const cStatic* GetCharacterName() const noexcept { return m_pCharacterName.get(); }
    std::uint32_t GetWarTime() const noexcept        { return m_dwWarTime; }

private:
    std::unique_ptr<cStatic> m_pTimeStatic;
    std::unique_ptr<cStatic> m_pCharacterName;

    std::uint32_t m_dwWarTime = 0;
    int           m_lastTick  = -1;

    FwClockFn m_getCurrentTimeFn = nullptr;
    void*     m_clockUserData    = nullptr;
};

} // namespace mxh::ui
