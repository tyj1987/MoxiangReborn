// cspin.hpp — modern port of 墨香 cSpin (numeric spin control).
//
// 1:1 port of legacy `cSpin` from
//   `墨香【源码】\[Client]MH\interface\cSpin.{h,cpp}`.
//
// A cSpin is a single-line numeric editor with an up arrow and a down
// arrow. The user types a number, or clicks the up/down arrows to
// increment / decrement the value by `m_Unit` (default 10). The value
// is bounded by `[m_minValue, m_maxValue]` (default 0..100).
//
// Legacy implementation depended on `cIMEex` (the full Korean/JP IME
// control) for its text buffer. The modern port reuses `cEditBox` as
// the underlying text storage and re-implements the numeric formatting
// (commas-as-thousands-separator, `_i64toa` round-trip) inline. The
// 1:1 contract preserved here is:
//   - `Init(x, y, wid, hei, basicImage, cbFUNC, id)` — the legacy 7-param
//     signature. Modern port uses `std::function` for the callback
//     (replacing the legacy `cbFUNC` typedef + global SpinUp/Down procs).
//   - `GetValue()` / `SetValue(value)` / `IncUnit()` / `DecUnit()` /
//     `SetMin/Max/Unit` — value accessors and mutators.
//   - `InitSpin(bufSize, strSize)` — re-initializes the text buffer
//     (delegates to cEditBox::InitEditbox) and resets value to 0.
//   - Two child buttons (`m_upBtn` / `m_downBtn`) added via
//     `AddSpinButton(unique_ptr<cButton>, SpinButtonKind)` since
//     `cWindow::Add` takes unique_ptr<cWindow> (not raw cWindow*).
//
// 1:1 quirks preserved:
//   - `m_Unit` defaults to 10 (legacy default).
//   - `m_minValue` defaults to 0, `m_maxValue` defaults to 100.
//   - `GetValue()` and `SetValue()` clamp to [min, max] range.
//   - `IncUnit()`/`DecUnit()` saturate at the range, with unsigned
//     underflow/overflow protection (same shape as legacy).
//   - Value display always includes thousands-separator commas (legacy
//     `AddComma` behavior; modern re-implements inline).

#pragma once

#include "cEditBox.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace mxh::ui {

class cButton;

// SPINUNIT: the integer type used for spin values. Legacy typedef'd to
// DWORD (32-bit unsigned); modern port uses std::uint32_t for parity.
using SPINUNIT = std::uint32_t;

// Click callback for the up/down spin buttons. Legacy uses global
// C-style procs (`SpinUpBtnProc` / `SpinDownBtnProc`); modern port uses
// std::function (consistent with cButton's ClickCallback).
using SpinButtonCallback =
    std::function<void(std::int32_t buttonId, void* userdata)>;

class cSpin : public cEditBox {
public:
    cSpin();
    ~cSpin() override;

    cSpin(const cSpin&) = delete;
    cSpin& operator=(const cSpin&) = delete;

    // Init: position, size, basic image, callback, id.
    // 1:1 with legacy 7-param signature (preserves the cEditBox::Init
    // 7-param footprint + adds the cbFUNC-equivalent std::function at
    // position 6).
    void Init(std::int32_t x, std::int32_t y, std::uint16_t wid,
              std::uint16_t hei, void* basicImage,
              SpinButtonCallback cb = {}, std::int32_t id = 0);

    // InitSpin: configure the text buffer size and reset value to 0.
    // 1:1 with legacy InitSpin(WORD spinStrSize, WORD strSize).
    void InitSpin(std::uint16_t spinStrSize, std::uint16_t strSize);

    // Value accessors. Both clamp to [m_minValue, m_maxValue].
    std::int32_t GetValue() const;
    void        SetValue(SPINUNIT value);

    // IncUnit / DecUnit: bump the value by m_Unit, saturating at the
    // range. Unsigned overflow is treated as "wrap to opposite end" to
    // match the legacy `value + m_Unit < value` check shape.
    void IncUnit();
    void DecUnit();

    // Min / max / unit setters & getters.
    void        SetUnit(SPINUNIT unit) noexcept  { m_Unit = unit; }
    SPINUNIT    GetUnit() const noexcept         { return m_Unit; }
    void        SetMin(SPINUNIT min) noexcept     { m_minValue = min; }
    SPINUNIT    GetMin() const noexcept           { return m_minValue; }
    void        SetMax(SPINUNIT max) noexcept     { m_maxValue = max; }
    SPINUNIT    GetMax() const noexcept           { return m_maxValue; }
    void        SetMinMax(SPINUNIT min = 0, SPINUNIT max = 100) noexcept {
        m_minValue = min;
        m_maxValue = max;
    }

    // AddSpinButton: register the up or down arrow button. The legacy
    // `Add(cWindow*)` only worked on cButton (filter by WT_BUTTON); the
    // modern port uses a typed enum to disambiguate the up vs down slot.
    // The caller (legacy GameIn / dialog resource loader) constructs
    // the cButton and passes the unique_ptr in. The first call wires
    // the up button, the second the down button. Subsequent calls are
    // silently ignored (matches legacy "if(!m_upBtn) ... else if(!m_downBtn) ..."
    // sequence).
    enum class SpinButtonKind { Up, Down };
    void AddSpinButton(std::unique_ptr<cButton> btn, SpinButtonKind kind);

    // Test accessors.
    const cButton* GetUpButton()   const noexcept { return m_upBtn.get(); }
    const cButton* GetDownButton() const noexcept { return m_downBtn.get(); }

private:
    // Parse the current cEditBox text as an integer (commas stripped).
    // Returns m_minValue if the text is empty / not numeric.
    SPINUNIT parseCurrentValue() const;

    // Format an integer as a decimal string with thousands-separator
    // commas (e.g. 12345 → "12,345"). Returns "" if the value is 0
    // and the legacy "0" was rendered as "0" — but we always emit
    // "0" to match legacy `itoa(value, ..., 10) + AddComma`.
    static std::string formatWithCommas(SPINUNIT value);

    std::unique_ptr<cButton> m_upBtn;
    std::unique_ptr<cButton> m_downBtn;

    SPINUNIT m_Unit      = 10;
    SPINUNIT m_minValue  = 0;
    SPINUNIT m_maxValue  = 100;
};

} // namespace mxh::ui
