// cmultilinetext_test.cpp — Phase 6.13 coverage for cMultiLineText.

#include "cMultiLineText.hpp"

#include <gtest/gtest.h>

TEST(CMultiLineText, DefaultStateIsInvalidAndEmpty) {
    mxh::ui::cMultiLineText m;
    EXPECT_FALSE(m.IsValid());
    EXPECT_EQ(m.LineCount(), 0u);
    EXPECT_TRUE(m.Empty());
}

TEST(CMultiLineText, InitMarksValidAndStoresConfig) {
    mxh::ui::cMultiLineText m;
    m.Init(2, 0xFF112233u, nullptr, 0xFFAABBCCu);
    EXPECT_TRUE(m.IsValid());
    EXPECT_EQ(m.GetFontIdx(), 2u);
    EXPECT_EQ(m.GetFGColor(), 0xFF112233u);
}

TEST(CMultiLineText, SetTextSplitsOnNewlines) {
    mxh::ui::cMultiLineText m;
    m.Init(0, 0xFF000000u);
    m.SetText("line1\nline2\nline3");
    EXPECT_EQ(m.LineCount(), 3u);
    EXPECT_EQ(m.GetLine(0).text, "line1");
    EXPECT_EQ(m.GetLine(1).text, "line2");
    EXPECT_EQ(m.GetLine(2).text, "line3");
}

TEST(CMultiLineText, SetTextWithTrailingNewlineDoesNotAddBlank) {
    mxh::ui::cMultiLineText m;
    m.Init(0, 0xFF000000u);
    m.SetText("a\nb\n");
    EXPECT_EQ(m.LineCount(), 2u);
    EXPECT_EQ(m.GetLine(0).text, "a");
    EXPECT_EQ(m.GetLine(1).text, "b");
}

TEST(CMultiLineText, SetTextEmptyClearsLines) {
    mxh::ui::cMultiLineText m;
    m.Init(0, 0xFF000000u);
    m.AddLine("a");
    m.AddLine("b");
    EXPECT_EQ(m.LineCount(), 2u);
    m.SetText("");
    EXPECT_EQ(m.LineCount(), 0u);
}

TEST(CMultiLineText, AddLineAppendsWithColor) {
    mxh::ui::cMultiLineText m;
    m.Init(0, 0xFF000000u);
    m.AddLine("hello", 0xFFFF0000u);
    m.AddLine("world", 0xFF00FF00u);
    EXPECT_EQ(m.LineCount(), 2u);
    EXPECT_EQ(m.GetLine(0).text, "hello");
    EXPECT_EQ(m.GetLine(0).color, 0xFFFF0000u);
    EXPECT_EQ(m.GetLine(1).color, 0xFF00FF00u);
    EXPECT_EQ(m.GetLine(0).len, 5u);
}

TEST(CMultiLineText, AddLineOverloadFromStdString) {
    mxh::ui::cMultiLineText m;
    m.Init(0, 0xFF000000u);
    m.AddLine(std::string("std string line"));
    EXPECT_EQ(m.LineCount(), 1u);
    EXPECT_EQ(m.GetLine(0).text, "std string line");
    EXPECT_EQ(m.GetLine(0).len, 15u);
}

TEST(CMultiLineText, AddNamePannelAddsSpacedLine) {
    mxh::ui::cMultiLineText m;
    m.Init(0, 0xFF000000u);
    m.AddNamePannel(10);
    EXPECT_EQ(m.LineCount(), 1u);
    EXPECT_EQ(m.GetLine(0).len, 10u);
    EXPECT_EQ(m.GetLine(0).text.size(), 10u);
    EXPECT_EQ(m.GetLine(0).text, "          ");
}

TEST(CMultiLineText, ReleaseClearsAndInvalidates) {
    mxh::ui::cMultiLineText m;
    m.Init(0, 0xFF000000u);
    m.AddLine("a"); m.AddLine("b");
    m.Release();
    EXPECT_FALSE(m.IsValid());
    EXPECT_EQ(m.LineCount(), 0u);
    EXPECT_TRUE(m.Empty());
}

TEST(CMultiLineText, PositionSetters) {
    mxh::ui::cMultiLineText m;
    m.SetXY(15, 25);
    EXPECT_EQ(m.GetX(), 15);
    EXPECT_EQ(m.GetY(), 25);
}

TEST(CMultiLineText, FontAndColorAndImageSetters) {
    mxh::ui::cMultiLineText m;
    m.SetFontIdx(5);
    EXPECT_EQ(m.GetFontIdx(), 5u);
    m.SetFGColor(0xFFAABBCCu);
    EXPECT_EQ(m.GetFGColor(), 0xFFAABBCCu);
    m.SetImageRGB(0xFF112233u);
    m.SetImageAlpha(0x80000000u);
    m.SetOptionAlpha(0x80000000u);
    // No getter for those — setters must not crash. This test is a
    // regression guard for the public setter surface.
}

TEST(CMultiLineText, OperatorAssignFromCString) {
    mxh::ui::cMultiLineText m;
    m.Init(0, 0xFF000000u);
    m = "first\nsecond";
    EXPECT_EQ(m.LineCount(), 2u);
    EXPECT_EQ(m.GetLine(0).text, "first");
    EXPECT_EQ(m.GetLine(1).text, "second");
}

TEST(CMultiLineText, GetLineOutOfRangeReturnsEmpty) {
    mxh::ui::cMultiLineText m;
    m.Init(0, 0xFF000000u);
    m.AddLine("only");
    const auto& out = m.GetLine(99);
    EXPECT_EQ(out.text, "");
    EXPECT_EQ(out.len,  0u);
}

TEST(CMultiLineText, LinesAccessorReadOnly) {
    mxh::ui::cMultiLineText m;
    m.Init(0, 0xFF000000u);
    m.AddLine("a");
    m.AddLine("b");
    const auto& lines = m.Lines();
    EXPECT_EQ(lines.size(), 2u);
    auto it = lines.begin();
    EXPECT_EQ(it->text, "a");
    ++it;
    EXPECT_EQ(it->text, "b");
}
