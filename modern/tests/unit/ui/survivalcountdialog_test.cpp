// survivalcountdialog_test.cpp — 1:1 port tests
// for 墨香 CSurvivalCountDialog.

#include "survivalcountdialog.hpp"
#include "cdialog.hpp"
#include "cstatic.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

using mxh::ui::cDialog;
using mxh::ui::cStatic;
using mxh::ui::cSurvivalCountDialog;

namespace {

struct LinkedDialog {
    cSurvivalCountDialog dlg;
    std::unique_ptr<cStatic> counterNum;
    std::unique_ptr<cStatic> winnerName;

    LinkedDialog() {
        dlg.Init(0, 0, 200, 200, nullptr, 0);
        counterNum = std::make_unique<cStatic>();
        counterNum->Init(0, 0, 50, 20, nullptr,
                         cSurvivalCountDialog::kIdAliveCounter);
        auto* cPtr = counterNum.get();
        dlg.Add(std::move(counterNum));

        winnerName = std::make_unique<cStatic>();
        winnerName->Init(0, 20, 100, 20, nullptr,
                         cSurvivalCountDialog::kIdWinnerName);
        auto* wPtr = winnerName.get();
        dlg.Add(std::move(winnerName));

        dlg.Linking();

        cPtr_ = cPtr;
        wPtr_ = wPtr;
    }

    cStatic* cPtr_ = nullptr;
    cStatic* wPtr_ = nullptr;
};

}  // namespace

TEST(CSurvivalCountDialogTest, CtorDoesNotCrash) {
    cSurvivalCountDialog dlg;
    SUCCEED();
}

TEST(CSurvivalCountDialogTest, DtorDoesNotCrash) {
    cSurvivalCountDialog dlg;
    SUCCEED();
}

TEST(CSurvivalCountDialogTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cSurvivalCountDialog>,
                  "cSurvivalCountDialog must inherit from cDialog");
    SUCCEED();
}

TEST(CSurvivalCountDialogTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cSurvivalCountDialog::kIdAliveCounter, 720);
    EXPECT_EQ(cSurvivalCountDialog::kIdWinnerName, 721);
}

TEST(CSurvivalCountDialogTest, IdConstantsAreUnique) {
    EXPECT_NE(cSurvivalCountDialog::kIdAliveCounter,
              cSurvivalCountDialog::kIdWinnerName);
}

TEST(CSurvivalCountDialogTest, SurvivalDefaultNamePlaceholder) {
    EXPECT_STREQ(cSurvivalCountDialog::kSurvivalDefaultName,
                "SURVIVAL_DEFAULT_NAME");
}

TEST(CSurvivalCountDialogTest, MaxCounterNumberIs99) {
    EXPECT_EQ(cSurvivalCountDialog::kMaxCounterNumber, 99u);
}

// ---------- Linking ----------

TEST(CSurvivalCountDialogTest, LinkingResolvesCounterNum) {
    LinkedDialog ld;
    EXPECT_EQ(ld.dlg.GetCounterNum(), ld.cPtr_);
}

TEST(CSurvivalCountDialogTest, LinkingResolvesWinnerName) {
    LinkedDialog ld;
    EXPECT_EQ(ld.dlg.GetWinnerName(), ld.wPtr_);
}

TEST(CSurvivalCountDialogTest, LinkingSetsCounterToZero) {
    LinkedDialog ld;
    // SetCounterNumber(0) → "00"
    EXPECT_EQ(ld.cPtr_->GetStaticText(), "00");
}

TEST(CSurvivalCountDialogTest, LinkingSetsWinnerName) {
    LinkedDialog ld;
    EXPECT_EQ(ld.wPtr_->GetStaticText(),
              cSurvivalCountDialog::kSurvivalDefaultName);
}

TEST(CSurvivalCountDialogTest, LinkingBeforeInitDoesNotCrash) {
    cSurvivalCountDialog dlg;
    dlg.Linking();
    SUCCEED();
}

TEST(CSurvivalCountDialogTest, LinkingWithoutChildrenDoesNotCrash) {
    cSurvivalCountDialog dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.Linking();
    EXPECT_EQ(dlg.GetCounterNum(), nullptr);
    EXPECT_EQ(dlg.GetWinnerName(), nullptr);
}

// ---------- InitSurvivalCountDlg ----------

TEST(CSurvivalCountDialogTest, InitSurvivalCountDlgDeactivates) {
    LinkedDialog ld;
    // TODO: real MAP-based dispatch when MAP/MAPTYPE ported.
    // Modern port always SetActive(false) for now.
    ld.dlg.InitSurvivalCountDlg(0);
    EXPECT_FALSE(ld.dlg.isActive());
}

TEST(CSurvivalCountDialogTest, InitSurvivalCountDlgBeforeInitIsSafe) {
    cSurvivalCountDialog dlg;
    dlg.InitSurvivalCountDlg(0);
    SUCCEED();
}

// ---------- SetCounterNumber ----------

TEST(CSurvivalCountDialogTest, SetCounterNumberZero) {
    LinkedDialog ld;
    ld.dlg.SetCounterNumber(0);
    EXPECT_EQ(ld.cPtr_->GetStaticText(), "00");
}

TEST(CSurvivalCountDialogTest, SetCounterNumberNine) {
    LinkedDialog ld;
    ld.dlg.SetCounterNumber(9);
    EXPECT_EQ(ld.cPtr_->GetStaticText(), "09");
}

TEST(CSurvivalCountDialogTest, SetCounterNumberTen) {
    LinkedDialog ld;
    ld.dlg.SetCounterNumber(10);
    EXPECT_EQ(ld.cPtr_->GetStaticText(), "10");
}

TEST(CSurvivalCountDialogTest, SetCounterNumberTwenty) {
    LinkedDialog ld;
    ld.dlg.SetCounterNumber(20);
    EXPECT_EQ(ld.cPtr_->GetStaticText(), "20");
}

TEST(CSurvivalCountDialogTest, SetCounterNumberNinetyNine) {
    LinkedDialog ld;
    ld.dlg.SetCounterNumber(99);
    EXPECT_EQ(ld.cPtr_->GetStaticText(), "99");
}

TEST(CSurvivalCountDialogTest, SetCounterNumberOver99) {
    // Active 1-cStatic version does NOT clamp.
    LinkedDialog ld;
    ld.dlg.SetCounterNumber(123);
    // 123/10 = 12, 123%10 = 3 → "123"
    EXPECT_EQ(ld.cPtr_->GetStaticText(), "123");
}

TEST(CSurvivalCountDialogTest, SetCounterNumberOverwritesPrevious) {
    LinkedDialog ld;
    ld.dlg.SetCounterNumber(50);
    EXPECT_EQ(ld.cPtr_->GetStaticText(), "50");
    ld.dlg.SetCounterNumber(75);
    EXPECT_EQ(ld.cPtr_->GetStaticText(), "75");
}

TEST(CSurvivalCountDialogTest, SetCounterNumberWithoutLinkingIsSafe) {
    cSurvivalCountDialog dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.SetCounterNumber(50);
    SUCCEED();
}

// ---------- SetWinnerName ----------

TEST(CSurvivalCountDialogTest, SetWinnerNameUpdatesText) {
    LinkedDialog ld;
    ld.dlg.SetWinnerName("Alice");
    EXPECT_EQ(ld.wPtr_->GetStaticText(), "Alice");
}

TEST(CSurvivalCountDialogTest, SetWinnerNameWithNullFallsBack) {
    LinkedDialog ld;
    ld.dlg.SetWinnerName("Bob");
    ld.dlg.SetWinnerName(nullptr);
    EXPECT_EQ(ld.wPtr_->GetStaticText(),
              cSurvivalCountDialog::kSurvivalDefaultName);
}

TEST(CSurvivalCountDialogTest, SetWinnerNameOverwritesPrevious) {
    LinkedDialog ld;
    ld.dlg.SetWinnerName("Alice");
    ld.dlg.SetWinnerName("Bob");
    EXPECT_EQ(ld.wPtr_->GetStaticText(), "Bob");
}

TEST(CSurvivalCountDialogTest, SetWinnerNameWithoutLinkingIsSafe) {
    cSurvivalCountDialog dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.SetWinnerName("Alice");
    SUCCEED();
}
