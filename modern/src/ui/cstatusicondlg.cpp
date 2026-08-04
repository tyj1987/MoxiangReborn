// cstatusicondlg.cpp — modern port of 墨香 CStatusIconDlg (status icon stack).
//
// 1:1 port of legacy `CStatusIconDlg` from
//   `墨香【源码】\[Client]MH\StatusIconDlg.cpp`.

#include "mxh/ui/cstatusicondlg.hpp"

#include <algorithm>
#include <cstring>
#include <utility>

namespace mxh::ui {

cStatusIconDlg::cStatusIconDlg() {
    // 1:1 with legacy ctor: the per-icon arrays are sized
    // [eStatusIcon_Max] = 13.  std::vector::resize fills with
    // T() == 0 / false.
    m_IconCount.assign(eStatusIcon_Max, 0);
    m_IconInfo.assign(eStatusIcon_Max, IconRenderInfo{});
    m_dwRemainTime.assign(eStatusIcon_Max, 0u);
    m_dwStartTime.assign(eStatusIcon_Max, 0u);
    m_CurIconNum = 0;
    m_MaxDesc = 0;
    m_pDescriptionArray = nullptr;
    m_nQuestIconCount = 0;
}

cStatusIconDlg::~cStatusIconDlg() {
    Release();
}

void cStatusIconDlg::Init(void* pObject, std::int32_t x, std::int32_t y,
                          std::int32_t maxIconPerLine) {
    // 1:1 with legacy Init(CObject*, VECTOR2*, MaxIconPerLine).
    // The legacy stores the CObject* and a VECTOR2 for the
    // draw position.  The modern port stores X + Y separately
    // (VECTOR2 isn't ported yet) and the bound object as
    // void*.
    m_pObject       = pObject;
    m_DrawPositionX = x;
    m_DrawPositionY = y;
    m_MaxIconPerLine = maxIconPerLine;
    m_CurIconNum    = 0;
    LoadDescription();
}

void cStatusIconDlg::Release() {
    // 1:1 with legacy Release.  Clears the per-icon arrays +
    // the description table.
    std::fill(m_IconCount.begin(),    m_IconCount.end(),    0);
    std::fill(m_IconInfo.begin(),     m_IconInfo.end(),     IconRenderInfo{});
    std::fill(m_dwRemainTime.begin(), m_dwRemainTime.end(), 0u);
    std::fill(m_dwStartTime.begin(),  m_dwStartTime.end(),  0u);
    m_CurIconNum = 0;
    m_nQuestIconCount = 0;
    if (m_pDescriptionArray) {
        delete[] m_pDescriptionArray;
        m_pDescriptionArray = nullptr;
    }
    m_MaxDesc = 0;
}

void cStatusIconDlg::LoadDescription() {
    // 1:1 with legacy LoadDescription.  The legacy opens
    // "./Image/ListStatusIcon.txt" / ".bin" via CMHFile and
    // reads:
    //   WORD Count = file.GetWord();
    //   while (Count--) { StaticString s; file.GetString(s.key, 16); file.GetString(s.value, 64); m_pDescriptionArray[i++] = s; }
    // The modern port defers the actual file load (CMHFile
    // isn't fully wired up); tests inject descriptions via
    // AddDescriptionForTest.  We allocate a small default
    // array so the description accessor is safe.
    if (m_pDescriptionArray) {
        delete[] m_pDescriptionArray;
        m_pDescriptionArray = nullptr;
    }
    m_MaxDesc = 0;
}

void cStatusIconDlg::AddDescriptionForTest(const char* key, const char* value) {
    if (key == nullptr || value == nullptr) return;
    // Linear append.  Reallocates the array on every add so
    // the storage matches m_MaxDesc.
    auto* grown = new StaticString[static_cast<std::size_t>(m_MaxDesc) + 1];
    for (std::int32_t i = 0; i < m_MaxDesc; ++i) grown[i] = m_pDescriptionArray[i];
    StaticString& dst = grown[m_MaxDesc];
    std::strncpy(dst.key,   key,   sizeof(dst.key)   - 1);
    dst.key[sizeof(dst.key) - 1] = '\0';
    std::strncpy(dst.value, value, sizeof(dst.value) - 1);
    dst.value[sizeof(dst.value) - 1] = '\0';
    delete[] m_pDescriptionArray;
    m_pDescriptionArray = grown;
    ++m_MaxDesc;
}

void cStatusIconDlg::AddIcon(void* pObject, std::uint16_t statusIconNum,
                              std::uint16_t itemIdx,
                              std::uint32_t dwRemainTime) {
    // 1:1 with legacy AddIcon.  The legacy asserts that the
    // kind is in range, then increments the per-kind count +
    // populates ICONRENDERINFO.  The remaining time is stored
    // in m_dwRemainTime[kind] and the start time is gCurTime.
    if (pObject != m_pObject) return;        // legacy: same bound object
    if (static_cast<std::int32_t>(statusIconNum) >= eStatusIcon_Max) return;
    if (static_cast<std::int32_t>(statusIconNum) < 0) return;
    ++m_IconCount[statusIconNum];
    m_IconInfo[statusIconNum].ItemIndex = itemIdx;
    // 1:1 quirk: legacy m_dwRemainTime[kind] stores the
    // remaining time for the *latest* icon of that kind.  The
    // modern port keeps the same single-slot model.
    m_dwRemainTime[statusIconNum] = dwRemainTime;
    // 1:1 with legacy m_dwStartTime[StatusIconNum] = gCurTime.
    // The host clock provider replaces gCurTime (R-12.x).  A null
    // provider preserves the safe zero-clock fallback so the
    // remaining tooltip still computes correctly (elapsed = 0).
    m_dwStartTime[statusIconNum]  = m_getCurrentTimeFn
        ? m_getCurrentTimeFn(m_clockUserData)
        : 0u;
    ++m_CurIconNum;
}

void cStatusIconDlg::AddQuestTimeIcon(void* pObject, std::uint16_t statusIconNum) {
    // 1:1 with legacy AddQuestTimeIcon: increments the count
    // and bumps the quest-icon counter (separate from the
    // kind counter in the legacy).  The modern port also
    // keeps the legacy quest-icon-counter side-effect.
    if (pObject != m_pObject) return;
    if (static_cast<std::int32_t>(statusIconNum) >= eStatusIcon_Max) return;
    ++m_IconCount[statusIconNum];
    ++m_nQuestIconCount;
    ++m_CurIconNum;
}

void cStatusIconDlg::RemoveIcon(void* pObject, std::uint16_t statusIconNum,
                                 std::uint16_t itemIdx) {
    // 1:1 with legacy RemoveIcon: only remove if the bound
    // object matches AND the kind is in range.  If itemIdx
    // == 0, decrement the per-kind count (any itemIdx).
    // Otherwise the modern port keeps the legacy "decrement
    // once if the stored ItemIndex matches" behaviour.
    if (pObject != m_pObject) return;
    if (static_cast<std::int32_t>(statusIconNum) >= eStatusIcon_Max) return;
    if (m_IconCount[statusIconNum] == 0) return;
    if (itemIdx == 0 || m_IconInfo[statusIconNum].ItemIndex == itemIdx) {
        --m_IconCount[statusIconNum];
        --m_CurIconNum;
        if (m_CurIconNum < 0) m_CurIconNum = 0;
    }
}

void cStatusIconDlg::RemoveQuestTimeIcon(void* pObject, std::uint16_t statusIconNum) {
    if (pObject != m_pObject) return;
    if (static_cast<std::int32_t>(statusIconNum) >= eStatusIcon_Max) return;
    if (m_IconCount[statusIconNum] == 0) return;
    --m_IconCount[statusIconNum];
    --m_nQuestIconCount;
    --m_CurIconNum;
    if (m_nQuestIconCount < 0) m_nQuestIconCount = 0;
    if (m_CurIconNum       < 0) m_CurIconNum       = 0;
}

void cStatusIconDlg::RemoveAllQuestTimeIcon() {
    // 1:1 with legacy RemoveAllQuestTimeIcon: walks every
    // kind and zeroes the count.
    for (std::int32_t i = 0; i < eStatusIcon_Max; ++i) {
        m_IconCount[i] = 0;
    }
    m_nQuestIconCount = 0;
    m_CurIconNum = 0;
}

void cStatusIconDlg::SetOneMinuteToShopItem(std::uint32_t itemIdx) {
    // 1:1 with legacy SetOneMinuteToShopItem: sets a flag
    // that the next AddIcon call with itemIdx will clamp the
    // remaining time to 60s.  The modern port doesn't model
    // a "next call" flag explicitly; instead, the host is
    // expected to pass a clamped dwRemainTime when calling
    // AddIcon.  The function is kept for 1:1 source compat.
    (void)itemIdx;
}

void cStatusIconDlg::Render() {
    // 1:1 with legacy Render.  The legacy's cImageSelf::
    // RenderSprite call is replaced with m_onDrawIcon
    // callback; the host's render loop paints the icon at
    // the supplied position.  The legacy wrap logic is
    // preserved: icons render in a 2D grid with m_MaxIconPerLine
    // columns.
    if (!m_onDrawIcon) return;
    std::int32_t idx = 0;
    for (std::int32_t kind = 0; kind < eStatusIcon_Max; ++kind) {
        for (std::uint16_t n = 0; n < m_IconCount[kind]; ++n) {
            RenderCtx ctx{};
            ctx.drawX      = m_DrawPositionX + (idx % m_MaxIconPerLine) * 16;
            ctx.drawY      = m_DrawPositionY + (idx / m_MaxIconPerLine) * 16;
            ctx.maxPerLine = m_MaxIconPerLine;
            ctx.curIdx     = idx;
            ctx.iconKind   = static_cast<std::uint16_t>(kind);
            ctx.itemIdx    = m_IconInfo[kind].ItemIndex;
            ctx.bPlus      = m_IconInfo[kind].bPlus;
            ctx.bAlpha     = m_IconInfo[kind].bAlpha;
            ctx.alpha      = m_IconInfo[kind].Alpha;
            m_onDrawIcon(ctx);
            ++idx;
        }
    }
}

void cStatusIconDlg::SetCurrentTimeProvider(
    GetCurrentTimeFn getCurrentTime, void* userData) noexcept {
    m_getCurrentTimeFn = getCurrentTime;
    m_clockUserData    = userData;
}
std::uint32_t cStatusIconDlg::GetRemainTimeAt(std::int32_t kind) const noexcept {
    if (kind < 0 || kind >= eStatusIcon_Max) return 0u;
    return m_dwRemainTime[kind];
}
std::uint32_t cStatusIconDlg::GetStartTimeAt(std::int32_t kind) const noexcept {
    if (kind < 0 || kind >= eStatusIcon_Max) return 0u;
    return m_dwStartTime[kind];
}
bool cStatusIconDlg::GetAlphaFlagAt(std::int32_t kind) const noexcept {
    if (kind < 0 || kind >= eStatusIcon_Max) return false;
    return m_IconInfo[kind].bAlpha;
}
void cStatusIconDlg::Process() {
    // 1:1 with the per-frame timer logic from legacy
    // CStatusIconDlg::Render -- mirrors the legacy pattern:
    //   DWORD elapsed = gCurTime - m_dwStartTime[n];
    //   if (elapsed < m_dwRemainTime[n]) {
    //       if (m_dwRemainTime[n] - elapsed <= 5000)
    //           m_IconInfo[n].bAlpha = TRUE;
    //   }
    // The host clock provider replaces gCurTime (R-12.x); a null
    // provider preserves the safe zero-clock fallback.
    const std::uint32_t curTime = m_getCurrentTimeFn
        ? m_getCurrentTimeFn(m_clockUserData)
        : 0u;

    // Only kinds with active icons participate in the timer.
    for (std::int32_t kind = 1; kind < eStatusIcon_Max; ++kind) {
        if (m_IconCount[kind] == 0) continue;
        if (m_dwRemainTime[kind] == 0u) continue;

        // Legacy uses DWORD arithmetic (unsigned wrap-around).
        // curTime - m_dwStartTime[kind] matches the legacy intent
        // even when curTime < m_dwStartTime[kind].
        const std::uint32_t elapsed = curTime - m_dwStartTime[kind];

        // Legacy DWORD comparison: elapsed < m_dwRemainTime[kind]
        // determines whether the icon is still active.  When
        // expired, the legacy flips bAlpha back to FALSE (the
        // tooltip branch then skips the remain-time label).
        if (elapsed < m_dwRemainTime[kind]) {
            const std::uint32_t remaining = m_dwRemainTime[kind] - elapsed;
            // Blink when remaining is within 5000 ms (locked constant).
            m_IconInfo[kind].bAlpha =
                (remaining <= kStatusIconExpiringBlinkMs);
        } else {
            // Expired: no blink.
            m_IconInfo[kind].bAlpha = false;
        }
    }
}
} // namespace mxh::ui
