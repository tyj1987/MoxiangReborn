// titanrecalldlg_test.cpp — 1:1 port tests for
// 墨香 CTitanRecallDlg.

#include "titanrecalldlg.hpp"
#include "progressbardlg.hpp"
#include "cobjectguagen.hpp"
#include "cstatic.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

using mxh::ui::cObjectGuagen;
using mxh::ui::cProgressBarDlg;
using mxh::ui::cStatic;
using mxh::ui::cTitanRecallDlg;

namespace {

struct LinkedDialog {
    cTitanRecallDlg dlg;
    std::unique_ptr<cObjectGuagen> progressGuagen;
    std::unique_ptr<cStatic> remaintime;

    LinkedDialog() {
        dlg.Init(0, 0, 200, 100, nullptr, 0);
        progressGuagen = std::make_unique<cObjectGuagen>();
        progressGuagen->Init(0, 0, 200, 20, nullptr,
                             cTitanRecallDlg::kIdProgressBarGage);
        auto* gPtr = progressGuagen.get();
        dlg.Add(std::move(progressGuagen));

        remaintime = std::make_unique<cStatic>();
        remaintime->Init(0, 20, 100, 20, nullptr,
                         cTitanRecallDlg::kIdRemaintimeTime);
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

TEST(CTitanRecallDlgTest, CtorDoesNotCrash) {
    cTitanRecallDlg dlg;
    SUCCEED();
}

TEST(CTitanRecallDlgTest, DtorDoesNotCrash) {
    cTitanRecallDlg dlg;
    SUCCEED();
}

TEST(CTitanRecallDlgTest, InheritsFromCProgressBarDlg) {
    static_assert(std::is_base_of_v<cProgressBarDlg, cTitanRecallDlg>,
                  "cTitanRecallDlg must inherit from cProgressBarDlg");
    SUCCEED();
}

TEST(CTitanRecallDlgTest, DefaultSuccessRecallIsFalse) {
    cTitanRecallDlg dlg;
    EXPECT_FALSE(dlg.GetSuccessRecall());
}

TEST(CTitanRecallDlgTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cTitanRecallDlg::kIdProgressBarGage, 690);
    EXPECT_EQ(cTitanRecallDlg::kIdRemaintimeTime, 691);
    EXPECT_EQ(cTitanRecallDlg::kIdCancelBtn, 692);
}

TEST(CTitanRecallDlgTest, BaseSuccessTimeIs7000) {
    EXPECT_EQ(cTitanRecallDlg::kBaseSuccessTime, 7000u);
}

TEST(CTitanRecallDlgTest, LinkingResolvesProgressGuagen) {
    LinkedDialog ld;
    EXPECT_EQ(ld.dlg.GetProgressGuagen(), ld.gPtr_);
}

TEST(CTitanRecallDlgTest, LinkingResolvesRemaintimeStatic) {
    LinkedDialog ld;
    EXPECT_EQ(ld.dlg.GetRemaintimeStatic(), ld.sPtr_);
}

TEST(CTitanRecallDlgTest, LinkingSetsSuccessTime) {
    LinkedDialog ld;
    EXPECT_EQ(ld.dlg.GetSuccessTime(), cTitanRecallDlg::kBaseSuccessTime);
}

TEST(CTitanRecallDlgTest, LinkingBeforeInitDoesNotCrash) {
    cTitanRecallDlg dlg;
    dlg.Linking();
    SUCCEED();
}

TEST(CTitanRecallDlgTest, LinkingWithoutChildrenDoesNotCrash) {
    cTitanRecallDlg dlg;
    dlg.Init(0, 0, 200, 100, nullptr, 0);
    dlg.Linking();
    EXPECT_EQ(dlg.GetProgressGuagen(), nullptr);
    // SuccessTime is still set
    EXPECT_EQ(dlg.GetSuccessTime(), cTitanRecallDlg::kBaseSuccessTime);
}

TEST(CTitanRecallDlgTest, OnActionEventReturnsTrue) {
    LinkedDialog ld;
    EXPECT_TRUE(ld.dlg.OnActionEvent(cTitanRecallDlg::kIdCancelBtn, nullptr, 0));
    EXPECT_TRUE(ld.dlg.OnActionEvent(999, nullptr, 0));
}

TEST(CTitanRecallDlgTest, OnActionEventBeforeInitIsSafe) {
    cTitanRecallDlg dlg;
    EXPECT_TRUE(dlg.OnActionEvent(cTitanRecallDlg::kIdCancelBtn, nullptr, 0));
}

TEST(CTitanRecallDlgTest, RenderIsNoOp) {
    LinkedDialog ld;
    ld.dlg.Render();
    SUCCEED();
}

TEST(CTitanRecallDlgTest, RenderBeforeInitDoesNotCrash) {
    cTitanRecallDlg dlg;
    dlg.Render();
    SUCCEED();
}

TEST(CTitanRecallDlgTest, SetSuccessRecallToggles) {
    cTitanRecallDlg dlg;
    EXPECT_FALSE(dlg.GetSuccessRecall());
    dlg.SetSuccessRecall(true);
    EXPECT_TRUE(dlg.GetSuccessRecall());
    dlg.SetSuccessRecall(false);
    EXPECT_FALSE(dlg.GetSuccessRecall());
}
