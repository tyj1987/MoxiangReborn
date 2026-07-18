// titanpartsprogressbardlg_test.cpp — 1:1 port
// tests for 墨香 CTitanPartsProgressBarDlg.

#include "titanpartsprogressbardlg.hpp"
#include "progressbardlg.hpp"
#include "cobjectguagen.hpp"
#include "cstatic.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

using mxh::ui::cObjectGuagen;
using mxh::ui::cProgressBarDlg;
using mxh::ui::cStatic;
using mxh::ui::cTitanPartsProgressBarDlg;

namespace {

struct LinkedDialog {
    cTitanPartsProgressBarDlg dlg;
    std::unique_ptr<cObjectGuagen> progressGuagen;
    std::unique_ptr<cStatic> remaintime;

    LinkedDialog() {
        dlg.Init(0, 0, 200, 100, nullptr, 0);
        progressGuagen = std::make_unique<cObjectGuagen>();
        progressGuagen->Init(0, 0, 200, 20, nullptr,
                             cTitanPartsProgressBarDlg::kIdProgressBarGage);
        auto* gPtr = progressGuagen.get();
        dlg.Add(std::move(progressGuagen));

        remaintime = std::make_unique<cStatic>();
        remaintime->Init(0, 20, 100, 20, nullptr,
                         cTitanPartsProgressBarDlg::kIdRemaintimeTime);
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

TEST(CTitanPartsProgressBarDlgTest, CtorDoesNotCrash) {
    cTitanPartsProgressBarDlg dlg;
    SUCCEED();
}

TEST(CTitanPartsProgressBarDlgTest, DtorDoesNotCrash) {
    cTitanPartsProgressBarDlg dlg;
    SUCCEED();
}

TEST(CTitanPartsProgressBarDlgTest, InheritsFromCProgressBarDlg) {
    static_assert(std::is_base_of_v<cProgressBarDlg, cTitanPartsProgressBarDlg>,
                  "cTitanPartsProgressBarDlg must inherit from cProgressBarDlg");
    SUCCEED();
}

TEST(CTitanPartsProgressBarDlgTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cTitanPartsProgressBarDlg::kIdProgressBarGage, 670);
    EXPECT_EQ(cTitanPartsProgressBarDlg::kIdRemaintimeTime, 671);
    EXPECT_EQ(cTitanPartsProgressBarDlg::kIdCancelBtn, 672);
}

TEST(CTitanPartsProgressBarDlgTest, LinkingResolvesProgressGuagen) {
    LinkedDialog ld;
    EXPECT_EQ(ld.dlg.GetProgressGuagen(), ld.gPtr_);
}

TEST(CTitanPartsProgressBarDlgTest, LinkingResolvesRemaintimeStatic) {
    LinkedDialog ld;
    EXPECT_EQ(ld.dlg.GetRemaintimeStatic(), ld.sPtr_);
}

TEST(CTitanPartsProgressBarDlgTest, LinkingBeforeInitDoesNotCrash) {
    cTitanPartsProgressBarDlg dlg;
    dlg.Linking();
    SUCCEED();
}

TEST(CTitanPartsProgressBarDlgTest, LinkingWithoutChildrenDoesNotCrash) {
    cTitanPartsProgressBarDlg dlg;
    dlg.Init(0, 0, 200, 100, nullptr, 0);
    dlg.Linking();
    EXPECT_EQ(dlg.GetProgressGuagen(), nullptr);
}

TEST(CTitanPartsProgressBarDlgTest, OnActionEventOnCancelCallsInitProgress) {
    LinkedDialog ld;
    ld.dlg.StartProgress();
    EXPECT_TRUE(ld.dlg.IsProgressStart());
    ld.dlg.OnActionEvent(cTitanPartsProgressBarDlg::kIdCancelBtn, nullptr, 0);
    EXPECT_FALSE(ld.dlg.IsProgressStart());
}

TEST(CTitanPartsProgressBarDlgTest, OnActionEventOnUnknownIdIsNoOp) {
    LinkedDialog ld;
    ld.dlg.StartProgress();
    ld.dlg.OnActionEvent(999, nullptr, 0);
    EXPECT_TRUE(ld.dlg.IsProgressStart());
}

TEST(CTitanPartsProgressBarDlgTest, OnActionEventBeforeInitIsSafe) {
    cTitanPartsProgressBarDlg dlg;
    dlg.OnActionEvent(cTitanPartsProgressBarDlg::kIdCancelBtn, nullptr, 0);
    SUCCEED();
}
