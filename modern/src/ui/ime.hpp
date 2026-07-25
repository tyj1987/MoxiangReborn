// mxh/ui/ime.hpp
// Phase 12.1: IME (Input Method Editor) adapter interface.
//
// The 1:1 legacy client uses Win32 IMM (imm32.lib) APIs to position
// the IME composition window next to the focused editbox caret, set
// the composition font, and toggle IME on/off when focus changes.
// Modern CJK clients on Windows continue to use IMM (the OS still
// ships it alongside TSF).
//
// This header defines a small adapter interface so the host app can
// install a platform-specific implementation without polluting
// mxh_ui (which is platform-agnostic). The interface intentionally
// avoids touching cEditBox / cWindowManager — those would require
// passing a caret position + edit type, which the legacy code did
// inline at the WM_IME_STARTCOMPOSITION message handler. Modern
// host apps (MoxianClient) get the caret position from their own
// message loop and pass it explicitly to the adapter.
//
// Hooks:
//   - onFocusEdit(edit_type, caret_x, caret_y, font_idx)
//       Called when an editbox gains focus. The host can use
//       edit_type (e.g. "edit", "spin", "textarea", "number")
//       to decide whether to enable IME (e.g. VCM_NUMBER should
//       disable it), position the composition window at the
//       caret, and set the composition font.
//
//   - onBlurEdit(edit_type)
//       Called when the focused editbox loses focus. Hosts use
//       this to close the IME candidate window and commit /
//       cancel any pending composition.
//
//   - onStartComposition()
//       Optional hook called for WM_IME_STARTCOMPOSITION. The
//       host's message loop is the right place to intercept this;
//       the adapter just stores a flag so the next onFocusEdit
//       can avoid re-positioning the window.
//
// All hooks are no-ops when no adapter is installed.
//
// Threading: install/uninstall are not synchronized. The host app
// should install the adapter once at startup (single-threaded)
// and never replace it at runtime.

#pragma once

#include <cstdint>

// Forward declarations for platform types. The Win32 IMM
// reference adapter (ime_win32_imm.cpp) declares its own
// installWin32Ime(HWND) that takes the concrete HWND; this
// header keeps the interface HWND-free so non-Win32 platforms
// don't see <windows.h> through mxh/ui/ime.hpp.
struct HWND__;  // Win32 HWND forward decl
using HWND = HWND__*;

namespace mxh::ui {

// Edit-box type tags (subset of the legacy WINDOW_TYPE / cEditBox
// classification that the legacy IME code branched on). Strings
// instead of an enum so the adapter is forward-compatible with
// new edit types without rebuilding the interface.
enum class ImeEditType : int {
    EditBox     = 0,   // cEditBox (free text)
    Spin        = 1,   // cSpin (numeric, +/- buttons)
    TextArea    = 2,   // cTextArea (multi-line)
    Number      = 3,   // numeric-only edit (legacy VCM_NUMBER);
                       // hosts should typically disable IME here
    Other       = 99,  // unknown / not an edit
};

// Adapter callbacks. All hooks may be null. The host app installs
// a fully-bound struct via installImeAdapter().
struct ImeAdapter {
    // Called when an editbox gains focus.
    //   edit_type : see ImeEditType
    //   caret_x, caret_y : caret position in client-area pixels
    //   font_idx   : legacy font index; hosts use this to look up
    //                the composition font in cFontManager
    void (*onFocusEdit)(ImeEditType edit_type,
                        int caret_x, int caret_y,
                        int font_idx) = nullptr;

    // Called when the focused editbox loses focus.
    void (*onBlurEdit)(ImeEditType edit_type) = nullptr;

    // Optional: WM_IME_STARTCOMPOSITION notification. Default
    // null because most hosts handle this directly in their
    // message loop and don't need the adapter to re-fire.
    void (*onStartComposition)() = nullptr;

    // Optional: query — should this edit_type accept IME input?
    // Default null means "always accept" (i.e. legacy behaviour
    // for non-numeric edits). Hosts that need VCM_NUMBER-style
    // suppression set this to a function that returns false for
    // ImeEditType::Number.
    bool (*acceptsIme)(ImeEditType edit_type) = nullptr;
};

// Install an IME adapter. Pass a default-initialized ImeAdapter
// to clear the current adapter (all hooks null = pure no-op).
void installImeAdapter(const ImeAdapter& adapter);

// True iff installImeAdapter() has been called with at least one
// non-null hook. Useful for tests and for hosts that want to
// short-circuit IME handling when no adapter is bound.
bool isImeAdapterInstalled();

// Win32 IMM reference adapter. Defined in ime_win32_imm.cpp
// (compiled only on _WIN32). The HWND parameter is the host's
// main window that ImmGetContext / ImmSetCompositionWindow will
// target. Pass nullptr to uninstall.
//
// On non-Windows platforms these symbols are not defined; the
// CMake glue should not link ime_win32_imm.cpp on those.
void installWin32Ime(HWND hwnd);
void uninstallWin32Ime();

// Internal accessors used by the hooks below. Not part of the
// public API but exposed so test code can drive the adapter
// without touching the host's install path. Production code
// should use installImeAdapter() + the legacy message loop.
namespace detail {
void ime_dispatch_focus(ImeEditType edit_type, int x, int y, int font_idx);
void ime_dispatch_blur(ImeEditType edit_type);
void ime_dispatch_start_composition();
bool ime_accepts(ImeEditType edit_type);
}  // namespace detail

}  // namespace mxh::ui
