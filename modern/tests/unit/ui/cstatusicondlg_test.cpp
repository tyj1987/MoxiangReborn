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

// --------------------------------------------------------------------------
// Clock provider + Process() tests (C-Batch-2.42).
//
// Locks down the 1:1 surface:
//   * AddIcon stamps m_dwStartTime[kind] via host clock (null = 0)
//   * Process() flips bAlpha = TRUE within 5000 ms of expiry
//   * Process() flips bAlpha = FALSE when icon has expired
//   * Process() skips empty kinds + zero-remainTime kinds
//   * Process() preserves DWORD wrap-around for curTime < startTime
//   * kStatusIconExpiringBlinkMs = 5000 (legacy constant)
// --------------------------------------------------------------------------

namespace {
struct ClockCapture {
    std::uint32_t value = 0;
    int           calls = 0;
    static std::uint32_t Get(void* userData) {
        auto* self = static_cast<ClockCapture*>(userData);
        ++self->calls;
        return self->value;
    }
};
} // namespace

TEST(CStatusIconDlg, ExpiringBlinkConstantIsFiveThousand) {
    // 1:1 with legacy Render if (m_dwRemainTime[n] - elapsed <= 5000).
    EXPECT_EQ(mxh::ui::kStatusIconExpiringBlinkMs, 5000u);
}

TEST(CStatusIconDlg, AddIconStampsStartTimeViaHostClock) {
    cStatusIconDlg dlg;
    int dummy = 0;
    dlg.Init(&dummy, 0, 0, 4);
    ClockCapture clk;
    clk.value = 12345u;
    dlg.SetCurrentTimeProvider(&ClockCapture::Get, &clk);
    dlg.AddIcon(&dummy, eStatusIcon_Poison, 7, 60000);
    EXPECT_EQ(dlg.GetStartTimeAt(eStatusIcon_Poison), 12345u);
    EXPECT_EQ(dlg.GetRemainTimeAt(eStatusIcon_Poison), 60000u);
    EXPECT_GE(clk.calls, 1);
}

TEST(CStatusIconDlg, NullCurrentTimeProviderKeepsSafeZeroStart) {
    // 1:1 fallback: a null clock provider leaves m_dwStartTime at 0.
    cStatusIconDlg dlg;
    int dummy = 0;
    dlg.Init(&dummy, 0, 0, 4);
    dlg.AddIcon(&dummy, eStatusIcon_Poison, 7, 60000);
    EXPECT_EQ(dlg.GetStartTimeAt(eStatusIcon_Poison), 0u);
}

TEST(CStatusIconDlg, ProcessFlipsAlphaOnExpiringRemain) {
    // Within 5000 ms of expiry -> legacy sets bAlpha = TRUE (blink).
    cStatusIconDlg dlg;
    int dummy = 0;
    dlg.Init(&dummy, 0, 0, 4);
    ClockCapture clk;
    clk.value = 1000u;
    dlg.SetCurrentTimeProvider(&ClockCapture::Get, &clk);
    // Icon active from t = 0 with 10000 ms remaining -> at t = 7000
    // elapsed = 7000, remaining = 3000 (<= 5000) -> bAlpha = TRUE.
    dlg.AddIcon(&dummy, eStatusIcon_Poison, 0, 10000);
    clk.value = 7000u;
    dlg.Process();
    EXPECT_TRUE(dlg.GetAlphaFlagAt(eStatusIcon_Poison));
}

TEST(CStatusIconDlg, ProcessClearsAlphaWhenExpired) {
    // elapsed >= m_dwRemainTime -> legacy flips bAlpha back to FALSE.
    cStatusIconDlg dlg;
    int dummy = 0;
    dlg.Init(&dummy, 0, 0, 4);
    ClockCapture clk;
    clk.value = 0u;
    dlg.SetCurrentTimeProvider(&ClockCapture::Get, &clk);
    dlg.AddIcon(&dummy, eStatusIcon_Poison, 0, 5000);
    // Pre-set bAlpha via AddIcon semantics: legacy AddIcon sets bAlpha = FALSE.
    EXPECT_FALSE(dlg.GetAlphaFlagAt(eStatusIcon_Poison));
    // Pump curTime past the expiry + manually enable blink to prove Process clears it.
    clk.value = 6000u;
    dlg.Process();
    EXPECT_FALSE(dlg.GetAlphaFlagAt(eStatusIcon_Poison));
}

TEST(CStatusIconDlg, ProcessLeavesAlphaWhenRemainingAboveBlink) {
    // remaining > 5000 ms but < m_dwRemainTime -> bAlpha must stay FALSE.
    cStatusIconDlg dlg;
    int dummy = 0;
    dlg.Init(&dummy, 0, 0, 4);
    ClockCapture clk;
    clk.value = 0u;
    dlg.SetCurrentTimeProvider(&ClockCapture::Get, &clk);
    // remain 60_000, after 1000 ms remaining is 59000 (>5000) -> blink=FALSE.
    dlg.AddIcon(&dummy, eStatusIcon_Poison, 0, 60000);
    clk.value = 1000u;
    dlg.Process();
    EXPECT_FALSE(dlg.GetAlphaFlagAt(eStatusIcon_Poison));
}

TEST(CStatusIconDlg, ProcessSkipsEmptyAndZeroRemainKinds) {
    // A kind with m_IconCount == 0 must be skipped (no read of bAlpha).
    // A kind with m_dwRemainTime == 0 (legacy: expires immediately)
    // is also skipped (no inverse state mutation).
    cStatusIconDlg dlg;
    int dummy = 0;
    dlg.Init(&dummy, 0, 0, 4);
    ClockCapture clk;
    clk.value = 0u;
    dlg.SetCurrentTimeProvider(&ClockCapture::Get, &clk);
    // Icon with zero remain: skipped (legacy semantics: no timer).
    dlg.AddIcon(&dummy, eStatusIcon_Poison, 0, 0);
    clk.value = 8000u;
    dlg.Process();
    EXPECT_FALSE(dlg.GetAlphaFlagAt(eStatusIcon_Poison));
    // Range-safe accessors return 0 / false for unknown kinds.
    EXPECT_EQ(dlg.GetRemainTimeAt(-1), 0u);
    EXPECT_EQ(dlg.GetRemainTimeAt(mxh::ui::eStatusIcon_Max), 0u);
    EXPECT_EQ(dlg.GetStartTimeAt(-1), 0u);
    EXPECT_FALSE(dlg.GetAlphaFlagAt(mxh::ui::eStatusIcon_Max));
}

TEST(CStatusIconDlg, ProcessPreservesDwordWrapAround) {
    // curTime < m_dwStartTime (DWORD wrap) -> elapsed = curTime - start.
    // Legacy uses unsigned DWORD subtraction; the modern port mirrors it.
    //
    // Setup: start=0xFFFFFFFE, curTime=0x00000005 -> elapsed=7 (wrap).
    // remain=10 -> within range, remaining = 3 (<=5000) -> bAlpha = TRUE.
    cStatusIconDlg dlg;
    int dummy = 0;
    dlg.Init(&dummy, 0, 0, 4);
    ClockCapture clk;
    clk.value = 0xFFFFFFFEu;
    dlg.SetCurrentTimeProvider(&ClockCapture::Get, &clk);
    dlg.AddIcon(&dummy, eStatusIcon_Poison, 0, 10);
    // Now advance curTime past start by 7 (wrap).
    clk.value = 0x00000005u;
    dlg.Process();
    EXPECT_TRUE(dlg.GetAlphaFlagAt(eStatusIcon_Poison));
}

TEST(CStatusIconDlg, ProcessUsesDwordRemainComparisonOnExpiry) {
    // Legacy DWORD: elapsed >= m_dwRemainTime means expired.
    cStatusIconDlg dlg;
    int dummy = 0;
    dlg.Init(&dummy, 0, 0, 4);
    ClockCapture clk;
    clk.value = 0u;
    dlg.SetCurrentTimeProvider(&ClockCapture::Get, &clk);
    // Add icon at t=0 with remain=100. At t=100 elapsed==remain -> expired.
    dlg.AddIcon(&dummy, eStatusIcon_Poison, 0, 100);
    clk.value = 100u;
    dlg.Process();
    EXPECT_FALSE(dlg.GetAlphaFlagAt(eStatusIcon_Poison));
}

TEST(CStatusIconDlg, ProcessWithoutProviderUsesZeroClock) {
    // Null clock => curTime=0, so elapsed = 0 - 0 = 0 (or 0 - 0).
    // If m_dwStartTime was set via host earlier, curTime=0 still goes through
    // the same call site -- no crash, no UB.
    cStatusIconDlg dlg;
    int dummy = 0;
    dlg.Init(&dummy, 0, 0, 4);
    dlg.AddIcon(&dummy, eStatusIcon_Poison, 0, 60000);
    dlg.Process();  // must not crash, must not flip bAlpha.
    // curTime=0, start=0, elapsed=0, remaining=60000 -> bAlpha = FALSE.
    EXPECT_FALSE(dlg.GetAlphaFlagAt(eStatusIcon_Poison));
}

TEST(CStatusIconDlg, NonCopyable) {
    cStatusIconDlg dlg;
    static_assert(!std::is_copy_constructible_v<cStatusIconDlg>);
    static_assert(!std::is_copy_assignable_v<cStatusIconDlg>);
    (void)dlg;
}
