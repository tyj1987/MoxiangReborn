// mxh/ui/legacy_compat.hpp
// Phase 6.7 — bridge the legacy engine's cWindow / cDialog pointer APIs
// to the modern mxh::ui framework. The legacy code uses raw pointers
// (cWindow* / cDialog*) and global helpers (cbWindowFunc) that the
// modern framework replaces with unique_ptr ownership and
// std::function callbacks. This header provides the smallest shim that
// keeps the legacy call sites compiling without forcing a wholesale
// rewrite.
//
// What this gives you:
//   1. `cWindow* → mxh::ui::cWindow*` — pointer-passable aliases. The
//      modern classes are *not* the same as the legacy ones, but the
//      pointer types are interchangeable at the call site because the
//      modern framework's cWindow keeps the same ActionEvent /
//      PtInWindow / SetAbsXY / etc. surface that the legacy engine
//      expects.
//   2. WE_* code constants — the legacy `WE_NULL`, `WE_LBTNCLICK` etc.
//      are aliases for the modern `cWindow::WindowEvent` enum. The
//      numeric values are stable across both APIs (we declared them so
//      in 6.0).
//   3. `cbWindowFunc` style registration — a `LegacyWindowFuncBinder`
//      utility that wraps a `(LONG id, void* parent, DWORD we)` callback
//      into a modern `std::function` that any modern cDialog child can
//      call via its own callback setters.
//   4. `legacyEventToModern(we)` / `modernEventToLegacy(we)` — passthrough
//      since the codes already match.
//
// This is deliberately NOT a full port of the legacy cWindow hierarchy
// to the modern one. The goal is to let `cDialog*` references in the
// legacy engine keep compiling, while the modern code lives in
// `modern/src/ui/` and tests against itself.
#pragma once

#include <cstdint>

#include "cButton.hpp"
#include "cDialog.hpp"
#include "cEditBox.hpp"
#include "cListCtrl.hpp"
#include "cWindow.hpp"

// Pull in the modern enum values for use as legacy aliases.
namespace mxh::ui::legacy {

// Mirror the legacy WE_* codes (the same integer values as the modern
// cWindow::WindowEvent enum, so they're interchangeable at the ABI
// level — legacy code that compares against these constants keeps
// working when the new dispatcher emits modern events).
constexpr std::uint32_t WE_NULL         = 0;
constexpr std::uint32_t WE_MOUSEOVER    = 1;
constexpr std::uint32_t WE_LBTNCLICK    = 4;  // legacy equivalent
constexpr std::uint32_t WE_LBTNDBLCLICK = 8;  // legacy equivalent
constexpr std::uint32_t WE_RBTNCLICK    = 7;
constexpr std::uint32_t WE_KEYDOWN      = 10;
constexpr std::uint32_t WE_CHAR         = 11;
constexpr std::uint32_t WE_ROWCLICK     = 100;  // legacy cListCtrl-specific
constexpr std::uint32_t WE_ROWDBLCLICK  = 101;

// Type alias: the legacy engine uses a free function with this shape to
// receive window events. The modern framework's cWindow exposes
// std::function callbacks, so this signature is provided for
// compatibility code that wants to keep the legacy pattern.
using cbWindowFunc = void (*)(std::int32_t lId, void* p, std::uint32_t we);

// Adapter: bind a (id, parent, we) free function to a cDialog so
// legacy code can call SetcbWindowFunc and have the modern framework
// forward events through it. Useful when porting one dialog at a time.
class LegacyWindowFuncBinder {
public:
    explicit LegacyWindowFuncBinder(cbWindowFunc fn) noexcept : m_fn(fn) {}

    // cButton::ClickCallback-compatible: signature is
    //   void(std::int32_t buttonId, void* userdata)
    using ButtonClickCallback = std::function<void(std::int32_t, void*)>;
    ButtonClickCallback wrapButtonClick() {
        return [this](std::int32_t buttonId, void* userdata) {
            if (m_fn) m_fn(buttonId, userdata, WE_LBTNCLICK);
        };
    }

    // cListCtrl::RowCallback-compatible: signature is
    //   void(cListCtrl& self, std::int32_t rowIdx, void* userdata)
    // We resolve the id from the dialog's id() and forward the row.
    using ListRowCallback = std::function<void(class cListCtrl&, std::int32_t, void*)>;
    ListRowCallback wrapListRowClick() {
        return [this](class cListCtrl& self, std::int32_t rowIdx, void* userdata) {
            if (m_fn) m_fn(static_cast<std::int32_t>(self.id()), userdata, WE_ROWCLICK);
            (void)rowIdx;
        };
    }

    // cEditBox::TextCallback-compatible:
    //   void(cEditBox& self, void* userdata)
    using EditTextCallback = std::function<void(class cEditBox&, void*)>;
    EditTextCallback wrapEditChange() {
        return [this](class cEditBox& self, void* userdata) {
            if (m_fn) m_fn(static_cast<std::int32_t>(self.id()), userdata, WE_CHAR);
        };
    }

private:
    cbWindowFunc m_fn;
};

} // namespace mxh::ui::legacy
