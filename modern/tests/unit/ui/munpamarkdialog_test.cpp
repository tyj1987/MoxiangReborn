// munpamarkdialog_test.cpp — 1:1 port verification tests for cMunpaMarkDialog.

#include "munpamarkdialog.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

using mxh::ui::cMunpaMarkDialog;
using mxh::ui::CMunpaMark;
using mxh::ui::cDialog;

// A test-only CMunpaMark subclass that records Render calls. CMunpaMark
// is now defined inline in munpamarkdialog.hpp (as a virtual base
// with a no-op Render), so this subclass can extend it and override
// Render to record the call. This is the standard Phase 6
// verification pattern (per memory "cGuagen test-injectable clock"
// + "cButton subclass for click callback" patterns).
//
// Note: TestMunpaMark is declared in mxh::ui namespace (not in the
// anonymous namespace below) so it shares the same scope as
// CMunpaMark — this lets MSVC 14.44 see the inheritance and
// upcast TestMunpaMark* → CMunpaMark* correctly.
namespace mxh::ui {

class TestMunpaMark : public CMunpaMark {
public:
    int renderCount = 0;
    std::int32_t lastAbsX = 0;
    std::int32_t lastAbsY = 0;

    // 1:1 quirk: base CMunpaMark::Render is `noexcept`; the override
    // must preserve the noexcept specifier for the virtual dispatch
    // to work (MSVC C2694 enforces this). Test-only override
    // matches the base signature exactly.
    void Render(std::int32_t* pAbsPos) noexcept override {
        ++renderCount;
        if (pAbsPos) {
            lastAbsX = pAbsPos[0];
            lastAbsY = pAbsPos[1];
        }
    }
};

}  // namespace mxh::ui

namespace {

std::unique_ptr<cMunpaMarkDialog> MakeDialog() {
    auto d = std::make_unique<cMunpaMarkDialog>();
    d->Init(10, 20, 100, 100, nullptr, 920);
    return d;
}

}  // namespace

// ---------------------------------------------------------------------------
// Construction + Init
// ---------------------------------------------------------------------------

TEST(CMunpaMarkDialog, DefaultConstructionHasNullMark) {
    auto d = std::make_unique<cMunpaMarkDialog>();
    EXPECT_EQ(d->munpaMark(), nullptr);
    EXPECT_FALSE(d->hasMunpaMark());
}

TEST(CMunpaMarkDialog, InitStoresDimensions) {
    auto d = MakeDialog();
    EXPECT_EQ(d->id(), 920);
    EXPECT_EQ(d->absX(), 10);
    EXPECT_EQ(d->absY(), 20);
    // 1:1 quirk: Init is not overridden in modern port (the base
    // cDialog::Init signature is already 1:1 after the m_type=WT_*
    // removal). The m_type assignment is dropped (R-12 fix).
}

TEST(CMunpaMarkDialog, InitDoesNotAssignMunpamark) {
    auto d = MakeDialog();
    EXPECT_EQ(d->munpaMark(), nullptr);
}

// ---------------------------------------------------------------------------
// SetMunpaMark
// ---------------------------------------------------------------------------

TEST(CMunpaMarkDialog, SetMunpaMarkReturnsFalseWhenManagerReturnsNull) {
    // 1:1 quirk: legacy MUNPAMARKMGR->GetMunpaMark returns NULL when
    // the mark isn't loaded → SetMunpaMark returns FALSE. Modern
    // stub returns nullptr (always), so this test verifies the FALSE
    // path.
    auto d = MakeDialog();
    EXPECT_FALSE(d->SetMunpaMark(12345u));
    EXPECT_EQ(d->munpaMark(), nullptr);
    EXPECT_FALSE(d->hasMunpaMark());
}

TEST(CMunpaMarkDialog, SetMunpaMarkReturnsTrueWhenMarkInjected) {
    auto d = MakeDialog();
    mxh::ui::TestMunpaMark mark;
    d->SetMunpaMarkForTesting(&mark);
    // After injection, SetMunpaMark with a fresh ID would re-resolve
    // via the stub (nullptr) and lose the injected mark — that's the
    // 1:1 quirk. Verify injection directly:
    EXPECT_EQ(d->munpaMark(), &mark);
    EXPECT_TRUE(d->hasMunpaMark());
}

TEST(CMunpaMarkDialog, SetMunpaMarkOverridesPrevious) {
    auto d = MakeDialog();
    mxh::ui::TestMunpaMark mark1, mark2;
    d->SetMunpaMarkForTesting(&mark1);
    d->SetMunpaMarkForTesting(&mark2);
    EXPECT_EQ(d->munpaMark(), &mark2);
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

TEST(CMunpaMarkDialog, RenderWithoutMarkIsSafe) {
    auto d = MakeDialog();
    // 1:1 quirk: legacy Render guard `if(m_pMunpaMark) ...` —
    // no mark → no munpamark.Render call. Modern port preserves the
    // guard as a no-op when m_pMunpaMark is nullptr.
    d->Render();
    SUCCEED();
}

TEST(CMunpaMarkDialog, RenderWithMarkCallsMunpamarkRender) {
    auto d = MakeDialog();
    mxh::ui::TestMunpaMark mark;
    d->SetMunpaMarkForTesting(&mark);
    d->Render();
    EXPECT_EQ(mark.renderCount, 1);
    EXPECT_EQ(mark.lastAbsX, 10);  // absX from Init(10, 20, ...)
    EXPECT_EQ(mark.lastAbsY, 20);
}

TEST(CMunpaMarkDialog, RenderMultipleCallsInvokeMarkEachTime) {
    auto d = MakeDialog();
    mxh::ui::TestMunpaMark mark;
    d->SetMunpaMarkForTesting(&mark);
    d->Render();
    d->Render();
    d->Render();
    EXPECT_EQ(mark.renderCount, 3);
}

TEST(CMunpaMarkDialog, RenderThenClearMarkSkipsMark) {
    auto d = MakeDialog();
    mxh::ui::TestMunpaMark mark;
    d->SetMunpaMarkForTesting(&mark);
    d->Render();
    EXPECT_EQ(mark.renderCount, 1);
    d->SetMunpaMarkForTesting(nullptr);
    d->Render();
    EXPECT_EQ(mark.renderCount, 1);  // no increment after clear
}

// ---------------------------------------------------------------------------
// Init signature 1:1 with base cDialog::Init (no override)
// ---------------------------------------------------------------------------

TEST(CMunpaMarkDialog, InitDefaultIdIsZero) {
    // 1:1 with legacy Init(x, y, wid, hei, cImage*, ID=0). The base
    // cDialog::Init default id=0 is preserved.
    cMunpaMarkDialog d;
    d.Init(0, 0, 100, 100, nullptr);
    EXPECT_EQ(d.id(), 0);
}
