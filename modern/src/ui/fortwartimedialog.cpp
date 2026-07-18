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
    // 1:1 with legacy: time-remaining tick + CObjectGuagen::SetValue.
    // 1:1 quirk: legacy uses `static int last = 0;` — preserves last
    // tick across calls (skip refresh when same second). Modern port
    // uses m_lastTick (member state) to avoid the global.
    // 1:1 quirk: legacy `gCurTime` is unported. Modern port has no
    // gCurTime; SetActiveWithTime already stored m_dwProcessTime, so
    // the elapsed-time math would require a global timer. We stub the
    // refresh body and delegate to base ActionEvent for hit-test.
    // R-12.x deferred: wire gCurTime port in a follow-up Phase 12.x.
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
        // Modern port has no gCurTime; we store the relative deadline
        // locally (m_dwProcessTime as an opaque value) and let
        // ActionEvent compute remaining time once gCurTime is ported.
        // For now, store dwTime directly as the "process time" so
        // tests can verify the round-trip.
        m_dwProcessTime = dwTime * 1000u;
        m_fBasicTime    = static_cast<float>(dwTime);
    } else {
        m_dwProcessTime = 0;
        m_fBasicTime    = 1.0f;
    }
    cDialog::SetActive(val);
}

// ---------------------------------------------------------------------------
// cFWTimeDialog
// ---------------------------------------------------------------------------

cFWTimeDialog::cFWTimeDialog() = default;
cFWTimeDialog::~cFWTimeDialog() = default;

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
    // 1:1 with legacy: per-second refresh of m_pTimeStatic + base dispatch.
    // gCurTime is unported so the elapsed-time math is stubbed. R-12.x.
    return cDialog::ActionEvent(mouseX, mouseY, mouseFlags);
}

void cFWTimeDialog::SetActiveWithTimeName(bool val, std::uint32_t dwTime, const char* pName) {
    if (val) {
        // 1:1 with legacy: m_dwWarTime = dwTime*1000 + gCurTime.
        // Modern port stores dwTime*1000 as the relative deadline.
        m_dwWarTime = dwTime * 1000u;
        if (m_pCharacterName && pName) {
            m_pCharacterName->SetStaticText(pName);
        }
    } else {
        m_dwWarTime = 0;
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
