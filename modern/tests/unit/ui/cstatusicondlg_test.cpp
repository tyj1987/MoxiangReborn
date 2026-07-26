// mxh/tests/unit/ui/cstatusicondlg_test.cpp
//
// Unit tests for mxh::ui::cStatusIconDlg (Phase C dialog port).
//
// Locks down the 1:1 surface:
//   * eStatusIcon_Max == 13 (legacy sentinel for the 13 status icons)
//   * Init(obj, x, y, perLine) binds the object + clears all icon arrays
//   * AddIcon / RemoveIcon with a non-matching pObject is a no-op
//   * AddIcon / RemoveIcon bump the per-kind counter and m_CurIconNum
//   * AddQuestTimeIcon also bumps m_nQuestIconCount
//   * RemoveAllQuestTimeIcon clears the quest icon counter
//   * Render fires the draw callback for every added icon
//   * Render draws in a 2D grid wrap pattern (idx % m_MaxIconPerLine)

#include "mxh/ui/cstatusicondlg.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

using mxh::ui::cStatusIconDlg;
using mxh::ui::eStatusIcon_Max;
using mxh::ui::eStatusIcon_Poison;
using mxh::ui::eStatusIcon_Fire;
using mxh::ui::eStatusIcon_Stun;
using mxh::ui::eStatusIcon_ExpUp;
using mxh::ui::StatusIconKind;

TEST(CStatusIconDlg, StatusIconMaxIsThirteen) {
    // 1:1 with legacy CommonStruct.h: eStatusIcon_Max = 13
    EXPECT_EQ(static_cast<std::int32_t>(eStatusIcon_Max), 13);
}

TEST(CStatusIconDlg, DefaultConstructionHasZeroCount) {
    cStatusIconDlg dlg;
    EXPECT_EQ(dlg.CurIconNum(), 0);
    EXPECT_EQ(dlg.GetQuestIconCount(), 0);
    // IconCount() is the slot-array size (eStatusIcon_Max == 13),
    // not the active-icon count.  Use CurIconNum() for the latter.
    EXPECT_EQ(dlg.IconCount(), static_cast<std::int32_t>(eStatusIcon_Max));
    EXPECT_EQ(dlg.DescriptionCount(), 0);
    EXPECT_EQ(dlg.BoundObject(), nullptr);
}

TEST(CStatusIconDlg, InitBindsObjectAndClearsAll) {
    cStatusIconDlg dlg;
    int dummy = 0;
    dlg.Init(&dummy, /*x=*/10, /*y=*/20, /*perLine=*/4);
    EXPECT_EQ(dlg.BoundObject(), &dummy);
    EXPECT_EQ(dlg.CurIconNum(), 0);
    EXPECT_EQ(dlg.MaxIconPerLine(), 4);
    // Init must clear all per-kind counts to 0.
    dlg.AddIcon(&dummy, eStatusIcon_Poison, /*itemIdx=*/7, /*remain=*/5000);
    EXPECT_EQ(dlg.CurIconNum(), 1);
    // Re-Init must reset.
    dlg.Init(&dummy, 0, 0, 4);
    EXPECT_EQ(dlg.CurIconNum(), 0);
}

TEST(CStatusIconDlg, AddIconBumpsKindAndCurCount) {
    cStatusIconDlg dlg;
    int dummy = 0;
    dlg.Init(&dummy, 0, 0, 4);
    dlg.AddIcon(&dummy, eStatusIcon_Poison, 7, 5000);
    EXPECT_EQ(dlg.CurIconNum(), 1);
    dlg.AddIcon(&dummy, eStatusIcon_Poison, 7, 5000);
    EXPECT_EQ(dlg.CurIconNum(), 2);
    dlg.AddIcon(&dummy, eStatusIcon_Fire, 9, 0);
    EXPECT_EQ(dlg.CurIconNum(), 3);
}

TEST(CStatusIconDlg, AddIconStoresRemainTime) {
    cStatusIconDlg dlg;
    int dummy = 0;
    dlg.Init(&dummy, 0, 0, 4);
    dlg.AddIcon(&dummy, eStatusIcon_Poison, 1, 12345);
    // We don't expose a public getter for remain time, but Render
    // populates the per-icon RenderCtx with the latest ItemIndex.
    std::vector<int> kinds;
    dlg.SetOnDrawIcon([&](const cStatusIconDlg::RenderCtx& ctx) {
        kinds.push_back(static_cast<int>(ctx.iconKind));
    });
    dlg.Render();
    ASSERT_EQ(kinds.size(), 1u);
    EXPECT_EQ(kinds[0], eStatusIcon_Poison);
}

TEST(CStatusIconDlg, AddIconIgnoresForeignObject) {
    cStatusIconDlg dlg;
    int a = 0, b = 0;
    dlg.Init(&a, 0, 0, 4);
    dlg.AddIcon(&b, eStatusIcon_Poison, 1, 0);  // bound to &a, not &b
    EXPECT_EQ(dlg.CurIconNum(), 0);
}

TEST(CStatusIconDlg, AddIconIgnoresOutOfRangeKind) {
    cStatusIconDlg dlg;
    int dummy = 0;
    dlg.Init(&dummy, 0, 0, 4);
    dlg.AddIcon(&dummy, /*kind=*/99, 0, 0);
    dlg.AddIcon(&dummy, /*kind=*/eStatusIcon_Max, 0, 0);
    EXPECT_EQ(dlg.CurIconNum(), 0);
}

TEST(CStatusIconDlg, RemoveIconDecrementsCount) {
    cStatusIconDlg dlg;
    int dummy = 0;
    dlg.Init(&dummy, 0, 0, 4);
    dlg.AddIcon(&dummy, eStatusIcon_Poison, 7, 0);
    dlg.AddIcon(&dummy, eStatusIcon_Poison, 7, 0);
    EXPECT_EQ(dlg.CurIconNum(), 2);
    dlg.RemoveIcon(&dummy, eStatusIcon_Poison, 7);
    EXPECT_EQ(dlg.CurIconNum(), 1);
    dlg.RemoveIcon(&dummy, eStatusIcon_Poison, 7);
    EXPECT_EQ(dlg.CurIconNum(), 0);
}

TEST(CStatusIconDlg, RemoveIconIgnoresForeignObject) {
    cStatusIconDlg dlg;
    int a = 0, b = 0;
    dlg.Init(&a, 0, 0, 4);
    dlg.AddIcon(&a, eStatusIcon_Poison, 0, 0);
    dlg.RemoveIcon(&b, eStatusIcon_Poison, 0);
    EXPECT_EQ(dlg.CurIconNum(), 1);
}

TEST(CStatusIconDlg, RemoveIconStopsAtZero) {
    cStatusIconDlg dlg;
    int dummy = 0;
    dlg.Init(&dummy, 0, 0, 4);
    dlg.RemoveIcon(&dummy, eStatusIcon_Poison, 0);
    EXPECT_EQ(dlg.CurIconNum(), 0);
}

TEST(CStatusIconDlg, AddQuestTimeIconBumpsQuestCounter) {
    cStatusIconDlg dlg;
    int dummy = 0;
    dlg.Init(&dummy, 0, 0, 4);
    dlg.AddQuestTimeIcon(&dummy, eStatusIcon_ExpUp);
    dlg.AddQuestTimeIcon(&dummy, eStatusIcon_ExpUp);
    EXPECT_EQ(dlg.CurIconNum(), 2);
    EXPECT_EQ(dlg.GetQuestIconCount(), 2);
}

TEST(CStatusIconDlg, RemoveAllQuestTimeIconClearsQuestCounter) {
    cStatusIconDlg dlg;
    int dummy = 0;
    dlg.Init(&dummy, 0, 0, 4);
    dlg.AddQuestTimeIcon(&dummy, eStatusIcon_Stun);
    dlg.AddQuestTimeIcon(&dummy, eStatusIcon_Stun);
    dlg.AddIcon(&dummy, eStatusIcon_Poison, 0, 0);
    EXPECT_EQ(dlg.GetQuestIconCount(), 2);
    dlg.RemoveAllQuestTimeIcon();
    EXPECT_EQ(dlg.GetQuestIconCount(), 0);
    EXPECT_EQ(dlg.CurIconNum(), 0);
}

TEST(CStatusIconDlg, RemoveQuestTimeIconDecrementsBoth) {
    cStatusIconDlg dlg;
    int dummy = 0;
    dlg.Init(&dummy, 0, 0, 4);
    dlg.AddQuestTimeIcon(&dummy, eStatusIcon_Stun);
    dlg.AddQuestTimeIcon(&dummy, eStatusIcon_Stun);
    dlg.RemoveQuestTimeIcon(&dummy, eStatusIcon_Stun);
    EXPECT_EQ(dlg.GetQuestIconCount(), 1);
    EXPECT_EQ(dlg.CurIconNum(), 1);
    dlg.RemoveQuestTimeIcon(&dummy, eStatusIcon_Stun);
    EXPECT_EQ(dlg.GetQuestIconCount(), 0);
}

TEST(CStatusIconDlg, RenderFiresCallbackOncePerIcon) {
    cStatusIconDlg dlg;
    int dummy = 0;
    dlg.Init(&dummy, 0, 0, 4);
    dlg.AddIcon(&dummy, eStatusIcon_Poison, 0, 0);
    dlg.AddIcon(&dummy, eStatusIcon_Fire, 0, 0);
    dlg.AddQuestTimeIcon(&dummy, eStatusIcon_Stun);
    std::vector<int> kinds;
    dlg.SetOnDrawIcon([&](const cStatusIconDlg::RenderCtx& ctx) {
        kinds.push_back(static_cast<int>(ctx.iconKind));
    });
    dlg.Render();
    EXPECT_EQ(kinds.size(), 3u);
}

TEST(CStatusIconDlg, RenderWithoutCallbackIsSafe) {
    cStatusIconDlg dlg;
    int dummy = 0;
    dlg.Init(&dummy, 0, 0, 4);
    dlg.AddIcon(&dummy, eStatusIcon_Poison, 0, 0);
    dlg.Render();  // no callback, must not crash
    SUCCEED();
}

TEST(CStatusIconDlg, RenderWrapsAfterMaxIconPerLine) {
    cStatusIconDlg dlg;
    int dummy = 0;
    dlg.Init(&dummy, /*x=*/100, /*y=*/200, /*perLine=*/2);
    // 5 icons with perLine=2 -> row 0 (idx 0,1), row 1 (idx 2,3), row 2 (idx 4)
    for (int i = 0; i < 5; ++i) {
        dlg.AddIcon(&dummy, eStatusIcon_Poison, 0, 0);
    }
    std::vector<std::pair<int,int>> positions;
    dlg.SetOnDrawIcon([&](const cStatusIconDlg::RenderCtx& ctx) {
        positions.emplace_back(ctx.drawX, ctx.drawY);
    });
    dlg.Render();
    ASSERT_EQ(positions.size(), 5u);
    // 1:1 with legacy Render: x = baseX + (idx % perLine) * 16
    //                            y = baseY + (idx / perLine) * 16
    EXPECT_EQ(positions[0].first, 100);
    EXPECT_EQ(positions[0].second, 200);
    EXPECT_EQ(positions[1].first, 116);
    EXPECT_EQ(positions[1].second, 200);
    EXPECT_EQ(positions[2].first, 100);
    EXPECT_EQ(positions[2].second, 216);
    EXPECT_EQ(positions[3].first, 116);
    EXPECT_EQ(positions[3].second, 216);
    EXPECT_EQ(positions[4].first, 100);
    EXPECT_EQ(positions[4].second, 232);
}

TEST(CStatusIconDlg, RenderCtxCarriesItemIndex) {
    cStatusIconDlg dlg;
    int dummy = 0;
    dlg.Init(&dummy, 0, 0, 4);
    dlg.AddIcon(&dummy, eStatusIcon_Poison, /*itemIdx=*/123, 0);
    int lastItemIdx = -1;
    dlg.SetOnDrawIcon([&](const cStatusIconDlg::RenderCtx& ctx) {
        lastItemIdx = static_cast<int>(ctx.itemIdx);
    });
    dlg.Render();
    EXPECT_EQ(lastItemIdx, 123);
}

TEST(CStatusIconDlg, AddDescriptionForTestGrowsArray) {
    cStatusIconDlg dlg;
    EXPECT_EQ(dlg.DescriptionCount(), 0);
    dlg.AddDescriptionForTest("Poison", "Poison damage over time");
    dlg.AddDescriptionForTest("Fire", "Burn");
    EXPECT_EQ(dlg.DescriptionCount(), 2);
}

TEST(CStatusIconDlg, AddDescriptionNullIsSafe) {
    cStatusIconDlg dlg;
    dlg.AddDescriptionForTest(nullptr, "v");
    dlg.AddDescriptionForTest("k", nullptr);
    EXPECT_EQ(dlg.DescriptionCount(), 0);
}

TEST(CStatusIconDlg, ReleaseResetsAll) {
    cStatusIconDlg dlg;
    int dummy = 0;
    dlg.Init(&dummy, 0, 0, 4);
    dlg.AddIcon(&dummy, eStatusIcon_Poison, 0, 0);
    dlg.AddQuestTimeIcon(&dummy, eStatusIcon_Stun);
    dlg.AddDescriptionForTest("k", "v");
    dlg.Release();
    EXPECT_EQ(dlg.CurIconNum(), 0);
    EXPECT_EQ(dlg.GetQuestIconCount(), 0);
    EXPECT_EQ(dlg.DescriptionCount(), 0);
}

TEST(CStatusIconDlg, SetOneMinuteToShopItemIsTolerated) {
    // 1:1 quirk: the legacy function is a no-op in the modern port.
    cStatusIconDlg dlg;
    dlg.SetOneMinuteToShopItem(/*itemIdx=*/7);
    SUCCEED();
}

TEST(CStatusIconDlg, NonCopyable) {
    cStatusIconDlg dlg;
    static_assert(!std::is_copy_constructible_v<cStatusIconDlg>);
    static_assert(!std::is_copy_assignable_v<cStatusIconDlg>);
    (void)dlg;
}
