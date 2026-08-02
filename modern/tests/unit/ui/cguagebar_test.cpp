#include "mxh/ui/cGuageBar.hpp"
#include "mxh/ui/cButton.hpp"
#include "mxh/ui/cGuagen.hpp"
#include "mxh/ui/cWindow.hpp"

#include <gtest/gtest.h>

#include <cstdint>

namespace mxh::ui::test {

TEST(CGuageBar, DefaultConstructionZeroesState) {
    cGuageBar bar;
    EXPECT_EQ(bar.GetInterval(), 0);
    EXPECT_EQ(bar.GetMinValue(), 0);
    EXPECT_EQ(bar.GetMaxValue(), 0);
    EXPECT_EQ(bar.GetCurValue(), 0);
    EXPECT_FLOAT_EQ(bar.GetBarRelPos(), 0.0f);
    EXPECT_FLOAT_EQ(bar.GetCurRate(), 0.0f);
    EXPECT_FALSE(bar.IsVertical());
    EXPECT_FALSE(bar.IsDrag());
    EXPECT_FALSE(bar.IsLocked());
    EXPECT_EQ(bar.GetBarBtnColor(), 0xFFFFFFFFu);
}

TEST(CGuageBar, InheritsGuagenAndWindow) {
    cGuageBar bar;
    cGuagen* guage = &bar;
    cWindow* window = &bar;
    EXPECT_NE(guage, nullptr);
    EXPECT_NE(window, nullptr);
    window->Render();
}

TEST(CGuageBar, InitGuageBarStoresIntervalAndOrientation) {
    cGuageBar bar;
    bar.InitGuageBar(120, true);
    EXPECT_EQ(bar.GetInterval(), 120);
    EXPECT_TRUE(bar.IsVertical());
}

TEST(CGuageBar, InitGuageBarClampsNonPositiveInterval) {
    cGuageBar bar;
    bar.InitGuageBar(0, false);
    EXPECT_EQ(bar.GetInterval(), 1);
    bar.InitGuageBar(-20, true);
    EXPECT_EQ(bar.GetInterval(), 1);
    EXPECT_TRUE(bar.IsVertical());
}

TEST(CGuageBar, InitValueMapsMinimumToStart) {
    cGuageBar bar;
    bar.InitGuageBar(100, false);
    bar.InitValue(10, 110, 10);
    EXPECT_EQ(bar.GetMinValue(), 10);
    EXPECT_EQ(bar.GetMaxValue(), 110);
    EXPECT_EQ(bar.GetCurValue(), 10);
    EXPECT_FLOAT_EQ(bar.GetBarRelPos(), 0.0f);
    EXPECT_FLOAT_EQ(bar.GetCurRate(), 0.0f);
}

TEST(CGuageBar, InitValueMapsMaximumToInterval) {
    cGuageBar bar;
    bar.InitGuageBar(100, false);
    bar.InitValue(10, 110, 110);
    EXPECT_FLOAT_EQ(bar.GetBarRelPos(), 100.0f);
    EXPECT_FLOAT_EQ(bar.GetCurRate(), 1.0f);
}

TEST(CGuageBar, InitValueMapsMidpoint) {
    cGuageBar bar;
    bar.InitGuageBar(100, false);
    bar.InitValue(0, 200, 50);
    EXPECT_FLOAT_EQ(bar.GetBarRelPos(), 25.0f);
    EXPECT_FLOAT_EQ(bar.GetCurRate(), 0.25f);
    EXPECT_FLOAT_EQ(bar.GetValue(), 0.25f);
}

TEST(CGuageBar, SetCurValueRepositions) {
    cGuageBar bar;
    bar.InitGuageBar(80, false);
    bar.InitValue(0, 100, 10);
    bar.SetCurValue(75);
    EXPECT_EQ(bar.GetCurValue(), 75);
    EXPECT_FLOAT_EQ(bar.GetBarRelPos(), 60.0f);
    EXPECT_FLOAT_EQ(bar.GetCurRate(), 0.75f);
}

TEST(CGuageBar, SetMinValueRepositions) {
    cGuageBar bar;
    bar.InitGuageBar(100, false);
    bar.InitValue(0, 100, 50);
    bar.SetMinValue(25);
    EXPECT_EQ(bar.GetMinValue(), 25);
    EXPECT_NEAR(bar.GetBarRelPos(), 33.333333f, 0.00001f);
}

TEST(CGuageBar, SetMaxValueRepositions) {
    cGuageBar bar;
    bar.InitGuageBar(100, false);
    bar.InitValue(0, 100, 50);
    bar.SetMaxValue(200);
    EXPECT_EQ(bar.GetMaxValue(), 200);
    EXPECT_FLOAT_EQ(bar.GetBarRelPos(), 25.0f);
}

TEST(CGuageBar, InvalidValueRangePreservesExistingPosition) {
    cGuageBar bar;
    bar.InitGuageBar(100, false);
    bar.InitValue(0, 100, 50);
    bar.SetMaxValue(0);
    EXPECT_EQ(bar.GetMaxValue(), 0);
    EXPECT_FLOAT_EQ(bar.GetBarRelPos(), 50.0f);
}

TEST(CGuageBar, SetIntervalRepositionsUsingNewInterval) {
    cGuageBar bar;
    bar.InitGuageBar(100, false);
    bar.InitValue(0, 100, 25);
    bar.SetInterval(200);
    EXPECT_EQ(bar.GetInterval(), 200);
    EXPECT_FLOAT_EQ(bar.GetBarRelPos(), 50.0f);
    EXPECT_FLOAT_EQ(bar.GetCurRate(), 0.25f);
}

TEST(CGuageBar, SetCurRateUpdatesPositionAndBaseValue) {
    cGuageBar bar;
    bar.InitGuageBar(100, false);
    bar.SetCurRate(0.4f);
    EXPECT_FLOAT_EQ(bar.GetBarRelPos(), 40.0f);
    EXPECT_FLOAT_EQ(bar.GetCurRate(), 0.4f);
    EXPECT_FLOAT_EQ(bar.GetValue(), 0.4f);
}

TEST(CGuageBar, SetCurRatePreservesLegacyUnclampedNegativeRate) {
    cGuageBar bar;
    bar.InitGuageBar(100, false);
    bar.SetCurRate(-0.25f);
    EXPECT_FLOAT_EQ(bar.GetBarRelPos(), -25.0f);
    EXPECT_FLOAT_EQ(bar.GetCurRate(), -0.25f);
    EXPECT_FLOAT_EQ(bar.GetValue(), -0.25f);
}

TEST(CGuageBar, SetCurRateLeavesLogicalRateAboveOne) {
    cGuageBar bar;
    bar.InitGuageBar(100, false);
    bar.SetCurRate(1.5f);
    EXPECT_FLOAT_EQ(bar.GetBarRelPos(), 150.0f);
    EXPECT_FLOAT_EQ(bar.GetCurRate(), 1.5f);
    EXPECT_FLOAT_EQ(bar.GetValue(), 1.0f);
}

TEST(CGuageBar, AddHorizontalButtonShrinksIntervalAndPositionsButton) {
    cGuageBar bar;
    cButton button;
    bar.InitGuageBar(100, false);
    bar.SetAbsXY(50, 60);
    button.Init(4, 5, 20, 10, nullptr, nullptr, nullptr, nullptr, nullptr, 0);
    bar.Add(&button);
    EXPECT_EQ(bar.GetInterval(), 80);
    EXPECT_EQ(button.absX(), 54);
    EXPECT_EQ(button.absY(), 65);
}

TEST(CGuageBar, AddVerticalButtonShrinksByHeight) {
    cGuageBar bar;
    cButton button;
    bar.InitGuageBar(100, true);
    button.Init(4, 5, 20, 30, nullptr, nullptr, nullptr, nullptr, nullptr, 0);
    bar.Add(&button);
    EXPECT_EQ(bar.GetInterval(), 70);
    EXPECT_TRUE(bar.IsVertical());
}

TEST(CGuageBar, AddClampsIntervalAfterButtonConsumesTrack) {
    cGuageBar bar;
    cButton button;
    bar.InitGuageBar(10, false);
    button.Init(0, 0, 25, 5, nullptr, nullptr, nullptr, nullptr, nullptr, 0);
    bar.Add(&button);
    EXPECT_EQ(bar.GetInterval(), 1);
}

TEST(CGuageBar, AddNullButtonIsSafe) {
    cGuageBar bar;
    bar.InitGuageBar(50, false);
    bar.Add(nullptr);
    EXPECT_EQ(bar.GetInterval(), 50);
}

TEST(CGuageBar, AddSecondButtonIsIgnored) {
    cGuageBar bar;
    cButton first;
    cButton second;
    bar.InitGuageBar(100, false);
    first.Init(0, 0, 10, 5, nullptr, nullptr, nullptr, nullptr, nullptr, 0);
    second.Init(0, 0, 30, 5, nullptr, nullptr, nullptr, nullptr, nullptr, 0);
    bar.Add(&first);
    bar.Add(&second);
    EXPECT_EQ(bar.GetInterval(), 90);
    EXPECT_EQ(second.absX(), 0);
}

TEST(CGuageBar, SetAbsXYMovesAttachedButtonWithRelativeOffset) {
    cGuageBar bar;
    cButton button;
    bar.InitGuageBar(100, false);
    button.Init(7, 9, 10, 10, nullptr, nullptr, nullptr, nullptr, nullptr, 0);
    bar.Add(&button);
    bar.SetAbsXY(100, 200);
    EXPECT_EQ(bar.absX(), 100);
    EXPECT_EQ(bar.absY(), 200);
    EXPECT_EQ(button.absX(), 107);
    EXPECT_EQ(button.absY(), 209);
}

TEST(CGuageBar, SetGuageLockStoresColorAndAppliesToButton) {
    cGuageBar bar;
    cButton button;
    button.Init(0, 0, 10, 10, nullptr, nullptr, nullptr, nullptr, nullptr, 0);
    bar.Add(&button);
    bar.SetGuageLock(true, 0x80112233u);
    EXPECT_TRUE(bar.IsLocked());
    EXPECT_EQ(bar.GetBarBtnColor(), 0x80112233u);
    EXPECT_EQ(button.imageRGB(), 0x80112233u);
    bar.SetGuageLock(false, 0xFF445566u);
    EXPECT_FALSE(bar.IsLocked());
    EXPECT_EQ(button.imageRGB(), 0xFF445566u);
}

TEST(CGuageBar, ActionEventIsInactiveUntilActivated) {
    cGuageBar bar;
    EXPECT_EQ(bar.ActionEvent(), 0u);
    bar.SetActive(true);
    EXPECT_EQ(bar.ActionEvent(), 0u);
}

TEST(CGuageBar, ActionEventIsSuppressedWhenLocked) {
    cGuageBar bar;
    bar.SetActive(true);
    bar.InitGuageBar(100, false);
    bar.SetCurRate(0.3f);
    bar.SetGuageLock(true, 0xFFFFFFFFu);
    EXPECT_EQ(bar.ActionEvent(), 0u);
    EXPECT_FLOAT_EQ(bar.GetCurRate(), 0.3f);
}

TEST(CGuageBar, ActionEventMirrorsRateToGuagen) {
    cGuageBar bar;
    bar.SetActive(true);
    bar.InitGuageBar(100, false);
    bar.SetCurRate(0.65f);
    EXPECT_EQ(bar.ActionEvent(), 0u);
    EXPECT_FLOAT_EQ(bar.GetValue(), 0.65f);
}

TEST(CGuageBar, RenderWithAttachedButtonIsSafe) {
    cGuageBar bar;
    cButton button;
    bar.InitGuageBar(100, false);
    button.Init(0, 0, 10, 10, nullptr, nullptr, nullptr, nullptr, nullptr, 0);
    bar.Add(&button);
    EXPECT_NO_THROW(bar.Render());
}

}  // namespace mxh::ui::test
