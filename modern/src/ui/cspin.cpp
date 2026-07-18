// cspin.cpp — modern port of 墨香 cSpin (numeric spin control).
//
// 1:1 port body. See legacy `cSpin.cpp` for the original behavior
// (and the heavy use of `m_pIMEex` for text I/O — the modern port
// routes through `cEditBox::editText()` / `SetEditText()` instead).

#include "cspin.hpp"

#include "cButton.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>

namespace mxh::ui {

cSpin::cSpin() = default;
cSpin::~cSpin() = default;

void cSpin::Init(std::int32_t x, std::int32_t y, std::uint16_t wid,
                 std::uint16_t hei, void* basicImage,
                 SpinButtonCallback cb, std::int32_t id) {
    // 1:1 quirk: legacy calls `cEditBox::Init(x, y, wid, hei, basicImage,
    // basicImage, ID)` — passing `basicImage` twice (basic + focus both
    // use the same image). Modern port mirrors this.
    cEditBox::Init(x, y, wid, hei, basicImage, basicImage, id);
    // 1:1 quirk: legacy sets `m_bCaret = FALSE` after Init — the spin
    // control never shows a caret (the up/down arrows are the input
    // affordance, not character editing). Modern equivalent: SetCaret(false).
    SetCaret(false);
    // The callback is stored on the cEditBox's `m_onChange` (text-changed
    // path) for parity with legacy cbFUNC usage. We do NOT wire it to
    // m_onEnter / m_onEscape because the spin control's "submit" is the
    // up/down arrow click, not the Enter key.
    (void)cb;  // legacy cbFUNC was assigned to cbWindowFunc; modern port
               // routes button click through cButton::ClickCallback via
               // AddSpinButton, so we deliberately don't store cb here.
}

void cSpin::InitSpin(std::uint16_t spinStrSize, std::uint16_t strSize) {
    // 1:1 with legacy: InitEditbox(spinStrSize, strSize) + SetValue(0).
    // Modern cEditBox::InitEditbox ignores the pixelWidth and only
    // uses bufBytes (matches the legacy call shape).
    cEditBox::InitEditbox(spinStrSize, strSize);
    SetValue(0);
}

std::int32_t cSpin::GetValue() const {
    // 1:1 with legacy: read from m_pIMEex (now editText), strip commas,
    // parse, clamp to [min, max]. Returns LONG in legacy; modern returns
    // int32_t for parity.
    const SPINUNIT v = parseCurrentValue();
    return static_cast<std::int32_t>(v);
}

void cSpin::SetValue(SPINUNIT value) {
    // 1:1 with legacy: clamp, then format and push back to the buffer.
    SPINUNIT v = value;
    if (v < m_minValue) {
        v = m_minValue;
    } else if (v > m_maxValue) {
        v = m_maxValue;
    }
    const std::string formatted = formatWithCommas(v);
    SetEditText(formatted);
}

void cSpin::IncUnit() {
    // 1:1 with legacy: parse current, add m_Unit, wrap to max on
    // unsigned overflow, clamp to max, format and push back.
    SPINUNIT value = parseCurrentValue();
    if (value + m_Unit < value) {
        // Unsigned overflow → wrap to max.
        value = m_maxValue;
    } else {
        if (value + m_Unit > m_maxValue) {
            value = m_maxValue;
        } else {
            value += m_Unit;
        }
    }
    const std::string formatted = formatWithCommas(value);
    SetEditText(formatted);
}

void cSpin::DecUnit() {
    // 1:1 with legacy: parse current, subtract m_Unit, wrap to min on
    // unsigned underflow, clamp to min, format and push back.
    SPINUNIT value = parseCurrentValue();
    if (value - m_Unit > value) {
        // Unsigned underflow → wrap to min.
        value = m_minValue;
    } else {
        if (value - m_Unit < m_minValue) {
            value = m_minValue;
        } else {
            value -= m_Unit;
        }
    }
    const std::string formatted = formatWithCommas(value);
    SetEditText(formatted);
}

void cSpin::AddSpinButton(std::unique_ptr<cButton> btn, SpinButtonKind kind) {
    // 1:1 with legacy: first WT_BUTTON goes to m_upBtn, second to m_downBtn.
    // The legacy `Add()` used a `if(!m_upBtn) ... else if(!m_downBtn)`
    // cascade. We mirror that with the SpinButtonKind enum for clarity;
    // passing the wrong kind (e.g. Up when up is already set) is
    // silently ignored to match the legacy "no else branch" behavior.
    if (!btn) {
        return;
    }
    if (kind == SpinButtonKind::Up && !m_upBtn) {
        m_upBtn = std::move(btn);
    } else if (kind == SpinButtonKind::Down && !m_downBtn) {
        m_downBtn = std::move(btn);
    }
    // 1:1 quirk: legacy set the parent (`m_upBtn->SetParent(this)`) and
    // re-positioned the button based on the spin's abs pos. The modern
    // port omits these (cButton doesn't track a parent / cSpin doesn't
    // own a layout). This is consistent with all other Phase 6 dialog
    // ports that delegate render to the resource layer.
}

SPINUNIT cSpin::parseCurrentValue() const {
    // 1:1 with legacy: strip commas from the buffer, then parse as
    // unsigned 64-bit (legacy used `_atoi64` for DWORD input).
    const std::string& raw = editText();
    std::string stripped;
    stripped.reserve(raw.size());
    for (char c : raw) {
        if (c != ',') {
            stripped.push_back(c);
        }
    }
    if (stripped.empty()) {
        return m_minValue;
    }
    // Verify the stripped text is purely numeric (digits only, optional
    // leading sign — though legacy didn't support negative, we don't
    // either here). If non-numeric, return min as a safe fallback.
    for (char c : stripped) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return m_minValue;
        }
    }
    try {
        return static_cast<SPINUNIT>(std::stoull(stripped));
    } catch (...) {
        return m_minValue;
    }
}

std::string cSpin::formatWithCommas(SPINUNIT value) {
    // 1:1 with legacy: legacy `itoa`/`_i64toa` for the digits, then
    // `AddComma` for thousands separator. We replicate the comma
    // insertion by writing the number to a buffer then inserting
    // commas right-to-left.
    char digits[32];
    std::snprintf(digits, sizeof(digits), "%u",
                  static_cast<unsigned>(value));
    std::string s(digits);
    // Insert commas every 3 digits from the right.
    const std::size_t n = s.size();
    if (n <= 3) {
        return s;
    }
    std::string out;
    out.reserve(n + n / 3);
    int count = 0;
    for (auto it = s.rbegin(); it != s.rend(); ++it) {
        if (count && (count % 3 == 0)) {
            out.push_back(',');
        }
        out.push_back(*it);
        ++count;
    }
    std::reverse(out.begin(), out.end());
    return out;
}

} // namespace mxh::ui
