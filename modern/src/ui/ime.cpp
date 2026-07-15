// mxh/ui/ime.cpp
// Phase 12.1: IME adapter dispatcher.
//
// Platform-agnostic: just stores the ImeAdapter singleton and
// routes dispatch_*() calls to whichever hooks are non-null. The
// platform-specific implementation (Win32 IMM, macOS TSM, etc.)
// lives in a separate .cpp and is the only one that knows about
// imm32 / TSM / libXIM / etc.
//
// All hooks default to no-op. installImeAdapter() is not
// synchronized — the host app is expected to call it once at
// startup (single-threaded).

#include "mxh/ui/ime.hpp"

namespace mxh::ui {

namespace {

ImeAdapter& adapter_singleton() {
    static ImeAdapter a;  // zero-initialized = all hooks null
    return a;
}

bool installed_flag = false;

}  // anonymous namespace

void installImeAdapter(const ImeAdapter& adapter) {
    adapter_singleton() = adapter;
    installed_flag =
           adapter.onFocusEdit != nullptr
        || adapter.onBlurEdit  != nullptr
        || adapter.onStartComposition != nullptr
        || adapter.acceptsIme  != nullptr;
}

bool isImeAdapterInstalled() {
    return installed_flag;
}

namespace detail {

void ime_dispatch_focus(ImeEditType edit_type, int x, int y, int font_idx) {
    if (adapter_singleton().onFocusEdit) {
        adapter_singleton().onFocusEdit(edit_type, x, y, font_idx);
    }
}

void ime_dispatch_blur(ImeEditType edit_type) {
    if (adapter_singleton().onBlurEdit) {
        adapter_singleton().onBlurEdit(edit_type);
    }
}

void ime_dispatch_start_composition() {
    if (adapter_singleton().onStartComposition) {
        adapter_singleton().onStartComposition();
    }
}

bool ime_accepts(ImeEditType edit_type) {
    if (adapter_singleton().acceptsIme) {
        return adapter_singleton().acceptsIme(edit_type);
    }
    // No explicit acceptsIme hook: default = "always accept" so
    // legacy edits (free text / textarea / spin) get IME. Hosts
    // that need numeric-only suppression install a hook that
    // returns false for ImeEditType::Number.
    return true;
}

}  // namespace detail
}  // namespace mxh::ui
