// helpdialog_test.cpp — Phase 6.16 / 0.13.49 coverage for
// cHelpDialog (in-game help browser). Tests the data model +
// state machine + HYPER array + OpenDialog / OpenLinkPage /
// EndDialog / HyperLinkParser dispatch. The engine-side
// HELPDICMGR singleton is replaced with SetContent(mainPage,
// dialogueList, hyperTextList); tests inject a fixed main page
// + dialogue list + hyper text list to exercise the flow.

#include "helpdialog.hpp"

#include "cDialogueList.hpp"
#include "cHyperTextList.hpp"
#include "cListDialogEx.hpp"
#include "cPage.hpp"

#include <gtest/gtest.h>

#include <memory>

namespace {

// BuildDlgWithChildren: 1:1 with the legacy resource-loader
// step. Adds the 1 cListDialogEx child before Linking so that
// findWindowById can resolve it.
void BuildDlgWithChildren(mxh::ui::cHelpDialog& d) {
    d.Init(0, 0, 400, 400, nullptr, 0);
    auto list = std::make_unique<mxh::ui::cListDialogEx>();
    list->InitLinkList(50);
    list->Init(0, 0, 0, 0, nullptr, mxh::ui::cHelpDialog::ID_LISTDLG);
    d.Add(std::unique_ptr<mxh::ui::cWindow>(list.release()));
    d.Linking();
}

// BuildContent: 1:1 with the engine-binder layer's job. Creates
// a cPage with 2 dialogues + 1 hyperlink, a cDialogueList with
// 1 msg id + 2 lines, a cHyperTextList with 1 entry. Returns
// the 3 owned objects via output pointers (caller takes
// ownership via unique_ptr).
void BuildContent(std::unique_ptr<mxh::ui::cPage>& outPage,
                  std::unique_ptr<mxh::ui::cDialogueList>& outDialogues,
                  std::unique_ptr<mxh::ui::cHyperTextList>& outHyper) {
    outPage = std::make_unique<mxh::ui::cPage>();
    outPage->Init(100);  // page id 100
    outPage->AddDialogue(1);
    outPage->AddDialogue(1);  // same msg id, second line

    outDialogues = std::make_unique<mxh::ui::cDialogueList>();
    outDialogues->AddLine(1, "first dialogue",  0xFFFFFFFFu, 0, 0);
    outDialogues->AddLine(1, "second dialogue", 0xFFFFFF00u, 1, 0);

    outHyper = std::make_unique<mxh::ui::cHyperTextList>();
    outHyper->AddEntry(42, "click here to navigate");
}

}  // namespace

TEST(CHelpDialog, DefaultConstructionIsValid) {
    mxh::ui::cHelpDialog d;
    EXPECT_EQ(d.GetListDlg(), nullptr);
    EXPECT_EQ(d.GetCurPageId(), 0u);
    EXPECT_EQ(d.GetHyperCount(), 0);
    EXPECT_EQ(d.GetMainPage(), nullptr);
    EXPECT_EQ(d.GetDialogueList(), nullptr);
    EXPECT_EQ(d.GetHyperTextList(), nullptr);
}

TEST(CHelpDialog, InheritsDialogTreeManagement) {
    mxh::ui::cHelpDialog d;
    EXPECT_EQ(d.childCount(), 0u);  // before Linking
}

TEST(CHelpDialog, ConstantsAreStable) {
    EXPECT_EQ(mxh::ui::cHelpDialog::MAX_REGIST_HYPERLINK, 70u);
    EXPECT_EQ(mxh::ui::cHelpDialog::ID_LISTDLG, 1);
    // 1:1 with legacy emLink_* values.
    EXPECT_EQ(static_cast<int>(mxh::ui::cHelpDialog::emLink_Null),  0);
    EXPECT_EQ(static_cast<int>(mxh::ui::cHelpDialog::emLink_Page),  1);
    EXPECT_EQ(static_cast<int>(mxh::ui::cHelpDialog::emLink_End),   2);
    EXPECT_EQ(static_cast<int>(mxh::ui::cHelpDialog::emLink_Open),  3);
}

TEST(CHelpDialog, LinkingResolvesListDialog) {
    mxh::ui::cHelpDialog d;
    BuildDlgWithChildren(d);
    EXPECT_NE(d.GetListDlg(), nullptr);
}

TEST(CHelpDialog, SetContentStoresPointers) {
    mxh::ui::cHelpDialog d;
    std::unique_ptr<mxh::ui::cPage> page;
    std::unique_ptr<mxh::ui::cDialogueList> dialogues;
    std::unique_ptr<mxh::ui::cHyperTextList> hyper;
    BuildContent(page, dialogues, hyper);
    d.SetContent(page.get(), dialogues.get(), hyper.get());
    EXPECT_EQ(d.GetMainPage(),       page.get());
    EXPECT_EQ(d.GetDialogueList(),   dialogues.get());
    EXPECT_EQ(d.GetHyperTextList(),  hyper.get());
}

TEST(CHelpDialog, OpenDialogWithoutContentReturnsFalse) {
    // 1:1 with legacy: HELPDICMGR->GetMainPage() == NULL →
    // OpenDialog returns FALSE. Modern port: m_pMainPage null
    // → returns false.
    mxh::ui::cHelpDialog d;
    BuildDlgWithChildren(d);
    EXPECT_FALSE(d.OpenDialog());
    EXPECT_EQ(d.GetHyperCount(), 0);
}

TEST(CHelpDialog, OpenDialogWithEmptyContentSucceeds) {
    // 1:1 with legacy: even if no dialogues / links, OpenDialog
    // returns TRUE. m_dwCurPageId is set from pPage->GetPageId.
    mxh::ui::cHelpDialog d;
    BuildDlgWithChildren(d);
    auto page = std::make_unique<mxh::ui::cPage>();
    page->Init(42);
    auto dialogues = std::make_unique<mxh::ui::cDialogueList>();
    auto hyper     = std::make_unique<mxh::ui::cHyperTextList>();
    d.SetContent(page.get(), dialogues.get(), hyper.get());
    EXPECT_TRUE(d.OpenDialog());
    EXPECT_EQ(d.GetCurPageId(), 42u);
    EXPECT_EQ(d.GetHyperCount(), 0);
}

TEST(CHelpDialog, OpenDialogPopulatesDialogueRows) {
    mxh::ui::cHelpDialog d;
    BuildDlgWithChildren(d);
    std::unique_ptr<mxh::ui::cPage> page;
    std::unique_ptr<mxh::ui::cDialogueList> dialogues;
    std::unique_ptr<mxh::ui::cHyperTextList> hyper;
    BuildContent(page, dialogues, hyper);
    d.SetContent(page.get(), dialogues.get(), hyper.get());
    EXPECT_TRUE(d.OpenDialog());
    // The list has 2 dialogue rows + 0 (no hyperlink added to
    // the page in BuildContent).
    EXPECT_EQ(d.GetListDlg()->LinkItemCount(), 2u);
    EXPECT_EQ(d.GetHyperCount(), 0);
}

TEST(CHelpDialog, OpenDialogPopulatesHyperRows) {
    mxh::ui::cHelpDialog d;
    BuildDlgWithChildren(d);
    auto page = std::make_unique<mxh::ui::cPage>();
    page->Init(100);
    page->AddDialogue(1);
    auto dialogues = std::make_unique<mxh::ui::cDialogueList>();
    dialogues->AddLine(1, "msg", 0xFFFFFFFFu, 0, 0);
    auto hyper = std::make_unique<mxh::ui::cHyperTextList>();
    hyper->AddEntry(42, "click here");
    // Add a hyperlink to the page (target page id = 999, link
    // type = emLink_Page, wLinkId = 42 → looks up cHyperTextList).
    mxh::ui::HYPERLINK link{};
    link.wLinkId   = 42;
    link.wLinkType = static_cast<std::uint16_t>(mxh::ui::cHelpDialog::emLink_Page);
    link.dwData    = 999;
    page->AddHyperLink(&link);
    d.SetContent(page.get(), dialogues.get(), hyper.get());
    EXPECT_TRUE(d.OpenDialog());
    // 1 dialogue row + 3 spacer rows + 1 hyper row = 5 rows.
    EXPECT_EQ(d.GetListDlg()->LinkItemCount(), 5u);
    // 1 hyperlink registered.
    EXPECT_EQ(d.GetHyperCount(), 1);
    // The HYPER entry has bUse=true and points to the last
    // list item (the hyper row).
    const auto* h = d.GetHyperAt(0);
    ASSERT_NE(h, nullptr);
    EXPECT_TRUE(h->bUse);
    EXPECT_EQ(h->dwListItemIdx, 4u);
    EXPECT_EQ(h->sHyper.wLinkId,   42u);
    EXPECT_EQ(h->sHyper.wLinkType, 1u);  // emLink_Page
    EXPECT_EQ(h->sHyper.dwData,    999u);
}

TEST(CHelpDialog, OpenDialogRegistersUpToMaxHyperlinks) {
    // 1:1 with legacy: if more than MAX_REGIST_HYPERLINK links
    // are added, the loop registers up to MAX_REGIST_HYPERLINK
    // and silently drops the rest. Modern port guards with
    // `m_nHyperCount < MAX_REGIST_HYPERLINK`.
    mxh::ui::cHelpDialog d;
    BuildDlgWithChildren(d);
    auto page = std::make_unique<mxh::ui::cPage>();
    page->Init(1);
    page->AddDialogue(1);
    auto dialogues = std::make_unique<mxh::ui::cDialogueList>();
    dialogues->AddLine(1, "msg", 0xFFFFFFFFu, 0, 0);
    auto hyper = std::make_unique<mxh::ui::cHyperTextList>();
    hyper->AddEntry(1, "link1");
    hyper->AddEntry(2, "link2");
    // Add MAX_REGIST_HYPERLINK + 5 hyperlinks.
    for (std::uint32_t i = 0; i < mxh::ui::cHelpDialog::MAX_REGIST_HYPERLINK + 5; ++i) {
        mxh::ui::HYPERLINK link{};
        link.wLinkId   = static_cast<std::uint16_t>(i % 2 + 1);
        link.wLinkType = static_cast<std::uint16_t>(mxh::ui::cHelpDialog::emLink_Page);
        link.dwData    = i;
        page->AddHyperLink(&link);
    }
    d.SetContent(page.get(), dialogues.get(), hyper.get());
    EXPECT_TRUE(d.OpenDialog());
    EXPECT_EQ(d.GetHyperCount(),
              static_cast<int>(mxh::ui::cHelpDialog::MAX_REGIST_HYPERLINK));
}

TEST(CHelpDialog, GetHyperInfoReturnsNullWhenEmpty) {
    mxh::ui::cHelpDialog d;
    BuildDlgWithChildren(d);
    EXPECT_EQ(d.GetHyperInfo(0), nullptr);
    EXPECT_EQ(d.GetHyperInfo(99), nullptr);
}

TEST(CHelpDialog, GetHyperInfoFindsRegisteredIdx) {
    mxh::ui::cHelpDialog d;
    BuildDlgWithChildren(d);
    auto page = std::make_unique<mxh::ui::cPage>();
    page->Init(100);
    page->AddDialogue(1);
    auto dialogues = std::make_unique<mxh::ui::cDialogueList>();
    dialogues->AddLine(1, "msg", 0xFFFFFFFFu, 0, 0);
    auto hyper = std::make_unique<mxh::ui::cHyperTextList>();
    hyper->AddEntry(42, "click");
    mxh::ui::HYPERLINK link{};
    link.wLinkId   = 42;
    link.wLinkType = static_cast<std::uint16_t>(mxh::ui::cHelpDialog::emLink_Page);
    link.dwData    = 999;
    page->AddHyperLink(&link);
    d.SetContent(page.get(), dialogues.get(), hyper.get());
    EXPECT_TRUE(d.OpenDialog());
    // GetHyperInfo(4) should find the hyper at list item 4.
    auto* h = d.GetHyperInfo(4);
    ASSERT_NE(h, nullptr);
    EXPECT_EQ(h->dwListItemIdx, 4u);
    EXPECT_EQ(h->sHyper.dwData, 999u);
}

TEST(CHelpDialog, GetHyperInfoReturnsNullForUnknownIdx) {
    mxh::ui::cHelpDialog d;
    BuildDlgWithChildren(d);
    auto page = std::make_unique<mxh::ui::cPage>();
    page->Init(100);
    page->AddDialogue(1);
    auto dialogues = std::make_unique<mxh::ui::cDialogueList>();
    dialogues->AddLine(1, "msg", 0xFFFFFFFFu, 0, 0);
    auto hyper = std::make_unique<mxh::ui::cHyperTextList>();
    hyper->AddEntry(42, "click");
    mxh::ui::HYPERLINK link{};
    link.wLinkId   = 42;
    link.wLinkType = static_cast<std::uint16_t>(mxh::ui::cHelpDialog::emLink_Page);
    link.dwData    = 999;
    page->AddHyperLink(&link);
    d.SetContent(page.get(), dialogues.get(), hyper.get());
    EXPECT_TRUE(d.OpenDialog());
    // idx=99 is not a hyper row.
    EXPECT_EQ(d.GetHyperInfo(99), nullptr);
}

TEST(CHelpDialog, EndDialogClearsState) {
    mxh::ui::cHelpDialog d;
    BuildDlgWithChildren(d);
    std::unique_ptr<mxh::ui::cPage> page;
    std::unique_ptr<mxh::ui::cDialogueList> dialogues;
    std::unique_ptr<mxh::ui::cHyperTextList> hyper;
    BuildContent(page, dialogues, hyper);
    d.SetContent(page.get(), dialogues.get(), hyper.get());
    EXPECT_TRUE(d.OpenDialog());
    EXPECT_EQ(d.GetHyperCount(), 0);
    // Add a hyperlink then re-open to populate the hyper array.
    mxh::ui::HYPERLINK link{};
    link.wLinkId   = 42;
    link.wLinkType = static_cast<std::uint16_t>(mxh::ui::cHelpDialog::emLink_Page);
    link.dwData    = 999;
    page->AddHyperLink(&link);
    EXPECT_TRUE(d.OpenDialog());
    EXPECT_EQ(d.GetHyperCount(), 1);
    d.EndDialog();
    EXPECT_EQ(d.GetHyperCount(), 0);
    EXPECT_EQ(d.GetListDlg()->LinkItemCount(), 0u);
}

TEST(CHelpDialog, SetActiveFalseTriggersEndDialog) {
    // 1:1 with legacy: SetActive(FALSE) → EndDialog.
    mxh::ui::cHelpDialog d;
    BuildDlgWithChildren(d);
    std::unique_ptr<mxh::ui::cPage> page;
    std::unique_ptr<mxh::ui::cDialogueList> dialogues;
    std::unique_ptr<mxh::ui::cHyperTextList> hyper;
    BuildContent(page, dialogues, hyper);
    mxh::ui::HYPERLINK link{};
    link.wLinkId   = 42;
    link.wLinkType = static_cast<std::uint16_t>(mxh::ui::cHelpDialog::emLink_Page);
    link.dwData    = 999;
    page->AddHyperLink(&link);
    d.SetContent(page.get(), dialogues.get(), hyper.get());
    EXPECT_TRUE(d.OpenDialog());
    EXPECT_EQ(d.GetHyperCount(), 1);
    d.SetActive(false);
    EXPECT_EQ(d.GetHyperCount(), 0);
}

TEST(CHelpDialog, OpenLinkPageSwapsContent) {
    // 1:1 with legacy: OpenLinkPage(dwPageId) swaps to a new
    // page and repopulates the list. Modern port: caller sets
    // m_pMainPage to the target before calling.
    mxh::ui::cHelpDialog d;
    BuildDlgWithChildren(d);
    std::unique_ptr<mxh::ui::cPage> page;
    std::unique_ptr<mxh::ui::cDialogueList> dialogues;
    std::unique_ptr<mxh::ui::cHyperTextList> hyper;
    BuildContent(page, dialogues, hyper);
    d.SetContent(page.get(), dialogues.get(), hyper.get());
    EXPECT_TRUE(d.OpenDialog());
    EXPECT_EQ(d.GetCurPageId(), 100u);
    // Swap m_pMainPage to a new page and call OpenLinkPage.
    auto page2 = std::make_unique<mxh::ui::cPage>();
    page2->Init(200);
    page2->AddDialogue(1);
    d.SetContent(page2.get(), dialogues.get(), hyper.get());
    EXPECT_TRUE(d.OpenLinkPage(200));
    EXPECT_EQ(d.GetCurPageId(), 200u);
}

TEST(CHelpDialog, OpenLinkPageWithoutMainPageReturnsFalse) {
    mxh::ui::cHelpDialog d;
    BuildDlgWithChildren(d);
    EXPECT_FALSE(d.OpenLinkPage(200));
}

TEST(CHelpDialog, HyperLinkParserEmLinkEndTriggersEndDialog) {
    // 1:1 with legacy: HyperLinkParser with emLink_End →
    // EndDialog.
    mxh::ui::cHelpDialog d;
    BuildDlgWithChildren(d);
    std::unique_ptr<mxh::ui::cPage> page;
    std::unique_ptr<mxh::ui::cDialogueList> dialogues;
    std::unique_ptr<mxh::ui::cHyperTextList> hyper;
    BuildContent(page, dialogues, hyper);
    mxh::ui::HYPERLINK link{};
    link.wLinkId   = 42;
    link.wLinkType = static_cast<std::uint16_t>(mxh::ui::cHelpDialog::emLink_End);
    link.dwData    = 0;
    page->AddHyperLink(&link);
    d.SetContent(page.get(), dialogues.get(), hyper.get());
    EXPECT_TRUE(d.OpenDialog());
    EXPECT_EQ(d.GetHyperCount(), 1);
    const auto* h = d.GetHyperAt(0);
    ASSERT_NE(h, nullptr);
    d.HyperLinkParser(h->dwListItemIdx);
    EXPECT_EQ(d.GetHyperCount(), 0);
}

TEST(CHelpDialog, HyperLinkParserWithEmptyIsNoOp) {
    mxh::ui::cHelpDialog d;
    BuildDlgWithChildren(d);
    // No OpenDialog called → m_nHyperCount == 0 → no-op.
    d.HyperLinkParser(0);
    d.HyperLinkParser(99);
    SUCCEED();
}

TEST(CHelpDialog, HyperLinkParserUnknownIdxIsNoOp) {
    mxh::ui::cHelpDialog d;
    BuildDlgWithChildren(d);
    std::unique_ptr<mxh::ui::cPage> page;
    std::unique_ptr<mxh::ui::cDialogueList> dialogues;
    std::unique_ptr<mxh::ui::cHyperTextList> hyper;
    BuildContent(page, dialogues, hyper);
    d.SetContent(page.get(), dialogues.get(), hyper.get());
    EXPECT_TRUE(d.OpenDialog());
    // idx=999 not in m_sHyper → no-op (nType stays -1).
    d.HyperLinkParser(999);
    SUCCEED();
}

TEST(CHelpDialog, HyperStructInitResetsFields) {
    mxh::ui::cHelpDialog::HelpHyper h{};
    h.bUse = true;
    h.dwListItemIdx = 42;
    h.sHyper.wLinkId = 7;
    h.Init();
    EXPECT_FALSE(h.bUse);
    EXPECT_EQ(h.dwListItemIdx, 0u);
    EXPECT_EQ(h.sHyper.wLinkId, 0u);
}

TEST(CHelpDialog, GetHyperAtOutOfRangeReturnsNull) {
    mxh::ui::cHelpDialog d;
    BuildDlgWithChildren(d);
    EXPECT_EQ(d.GetHyperAt(-1), nullptr);
    EXPECT_EQ(d.GetHyperAt(0), nullptr);
    EXPECT_EQ(d.GetHyperAt(99), nullptr);
}

TEST(CHelpDialog, OpenDialogWithoutListDlgIsSafe) {
    // Linking not called → m_pListDlg is null. OpenDialog
    // should still work (no list population, but no crash).
    mxh::ui::cHelpDialog d;
    auto page = std::make_unique<mxh::ui::cPage>();
    page->Init(1);
    page->AddDialogue(1);
    auto dialogues = std::make_unique<mxh::ui::cDialogueList>();
    dialogues->AddLine(1, "msg", 0xFFFFFFFFu, 0, 0);
    auto hyper = std::make_unique<mxh::ui::cHyperTextList>();
    d.SetContent(page.get(), dialogues.get(), hyper.get());
    EXPECT_TRUE(d.OpenDialog());
    EXPECT_EQ(d.GetCurPageId(), 1u);
}

TEST(CHelpDialog, NotCopyable) {
    // 1:1 with the legacy not-copyable-by-default behavior.
    // Modern port deletes copy ctor + copy assign to enforce
    // ownership uniqueness of the m_linkItems vector.
    static_assert(!std::is_copy_constructible_v<mxh::ui::cHelpDialog>);
    static_assert(!std::is_copy_assignable_v<mxh::ui::cHelpDialog>);
}
