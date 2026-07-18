// guildlevelupdialog_test.cpp — 1:1 port verification tests for cGuildLevelUpDialog.

#include "guildlevelupdialog.hpp"
#include "cstatic.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

using mxh::ui::cGuildLevelUpDialog;
using mxh::ui::cStatic;

namespace {

constexpr int kNumTiers  = cGuildLevelUpDialog::kNumTiers;   // 4
constexpr int kNumLevels = cGuildLevelUpDialog::kNumLevels;  // 5

// cDialog::Init is `Init(x, y, wid, hei, basicImage, id)` — pass a small
// size so the dialog is laid out but tests don't rely on pixel positions.
// cDialog is non-copyable (deletes copy ctor), so we return a unique_ptr.
std::unique_ptr<cGuildLevelUpDialog> MakeDialog() {
    auto dlg = std::make_unique<cGuildLevelUpDialog>();
    dlg->Init(0, 0, 200, 200, nullptr, 739);
    return dlg;
}

}  // namespace

// ---------------------------------------------------------------------------
// Construction + constants
// ---------------------------------------------------------------------------

TEST(CGuildLevelUpDialog, ConstantsAreLegacySized) {
    EXPECT_EQ(kNumTiers, 4);
    EXPECT_EQ(kNumLevels, 5);
}

TEST(CGuildLevelUpDialog, DefaultCtorLeavesCurrentLevelAtZero) {
    cGuildLevelUpDialog dlg;
    EXPECT_EQ(dlg.GetCurrentLevel(), 0u);
    // All accessor slots are nullptr before Linking().
    for (int i = 0; i < kNumTiers; ++i) {
        EXPECT_EQ(dlg.GetLevelupNotComplete(i), nullptr);
        EXPECT_EQ(dlg.GetLevelupComplete(i), nullptr);
    }
    for (int i = 0; i < kNumLevels; ++i) {
        EXPECT_EQ(dlg.GetLevel(i), nullptr);
    }
}

TEST(CGuildLevelUpDialog, AccessorRejectsOutOfRange) {
    cGuildLevelUpDialog dlg;
    EXPECT_EQ(dlg.GetLevelupNotComplete(-1), nullptr);
    EXPECT_EQ(dlg.GetLevelupNotComplete(kNumTiers), nullptr);
    EXPECT_EQ(dlg.GetLevelupComplete(-1), nullptr);
    EXPECT_EQ(dlg.GetLevelupComplete(kNumTiers), nullptr);
    EXPECT_EQ(dlg.GetLevel(-1), nullptr);
    EXPECT_EQ(dlg.GetLevel(kNumLevels), nullptr);
}

// ---------------------------------------------------------------------------
// Linking()
// ---------------------------------------------------------------------------

TEST(CGuildLevelUpDialog, LinkingMaterializesAllThirteenStatics) {
    auto dlg = MakeDialog();
    dlg->Linking();
    for (int i = 0; i < kNumTiers; ++i) {
        EXPECT_NE(dlg->GetLevelupNotComplete(i), nullptr) << "nc tier " << i;
        EXPECT_NE(dlg->GetLevelupComplete(i), nullptr)    << "co tier " << i;
    }
    for (int i = 0; i < kNumLevels; ++i) {
        EXPECT_NE(dlg->GetLevel(i), nullptr) << "lv " << i;
    }
}

TEST(CGuildLevelUpDialog, LinkingIdempotent) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->Linking();  // second Linking must not crash / leak
    for (int i = 0; i < kNumTiers; ++i) {
        EXPECT_NE(dlg->GetLevelupNotComplete(i), nullptr) << "nc tier " << i;
        EXPECT_NE(dlg->GetLevelupComplete(i), nullptr)    << "co tier " << i;
    }
    for (int i = 0; i < kNumLevels; ++i) {
        EXPECT_NE(dlg->GetLevel(i), nullptr) << "lv " << i;
    }
}

TEST(CGuildLevelUpDialog, LinkingSetsStaticIds) {
    auto dlg = MakeDialog();
    dlg->Linking();
    // 1:1 quirk: id range 740..752 maps to GD_LU* in the legacy.
    EXPECT_EQ(dlg->GetLevelupNotComplete(0)->id(), 740);
    EXPECT_EQ(dlg->GetLevelupNotComplete(3)->id(), 743);
    EXPECT_EQ(dlg->GetLevelupComplete(0)->id(),    744);
    EXPECT_EQ(dlg->GetLevelupComplete(3)->id(),    747);
    EXPECT_EQ(dlg->GetLevel(0)->id(),              748);
    EXPECT_EQ(dlg->GetLevel(4)->id(),              752);
}

// ---------------------------------------------------------------------------
// SetLevel(0..5)
// ---------------------------------------------------------------------------

TEST(CGuildLevelUpDialog, SetLevelZeroIsIgnored) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->SetLevel(0);
    EXPECT_EQ(dlg->GetCurrentLevel(), 0u);  // still zero, no state change
}

TEST(CGuildLevelUpDialog, SetLevelSixIsIgnored) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->SetLevel(6);
    EXPECT_EQ(dlg->GetCurrentLevel(), 0u);
}

TEST(CGuildLevelUpDialog, SetLevel1MarksZeroTiersComplete) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->SetLevel(1);
    EXPECT_EQ(dlg->GetCurrentLevel(), 1u);
    for (int i = 0; i < kNumTiers; ++i) {
        EXPECT_TRUE(dlg->GetLevelupNotComplete(i)->isVisible())  << "nc[" << i << "]";
        EXPECT_FALSE(dlg->GetLevelupComplete(i)->isVisible())    << "co[" << i << "]";
    }
}

TEST(CGuildLevelUpDialog, SetLevel5MarksAllTiersComplete) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->SetLevel(5);
    EXPECT_EQ(dlg->GetCurrentLevel(), 5u);
    for (int i = 0; i < kNumTiers; ++i) {
        EXPECT_FALSE(dlg->GetLevelupNotComplete(i)->isVisible()) << "nc[" << i << "]";
        EXPECT_TRUE(dlg->GetLevelupComplete(i)->isVisible())     << "co[" << i << "]";
    }
}

TEST(CGuildLevelUpDialog, SetLevel3MarksTwoTiersComplete) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->SetLevel(3);
    EXPECT_EQ(dlg->GetCurrentLevel(), 3u);
    for (int i = 0; i < 2; ++i) {
        EXPECT_FALSE(dlg->GetLevelupNotComplete(i)->isVisible());
        EXPECT_TRUE(dlg->GetLevelupComplete(i)->isVisible());
    }
    for (int i = 2; i < kNumTiers; ++i) {
        EXPECT_TRUE(dlg->GetLevelupNotComplete(i)->isVisible());
        EXPECT_FALSE(dlg->GetLevelupComplete(i)->isVisible());
    }
}

TEST(CGuildLevelUpDialog, SetLevelRecolorsHighlight) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->SetLevel(3);
    // 1:1 quirk: legacy RGB_HALF(255,255,0) -> modern 0xFFFFFF00 (ARGB).
    EXPECT_EQ(dlg->GetLevel(2)->GetFGColor(), 0xFFFFFF00u);
    // Other labels are white (RGB_HALF(255,255,255) = 0xFFFFFFFF ARGB).
    for (int i = 0; i < kNumLevels; ++i) {
        if (i == 2) continue;
        EXPECT_EQ(dlg->GetLevel(i)->GetFGColor(), 0xFFFFFFFFu) << "i=" << i;
    }
}

TEST(CGuildLevelUpDialog, SetLevelTwiceOverridesHighlight) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->SetLevel(2);
    EXPECT_EQ(dlg->GetLevel(1)->GetFGColor(), 0xFFFFFF00u);
    dlg->SetLevel(4);
    EXPECT_EQ(dlg->GetLevel(1)->GetFGColor(), 0xFFFFFFFFu);  // reset
    EXPECT_EQ(dlg->GetLevel(3)->GetFGColor(), 0xFFFFFF00u);  // new highlight
}

// ---------------------------------------------------------------------------
// SetActive(bool)
// ---------------------------------------------------------------------------

TEST(CGuildLevelUpDialog, SetActiveTrueReappliesCurrentLevel) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->SetLevel(2);
    dlg->SetActive(true);
    EXPECT_TRUE(dlg->isActive());
    EXPECT_EQ(dlg->GetCurrentLevel(), 2u);
    EXPECT_EQ(dlg->GetLevel(1)->GetFGColor(), 0xFFFFFF00u);
}

TEST(CGuildLevelUpDialog, SetActiveTrueWithoutPriorLevelIsNoOp) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->SetActive(true);
    EXPECT_TRUE(dlg->isActive());
    EXPECT_EQ(dlg->GetCurrentLevel(), 0u);  // still 0
}

TEST(CGuildLevelUpDialog, SetActiveFalseDelegatesToBase) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->SetActive(true);
    dlg->SetActive(false);
    EXPECT_FALSE(dlg->isActive());
}

TEST(CGuildLevelUpDialog, SetActiveFalseAfterLevelPreservesState) {
    auto dlg = MakeDialog();
    dlg->Linking();
    dlg->SetLevel(4);
    dlg->SetActive(false);
    EXPECT_EQ(dlg->GetCurrentLevel(), 4u);  // level state preserved
}
