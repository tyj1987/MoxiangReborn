// uniqueitemmixprogressbardlg_test.cpp — 1:1 port
// tests for 墨香 CUniqueItemMixProgressBarDlg.

#include "uniqueitemmixprogressbardlg.hpp"
#include "progressbardlg.hpp"
#include "cobjectguagen.hpp"
#include "cstatic.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

using mxh::ui::cObjectGuagen;
using mxh::ui::cProgressBarDlg;
using mxh::ui::cStatic;
using mxh::ui::cUniqueItemMixProgressBarDlg;

namespace {

struct LinkedDialog {
    cUniqueItemMixProgressBarDlg dlg;
    std::unique_ptr<cObjectGuagen> progressGuagen;
    std::unique_ptr<cStatic> remaintime;

    LinkedDialog() {
        dlg.Init(0, 0, 200, 100, nullptr, 0);
        progressGuagen = std::make_unique<cObjectGuagen>();
        progressGuagen->Init(0, 0, 200, 20, nullptr,
                             cUniqueItemMixProgressBarDlg::kIdProgressBarGage);
        auto* gPtr = progressGuagen.get();
        dlg.Add(std::move(progressGuagen));

        remaintime = std::make_unique<cStatic>();
        remaintime->Init(0, 20, 100, 20, nullptr,
                         cUniqueItemMixProgressBarDlg::kIdRemaintimeTime);
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

TEST(CUniqueItemMixProgressBarDlgTest, CtorDoesNotCrash) {
    cUniqueItemMixProgressBarDlg dlg;
    SUCCEED();
}

TEST(CUniqueItemMixProgressBarDlgTest, DtorDoesNotCrash) {
    cUniqueItemMixProgressBarDlg dlg;
    SUCCEED();
}

TEST(CUniqueItemMixProgressBarDlgTest, InheritsFromCProgressBarDlg) {
    static_assert(std::is_base_of_v<cProgressBarDlg, cUniqueItemMixProgressBarDlg>,
                  "cUniqueItemMixProgressBarDlg must inherit from cProgressBarDlg");
    SUCCEED();
}

TEST(CUniqueItemMixProgressBarDlgTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cUniqueItemMixProgressBarDlg::kIdProgressBarGage, 680);
    EXPECT_EQ(cUniqueItemMixProgressBarDlg::kIdRemaintimeTime, 681);
    EXPECT_EQ(cUniqueItemMixProgressBarDlg::kIdCancelBtn, 682);
}

TEST(CUniqueItemMixProgressBarDlgTest, LinkingResolvesProgressGuagen) {
    LinkedDialog ld;
    EXPECT_EQ(ld.dlg.GetProgressGuagen(), ld.gPtr_);
}

TEST(CUniqueItemMixProgressBarDlgTest, LinkingResolvesRemaintimeStatic) {
    LinkedDialog ld;
    EXPECT_EQ(ld.dlg.GetRemaintimeStatic(), ld.sPtr_);
}

TEST(CUniqueItemMixProgressBarDlgTest, LinkingBeforeInitDoesNotCrash) {
    cUniqueItemMixProgressBarDlg dlg;
    dlg.Linking();
    SUCCEED();
}

TEST(CUniqueItemMixProgressBarDlgTest, LinkingWithoutChildrenDoesNotCrash) {
    cUniqueItemMixProgressBarDlg dlg;
    dlg.Init(0, 0, 200, 100, nullptr, 0);
    dlg.Linking();
    EXPECT_EQ(dlg.GetProgressGuagen(), nullptr);
}

TEST(CUniqueItemMixProgressBarDlgTest, OnActionEventOnCancelCallsInitProgress) {
    LinkedDialog ld;
    ld.dlg.StartProgress();
    EXPECT_TRUE(ld.dlg.IsProgressStart());
    ld.dlg.OnActionEvent(cUniqueItemMixProgressBarDlg::kIdCancelBtn, nullptr, 0);
    EXPECT_FALSE(ld.dlg.IsProgressStart());
}

TEST(CUniqueItemMixProgressBarDlgTest, OnActionEventOnUnknownIdIsNoOp) {
    LinkedDialog ld;
    ld.dlg.StartProgress();
    ld.dlg.OnActionEvent(999, nullptr, 0);
    EXPECT_TRUE(ld.dlg.IsProgressStart());
}

TEST(CUniqueItemMixProgressBarDlgTest, OnActionEventBeforeInitIsSafe) {
    cUniqueItemMixProgressBarDlg dlg;
    dlg.OnActionEvent(cUniqueItemMixProgressBarDlg::kIdCancelBtn, nullptr, 0);
    SUCCEED();
}
