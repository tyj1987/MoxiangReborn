// cstatic_test.cpp — Phase 6.12 coverage for cStatic (text label).

#include "cStatic.hpp"

#include <gtest/gtest.h>

TEST(CStatic, DefaultTextIsEmpty) {
    mxh::ui::cStatic s;
    EXPECT_EQ(s.GetStaticText(), "");
    EXPECT_EQ(s.GetStaticValue(), 0);
}

TEST(CStatic, SetStaticTextStoresAndReturns) {
    mxh::ui::cStatic s;
    s.SetStaticText("Hello");
    EXPECT_EQ(s.GetStaticText(), "Hello");
    s.SetStaticText("World");
    EXPECT_EQ(s.GetStaticText(), "World");
}

TEST(CStatic, SetStaticValueFormatsAsDecimal) {
    mxh::ui::cStatic s;
    s.SetStaticValue(42);
    EXPECT_EQ(s.GetStaticText(), "42");
    s.SetStaticValue(-1);
    EXPECT_EQ(s.GetStaticText(), "-1");
}

TEST(CStatic, GetStaticValueParsesText) {
    mxh::ui::cStatic s;
    s.SetStaticText("123");
    EXPECT_EQ(s.GetStaticValue(), 123);
    s.SetStaticText("not a number");
    EXPECT_EQ(s.GetStaticValue(), 0);
}

TEST(CStatic, FontIdxSetterGetter) {
    mxh::ui::cStatic s;
    EXPECT_EQ(s.GetFontIdx(), 0u);
    s.SetFontIdx(7);
    EXPECT_EQ(s.GetFontIdx(), 7u);
}

TEST(CStatic, MultiLineSetterGetter) {
    mxh::ui::cStatic s;
    EXPECT_FALSE(s.IsMultiLine());
    s.SetMultiLine(true);
    EXPECT_TRUE(s.IsMultiLine());
    s.SetMultiLine(false);
    EXPECT_FALSE(s.IsMultiLine());
}

TEST(CStatic, TextXYStoresCoordinates) {
    // 1:1 with legacy cStatic::SetTextXY: the text-position
    // offset is stored verbatim and survives a re-Init
    // (cDialog::Init only resets the dialog's abs/rel/valid
    // xy + id, not the cStatic's text offset).
    mxh::ui::cStatic s;
    s.SetTextXY(13, 2);
    EXPECT_EQ(s.GetTextX(), 13);
    EXPECT_EQ(s.GetTextY(), 2);
    // Re-Init must not clobber the text offset.
    s.Init(0, 0, 100, 20, nullptr, 5);
    EXPECT_EQ(s.GetTextX(), 13);
    EXPECT_EQ(s.GetTextY(), 2);
    // SetTextXY is a re-write, not an add — call it again
    // and verify the values track the new args.
    s.SetTextXY(7, 9);
    EXPECT_EQ(s.GetTextX(), 7);
    EXPECT_EQ(s.GetTextY(), 9);
}

TEST(CStatic, FGAndShadowColorSetters) {
    mxh::ui::cStatic s;
    s.SetFGColor(0xFF112233u);
    EXPECT_EQ(s.GetFGColor(), 0xFF112233u);
    s.SetShadow(true);
    s.SetShadowTextXY(1, 1);
    s.SetShadowColor(0xFFAABBCCu);
    EXPECT_TRUE(s.HasShadow());
}

TEST(CStatic, AlignSetterGetter) {
    mxh::ui::cStatic s;
    EXPECT_EQ(s.GetAlign(), mxh::ui::cStatic::Align::Left);
    s.SetAlign(mxh::ui::cStatic::Align::Center);
    EXPECT_EQ(s.GetAlign(), mxh::ui::cStatic::Align::Center);
    s.SetAlign(mxh::ui::cStatic::Align::Right);
    EXPECT_EQ(s.GetAlign(), mxh::ui::cStatic::Align::Right);
}

TEST(CStatic, ShadowTextXYSetterGetter) {
    // 1:1 with legacy cStatic::SetShadowTextXY + the new
    // GetShadowTextX / GetShadowTextY accessors added to round
    // out the (x, y) setter pair (test was previously missing
    // the equivalent round-trip for the shadow text offset).
    mxh::ui::cStatic s;
    s.SetShadowTextXY(2, 3);
    EXPECT_EQ(s.GetShadowTextX(), 2);
    EXPECT_EQ(s.GetShadowTextY(), 3);
    s.SetShadowTextXY(-1, -1);
    EXPECT_EQ(s.GetShadowTextX(), -1);
    EXPECT_EQ(s.GetShadowTextY(), -1);
}

TEST(CStatic, ShadowColorSetterGetter) {
    // 1:1 with legacy cStatic::SetShadowColor + the new
    // GetShadowColor accessor (test was previously missing
    // the equivalent round-trip for the shadow color value).
    mxh::ui::cStatic s;
    EXPECT_EQ(s.GetShadowColor(), 0xFF000000u);  // default black
    s.SetShadowColor(0xFFAABBCCu);
    EXPECT_EQ(s.GetShadowColor(), 0xFFAABBCCu);
}
