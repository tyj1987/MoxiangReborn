// fortwartimedialog.cpp — modern port of 墨香 FortWarDialog (subset)
//
// 1:1 port body. See legacy `FortWarDialog.cpp` for the original.

#include "fortwartimedialog.hpp"

#include "cObjectGuagen.hpp"
#include "cStatic.hpp"

#include <cstdio>
#include <cstring>

namespace mxh::ui {

// 1:1 with legacy `CHATMGR->GetChatMsg(1043)` placeholder. Legacy
// formats: "[채굴/세공] %d 초 남음" or similar. Modern port uses a literal
// placeholder + 1:1 with the format "%d" only.
constexpr const char* kEngraveRemainFmt = "Engrave time: %d";

// 1:1 with legacy timer format: "%02d:%02d" (MM:SS).
constexpr const char* kTimerFormat = "%02d:%02d";

constexpr std::size_t kFmtBufSize = 128;

// ---------------------------------------------------------------------------
// cFWEngraveDialog
// ---------------------------------------------------------------------------

cFWEngraveDialog::cFWEngraveDialog() = default;
cFWEngraveDialog::~cFWEngraveDialog() = default;

void cFWEngraveDialog::SetCurrentTimeProvider(
    FwClockFn getCurrentTime, void* userData) noexcept {
    m_getCurrentTimeFn = getCurrentTime;
    m_clockUserData = userData;
}

void cFWEngraveDialog::SetChatMessageFn(
    FwChatMsgFn getChatMsg, void* userData) noexcept {
    m_getChatMsgFn = getChatMsg;
    m_chatUserData = userData;
}

void cFWEngraveDialog::Linking() {
    // 1:1 with legacy: cStatic + CObjectGuagen (modern cGuagen subclass).
    {
        auto p = std::make_unique<cObjectGuagen>();
        p->Init(0, 0, 100, 16, nullptr, kIdEngraveGuage);
        m_pEngraveGuage = std::move(p);
    }
    {
        auto p = std::make_unique<cStatic>();
        p->Init(0, 0, 100, 16, nullptr, kIdRemaintimeText);
        m_pRemaintimeStatic = std::move(p);
    }
}

std::uint32_t cFWEngraveDialog::ActionEvent(std::int32_t mouseX,
                                            std::int32_t mouseY,
                                            std::uint32_t mouseFlags) {
    if (!isEnabled()) {
        return 0;  // WE_NULL
    }
    // 1:1 with legacy CFWEngraveDialog::ActionEvent. The legacy is:
    //   int nLimitTime = ((int)(m_dwProcessTime - gCurTime)) / 1000;
    //   if (nLimitTime < 0) nLimitTime = 0;
    //   static int last = 0;
    //   if (last != nLimitTime) {
    //     sprintf(buf, CHATMGR->GetChatMsg(1043), nLimitTime);
    //     m_pRemaintimeStatic->SetStaticText(buf);
    //     last = nLimitTime;
    //     m_pEngraveGuagen->SetValue(nLimitTime/m_fBasicTime, (DWORD)m_fBasicTime);
    //   }
    //   we = cDialog::ActionEvent(mouseInfo);
    //
    // 1:1 quirk: legacy `static int last = 0;` is global static.
    // Modern port uses m_lastTick (member state, init -1) so tests
    // can verify the throttling behavior.
    //
    // 1:1 quirk: legacy unsigned DWORD subtraction preserves 32-bit
    // wrap-around. Modern port retains the same DWORD semantics
    // (m_dwProcessTime - curTime) so the negative cast to int and
    // the /1000 floored division match the legacy overflow output.
    //
    // The clock provider is OPTIONAL. A null provider falls back to
    // a safe zero clock so tests can verify the no-refresh branch.
    // The chat message fn is OPTIONAL. A null provider falls back to
    // the literal `kEngraveRemainFmt`.
    if (m_getCurrentTimeFn) {
        const std::uint32_t curTime = m_getCurrentTimeFn(m_clockUserData);
        int nLimitTime = static_cast<int>(m_dwProcessTime - curTime) / 1000;
        if (nLimitTime < 0) {
            nLimitTime = 0;
        }
        if (m_lastTick != nLimitTime) {
            char buf[kFmtBufSize] = {};
            const char* fmt = m_getChatMsgFn
                ? m_getChatMsgFn(1043, m_chatUserData)
                : kEngraveRemainFmt;
            std::snprintf(buf, sizeof(buf), fmt, nLimitTime);
            if (m_pRemaintimeStatic) {
                m_pRemaintimeStatic->SetStaticText(buf);
            }
            if (m_pEngraveGuage && m_fBasicTime > 0.0f) {
                m_pEngraveGuage->SetValue(
                    static_cast<GUAGEVAL>(nLimitTime) / m_fBasicTime,
                    static_cast<std::uint32_t>(m_fBasicTime));
            }
            m_lastTick = nLimitTime;
        }
    }
    return cDialog::ActionEvent(mouseX, mouseY, mouseFlags);
}

void cFWEngraveDialog::OnActionEvent(std::int32_t lId, void* p, std::uint32_t we) {
    // 1:1 with legacy OnActionEvent(WE_BTNCLICK + FW_ENGRAVECANCEL):
    //   MSGBASE msg;
    //   msg.Category = MP_FORTWAR;
    //   msg.Protocol = MP_FORTWAR_ENGRAVE_CANCEL_SYN;
    //   msg.dwObjectID = HEROID;
    //   NETWORK->Send(&msg, sizeof(msg));
    // All four singletons (HERO + NETWORK + MP_FORTWAR*) are unported.
    // 1:1 with legacy: only FW_ENGRAVECANCEL id is dispatched. The
    // legacy class hardcodes the id; modern port keeps the same hardcode
    // so a future port can wire it up. R-12.x deferred.
    (void)lId; (void)p; (void)we;
}

void cFWEngraveDialog::SetActiveWithTime(bool val, std::uint32_t dwTime) {
    if (val) {
        // 1:1 with legacy: m_dwProcessTime = gCurTime + dwTime*1000 (ms).
        // Modern port reads gCurTime through the OPTIONAL host clock
        // provider. A null provider preserves the safe zero-clock
        // fallback (m_dwProcessTime = dwTime*1000) so the dialog can
        // still tick when wired to a stub.
        const std::uint32_t curTime = m_getCurrentTimeFn
            ? m_getCurrentTimeFn(m_clockUserData)
            : 0u;
        m_dwProcessTime = curTime + dwTime * 1000u;
        m_fBasicTime    = static_cast<float>(dwTime);
        m_lastTick      = -1;  // force refresh on next ActionEvent
    } else {
        m_dwProcessTime = 0;
        m_fBasicTime    = 1.0f;
        m_lastTick      = -1;
    }
    cDialog::SetActive(val);
}

// ---------------------------------------------------------------------------
// cFWTimeDialog
// ---------------------------------------------------------------------------

cFWTimeDialog::cFWTimeDialog() = default;
cFWTimeDialog::~cFWTimeDialog() = default;

void cFWTimeDialog::SetCurrentTimeProvider(
    FwClockFn getCurrentTime, void* userData) noexcept {
    m_getCurrentTimeFn = getCurrentTime;
    m_clockUserData = userData;
}

void cFWTimeDialog::Linking() {
    {
        auto p = std::make_unique<cStatic>();
        p->Init(0, 0, 80, 16, nullptr, kIdTimeStatic);
        m_pTimeStatic = std::move(p);
    }
    {
        auto p = std::make_unique<cStatic>();
        p->Init(0, 0, 100, 16, nullptr, kIdCharacterName);
        m_pCharacterName = std::move(p);
    }
}

std::uint32_t cFWTimeDialog::ActionEvent(std::int32_t mouseX,
                                        std::int32_t mouseY,
                                        std::uint32_t mouseFlags) {
    if (!isEnabled()) {
        return 0;  // WE_NULL
    }
    // 1:1 with legacy CFWTimeDialog::ActionEvent. The legacy is:
    //   int nLimitTime = ((int)(m_dwWarTime - gCurTime)) / 1000;
    //   if (nLimitTime < 0) nLimitTime = 0;
    //   static int last = 0;
    //   if (last != nLimitTime) {
    //     sprintf(buf, "%02d:%02d", nLimitTime / 60, nLimitTime % 60);
    //     m_pTimeStatic->SetStaticText(buf);
    //   }
    //   we = cDialog::ActionEvent(mouseInfo);
    //
    // 1:1 quirk: legacy `static int last = 0;` is global static.
    // Modern port uses m_lastTick (member state, init -1) so tests
    // can verify the throttling behavior.
    //
    // 1:1 quirk: legacy unsigned DWORD subtraction preserves 32-bit
    // wrap-around. Modern port retains the same DWORD semantics.
    //
    // The clock provider is OPTIONAL. A null provider falls back to
    // a safe zero clock so tests can verify the no-refresh branch.
    if (m_getCurrentTimeFn) {
        const std::uint32_t curTime = m_getCurrentTimeFn(m_clockUserData);
        int nLimitTime = static_cast<int>(m_dwWarTime - curTime) / 1000;
        if (nLimitTime < 0) {
            nLimitTime = 0;
        }
        if (m_lastTick != nLimitTime) {
            char buf[kFmtBufSize] = {};
            std::snprintf(buf, sizeof(buf), kTimerFormat,
                          nLimitTime / 60, nLimitTime % 60);
            if (m_pTimeStatic) {
                m_pTimeStatic->SetStaticText(buf);
            }
            m_lastTick = nLimitTime;
        }
    }
    return cDialog::ActionEvent(mouseX, mouseY, mouseFlags);
}

void cFWTimeDialog::SetActiveWithTimeName(bool val, std::uint32_t dwTime, const char* pName) {
    if (val) {
        // 1:1 with legacy: m_dwWarTime = dwTime*1000 + gCurTime.
        // Modern port reads gCurTime through the OPTIONAL host clock
        // provider. A null provider preserves the safe zero-clock
        // fallback (m_dwWarTime = dwTime*1000) so the dialog can
        // still tick when wired to a stub.
        const std::uint32_t curTime = m_getCurrentTimeFn
            ? m_getCurrentTimeFn(m_clockUserData)
            : 0u;
        m_dwWarTime = dwTime * 1000u + curTime;
        m_lastTick  = -1;  // force refresh on next ActionEvent
        if (m_pCharacterName && pName) {
            m_pCharacterName->SetStaticText(pName);
        }
    } else {
        m_dwWarTime = 0;
        m_lastTick  = -1;
        if (m_pCharacterName) {
            m_pCharacterName->SetStaticText("");
        }
    }
    cDialog::SetActive(val);
}

void cFWTimeDialog::SetCharacterName(const char* pName) {
    if (m_pCharacterName && pName) {
        m_pCharacterName->SetStaticText(pName);
    }
}

} // namespace mxh::ui
