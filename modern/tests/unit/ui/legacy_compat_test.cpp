// tests/unit/ui/legacy_compat_test.cpp
// Phase 6.7 unit tests for the modern <-> legacy bridge.
#include <gtest/gtest.h>

#include <atomic>
#include <functional>

#include "cButton.hpp"
#include "cDialog.hpp"
#include "cListCtrl.hpp"
#include "cWindow.hpp"
#include "legacy_compat.hpp"

using mxh::ui::cButton;
using mxh::ui::cDialog;
using mxh::ui::cListCtrl;
using mxh::ui::cWindow;

namespace {
int g_basicImg = 1;

std::function<void(std::int32_t, void*, std::uint32_t)> s_fn;

void shim(std::int32_t id, void* p, std::uint32_t we) { s_fn(id, p, we); }
} // namespace

TEST(LegacyCompat, WE_ConstantsMatchLegacy) {
    EXPECT_EQ(mxh::ui::legacy::WE_NULL, 0u);
    EXPECT_EQ(mxh::ui::legacy::WE_PUSHUP, 16u);
    EXPECT_EQ(mxh::ui::legacy::WE_PUSHDOWN, 32u);
    EXPECT_EQ(mxh::ui::legacy::WE_BTNCLICK, 64u);
    EXPECT_EQ(mxh::ui::legacy::WE_RBTNCLICK, 512u);
    EXPECT_EQ(mxh::ui::legacy::WE_LBTNCLICK, 1024u);
    EXPECT_EQ(mxh::ui::legacy::WE_ROWCLICK, 4096u);
    EXPECT_EQ(mxh::ui::legacy::WE_LBTNDBLCLICK, 65536u);
    EXPECT_EQ(mxh::ui::legacy::WE_MOUSEOVER, 1048576u);
    EXPECT_EQ(mxh::ui::legacy::WE_ROWDBLCLICK, 4194304u);
}

TEST(LegacyCompat, LegacyCbWindowFuncIsCallable) {
    int callCount = 0;
    std::int32_t lastId = 0;
    std::uint32_t lastWe = 0;
    s_fn = [&](std::int32_t id, void* p, std::uint32_t we) {
        ++callCount;
        lastId = id;
        lastWe = we;
        (void)p;
    };
    auto legacy = static_cast<mxh::ui::legacy::cbWindowFunc>(&shim);
    legacy(42, nullptr, mxh::ui::legacy::WE_LBTNCLICK);
    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(lastId, 42);
    EXPECT_EQ(lastWe, mxh::ui::legacy::WE_LBTNCLICK);
}

TEST(LegacyCompat, BinderWrapsButtonClick) {
    static std::int32_t s_legacyId = 0;
    static std::uint32_t s_legacyWe = 0;
    static std::int32_t s_legacyCount = 0;
    s_fn = [](std::int32_t id, void*, std::uint32_t we) {
        s_legacyId = id; s_legacyWe = we; ++s_legacyCount;
    };
    auto local_shim = [](std::int32_t id, void* p, std::uint32_t we) { s_fn(id, p, we); };
    mxh::ui::legacy::LegacyWindowFuncBinder binder(local_shim);
    auto wrapped = binder.wrapButtonClick();
    cButton btn;
    btn.Init(0, 0, 100, 100, nullptr, nullptr, nullptr, wrapped, nullptr, 7);
    s_legacyCount = 0;
    btn.ActionEvent(50, 50, cWindow::MouseFlagLButton);  // press
    btn.ActionEvent(50, 50, 0);                          // release -> click
    EXPECT_EQ(s_legacyCount, 1);
    EXPECT_EQ(s_legacyId, 7);
    EXPECT_EQ(s_legacyWe, mxh::ui::legacy::WE_BTNCLICK);
}

TEST(LegacyCompat, DialogChildAddWorksThroughShimTypes) {
    // The modern cDialog accepts unique_ptr<cWindow>-derived children.
    // The legacy code stores raw cWindow* and cDialog* in a list; the
    // shim here only confirms the modern API still works end-to-end so
    // the legacy engine can keep using cDialog* in its declarations.
    cDialog d;
    d.Init(0, 0, 200, 200, &g_basicImg, 1);
    cDialog* dPtr = &d;
    auto btn = std::make_unique<cButton>();
    btn->Init(0, 0, 50, 20, nullptr, nullptr, nullptr, nullptr, nullptr, 5);
    cButton* rawBtn = btn.get();
    d.Add(std::move(btn));
    // The legacy engine would then do: cWindow* w = dPtr->GetWindowForID(5);
    // The modern equivalent is findWindowById, which cDialog already
    // delegates to the tree.
    EXPECT_EQ(dPtr->findWindowById(5), rawBtn);
}

TEST(LegacyCompat, ListCtrlRowClickUsesLegacyCode) {
    // The legacy cListCtrl calls cbWindowFunc with WE_ROWCLICK on
    // click. The modern framework uses RowCallback; the shim shows
    // that both are interchangeable from the call-site perspective.
    cDialog d;
    d.Init(0, 0, 200, 300, &g_basicImg, 1);
    auto list = std::make_unique<cListCtrl>();
    list->Init(0, 0, 200, 300, &g_basicImg, 2);
    list->InitListCtrlImage(nullptr, 25, nullptr, 20, nullptr);
    list->InitListCtrl(1, 5);
    list->AddRow({{"row1"}});
    int clickCount = 0;
    list->SetClickFunc([&](cListCtrl&, std::int32_t row, void*) {
        ++clickCount; EXPECT_EQ(row, 0);
    });
    d.Add(std::move(list));
    d.SetActive(true);
    // Click on row 0 (y in (25, 45]).
    d.ActionEvent(50, 30, cWindow::MouseFlagLButton);
    EXPECT_EQ(clickCount, 1);
}

TEST(LegacyCompat, ModernEventsConvertExplicitlyToLegacy) {
    EXPECT_EQ(mxh::ui::legacy::modernEventToLegacy(cWindow::WindowEvent::Null),
              mxh::ui::legacy::WE_NULL);
    EXPECT_EQ(mxh::ui::legacy::modernEventToLegacy(cWindow::WindowEvent::MouseMove),
              mxh::ui::legacy::WE_MOUSEOVER);
    EXPECT_EQ(mxh::ui::legacy::modernEventToLegacy(cWindow::WindowEvent::LButtonDown),
              mxh::ui::legacy::WE_LBTNCLICK);
    EXPECT_EQ(mxh::ui::legacy::modernEventToLegacy(cWindow::WindowEvent::LButtonClick),
              mxh::ui::legacy::WE_BTNCLICK);
}

TEST(LegacyCompat, LegacyEventsConvertExplicitlyToModern) {
    EXPECT_EQ(mxh::ui::legacy::legacyEventToModern(mxh::ui::legacy::WE_MOUSEOVER),
              cWindow::WindowEvent::MouseMove);
    EXPECT_EQ(mxh::ui::legacy::legacyEventToModern(mxh::ui::legacy::WE_LBTNCLICK),
              cWindow::WindowEvent::LButtonDown);
    EXPECT_EQ(mxh::ui::legacy::legacyEventToModern(mxh::ui::legacy::WE_BTNCLICK),
              cWindow::WindowEvent::LButtonClick);
    EXPECT_EQ(mxh::ui::legacy::legacyEventToModern(0xDEADBEEFu),
              cWindow::WindowEvent::Null);
}
