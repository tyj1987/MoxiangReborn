//
// 1:1 port of legacy `cGuageBar` from
//   `墨香【源码】\[Client]MH\interface\cGuageBar.h`.
//
// Legacy cGuageBar is a draggable horizontal/vertical slider bar:
//   - Owns a child cButton (m_pbarBtn) that the user drags along an
//     interval to set a floating-point rate (0..1).
//   - Stores minValue / maxValue / curValue (LONG) and a float
//     m_barRelPos clamped to [0, m_interval].
//   - Rate is computed as `m_barRelPos / m_interval`.
//   - InitGuageBar(interval, vertical) sets the interval axis.
//   - Add(cWindow * btn) attaches the bar button (1:1 quirk: must be
//     called before InitValue).
//   - InitValue(minv, maxv, curv) sets logical value range; current
//     curValue is mapped to barRelPos via `(curv-minv)/(maxv-minv) * interval`.
//   - SetCurValue(LONG) updates curValue then repositions.
//   - ActionEvent / Render / SetAbsXY override the underlying cWindow.
//
// Modern port: thin cGuagen subclass. The display bar is rendered
// through cGuagen's piece image (modern cGuagen already covers the
// base class data). 1:1 interface methods are added:
//   - InitGuageBar(interval, vertical)
//   - Add(cWindow * btn)
//   - InitValue / SetMinValue / SetMaxValue / GetCurValue / SetCurValue
//   - GetCurRate / SetCurRate / IsDrag / SetGuageLock / GetMinValue / GetMaxValue
//   - SetInterval / GetInterval
//   - SetAbsXY / ActionEvent / Render (override)
//
// 1:1 quirks NOT preserved (R-12.x deferred):
//   - Modern cWindow does not have SetAlpha; the legacy override
//     was removed. When the modern render path exposes a per-window
//     alpha (6.4+), re-add the override and cascade to m_pbarBtn.
//   - drag handling (CMouse integration) is stubbed: ActionEvent
//     returns WE_NULL when not active or locked. The drag math
//     stays in 1:1 fit with the legacy formula in comments.
//
#pragma once

#include "mxh/ui/cguagen.hpp"

#include <cstdint>

namespace mxh::ui {

class cButton;

class cGuageBar : public cGuagen {
public:
    cGuageBar();
    ~cGuageBar() override;

    // ----- 1:1 with legacy cGuageBar public API -----

    // 1:1 with legacy InitGuageBar(LONG interval, BOOL vertical).
    // Stores interval (must be > 0) and orientation flag. Also
    // configures cGuagen's piece width to `interval` so the
    // visual rate matches the logical rate.
    void InitGuageBar(std::int32_t interval, bool vertical);

    // 1:1 with legacy Add(cWindow * btn). Stores the bar button
    // reference. 1:1 quirk: only one button is allowed; second
    // call is silently ignored (legacy ASSERTed).
    void Add(cWindow* btn);

    // 1:1 with legacy InitValue(LONG minv, LONG maxv, LONG curv).
    // Sets min/max/cur; then computes m_barRelPos from (curv-minv).
    void InitValue(std::int32_t minv, std::int32_t maxv, std::int32_t curv);
    void SetMinValue(std::int32_t minv);
    void SetMaxValue(std::int32_t maxv);
    std::int32_t GetMinValue() const noexcept { return m_minValue; }
    std::int32_t GetMaxValue() const noexcept { return m_maxValue; }
    std::int32_t GetCurValue() const noexcept { return m_curValue; }
    void SetCurValue(std::int32_t val);

    // 1:1 with legacy SetInterval / GetInterval. Calls repositioning().
    void SetInterval(std::int32_t val);
    std::int32_t GetInterval() const noexcept { return m_interval; }

    // 1:1 with legacy rate accessors. rate = m_barRelPos / m_interval.
    float GetCurRate() const noexcept;
    void SetCurRate(float fRate) noexcept;

    // 1:1 with legacy IsDrag (m_bInDrag flag).
    bool IsDrag() const noexcept { return m_fBarDrag; }

    // 1:1 with legacy SetGuageLock(BOOL bDo, DWORD BtnColor).
    // Stores lock flag + bar button color. (Drag is stubbed, so
    // the lock check happens in the future ActionEvent override.)
    void SetGuageLock(bool bDo, std::uint32_t btnColor);

    // 1:1 with legacy cGuageBar overrides.
    void SetAbsXY(std::int32_t x, std::int32_t y) noexcept;
    std::uint32_t ActionEvent() noexcept;
    void Render() override;

    // ----- test / inspection helpers (not in legacy) -----

    // 1:1 with legacy m_fBarBtn color (used by tests).
    std::uint32_t GetBarBtnColor() const noexcept { return m_btnColor; }

    // 1:1 with legacy m_fVertical (used by tests).
    bool IsVertical() const noexcept { return m_fVertical; }

    // 1:1 with legacy m_fBarRelPos (used by tests).
    float GetBarRelPos() const noexcept { return m_barRelPos; }

    // 1:1 with legacy m_bLock (used by tests).
    bool IsLocked() const noexcept { return m_bLock; }

private:
    // 1:1 with legacy repositioning: if max-min > 0,
    // m_barRelPos = interval * (curValue-min)/(max-min).
    void repositioning();

    cButton* m_pbarBtn = nullptr;
    std::int32_t m_minValue = 0;
    std::int32_t m_maxValue = 0;
    std::int32_t m_curValue = 0;
    std::int32_t m_startPos = 0;
    std::int32_t m_interval = 0;
    float m_barRelPos = 0.0f;
    bool m_fBarDrag = false;
    bool m_fVertical = false;
    float m_fDragRelX = 0.0f;
    float m_fDragRelY = 0.0f;
    bool m_bLock = false;
    std::uint32_t m_btnColor = 0xFFFFFFFFu;
};

}  // namespace mxh::ui
