// titanmixprogressbardlg_test.cpp — 1:1 port tests
// for 墨香 CTitanMixProgressBarDlg.
//
// Verifies:
//   - ctor does not crash
//   - Dtor does not crash
//   - Inherits from cProgressBarDlg
//   - 3 id constants match expected local range
//   - Linking resolves the 2 children + sets the
//     base class fields
//   - OnActionEvent on cancel id calls InitProgress
//   - OnActionEvent on unknown id is no-op
//   - OnActionEvent before Init does not crash
//   - SuccessProcess is no-op
//   - Linking before Init does not crash
//   - Linking without children does not crash

#include "titanmixprogressbardlg.hpp"
#include "progressbardlg.hpp"
#include "cobjectguagen.hpp"
#include "cstatic.hpp"
#include "cwindow.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

using mxh::ui::cObjectGuagen;
using mxh::ui::cProgressBarDlg;
using mxh::ui::cStatic;
using mxh::ui::cTitanMixProgressBarDlg;
using mxh::ui::cWindow;

namespace {

// helper: build a cTitanMixProgressBarDlg + 2 children
// + Linking
struct LinkedDialog {
    cTitanMixProgressBarDlg dlg;
    std::unique_ptr<cObjectGuagen> progressGuagen;
    std::unique_ptr<cStatic> remaintime;

    LinkedDialog() {
        dlg.Init(0, 0, 200, 100, nullptr, 0);
        progressGuagen = std::make_unique<cObjectGuagen>();
        progressGuagen->Init(0, 0, 200, 20, nullptr,
                             cTitanMixProgressBarDlg::kIdProgressBarGage);
        auto* gPtr = progressGuagen.get();
        dlg.Add(std::move(progressGuagen));

        remaintime = std::make_unique<cStatic>();
        remaintime->Init(0, 20, 100, 20, nullptr,
                         cTitanMixProgressBarDlg::kIdRemaintimeTime);
        auto* sPtr = remaintime.get();
        dlg.Add(std::move(remaintime));

        dlg.Linking();

        gPtr_ = gPtr;
        sPtr_ = sPtr;
    }

    cObjectGuagen* gPtr_ = nullptr;
    cStatic* sPtr_ = nullptr;
};

}  // namespace

// ---------- ctor / dtor ----------

TEST(CTitanMixProgressBarDlgTest, CtorDoesNotCrash) {
    cTitanMixProgressBarDlg dlg;
    SUCCEED();
}

TEST(CTitanMixProgressBarDlgTest, DtorDoesNotCrash) {
    cTitanMixProgressBarDlg dlg;
    SUCCEED();
}

TEST(CTitanMixProgressBarDlgTest, InheritsFromCProgressBarDlg) {
    static_assert(std::is_base_of_v<cProgressBarDlg, cTitanMixProgressBarDlg>,
                  "cTitanMixProgressBarDlg must inherit from cProgressBarDlg");
    SUCCEED();
}

// ---------- id range ----------

TEST(CTitanMixProgressBarDlgTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cTitanMixProgressBarDlg::kIdProgressBarGage, 660);
    EXPECT_EQ(cTitanMixProgressBarDlg::kIdRemaintimeTime, 661);
    EXPECT_EQ(cTitanMixProgressBarDlg::kIdCancelBtn, 662);
}

TEST(CTitanMixProgressBarDlgTest, IdConstantsAreUnique) {
    EXPECT_NE(cTitanMixProgressBarDlg::kIdProgressBarGage,
              cTitanMixProgressBarDlg::kIdRemaintimeTime);
    EXPECT_NE(cTitanMixProgressBarDlg::kIdProgressBarGage,
              cTitanMixProgressBarDlg::kIdCancelBtn);
    EXPECT_NE(cTitanMixProgressBarDlg::kIdRemaintimeTime,
              cTitanMixProgressBarDlg::kIdCancelBtn);
}

// ---------- Linking ----------

TEST(CTitanMixProgressBarDlgTest, LinkingResolvesProgressGuagen) {
    LinkedDialog ld;
    EXPECT_EQ(ld.dlg.GetProgressGuagen(), ld.gPtr_);
}

TEST(CTitanMixProgressBarDlgTest, LinkingResolvesRemaintimeStatic) {
    LinkedDialog ld;
    EXPECT_EQ(ld.dlg.GetRemaintimeStatic(), ld.sPtr_);
}

TEST(CTitanMixProgressBarDlgTest, LinkingBeforeInitDoesNotCrash) {
    cTitanMixProgressBarDlg dlg;
    dlg.Linking();
    SUCCEED();
}

TEST(CTitanMixProgressBarDlgTest, LinkingWithoutChildrenDoesNotCrash) {
    cTitanMixProgressBarDlg dlg;
    dlg.Init(0, 0, 200, 100, nullptr, 0);
    dlg.Linking();
    // Both children are nullptr.
    EXPECT_EQ(dlg.GetProgressGuagen(), nullptr);
    EXPECT_EQ(dlg.GetRemaintimeStatic(), nullptr);
}

// ---------- OnActionEvent ----------

TEST(CTitanMixProgressBarDlgTest, OnActionEventOnCancelCallsInitProgress) {
    LinkedDialog ld;
    ld.dlg.StartProgress();
    EXPECT_TRUE(ld.dlg.IsProgressStart());
    ld.dlg.OnActionEvent(cTitanMixProgressBarDlg::kIdCancelBtn, nullptr, 0);
    EXPECT_FALSE(ld.dlg.IsProgressStart());
}

TEST(CTitanMixProgressBarDlgTest, OnActionEventOnUnknownIdIsNoOp) {
    LinkedDialog ld;
    ld.dlg.StartProgress();
    EXPECT_TRUE(ld.dlg.IsProgressStart());
    ld.dlg.OnActionEvent(999, nullptr, 0);  // unknown id
    EXPECT_TRUE(ld.dlg.IsProgressStart());  // still in progress
}

TEST(CTitanMixProgressBarDlgTest, OnActionEventBeforeInitIsSafe) {
    cTitanMixProgressBarDlg dlg;
    dlg.OnActionEvent(cTitanMixProgressBarDlg::kIdCancelBtn, nullptr, 0);
    SUCCEED();
}

// ---------- SuccessProcess ----------

TEST(CTitanMixProgressBarDlgTest, SuccessProcessIsNoOp) {
    cTitanMixProgressBarDlg dlg;
    dlg.SuccessProcess();
    SUCCEED();
}
