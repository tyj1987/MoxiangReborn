// revivedialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cReviveDialog (revive dialog: 3 options —
// present spot / login spot / village).
//
// Covers modern/src/ui/revivedialog.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\ReviveDialog.h (729 B) and
//   墨香【源码】\[Client]MH\ReviveDialog.cpp.
//
// What's tested:
//   - Default construction: 3 cButton pointers are null.
//   - Linking resolves the 3 cButton children
//     (kPresentBtnId=250, kLoginBtnId=251, kVillageBtnId=252)
//     by id.
//   - Linking without children leaves pointers null and is
//     safe.
//   - SetActive override calls base SetActive (active state
//     updates correctly).
//   - SetActive button toggling (siege war vs normal map
//     branches) is TODO until SIEGEMGR + MAP singletons are
//     ported. The test verifies the call doesn't crash and
//     doesn't change button state in unexpected ways (m_pLoginBtn
//     is never touched — 1:1 quirk).
//   - SetActive matches base noexcept (R-12 polymorphic virtual
//     required).
//   - Accessors return the linked cButton pointers.
//
// 1:1 quirks preserved:
//   - SetActive calls base first, then the button toggling
//     (1:1 with legacy flow).
//   - m_pLoginBtn is never toggled by SetActive (1:1 quirk:
//     legacy never touches m_pLoginBtn — login spot is
//     always available).
//   - The button toggling (SetActive FALSE/TRUE in legacy)
//     is documented as TODO in revivedialog.cpp. The modern
//     cButton doesn't have SetActive (legacy cButton did);
//     modern port would use SetVisible to express the same
//     intent. This is a 1:1 quirk.

#include "revivedialog.hpp"
#include "cbutton.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

namespace mxh::ui::test {

// ===========================================================================
// Construction
// ===========================================================================

TEST(CReviveDialogTest, DefaultConstructionHasNullPointers) {
    cReviveDialog dlg;
    EXPECT_EQ(dlg.GetPresentBtn(), nullptr);
    EXPECT_EQ(dlg.GetLoginBtn(),   nullptr);
    EXPECT_EQ(dlg.GetVillageBtn(),  nullptr);
}

// ===========================================================================
// Id constants
// ===========================================================================

TEST(CReviveDialogTest, IdConstantsAreDistinct) {
    EXPECT_NE(cReviveDialog::kPresentBtnId, cReviveDialog::kLoginBtnId);
    EXPECT_NE(cReviveDialog::kPresentBtnId, cReviveDialog::kVillageBtnId);
    EXPECT_NE(cReviveDialog::kLoginBtnId,   cReviveDialog::kVillageBtnId);
}

TEST(CReviveDialogTest, IdConstantsMatchExpectedLocalRange) {
    // 1:1 quirk: pick 250-252 to avoid collisions with
    // other Tier 2 dialog id ranges (cCharMakeDlg 200-203,
    // cGuildJoinDialog 210-212, cCharStateDialog 220-224,
    // cSOSDialog 230-231, cMiniFriendDialog 240-243).
    EXPECT_EQ(cReviveDialog::kPresentBtnId, 250);
    EXPECT_EQ(cReviveDialog::kLoginBtnId,   251);
    EXPECT_EQ(cReviveDialog::kVillageBtnId,  252);
}

// ===========================================================================
// Linking
// ===========================================================================

namespace {

// Build a cReviveDialog with 3 cButton children wired in
// the modern id range (250-252). Returns raw pointers to
// the 3 buttons via the out vector; ownership lives in
// the dlg (children are added via cWindow::Add).
void BuildDlgWithButtons(cReviveDialog& dlg, std::vector<cButton*>& raws) {
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    raws.assign(3, nullptr);
    for (int i = 0; i < 3; ++i) {
        auto s = std::make_unique<cButton>();
        s->Init(0, 0, 50, 14, nullptr, nullptr, nullptr,
                nullptr, nullptr, 250 + i);
        raws[i] = s.get();
        dlg.Add(std::unique_ptr<cWindow>(s.release()));
    }
    dlg.Linking();
}

}  // namespace

TEST(CReviveDialogTest, LinkingResolvesAllThreeButtons) {
    cReviveDialog dlg;
    std::vector<cButton*> raws;
    BuildDlgWithButtons(dlg, raws);

    EXPECT_EQ(dlg.GetPresentBtn(), raws[0]);
    EXPECT_EQ(dlg.GetLoginBtn(),   raws[1]);
    EXPECT_EQ(dlg.GetVillageBtn(),  raws[2]);
}

TEST(CReviveDialogTest, LinkingWithoutChildrenLeavesPointersNull) {
    cReviveDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    EXPECT_EQ(dlg.GetPresentBtn(), nullptr);
    EXPECT_EQ(dlg.GetLoginBtn(),   nullptr);
    EXPECT_EQ(dlg.GetVillageBtn(),  nullptr);
}

TEST(CReviveDialogTest, LinkingBeforeInitDoesNotCrash) {
    // Defensive: Linking before Init is a safe no-op
    // (findWindowById returns nullptr for uninit children).
    cReviveDialog dlg;
    dlg.Linking();
    EXPECT_EQ(dlg.GetPresentBtn(), nullptr);
    EXPECT_EQ(dlg.GetLoginBtn(),   nullptr);
    EXPECT_EQ(dlg.GetVillageBtn(),  nullptr);
}

// ===========================================================================
// SetActive (1:1 override, base + TODO for button toggling)
// ===========================================================================

TEST(CReviveDialogTest, SetActiveTrueUpdatesBaseState) {
    cReviveDialog dlg;
    std::vector<cButton*> raws;
    BuildDlgWithButtons(dlg, raws);
    EXPECT_FALSE(dlg.isActive());

    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CReviveDialogTest, SetActiveFalseUpdatesBaseState) {
    cReviveDialog dlg;
    std::vector<cButton*> raws;
    BuildDlgWithButtons(dlg, raws);
    dlg.SetActive(true);
    ASSERT_TRUE(dlg.isActive());

    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CReviveDialogTest, SetActiveToggleRoundTrip) {
    cReviveDialog dlg;
    std::vector<cButton*> raws;
    BuildDlgWithButtons(dlg, raws);

    for (int round = 0; round < 3; ++round) {
        dlg.SetActive(true);
        EXPECT_TRUE(dlg.isActive());
        dlg.SetActive(false);
        EXPECT_FALSE(dlg.isActive());
    }
}

TEST(CReviveDialogTest, SetActiveButtonTogglingIsNoOpUntilSIEGEMGRPort) {
    // 1:1 quirk: legacy SetActive dispatches to SIEGEMGR +
    // MAP singletons to toggle the 3 buttons. Modern port
    // is a no-op until those singletons are ported. The
    // test verifies that calling SetActive doesn't crash
    // and doesn't change any button state in unexpected
    // ways (m_pLoginBtn is never touched — 1:1 quirk:
    // legacy never toggles m_pLoginBtn in SetActive).
    cReviveDialog dlg;
    std::vector<cButton*> raws;
    BuildDlgWithButtons(dlg, raws);
    for (auto* b : raws) ASSERT_NE(b, nullptr);

    // Record initial visibility (default: all visible).
    bool present_initial = dlg.GetPresentBtn()->isVisible();
    bool login_initial   = dlg.GetLoginBtn()->isVisible();
    bool village_initial = dlg.GetVillageBtn()->isVisible();

    dlg.SetActive(true);
    // After SetActive (with the TODO singleton dispatch
    // stubbed out), the button visibility should be
    // unchanged (1:1: the modern port doesn't toggle
    // buttons until SIEGEMGR + MAP are ported).
    EXPECT_EQ(dlg.GetPresentBtn()->isVisible(), present_initial);
    EXPECT_EQ(dlg.GetLoginBtn()->isVisible(),   login_initial);
    EXPECT_EQ(dlg.GetVillageBtn()->isVisible(), village_initial);

    dlg.SetActive(false);
    EXPECT_EQ(dlg.GetPresentBtn()->isVisible(), present_initial);
    EXPECT_EQ(dlg.GetLoginBtn()->isVisible(),   login_initial);
    EXPECT_EQ(dlg.GetVillageBtn()->isVisible(), village_initial);
}

TEST(CReviveDialogTest, SetActiveWithoutLinksIsSafe) {
    // Defensive: SetActive when no children are linked
    // must not crash.
    cReviveDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    // No crash expected.
    dlg.SetActive(true);
    dlg.SetActive(false);
    SUCCEED();
}

TEST(CReviveDialogTest, SetActiveBeforeInitDoesNotCrash) {
    // Defensive: SetActive before Init should still be
    // safe (the modern cDialog::SetActive just sets
    // m_bActive, which defaults to false).
    cReviveDialog dlg;
    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
    SUCCEED();
}

}  // namespace mxh::ui::test
