// cpage_test.cpp — Phase 6.15 / 0.13.49 coverage for cPage +
// cPageBase. Tests the data model + state machine + dialogue /
// hyperlink management. Engine-side cLinkedList / HYPERLINK are
// replaced with std::vector + a placeholder struct (see cpage.hpp
// / cpage.cpp for the modern-port simplifications).

#include "cpage.hpp"

#include <gtest/gtest.h>

namespace {

// Test-injectable counter for GetRandomDialogue.
std::uint32_t g_testRand = 0;
std::uint32_t TestRandFn() { return g_testRand; }

}  // namespace

TEST(CPageBase, DefaultConstruction) {
    mxh::ui::cPageBase p;
    EXPECT_EQ(p.GetPageId(), 0u);
    EXPECT_EQ(p.GetDialogueCount(), 0);
    EXPECT_EQ(p.GetNextPageId(), -1);
    EXPECT_EQ(p.GetPrevPageId(), -1);
}

TEST(CPageBase, InitSetsPageId) {
    mxh::ui::cPageBase p;
    p.Init(42);
    EXPECT_EQ(p.GetPageId(), 42u);
}

TEST(CPageBase, AddDialogueAppends) {
    mxh::ui::cPageBase p;
    p.AddDialogue(100);
    p.AddDialogue(200);
    p.AddDialogue(300);
    EXPECT_EQ(p.GetDialogueCount(), 3);
}

TEST(CPageBase, GetRandomDialogueEmptyReturnsZero) {
    mxh::ui::cPageBase p;
    EXPECT_EQ(p.GetRandomDialogue(), 0u);
}

TEST(CPageBase, GetRandomDialogueSingleReturnsThatOne) {
    mxh::ui::cPageBase p;
    p.AddDialogue(42);
    EXPECT_EQ(p.GetRandomDialogue(), 42u);
}

TEST(CPageBase, GetRandomDialogueMultipleUsesTestCounter) {
    mxh::ui::cPageBase p;
    p.AddDialogue(100);
    p.AddDialogue(200);
    p.AddDialogue(300);
    mxh::ui::SetPageRandomForTesting(&TestRandFn);
    g_testRand = 0;  EXPECT_EQ(p.GetRandomDialogue(), 100u);
    g_testRand = 1;  EXPECT_EQ(p.GetRandomDialogue(), 200u);
    g_testRand = 2;  EXPECT_EQ(p.GetRandomDialogue(), 300u);
    g_testRand = 3;  EXPECT_EQ(p.GetRandomDialogue(), 100u);  // wraps
    g_testRand = 99; EXPECT_EQ(p.GetRandomDialogue(), 100u);  // wraps
    mxh::ui::SetPageRandomForTesting(nullptr);
}

TEST(CPageBase, NextPrevPageIdRoundTrip) {
    mxh::ui::cPageBase p;
    EXPECT_EQ(p.GetNextPageId(), -1);
    p.SetNextPageId(5);
    EXPECT_EQ(p.GetNextPageId(), 5);
    p.SetPrevPageId(3);
    EXPECT_EQ(p.GetPrevPageId(), 3);
}

TEST(CPageBase, RemoveAllClearsState) {
    mxh::ui::cPageBase p;
    p.Init(10);
    p.AddDialogue(1);
    p.AddDialogue(2);
    p.SetNextPageId(20);
    p.SetPrevPageId(5);
    p.RemoveAll();
    EXPECT_EQ(p.GetDialogueCount(), 0);
    EXPECT_EQ(p.GetPageId(), 0u);
    EXPECT_EQ(p.GetNextPageId(), -1);
    EXPECT_EQ(p.GetPrevPageId(), -1);
}

TEST(CPage, DefaultConstructionHasNoHyperlinks) {
    mxh::ui::cPage p;
    EXPECT_EQ(p.GetHyperLinkCount(), 0);
}

TEST(CPage, AddHyperLinkAppends) {
    mxh::ui::cPage p;
    mxh::ui::HYPERLINK link{};
    link.wLinkId   = 1;
    link.wLinkType = 1;  // emLink_Page
    link.dwData    = 42; // target page id
    p.AddHyperLink(&link);
    EXPECT_EQ(p.GetHyperLinkCount(), 1);
}

TEST(CPage, GetHyperTextReturnsPointer) {
    mxh::ui::cPage p;
    mxh::ui::HYPERLINK link{};
    link.wLinkId   = 7;
    link.wLinkType = 1;
    link.dwData    = 42;
    p.AddHyperLink(&link);
    const auto* pLink = p.GetHyperText(0);
    ASSERT_NE(pLink, nullptr);
    EXPECT_EQ(pLink->wLinkId, 7u);
    EXPECT_EQ(pLink->wLinkType, 1u);
    EXPECT_EQ(pLink->dwData, 42u);
}

TEST(CPage, GetHyperTextOutOfRangeReturnsNull) {
    mxh::ui::cPage p;
    EXPECT_EQ(p.GetHyperText(0), nullptr);
    EXPECT_EQ(p.GetHyperText(99), nullptr);
}

TEST(CPage, RemoveAllClearsDialoguesAndHyperlinks) {
    mxh::ui::cPage p;
    p.AddDialogue(1);
    p.AddDialogue(2);
    mxh::ui::HYPERLINK link{};
    link.wLinkId = 3;
    link.dwData  = 100;
    p.AddHyperLink(&link);
    EXPECT_EQ(p.GetDialogueCount(), 2);
    EXPECT_EQ(p.GetHyperLinkCount(), 1);
    p.RemoveAll();
    EXPECT_EQ(p.GetDialogueCount(), 0);
    EXPECT_EQ(p.GetHyperLinkCount(), 0);
}

TEST(CPage, AddNullHyperlinkIsSafe) {
    mxh::ui::cPage p;
    p.AddHyperLink(nullptr);
    EXPECT_EQ(p.GetHyperLinkCount(), 0);
}

TEST(CPage, InheritsPageBaseBehavior) {
    mxh::ui::cPage p;
    p.Init(100);
    p.AddDialogue(1);
    p.AddDialogue(2);
    EXPECT_EQ(p.GetPageId(), 100u);
    EXPECT_EQ(p.GetDialogueCount(), 2);
    EXPECT_EQ(p.GetNextPageId(), -1);
}

TEST(CPage, MultipleHyperlinks) {
    mxh::ui::cPage p;
    for (std::uint32_t i = 0; i < 5; ++i) {
        mxh::ui::HYPERLINK link{};
        link.wLinkId = static_cast<std::uint16_t>(i);
        link.dwData  = i * 10;
        p.AddHyperLink(&link);
    }
    EXPECT_EQ(p.GetHyperLinkCount(), 5);
    for (std::uint32_t i = 0; i < 5; ++i) {
        const auto* pLink = p.GetHyperText(i);
        ASSERT_NE(pLink, nullptr);
        EXPECT_EQ(pLink->wLinkId, i);
        EXPECT_EQ(pLink->dwData, i * 10u);
    }
}
