// progressbardlg_test.cpp — 1:1 port tests for
// 墨香 CProgressBarDlg (base progress bar dialog).
//
// Verifies:
//   - ctor does not crash
//   - Dtor does not crash
//   - Inherits from cDialog
//   - Default state is zero
//   - SetActive(true) updates base state
//   - SetActive(false) resets 4 state fields
//   - SetActive(false) calls SetValue(0, 0) on
//     cObjectGuagen
//   - SetActive(false) without progress guagen is
//     safe
//   - InitProgress resets 4 state fields
//   - InitProgress sets SetActive(false)
//   - StartProgress calls InitProgress + sets
//     m_bProgressStart=true + SetActive(true)
//   - StartProgress before SetSuccessTime has 0
//     success time (m_dwProcessTime calculation is
//     TODO)
//   - Process returns without state update when
//     not started
//   - Process returns when started (TODO body)
//   - Process before Init is safe
//   - SetSuccessTime sets the field
//   - SetSuccessProgress sets the field
//   - GetSuccessProgress returns the field
//   - Render is no-op
//   - Subclass Linking setters work

#include "progressbardlg.hpp"
#include "cdialog.hpp"
#include "cobjectguagen.hpp"
#include "cguagen.hpp"
#include "cstatic.hpp"
#include "cwindow.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

using mxh::ui::cDialog;
using mxh::ui::cGuagen;
using mxh::ui::cObjectGuagen;
using mxh::ui::cProgressBarDlg;
using mxh::ui::cStatic;
using mxh::ui::cWindow;

namespace {

// helper: build a cProgressBarDlg + 1 cObjectGuagen +
// 1 cStatic + subclass-style Linking via setters
struct LinkedDialog {
    cProgressBarDlg dlg;
    std::unique_ptr<cObjectGuagen> progressGuagen;
    std::unique_ptr<cStatic> remaintime;

    LinkedDialog() {
        dlg.Init(0, 0, 200, 100, nullptr, 0);
        progressGuagen = std::make_unique<cObjectGuagen>();
        progressGuagen->Init(0, 0, 200, 20, nullptr, 0);
        auto* gPtr = progressGuagen.get();
        dlg.Add(std::move(progressGuagen));

        remaintime = std::make_unique<cStatic>();
        remaintime->Init(0, 20, 100, 20, nullptr, 0);
        auto* sPtr = remaintime.get();
        dlg.Add(std::move(remaintime));

        // Subclass-style Linking (legacy base class
        // doesn't have its own Linking — each
        // subclass does it).
        dlg.SetProgressGuagen(gPtr);
        dlg.SetRemaintimeStatic(sPtr);

        gPtr_ = gPtr;
        sPtr_ = sPtr;
    }

    cObjectGuagen* gPtr_ = nullptr;
    cStatic* sPtr_ = nullptr;
};

}  // namespace

// ---------- ctor / dtor ----------

TEST(CProgressBarDlgTest, CtorDoesNotCrash) {
    cProgressBarDlg dlg;
    SUCCEED();
}

TEST(CProgressBarDlgTest, DtorDoesNotCrash) {
    cProgressBarDlg dlg;
    SUCCEED();
}

TEST(CProgressBarDlgTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cProgressBarDlg>,
                  "cProgressBarDlg must inherit from cDialog");
    SUCCEED();
}

TEST(CProgressBarDlgTest, DefaultStateIsZero) {
    cProgressBarDlg dlg;
    EXPECT_FALSE(dlg.IsProgressStart());
    EXPECT_FALSE(dlg.GetSuccessProgress());
    EXPECT_EQ(dlg.GetProcessTime(), 0u);
    EXPECT_EQ(dlg.GetCurrentTime(), 0u);
    EXPECT_EQ(dlg.GetSuccessTime(), 0u);
    EXPECT_EQ(dlg.GetProgressGuagen(), nullptr);
    EXPECT_EQ(dlg.GetRemaintimeStatic(), nullptr);
}

// ---------- SetActive ----------

TEST(CProgressBarDlgTest, SetActiveTrueUpdatesBaseState) {
    cProgressBarDlg dlg;
    dlg.Init(0, 0, 200, 100, nullptr, 0);
    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CProgressBarDlgTest, SetActiveFalseUpdatesBaseState) {
    LinkedDialog ld;
    ld.dlg.SetActive(true);
    EXPECT_TRUE(ld.dlg.isActive());
    ld.dlg.SetActive(false);
    EXPECT_FALSE(ld.dlg.isActive());
}

TEST(CProgressBarDlgTest, SetActiveFalseResetsStateFields) {
    LinkedDialog ld;
    // Manually set state fields via StartProgress
    // (which calls InitProgress + sets fields, but
    // the gCurTime-based fields are left at 0 in
    // modern port)
    ld.dlg.StartProgress();
    EXPECT_TRUE(ld.dlg.IsProgressStart());
    // Now SetActive(false) should reset
    ld.dlg.SetActive(false);
    EXPECT_FALSE(ld.dlg.IsProgressStart());
    EXPECT_EQ(ld.dlg.GetProcessTime(), 0u);
    EXPECT_EQ(ld.dlg.GetCurrentTime(), 0u);
}

TEST(CProgressBarDlgTest, SetActiveFalseResetsGuagenValue) {
    LinkedDialog ld;
    // Set value to 0.5f
    ld.gPtr_->SetValue(0.5f, 0);
    EXPECT_FLOAT_EQ(ld.gPtr_->GetValue(), 0.5f);
    // SetActive(false) should call SetValue(0, 0)
    ld.dlg.SetActive(false);
    EXPECT_FLOAT_EQ(ld.gPtr_->GetValue(), 0.0f);
}

TEST(CProgressBarDlgTest, SetActiveFalseWithoutGuagenIsSafe) {
    cProgressBarDlg dlg;
    dlg.Init(0, 0, 200, 100, nullptr, 0);
    dlg.SetActive(false);
    SUCCEED();
}

TEST(CProgressBarDlgTest, SetActiveBeforeInitDoesNotCrash) {
    cProgressBarDlg dlg;
    dlg.SetActive(true);
    SUCCEED();
}

// ---------- InitProgress ----------

TEST(CProgressBarDlgTest, InitProgressResetsState) {
    LinkedDialog ld;
    // Set m_bSuccessProgress to true
    ld.dlg.SetSuccessProgress(true);
    EXPECT_TRUE(ld.dlg.GetSuccessProgress());
    // InitProgress should reset
    ld.dlg.InitProgress();
    EXPECT_FALSE(ld.dlg.IsProgressStart());
    EXPECT_FALSE(ld.dlg.GetSuccessProgress());
    EXPECT_EQ(ld.dlg.GetProcessTime(), 0u);
    EXPECT_EQ(ld.dlg.GetCurrentTime(), 0u);
}

TEST(CProgressBarDlgTest, InitProgressSetsActiveFalse) {
    LinkedDialog ld;
    ld.dlg.SetActive(true);
    EXPECT_TRUE(ld.dlg.isActive());
    ld.dlg.InitProgress();
    EXPECT_FALSE(ld.dlg.isActive());
}

// ---------- StartProgress ----------

TEST(CProgressBarDlgTest, StartProgressSetsProgressStart) {
    LinkedDialog ld;
    EXPECT_FALSE(ld.dlg.IsProgressStart());
    ld.dlg.StartProgress();
    EXPECT_TRUE(ld.dlg.IsProgressStart());
}

TEST(CProgressBarDlgTest, StartProgressSetsActiveTrue) {
    LinkedDialog ld;
    EXPECT_FALSE(ld.dlg.isActive());
    ld.dlg.StartProgress();
    EXPECT_TRUE(ld.dlg.isActive());
}

TEST(CProgressBarDlgTest, StartProgressWithSuccessTime) {
    LinkedDialog ld;
    ld.dlg.SetSuccessTime(5000);
    ld.dlg.StartProgress();
    EXPECT_TRUE(ld.dlg.IsProgressStart());
    // m_dwProcessTime calculation is TODO (gCurTime)
    // so it remains 0 in modern port.
    EXPECT_EQ(ld.dlg.GetProcessTime(), 0u);
}

TEST(CProgressBarDlgTest, StartProgressWithoutLinkingIsSafe) {
    cProgressBarDlg dlg;
    dlg.Init(0, 0, 200, 100, nullptr, 0);
    dlg.StartProgress();
    EXPECT_TRUE(dlg.IsProgressStart());
}

// ---------- Process ----------

TEST(CProgressBarDlgTest, ProcessBeforeStartIsNoOp) {
    LinkedDialog ld;
    EXPECT_FALSE(ld.dlg.IsProgressStart());
    // Process should return early when not started
    ld.dlg.Process();
    // State should remain at default
    EXPECT_EQ(ld.dlg.GetCurrentTime(), 0u);
    EXPECT_EQ(ld.dlg.GetProcessTime(), 0u);
}

TEST(CProgressBarDlgTest, ProcessAfterStartIsNoOp) {
    LinkedDialog ld;
    ld.dlg.StartProgress();
    // Process after start is TODO (gCurTime-based
    // tick). State should not change in modern port.
    ld.dlg.Process();
    EXPECT_EQ(ld.dlg.GetCurrentTime(), 0u);
}

TEST(CProgressBarDlgTest, ProcessBeforeInitIsSafe) {
    cProgressBarDlg dlg;
    dlg.Process();
    SUCCEED();
}

// ---------- SetSuccessTime / SetSuccessProgress ----------

TEST(CProgressBarDlgTest, SetSuccessTimeUpdatesField) {
    LinkedDialog ld;
    EXPECT_EQ(ld.dlg.GetSuccessTime(), 0u);
    ld.dlg.SetSuccessTime(10000);
    EXPECT_EQ(ld.dlg.GetSuccessTime(), 10000u);
}

TEST(CProgressBarDlgTest, SetSuccessProgressToggles) {
    LinkedDialog ld;
    EXPECT_FALSE(ld.dlg.GetSuccessProgress());
    ld.dlg.SetSuccessProgress(true);
    EXPECT_TRUE(ld.dlg.GetSuccessProgress());
    ld.dlg.SetSuccessProgress(false);
    EXPECT_FALSE(ld.dlg.GetSuccessProgress());
}

// ---------- Render ----------

TEST(CProgressBarDlgTest, RenderIsNoOp) {
    LinkedDialog ld;
    ld.dlg.Render();
    SUCCEED();
}

TEST(CProgressBarDlgTest, RenderBeforeInitDoesNotCrash) {
    cProgressBarDlg dlg;
    dlg.Render();
    SUCCEED();
}

// ---------- Subclass Linking setters ----------

TEST(CProgressBarDlgTest, SetProgressGuagenUpdatesGetter) {
    cProgressBarDlg dlg;
    cObjectGuagen g;
    g.Init(0, 0, 100, 20, nullptr, 0);
    dlg.SetProgressGuagen(&g);
    EXPECT_EQ(dlg.GetProgressGuagen(), &g);
}

TEST(CProgressBarDlgTest, SetRemaintimeStaticUpdatesGetter) {
    cProgressBarDlg dlg;
    cStatic s;
    s.Init(0, 0, 100, 20, nullptr, 0);
    dlg.SetRemaintimeStatic(&s);
    EXPECT_EQ(dlg.GetRemaintimeStatic(), &s);
}

TEST(CProgressBarDlgTest, SetProgressGuagenWithNullptr) {
    cProgressBarDlg dlg;
    dlg.SetProgressGuagen(nullptr);
    EXPECT_EQ(dlg.GetProgressGuagen(), nullptr);
}

TEST(CProgressBarDlgTest, SetRemaintimeStaticWithNullptr) {
    cProgressBarDlg dlg;
    dlg.SetRemaintimeStatic(nullptr);
    EXPECT_EQ(dlg.GetRemaintimeStatic(), nullptr);
}
