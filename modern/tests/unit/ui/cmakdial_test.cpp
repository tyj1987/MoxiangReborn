// cmakdial_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cCharMakeDlg (character creation
// dialog).
//
// Covers modern/src/ui/cmakdial.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\CharMakeDialog.h (659 B) and
//   墨香【源码】\[Client]MH\CharMakeDialog.cpp (1,689 B).
//
// What's tested:
//   - Default construction: all 4 cStatic pointers are null.
//   - Linking resolves the 4 cStatic children (m/f hair +
//     m/f face) by id range 200-203.
//   - ChangeComboStatus(0) = male: M hair + M face visible,
//     W hair + W face hidden.
//   - ChangeComboStatus(1) = female: W visible, M hidden.
//   - ChangeComboStatus with unlinked children is safe
//     (no crash, no state change in the other 3 pointers).
//   - Accessors return the linked cStatic pointers.
//   - OnActionEvent is a no-op until CharMakeManager is
//     ported (deferred; the dialog is still testable
//     through Linking + ChangeComboStatus).
//
// 1:1 quirks (preserved from legacy):
//   - The 4 children were originally cComboBoxEx* but
//     downgraded to cStatic* in the 2008-era code. The
//     modern port mirrors the downgraded form (a cStatic
//     that is shown/hidden based on sex selection; the
//     actual selection lives in CharMakeManager and the
//     left/right arrow buttons dispatch through it).
//   - Sex encoding: 0 = male, 1 = female (1:1 with legacy).
//   - The legacy cStatic does NOT define SetActive (the
//     legacy cCharMakeDlg::ChangeComboStatus does not
//     compile cleanly — see KNOWN_BUGS R-12). The modern
//     port uses SetVisible / isVisible on cWindow (which
//     cStatic inherits) to express the show/hide intent.

#include "cmakdial.hpp"
#include "cstatic.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace mxh::ui::test {

// ===========================================================================
// Construction
// ===========================================================================

TEST(CCharMakeDlgTest, DefaultConstructionHasNullPointers) {
    cCharMakeDlg dlg;
    EXPECT_EQ(dlg.GetManHair(),   nullptr);
    EXPECT_EQ(dlg.GetWomanHair(), nullptr);
    EXPECT_EQ(dlg.GetManFace(),   nullptr);
    EXPECT_EQ(dlg.GetWomanFace(), nullptr);
}

// ===========================================================================
// Linking
// ===========================================================================

TEST(CCharMakeDlgTest, LinkingResolvesAllFourStatics) {
    cCharMakeDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    // Add 4 cStatic children with the expected id range
    // (200-203 for m/f hair + m/f face).
    std::vector<cStatic*> raws(4, nullptr);
    for (int i = 0; i < 4; ++i) {
        auto s = std::make_unique<cStatic>();
        s->Init(0, 0, 50, 14, nullptr, 200 + i);
        raws[i] = s.get();
        dlg.Add(std::unique_ptr<cWindow>(s.release()));
    }
    dlg.Linking();

    EXPECT_EQ(dlg.GetManHair(),   raws[0]);
    EXPECT_EQ(dlg.GetWomanHair(), raws[1]);
    EXPECT_EQ(dlg.GetManFace(),   raws[2]);
    EXPECT_EQ(dlg.GetWomanFace(), raws[3]);
}

TEST(CCharMakeDlgTest, LinkingWithoutChildrenLeavesPointersNull) {
    // Linking with no matching children leaves all 4
    // pointers null (defensive — the dialog should not
    // crash when used without a fully-populated child tree).
    cCharMakeDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    EXPECT_EQ(dlg.GetManHair(),   nullptr);
    EXPECT_EQ(dlg.GetWomanHair(), nullptr);
    EXPECT_EQ(dlg.GetManFace(),   nullptr);
    EXPECT_EQ(dlg.GetWomanFace(), nullptr);
}

// ===========================================================================
// ChangeComboStatus
// ===========================================================================

namespace {

// Build a cCharMakeDlg with 4 cStatic children wired in
// the modern id range (200-203). Returns raw pointers to
// the 4 statics via the out vector; ownership lives in
// the dlg (the statics are children, not separately
// managed).
void BuildDlgWithStatics(cCharMakeDlg& dlg, std::vector<cStatic*>& raws) {
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    raws.assign(4, nullptr);
    for (int i = 0; i < 4; ++i) {
        auto s = std::make_unique<cStatic>();
        s->Init(0, 0, 50, 14, nullptr, 200 + i);
        raws[i] = s.get();
        dlg.Add(std::unique_ptr<cWindow>(s.release()));
    }
    dlg.Linking();
    // Start with all statics visible (ChangeComboStatus
    // toggles them on/off based on sex).
    for (auto* s : raws) s->SetVisible(true);
}

}  // namespace

TEST(CCharMakeDlgTest, ChangeComboStatusMaleShowsMStatics) {
    cCharMakeDlg dlg;
    std::vector<cStatic*> statics;
    BuildDlgWithStatics(dlg, statics);
    ASSERT_NE(dlg.GetManHair(),   nullptr);
    ASSERT_NE(dlg.GetWomanHair(), nullptr);
    ASSERT_NE(dlg.GetManFace(),   nullptr);
    ASSERT_NE(dlg.GetWomanFace(), nullptr);

    dlg.ChangeComboStatus(0);  // 0 = male
    EXPECT_TRUE(dlg.GetManHair()->isVisible());
    EXPECT_FALSE(dlg.GetWomanHair()->isVisible());
    EXPECT_TRUE(dlg.GetManFace()->isVisible());
    EXPECT_FALSE(dlg.GetWomanFace()->isVisible());
}

TEST(CCharMakeDlgTest, ChangeComboStatusFemaleShowsWStatics) {
    cCharMakeDlg dlg;
    std::vector<cStatic*> statics;
    BuildDlgWithStatics(dlg, statics);
    ASSERT_NE(dlg.GetManHair(),   nullptr);
    ASSERT_NE(dlg.GetWomanHair(), nullptr);
    ASSERT_NE(dlg.GetManFace(),   nullptr);
    ASSERT_NE(dlg.GetWomanFace(), nullptr);

    dlg.ChangeComboStatus(1);  // 1 = female
    EXPECT_FALSE(dlg.GetManHair()->isVisible());
    EXPECT_TRUE(dlg.GetWomanHair()->isVisible());
    EXPECT_FALSE(dlg.GetManFace()->isVisible());
    EXPECT_TRUE(dlg.GetWomanFace()->isVisible());
}

TEST(CCharMakeDlgTest, ChangeComboStatusWithoutLinksIsSafe) {
    // Calling ChangeComboStatus when no children are linked
    // must not crash. The modern port null-checks each
    // pointer before calling SetVisible (the legacy code
    // dereferences unconditionally and would crash — the
    // modern port is more defensive).
    cCharMakeDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    // No crash expected.
    dlg.ChangeComboStatus(0);
    dlg.ChangeComboStatus(1);
    SUCCEED();
}

TEST(CCharMakeDlgTest, ChangeComboStatusToggleRoundTrip) {
    // 1:1 quirk: ChangeComboStatus can be called repeatedly;
    // the legacy doesn't memo the last state, it just
    // applies the current sex. Test that alternating male /
    // female produces the correct pair of active states
    // each time.
    cCharMakeDlg dlg;
    std::vector<cStatic*> statics;
    BuildDlgWithStatics(dlg, statics);
    ASSERT_NE(dlg.GetManHair(),   nullptr);
    ASSERT_NE(dlg.GetWomanHair(), nullptr);
    ASSERT_NE(dlg.GetManFace(),   nullptr);
    ASSERT_NE(dlg.GetWomanFace(), nullptr);

    for (int round = 0; round < 3; ++round) {
        dlg.ChangeComboStatus(0);
        EXPECT_TRUE(dlg.GetManHair()->isVisible());
        EXPECT_FALSE(dlg.GetWomanHair()->isVisible());
        EXPECT_TRUE(dlg.GetManFace()->isVisible());
        EXPECT_FALSE(dlg.GetWomanFace()->isVisible());

        dlg.ChangeComboStatus(1);
        EXPECT_FALSE(dlg.GetManHair()->isVisible());
        EXPECT_TRUE(dlg.GetWomanHair()->isVisible());
        EXPECT_FALSE(dlg.GetManFace()->isVisible());
        EXPECT_TRUE(dlg.GetWomanFace()->isVisible());
    }
}

// ===========================================================================
// OnActionEvent (deferred to CharMakeManager port)
// ===========================================================================

TEST(CCharMakeDlgTest, OnActionEventIsNoOpUntilManagerPort) {
    // 1:1 quirk: OnActionEvent in the modern port is a
    // no-op until CharMakeManager is ported. The dialog
    // itself is still testable through Linking +
    // ChangeComboStatus. The TODO in cmakdial.cpp documents
    // the dispatch logic that will be added.
    cCharMakeDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.OnActionEvent(/*CMID_SexLeft=*/100, /*p=*/nullptr, /*we=*/0x10);
    // No crash, no observable state change.
    EXPECT_EQ(dlg.GetManHair(), nullptr);
    SUCCEED();
}

}  // namespace mxh::ui::test

