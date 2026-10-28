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
//   2. WE_* constants copied exactly from legacy cWindowDef.h.
//      Modern internal events use a compact enum, so bridge callers
//      translate explicitly instead of comparing raw values.
//   3. `cbWindowFunc` style registration — a `LegacyWindowFuncBinder`
//      utility that wraps a `(LONG id, void* parent, DWORD we)` callback
//      into a modern `std::function` that any modern cDialog child can
//      call via its own callback setters.
//   4. legacyEventToModern(we) / modernEventToLegacy(we) — explicit
//      translation between the legacy UI ABI and modern internals.
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
#include "legacy_window_event.hpp"

// Pull in the modern enum values for use as legacy aliases.
namespace mxh::ui::legacy {

// Exact values from legacy interface/cWindowDef.h WINDOW_EVENT.
constexpr std::uint32_t WE_NULL = legacy_window_event::kNull;
constexpr std::uint32_t WE_CLOSEWINDOW = legacy_window_event::kCloseWindow;
constexpr std::uint32_t WE_TOPWINDOW = legacy_window_event::kTopWindow;
constexpr std::uint32_t WE_CHANGETEXT = legacy_window_event::kChangeText;
constexpr std::uint32_t WE_RETURN = legacy_window_event::kReturn;
constexpr std::uint32_t WE_PUSHUP = legacy_window_event::kPushUp;
constexpr std::uint32_t WE_PUSHDOWN = legacy_window_event::kPushDown;
constexpr std::uint32_t WE_BTNCLICK = legacy_window_event::kButtonClick;
constexpr std::uint32_t WE_SPINBTNUP = legacy_window_event::kSpinButtonUp;
constexpr std::uint32_t WE_SPINBTNDOWN = legacy_window_event::kSpinButtonDown;
constexpr std::uint32_t WE_RBTNCLICK = legacy_window_event::kRightButtonClick;
constexpr std::uint32_t WE_LBTNCLICK = legacy_window_event::kLeftButtonClick;
constexpr std::uint32_t WE_COMBOBOXSELECT = legacy_window_event::kComboBoxSelect;
constexpr std::uint32_t WE_ROWCLICK = legacy_window_event::kRowClick;
constexpr std::uint32_t WE_CELLSELECT = legacy_window_event::kCellSelect;
constexpr std::uint32_t WE_CHECKED = legacy_window_event::kChecked;
constexpr std::uint32_t WE_NOTCHECKED = legacy_window_event::kNotChecked;
constexpr std::uint32_t WE_LBTNDBLCLICK = legacy_window_event::kLeftButtonDoubleClick;
constexpr std::uint32_t WE_RBTNDBLCLICK = legacy_window_event::kRightButtonDoubleClick;
constexpr std::uint32_t WE_DESTROY = legacy_window_event::kDestroy;
constexpr std::uint32_t WE_SETFOCUSON = legacy_window_event::kSetFocusOn;
constexpr std::uint32_t WE_MOUSEOVER = legacy_window_event::kMouseOver;
constexpr std::uint32_t WE_ACTIVEWINDOW = legacy_window_event::kActiveWindow;
constexpr std::uint32_t WE_ROWDBLCLICK = legacy_window_event::kRowDoubleClick;

constexpr std::uint32_t WE_KEYDOWN = static_cast<std::uint32_t>(cWindow::WindowEvent::KeyDown);
constexpr std::uint32_t WE_CHAR = static_cast<std::uint32_t>(cWindow::WindowEvent::Char_);

inline constexpr std::uint32_t modernEventToLegacy(cWindow::WindowEvent event) noexcept {
    switch (event) {
    case cWindow::WindowEvent::Null: return WE_NULL;
    case cWindow::WindowEvent::MouseMove: return WE_MOUSEOVER;
    case cWindow::WindowEvent::LButtonDown: return WE_LBTNCLICK;
    case cWindow::WindowEvent::LButtonClick: return WE_BTNCLICK;
    case cWindow::WindowEvent::RButtonDown:
    case cWindow::WindowEvent::RButtonClick: return WE_RBTNCLICK;
    case cWindow::WindowEvent::KeyDown: return WE_KEYDOWN;
    case cWindow::WindowEvent::Char_: return WE_CHAR;
    default: return WE_NULL;
    }
}

inline constexpr cWindow::WindowEvent legacyEventToModern(std::uint32_t event) noexcept {
    switch (event) {
    case WE_MOUSEOVER: return cWindow::WindowEvent::MouseMove;
    case WE_LBTNCLICK: return cWindow::WindowEvent::LButtonDown;
    case WE_BTNCLICK: return cWindow::WindowEvent::LButtonClick;
    case WE_RBTNCLICK: return cWindow::WindowEvent::RButtonClick;
    case WE_KEYDOWN: return cWindow::WindowEvent::KeyDown;
    case WE_CHAR: return cWindow::WindowEvent::Char_;
    default: return cWindow::WindowEvent::Null;
    }
}

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
            if (m_fn) m_fn(buttonId, userdata, WE_BTNCLICK);
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
            if (m_fn) m_fn(static_cast<std::int32_t>(self.id()), userdata, WE_CHANGETEXT);
        };
    }

private:
    cbWindowFunc m_fn;
};

} // namespace mxh::ui::legacy
