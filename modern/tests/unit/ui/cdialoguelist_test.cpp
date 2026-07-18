// cdialoguelist_test.cpp — Phase 6.15 / 0.13.49 coverage for
// cDialogueList. Tests the data model + AddLine / GetDialogue
// + color accessors. File loading is stubbed (CMHFile is
// engine-side); the modern port's data-side primitive is
// AddLine.

#include "cdialoguelist.hpp"

#include <gtest/gtest.h>

TEST(CDialogueList, DefaultConstruction) {
    mxh::ui::cDialogueList d;
    EXPECT_EQ(d.GetDefaultColor(), mxh::ui::NORMAL_COLOR_DEFAULT);
    EXPECT_EQ(d.GetStressColor(), mxh::ui::STRESS_COLOR_DEFAULT);
    // All 12800 msg-id slots are empty.
    for (std::uint32_t i = 0; i < mxh::ui::MAX_DIALOGUE_COUNT; ++i) {
        EXPECT_EQ(d.GetDialoguesForTesting()[i].size(), 0u);
    }
}

TEST(CDialogueList, InheritsBaselineApi) {
    mxh::ui::cDialogueList d;
    // Smoke check: file-loading + parsing are no-ops (no crash).
    d.LoadDialogueListFile("test.txt", "rt");
    d.LoadDialogueList(42, nullptr);
    d.ParsingLine(42, "hello");
    SUCCEED();
}

TEST(CDialogueList, ColorSetterRoundTrip) {
    mxh::ui::cDialogueList d;
    d.SetDefaultColor(0xFF112233u);
    EXPECT_EQ(d.GetDefaultColor(), 0xFF112233u);
    d.SetStressColor(0xFFAABBCCu);
    EXPECT_EQ(d.GetStressColor(), 0xFFAABBCCu);
}

TEST(CDialogueList, AddLineAppends) {
    mxh::ui::cDialogueList d;
    d.AddLine(42, "hello world", 0xFFFFFFFFu, 0, 0);
    d.AddLine(42, "second line", 0xFFFFFF00u, 1, 0);
    EXPECT_EQ(d.GetDialoguesForTesting()[42].size(), 2u);
}

TEST(CDialogueList, GetDialogueReturnsPointer) {
    mxh::ui::cDialogueList d;
    d.AddLine(42, "first", 0xFF000000u, 0, 0);
    d.AddLine(42, "second", 0xFFFF0000u, 1, 0);
    auto* p0 = d.GetDialogue(42, 0);
    auto* p1 = d.GetDialogue(42, 1);
    ASSERT_NE(p0, nullptr);
    ASSERT_NE(p1, nullptr);
    EXPECT_STREQ(p0->str, "first");
    EXPECT_STREQ(p1->str, "second");
    EXPECT_EQ(p0->wLine, 0u);
    EXPECT_EQ(p1->wLine, 1u);
}

TEST(CDialogueList, GetDialogueOutOfRangeReturnsNull) {
    mxh::ui::cDialogueList d;
    d.AddLine(42, "only", 0xFFFFFFFFu, 0, 0);
    EXPECT_EQ(d.GetDialogue(42, 1), nullptr);
    EXPECT_EQ(d.GetDialogue(42, 99), nullptr);
    EXPECT_EQ(d.GetDialogue(9999, 0), nullptr);
    EXPECT_EQ(d.GetDialogue(99999, 0), nullptr);
}

TEST(CDialogueList, AddLineOutOfRangeIgnored) {
    mxh::ui::cDialogueList d;
    d.AddLine(99999, "should be ignored", 0xFFFFFFFFu, 0, 0);
    // No crash, no state change.
    SUCCEED();
}

TEST(CDialogueList, AddLineCopiesString) {
    // 1:1 with legacy: the legacy uses strcpy; modern uses
    // strncpy with NUL terminator.
    mxh::ui::cDialogueList d;
    const char* msg = "test message";
    d.AddLine(42, msg, 0xFFFFFFFFu, 0, 0);
    auto* p = d.GetDialogue(42, 0);
    ASSERT_NE(p, nullptr);
    EXPECT_STREQ(p->str, msg);
    // Modifying the source doesn't affect the stored copy.
    msg = "modified";
    EXPECT_STREQ(p->str, "test message");
}

TEST(CDialogueList, AddLineWithNullStringIsSafe) {
    mxh::ui::cDialogueList d;
    d.AddLine(42, nullptr, 0xFFFFFFFFu, 0, 0);
    auto* p = d.GetDialogue(42, 0);
    ASSERT_NE(p, nullptr);
    EXPECT_STREQ(p->str, "");  // empty
}

TEST(CDialogueList, AddLineWithLongStringTruncates) {
    mxh::ui::cDialogueList d;
    std::string longStr(2000, 'x');
    d.AddLine(42, longStr.c_str(), 0xFFFFFFFFu, 0, 0);
    auto* p = d.GetDialogue(42, 0);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(std::strlen(p->str), 1023u);  // truncated to DIALOGUE::str size - 1
}

TEST(CDialogueList, DialogueStructInit) {
    mxh::ui::DIALOGUE d{};
    d.Init();
    EXPECT_EQ(d.dwColor, 0xFFFFFFFFu);
    EXPECT_EQ(d.wLine, 0u);
    EXPECT_EQ(d.wType, 0u);
    EXPECT_STREQ(d.str, "");
}

TEST(CDialogueList, AddLineStoresColorAndLineAndType) {
    mxh::ui::cDialogueList d;
    d.AddLine(42, "msg", 0xFFAA00CCu, 7, 3);
    auto* p = d.GetDialogue(42, 0);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->dwColor, 0xFFAA00CCu);
    EXPECT_EQ(p->wLine, 7u);
    EXPECT_EQ(p->wType, 3u);
}

TEST(CDialogueList, AddLineMultipleMsgIdsAreIndependent) {
    mxh::ui::cDialogueList d;
    d.AddLine(1, "msg1", 0xFFFFFFFFu, 0, 0);
    d.AddLine(2, "msg2", 0xFFFFFFFFu, 0, 0);
    d.AddLine(3, "msg3", 0xFFFFFFFFu, 0, 0);
    EXPECT_EQ(d.GetDialoguesForTesting()[1].size(), 1u);
    EXPECT_EQ(d.GetDialoguesForTesting()[2].size(), 1u);
    EXPECT_EQ(d.GetDialoguesForTesting()[3].size(), 1u);
    EXPECT_STREQ(d.GetDialogue(1, 0)->str, "msg1");
    EXPECT_STREQ(d.GetDialogue(2, 0)->str, "msg2");
    EXPECT_STREQ(d.GetDialogue(3, 0)->str, "msg3");
}

TEST(CDialogueList, AddLineRespectsExplicitLineIndex) {
    // 1:1 with legacy: AddLine takes an explicit wLine arg;
    // the legacy uses cPtrList::AddTail which appends in
    // order. Modern uses vector::push_back, which also
    // appends.
    mxh::ui::cDialogueList d;
    d.AddLine(42, "line 5", 0xFFFFFFFFu, 5, 0);
    d.AddLine(42, "line 6", 0xFFFFFFFFu, 6, 0);
    EXPECT_EQ(d.GetDialogue(42, 0)->wLine, 5u);
    EXPECT_EQ(d.GetDialogue(42, 1)->wLine, 6u);
}

TEST(CDialogueList, MaxDialogueCountIsStable) {
    EXPECT_EQ(mxh::ui::MAX_DIALOGUE_COUNT, 12800u);
    mxh::ui::cDialogueList d;
    EXPECT_EQ(d.GetDialoguesForTesting().size(),
              static_cast<std::size_t>(mxh::ui::MAX_DIALOGUE_COUNT));
}
