// ime_test.cpp - Phase 12.1 IME adapter dispatcher tests
//
// Covers modern/include/mxh/ui/ime.hpp + src/ui/ime.cpp. The Win32
// IMM reference adapter (src/ui/ime_win32_imm.cpp) is NOT tested
// here — it would require a real hwnd and Win32 message loop.
// The reference adapter is exercised manually via the host app
// (MoxianClient) when CJK builds run.
//
// These tests pin the dispatcher contract: install a custom
// adapter with lambdas, fire dispatch_* calls, observe the args
// the lambdas received, and confirm uninstall brings everything
// back to no-op.

#include "mxh/ui/ime.hpp"

#include <gtest/gtest.h>

#include <atomic>

namespace mxh::ui::test {

// Helper: a counting adapter that records the most-recent call
// and its args. Used by every test below.
struct CountingAdapter {
    std::atomic<int> focus_calls{0};
    std::atomic<int> blur_calls{0};
    std::atomic<int> start_calls{0};
    std::atomic<int> accept_calls{0};
    ImeEditType last_focus_type = ImeEditType::Other;
    ImeEditType last_blur_type  = ImeEditType::Other;
    int last_x = -1, last_y = -1, last_font = -1;
    bool last_accept = false;

    ImeAdapter make() = delete;  // unused — see module-level recorder
};

// The C-style function pointers in ImeAdapter can't capture
// state directly. The dispatcher tests use a small global
// recorder to observe the args.
namespace {

struct Recorder {
    int focus_calls = 0;
    int blur_calls = 0;
    int start_calls = 0;
    int accept_calls = 0;
    ImeEditType last_focus_type = ImeEditType::Other;
    ImeEditType last_blur_type  = ImeEditType::Other;
    int last_x = 0, last_y = 0, last_font = 0;
    bool last_accept = true;
    Recorder* prev = nullptr;
};

Recorder* g_recorder = nullptr;

void on_focus(ImeEditType t, int x, int y, int f) {
    if (!g_recorder) return;
    ++g_recorder->focus_calls;
    g_recorder->last_focus_type = t;
    g_recorder->last_x = x;
    g_recorder->last_y = y;
    g_recorder->last_font = f;
}

void on_blur(ImeEditType t) {
    if (!g_recorder) return;
    ++g_recorder->blur_calls;
    g_recorder->last_blur_type = t;
}

void on_start() {
    if (!g_recorder) return;
    ++g_recorder->start_calls;
}

bool on_accept(ImeEditType t) {
    if (!g_recorder) return true;
    ++g_recorder->accept_calls;
    g_recorder->last_focus_type = t;  // reuse field for "queried"
    return t != ImeEditType::Number;
}

ImeAdapter make_full_adapter() {
    ImeAdapter a;
    a.onFocusEdit        = &on_focus;
    a.onBlurEdit         = &on_blur;
    a.onStartComposition = &on_start;
    a.acceptsIme         = &on_accept;
    return a;
}

class ImeTest : public ::testing::Test {
protected:
    void SetUp() override {
        recorder.prev = g_recorder;
        g_recorder = &recorder;
        // Start with a clean adapter (no hooks) so each test
        // explicitly installs.
        installImeAdapter(ImeAdapter{});
    }
    void TearDown() override {
        installImeAdapter(ImeAdapter{});
        g_recorder = recorder.prev;
    }
    Recorder recorder{};
};

}  // namespace

// ===========================================================================
// Install / uninstall
// ===========================================================================

TEST_F(ImeTest, DefaultIsNotInstalled) {
    EXPECT_FALSE(isImeAdapterInstalled());
}

TEST_F(ImeTest, InstallFullAdapterMarksInstalled) {
    installImeAdapter(make_full_adapter());
    EXPECT_TRUE(isImeAdapterInstalled());
}

TEST_F(ImeTest, UninstallClearsHooks) {
    installImeAdapter(make_full_adapter());
    EXPECT_TRUE(isImeAdapterInstalled());
    installImeAdapter(ImeAdapter{});
    EXPECT_FALSE(isImeAdapterInstalled());
    // Dispatching after uninstall must be a no-op.
    detail::ime_dispatch_focus(ImeEditType::EditBox, 10, 20, 3);
    EXPECT_EQ(recorder.focus_calls, 0);
}

// ===========================================================================
// onFocusEdit dispatch
// ===========================================================================

TEST_F(ImeTest, FocusDispatchForwardsArgs) {
    installImeAdapter(make_full_adapter());
    detail::ime_dispatch_focus(ImeEditType::TextArea, 100, 50, 7);
    EXPECT_EQ(recorder.focus_calls, 1);
    EXPECT_EQ(recorder.last_focus_type, ImeEditType::TextArea);
    EXPECT_EQ(recorder.last_x, 100);
    EXPECT_EQ(recorder.last_y, 50);
    EXPECT_EQ(recorder.last_font, 7);
}

TEST_F(ImeTest, FocusDispatchAccumulates) {
    installImeAdapter(make_full_adapter());
    detail::ime_dispatch_focus(ImeEditType::EditBox, 1, 2, 0);
    detail::ime_dispatch_focus(ImeEditType::TextArea, 3, 4, 0);
    detail::ime_dispatch_focus(ImeEditType::Spin, 5, 6, 0);
    EXPECT_EQ(recorder.focus_calls, 3);
    EXPECT_EQ(recorder.last_focus_type, ImeEditType::Spin);
}

TEST_F(ImeTest, FocusDispatchWithNullHookIsNoOp) {
    ImeAdapter a;
    a.onFocusEdit = nullptr;  // explicit
    installImeAdapter(a);
    detail::ime_dispatch_focus(ImeEditType::EditBox, 1, 2, 3);
    EXPECT_EQ(recorder.focus_calls, 0);
    SUCCEED();
}

// ===========================================================================
// onBlurEdit dispatch
// ===========================================================================

TEST_F(ImeTest, BlurDispatchForwardsType) {
    installImeAdapter(make_full_adapter());
    detail::ime_dispatch_blur(ImeEditType::TextArea);
    EXPECT_EQ(recorder.blur_calls, 1);
    EXPECT_EQ(recorder.last_blur_type, ImeEditType::TextArea);
}

TEST_F(ImeTest, BlurDispatchWithNullHookIsNoOp) {
    ImeAdapter a;
    a.onBlurEdit = nullptr;
    installImeAdapter(a);
    detail::ime_dispatch_blur(ImeEditType::EditBox);
    EXPECT_EQ(recorder.blur_calls, 0);
}

// ===========================================================================
// onStartComposition dispatch
// ===========================================================================

TEST_F(ImeTest, StartCompositionDispatch) {
    installImeAdapter(make_full_adapter());
    detail::ime_dispatch_start_composition();
    EXPECT_EQ(recorder.start_calls, 1);
    detail::ime_dispatch_start_composition();
    EXPECT_EQ(recorder.start_calls, 2);
}

// ===========================================================================
// acceptsIme
// ===========================================================================

TEST_F(ImeTest, AcceptsImeWithHookConsultsHook) {
    installImeAdapter(make_full_adapter());
    // The default on_accept rejects Number, accepts everything else.
    EXPECT_TRUE(detail::ime_accepts(ImeEditType::EditBox));
    EXPECT_TRUE(detail::ime_accepts(ImeEditType::TextArea));
    EXPECT_TRUE(detail::ime_accepts(ImeEditType::Spin));
    EXPECT_FALSE(detail::ime_accepts(ImeEditType::Number));
    EXPECT_EQ(recorder.accept_calls, 4);
}

TEST_F(ImeTest, AcceptsImeWithoutHookDefaultsToTrue) {
    // No acceptsIme hook installed → default = always accept.
    // This matches legacy behaviour for non-numeric edits; the
    // Number suppression is opt-in via the hook.
    ImeAdapter a;
    installImeAdapter(a);
    EXPECT_TRUE(detail::ime_accepts(ImeEditType::Number));
    EXPECT_TRUE(detail::ime_accepts(ImeEditType::EditBox));
}

// ===========================================================================
// Hook independence
// ===========================================================================

TEST_F(ImeTest, EachHookCanBeInstalledIndependently) {
    // Install only onFocusEdit. The other three hooks should
    // remain null and dispatch to them must be a no-op.
    ImeAdapter a;
    a.onFocusEdit = &on_focus;
    installImeAdapter(a);
    EXPECT_TRUE(isImeAdapterInstalled());
    detail::ime_dispatch_focus(ImeEditType::EditBox, 1, 2, 3);
    detail::ime_dispatch_blur(ImeEditType::EditBox);
    detail::ime_dispatch_start_composition();
    detail::ime_accepts(ImeEditType::EditBox);
    EXPECT_EQ(recorder.focus_calls, 1);
    EXPECT_EQ(recorder.blur_calls, 0);
    EXPECT_EQ(recorder.start_calls, 0);
    EXPECT_EQ(recorder.accept_calls, 0);
}

// ===========================================================================
// Re-install replaces previous adapter
// ===========================================================================

TEST_F(ImeTest, ReinstallReplacesHooks) {
    // First install with only on_focus.
    ImeAdapter a1;
    a1.onFocusEdit = &on_focus;
    installImeAdapter(a1);
    detail::ime_dispatch_focus(ImeEditType::EditBox, 1, 1, 0);
    EXPECT_EQ(recorder.focus_calls, 1);

    // Now install with only on_blur. The previous on_focus must
    // no longer fire.
    ImeAdapter a2;
    a2.onBlurEdit = &on_blur;
    installImeAdapter(a2);
    detail::ime_dispatch_focus(ImeEditType::EditBox, 2, 2, 0);
    detail::ime_dispatch_blur(ImeEditType::TextArea);
    EXPECT_EQ(recorder.focus_calls, 1) << "stale onFocus should not fire after reinstall";
    EXPECT_EQ(recorder.blur_calls, 1);
}

}  // namespace mxh::ui::test
