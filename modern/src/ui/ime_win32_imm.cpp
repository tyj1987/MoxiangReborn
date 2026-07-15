// mxh/ui/ime_win32_imm.cpp
// Phase 12.1: Win32 IMM reference implementation for the IME
// adapter interface (mxh::ui::ImeAdapter).
//
// Build notes:
//   * Compiled only on _WIN32 (the imm32.lib + windows.h headers
//     don't exist elsewhere). The CMake glue adds this .cpp to
//     mxh_ui's source list conditionally on WIN32.
//   * Pulls in imm32.lib and the Win32 GDI headers (for LOGFONT
//     when setting the composition font). The host app is expected
//     to provide the hwnd and font via installWin32Ime().
//
// Legacy mapping:
//   TAIWAN / HK / JAPAN  builds  called ImmGetContext on
//   focus + ImmSetOpenStatus(TRUE) + ImmNotifyIME(CPS_CANCEL) +
//   ImmSetCompositionFont / ImmSetCompositionWindow. The
//   reference adapter below mirrors that pattern: it stores the
//   focused edit type + caret position on focus, and uses
//   ImmSetCompositionWindow to anchor the candidate window at
//   the caret on the next WM_IME_STARTCOMPOSITION message.

#include "mxh/ui/ime.hpp"

#ifdef _WIN32

#include <windows.h>
#include <imm.h>

namespace mxh::ui {

namespace {

// Per-adapter state. The legacy code uses globals; we keep that
// pattern to minimize structural changes (one host = one adapter
// at a time).
struct Win32ImeState {
    HWND hwnd = nullptr;          // host app's main window
    ImeEditType focused = ImeEditType::Other;
    int caret_x = 0;
    int caret_y = 0;
    int font_idx = 0;
    bool composition_open = false;  // tracks ImmSetOpenStatus
};

Win32ImeState& state() {
    static Win32ImeState s;
    return s;
}

// Internal: apply the current state to Win32 IMM (call after
// focus / caret / hwnd change). Idempotent. Skips if the host
// hasn't installed an hwnd yet.
void apply_state_to_imm() {
    Win32ImeState& s = state();
    if (!s.hwnd) return;
    HIMC himc = ImmGetContext(s.hwnd);
    if (!himc) return;

    // Open / close based on edit type. Number-only edits should
    // not have IME on (legacy VCM_NUMBER suppression).
    const bool wants_ime = (s.focused != ImeEditType::Number)
                        && (s.focused != ImeEditType::Other);
    ImmSetOpenStatus(himc, wants_ime);

    // Position the composition window at the caret. The legacy
    // code used a 512x20 rect right of the caret; we use the
    // caret itself as the top-left and the right/bottom as a
    // 512x20 box. The actual OS candidate window follows the
    // caret automatically in most IMEs, but the composition
    // window we set here is what TSF / IMM uses for the
    // candidate-list drop-down anchor.
    if (wants_ime) {
        COMPOSITIONFORM cf{};
        cf.dwStyle = CFS_RECT;
        cf.ptCurrentPos.x = s.caret_x;
        cf.ptCurrentPos.y = s.caret_y;
        cf.rcArea.left   = s.caret_x;
        cf.rcArea.top    = s.caret_y;
        cf.rcArea.right  = s.caret_x + 512;
        cf.rcArea.bottom = s.caret_y + 20;
        ImmSetCompositionWindow(himc, &cf);
    }

    ImmReleaseContext(s.hwnd, himc);
}

void on_focus_edit(ImeEditType edit_type, int x, int y, int font_idx) {
    Win32ImeState& s = state();
    s.focused  = edit_type;
    s.caret_x  = x;
    s.caret_y  = y;
    s.font_idx = font_idx;
    apply_state_to_imm();
}

void on_blur_edit(ImeEditType edit_type) {
    (void)edit_type;
    Win32ImeState& s = state();
    s.focused = ImeEditType::Other;
    if (!s.hwnd) return;
    HIMC himc = ImmGetContext(s.hwnd);
    if (!himc) return;
    ImmSetOpenStatus(himc, FALSE);
    ImmNotifyIME(himc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
    ImmNotifyIME(himc, NI_CLOSECANDIDATE, 0, 0);
    ImmReleaseContext(s.hwnd, himc);
    s.composition_open = false;
}

void on_start_composition() {
    state().composition_open = true;
}

bool accepts_ime(ImeEditType edit_type) {
    // Legacy behaviour: numeric-only / non-edit windows reject IME.
    return edit_type != ImeEditType::Number
        && edit_type != ImeEditType::Other;
}

ImeAdapter make_win32_adapter() {
    ImeAdapter a;
    a.onFocusEdit        = &on_focus_edit;
    a.onBlurEdit         = &on_blur_edit;
    a.onStartComposition = &on_start_composition;
    a.acceptsIme         = &accepts_ime;
    return a;
}

}  // anonymous namespace

// Host-callable entry point. Pass the main window handle the IME
// should target. Subsequent installImeAdapter() calls will use
// this hwnd until uninstallWin32Ime() is called.
void installWin32Ime(HWND hwnd) {
    state().hwnd = hwnd;
    installImeAdapter(make_win32_adapter());
}

void uninstallWin32Ime() {
    state() = Win32ImeState{};  // reset all fields
    installImeAdapter(ImeAdapter{});  // clear hooks
}

}  // namespace mxh::ui

#endif  // _WIN32
