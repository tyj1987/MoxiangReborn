// mpguagedialog_test.cpp — 1:1 port tests for
// 墨香 CMPGuageDialog (event-map timer + experience
// gauge).
//
// Verifies:
//   - ctor does not crash
//   - Dtor does not crash
//   - Inherits from cDialog
//   - 4 id constants (kIdExpGuage=610, kIdTime=611,
//     kIdExpPercent=612, kIdTitle=613)
//   - 1 placeholder string (kEventMapTitle)
//   - 3 bFlag enum constants (kFlagReady=0,
//     kFlagActive=1, kFlagStopped=2)
//   - kRedTextThreshold = 30000
//   - Linking resolves the 3 cStatic
//   - SetExpGuage updates m_ExpPercent text with
//     "%4.2f%%" format
//   - SetTime updates m_Time text with "%02u:%02u"
//     format
//   - SetTime with remainTime < 30000 sets red color
//   - SetTime with remainTime >= 30000 keeps color
//   - SetEventMapTimer with kFlagReady sets blue
//   - SetEventMapTimer with kFlagStopped sets blue
//   - SetEventMapTimer with kFlagActive + remainTime
//     < 30000 sets red
//   - SetEventMapTimer with kFlagActive + remainTime
//     >= 30000 keeps color
//   - ShowEventMap sets m_pTitle text
//   - ShowEventMap activates the dialog
//   - SetExpGuage / SetTime / SetEventMapTimer /
//     ShowEventMap without Linking do not crash

#include "mpguagedialog.hpp"
#include "cdialog.hpp"
#include "cstatic.hpp"
#include "cwindow.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <type_traits>

using mxh::ui::cDialog;
using mxh::ui::cMPGuageDialog;
using mxh::ui::cStatic;
using mxh::ui::cWindow;

namespace {

// helper: build a cMPGuageDialog + 3 cStatic + Linking()
struct LinkedDialog {
    cMPGuageDialog dlg;
    std::unique_ptr<cStatic> timeStatic;
    std::unique_ptr<cStatic> expPercent;
    std::unique_ptr<cStatic> title;

    LinkedDialog() {
        dlg.Init(0, 0, 200, 200, nullptr, 0);
        timeStatic = std::make_unique<cStatic>();
        timeStatic->Init(0, 0, 50, 20, nullptr, cMPGuageDialog::kIdTime);
        auto* timePtr = timeStatic.get();
        dlg.Add(std::move(timeStatic));

        expPercent = std::make_unique<cStatic>();
        expPercent->Init(0, 30, 50, 20, nullptr, cMPGuageDialog::kIdExpPercent);
        auto* expPtr = expPercent.get();
        dlg.Add(std::move(expPercent));

        title = std::make_unique<cStatic>();
        title->Init(0, 60, 50, 20, nullptr, cMPGuageDialog::kIdTitle);
        auto* titlePtr = title.get();
        dlg.Add(std::move(title));

        dlg.Linking();

        timePtr_ = timePtr;
        expPtr_ = expPtr;
        titlePtr_ = titlePtr;
    }

    cStatic* timePtr_ = nullptr;
    cStatic* expPtr_ = nullptr;
    cStatic* titlePtr_ = nullptr;
};

}  // namespace

// ---------- ctor / dtor ----------

TEST(CMPGuageDialogTest, CtorDoesNotCrash) {
    cMPGuageDialog dlg;
    SUCCEED();
}

TEST(CMPGuageDialogTest, DtorDoesNotCrash) {
    cMPGuageDialog dlg;
    SUCCEED();
}

TEST(CMPGuageDialogTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cMPGuageDialog>,
                  "cMPGuageDialog must inherit from cDialog");
    SUCCEED();
}

// ---------- id range ----------

TEST(CMPGuageDialogTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cMPGuageDialog::kIdExpGuage, 610);
    EXPECT_EQ(cMPGuageDialog::kIdTime, 611);
    EXPECT_EQ(cMPGuageDialog::kIdExpPercent, 612);
    EXPECT_EQ(cMPGuageDialog::kIdTitle, 613);
}

TEST(CMPGuageDialogTest, IdConstantsAreUnique) {
    EXPECT_NE(cMPGuageDialog::kIdExpGuage, cMPGuageDialog::kIdTime);
    EXPECT_NE(cMPGuageDialog::kIdExpGuage, cMPGuageDialog::kIdExpPercent);
    EXPECT_NE(cMPGuageDialog::kIdExpGuage, cMPGuageDialog::kIdTitle);
    EXPECT_NE(cMPGuageDialog::kIdTime, cMPGuageDialog::kIdExpPercent);
    EXPECT_NE(cMPGuageDialog::kIdTime, cMPGuageDialog::kIdTitle);
    EXPECT_NE(cMPGuageDialog::kIdExpPercent, cMPGuageDialog::kIdTitle);
}

TEST(CMPGuageDialogTest, EventMapTitlePlaceholderMatchesExpected) {
    EXPECT_STREQ(cMPGuageDialog::kEventMapTitle, "EVENT_MAP_TITLE");
}

TEST(CMPGuageDialogTest, RedTextThresholdMatchesExpected) {
    EXPECT_EQ(cMPGuageDialog::kRedTextThreshold, 30000u);
}

TEST(CMPGuageDialogTest, FlagEnumConstantsMatchLegacyValues) {
    EXPECT_EQ(cMPGuageDialog::kFlagReady, 0);
    EXPECT_EQ(cMPGuageDialog::kFlagActive, 1);
    EXPECT_EQ(cMPGuageDialog::kFlagStopped, 2);
}

// ---------- Linking ----------

TEST(CMPGuageDialogTest, LinkingResolvesAllThreeCStatics) {
    LinkedDialog ld;
    // Linking should resolve all 3 cStatic. We
    // verify by inspecting their text after
    // SetExpGuage / SetTime / ShowEventMap.
    ld.dlg.SetTime(60000);  // 1:00
    EXPECT_EQ(ld.timePtr_->GetStaticText(), "01:00");
    ld.dlg.SetExpGuage(0.5f);
    EXPECT_EQ(ld.expPtr_->GetStaticText(), "50.00%");
    ld.dlg.ShowEventMap();
    EXPECT_EQ(ld.titlePtr_->GetStaticText(), cMPGuageDialog::kEventMapTitle);
}

TEST(CMPGuageDialogTest, LinkingBeforeInitDoesNotCrash) {
    cMPGuageDialog dlg;
    dlg.Linking();
    SUCCEED();
}

TEST(CMPGuageDialogTest, LinkingWithoutChildrenDoesNotCrash) {
    cMPGuageDialog dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.Linking();
    // SetExpGuage / SetTime / ShowEventMap must
    // not crash when children are missing.
    dlg.SetExpGuage(0.5f);
    dlg.SetTime(60000);
    dlg.SetEventMapTimer(60000, cMPGuageDialog::kFlagReady);
    dlg.ShowEventMap();
    SUCCEED();
}

// ---------- SetExpGuage ----------

TEST(CMPGuageDialogTest, SetExpGuageUpdatesExpPercentText) {
    LinkedDialog ld;
    ld.dlg.SetExpGuage(0.5f);
    EXPECT_EQ(ld.expPtr_->GetStaticText(), "50.00%");
}

TEST(CMPGuageDialogTest, SetExpGuageZeroPercent) {
    LinkedDialog ld;
    ld.dlg.SetExpGuage(0.0f);
    EXPECT_EQ(ld.expPtr_->GetStaticText(), "0.00%");
}

TEST(CMPGuageDialogTest, SetExpGuageFullPercent) {
    LinkedDialog ld;
    ld.dlg.SetExpGuage(1.0f);
    EXPECT_EQ(ld.expPtr_->GetStaticText(), "100.00%");
}

TEST(CMPGuageDialogTest, SetExpGuageWithoutLinkingIsSafe) {
    cMPGuageDialog dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.SetExpGuage(0.5f);
    SUCCEED();
}

// ---------- SetTime ----------

TEST(CMPGuageDialogTest, SetTimeFormatsOneMinute) {
    LinkedDialog ld;
    ld.dlg.SetTime(60000);  // 60000 ms = 1:00
    EXPECT_EQ(ld.timePtr_->GetStaticText(), "01:00");
}

TEST(CMPGuageDialogTest, SetTimeFormatsThirtySeconds) {
    LinkedDialog ld;
    ld.dlg.SetTime(30000);  // 30000 ms = 0:30
    EXPECT_EQ(ld.timePtr_->GetStaticText(), "00:30");
}

TEST(CMPGuageDialogTest, SetTimeFormatsMixed) {
    LinkedDialog ld;
    ld.dlg.SetTime(125000);  // 125 sec = 2:05
    EXPECT_EQ(ld.timePtr_->GetStaticText(), "02:05");
}

TEST(CMPGuageDialogTest, SetTimeBelowThresholdSetsRedColor) {
    LinkedDialog ld;
    ld.dlg.SetTime(29999);  // < 30000
    EXPECT_EQ(ld.timePtr_->GetFGColor(), 0xFFFF0000u);
}

TEST(CMPGuageDialogTest, SetTimeAtThresholdDoesNotSetRed) {
    LinkedDialog ld;
    // First set blue (so we can verify it doesn't change)
    ld.timePtr_->SetFGColor(0xFF0000FFu);
    ld.dlg.SetTime(30000);  // == 30000 (not < 30000)
    EXPECT_EQ(ld.timePtr_->GetFGColor(), 0xFF0000FFu);
}

TEST(CMPGuageDialogTest, SetTimeWithoutLinkingIsSafe) {
    cMPGuageDialog dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.SetTime(60000);
    SUCCEED();
}

// ---------- SetEventMapTimer ----------

TEST(CMPGuageDialogTest, SetEventMapTimerReadySetsBlue) {
    LinkedDialog ld;
    ld.dlg.SetEventMapTimer(60000, cMPGuageDialog::kFlagReady);
    EXPECT_EQ(ld.timePtr_->GetFGColor(), 0xFF0000FFu);
    EXPECT_EQ(ld.timePtr_->GetStaticText(), "01:00");
}

TEST(CMPGuageDialogTest, SetEventMapTimerStoppedSetsBlue) {
    LinkedDialog ld;
    ld.dlg.SetEventMapTimer(60000, cMPGuageDialog::kFlagStopped);
    EXPECT_EQ(ld.timePtr_->GetFGColor(), 0xFF0000FFu);
    EXPECT_EQ(ld.timePtr_->GetStaticText(), "01:00");
}

TEST(CMPGuageDialogTest, SetEventMapTimerActiveBelowThresholdSetsRed) {
    LinkedDialog ld;
    ld.dlg.SetEventMapTimer(29999, cMPGuageDialog::kFlagActive);
    EXPECT_EQ(ld.timePtr_->GetFGColor(), 0xFFFF0000u);
    EXPECT_EQ(ld.timePtr_->GetStaticText(), "00:29");
}

TEST(CMPGuageDialogTest, SetEventMapTimerActiveAtThresholdKeepsColor) {
    LinkedDialog ld;
    ld.timePtr_->SetFGColor(0xFF00FF00u);  // green
    ld.dlg.SetEventMapTimer(30000, cMPGuageDialog::kFlagActive);
    EXPECT_EQ(ld.timePtr_->GetFGColor(), 0xFF00FF00u);
}

TEST(CMPGuageDialogTest, SetEventMapTimerUnknownFlagDoesNotCrash) {
    LinkedDialog ld;
    ld.dlg.SetEventMapTimer(60000, 99);  // unknown flag
    SUCCEED();
}

TEST(CMPGuageDialogTest, SetEventMapTimerWithoutLinkingIsSafe) {
    cMPGuageDialog dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.SetEventMapTimer(60000, cMPGuageDialog::kFlagReady);
    SUCCEED();
}

// ---------- ShowEventMap ----------

TEST(CMPGuageDialogTest, ShowEventMapSetsTitle) {
    LinkedDialog ld;
    ld.dlg.ShowEventMap();
    EXPECT_EQ(ld.titlePtr_->GetStaticText(), cMPGuageDialog::kEventMapTitle);
}

TEST(CMPGuageDialogTest, ShowEventMapActivatesDialog) {
    LinkedDialog ld;
    EXPECT_FALSE(ld.dlg.isActive());
    ld.dlg.ShowEventMap();
    EXPECT_TRUE(ld.dlg.isActive());
}

TEST(CMPGuageDialogTest, ShowEventMapWithoutLinkingIsSafe) {
    cMPGuageDialog dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.ShowEventMap();
    EXPECT_TRUE(dlg.isActive());
}
