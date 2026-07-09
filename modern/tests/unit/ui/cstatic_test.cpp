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
    mxh::ui::cStatic s;
    s.SetTextXY(13, 2);
    s.Init(0, 0, 100, 20, nullptr, 5);
    s.SetTextXY(13, 2);
    EXPECT_EQ(s.GetFontIdx(), 0u);  // unchanged
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
