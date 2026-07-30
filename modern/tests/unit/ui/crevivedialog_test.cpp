//
// Unit tests for mxh::ui::cReviveDialog (Phase C dialog port).
//
// Locks down the 1:1 surface:
//  * Constants: kIdPresentBtn=30, kIdLoginBtn=31, kIdVillageBtn=32
//  * Default construction: all 3 buttons are null
//  * Linking is a no-op (host injects buttons first)
//  * SetActive forwards to cDialog::SetActive
//  * SetActive(outside siege): Present ON, Village OFF
//  * SetActive(in siege): Present OFF, Village ON
//  * SetActive with no callbacks defaults to non-siege
//  * SetActive without linked buttons is a no-op for
//    button toggling
//  * SetActive can be called multiple times safely
//  * SetActiveFalse also runs the siege branch
//  * SetButtonsForTest stores pointers
//  * NonCopyable
//

#include "mxh/ui/crevivedialog.hpp"
#include "mxh/ui/cbutton.hpp"

#include <gtest/gtest.h>

#include <type_traits>

using mxh::ui::cButton;
using mxh::ui::cReviveDialog;

namespace {

struct Harness {
    cReviveDialog dlg;
    cButton present, login, village;

    Harness() {
        dlg.SetButtonsForTest(&present, &login, &village);
    }
};

std::uint32_t g_mapNum     = 0;
std::uint32_t g_siegeMap   = 0;

std::uint32_t faMapNum(void* /*user*/)    { return g_mapNum; }
std::uint32_t faSiegeMap(void* /*user*/)  { return g_siegeMap; }

}  // namespace


TEST(CReviveDialog, ConstantsMatchLegacy) {
    EXPECT_EQ(cReviveDialog::kIdPresentBtn,  30);
    EXPECT_EQ(cReviveDialog::kIdLoginBtn,    31);
    EXPECT_EQ(cReviveDialog::kIdVillageBtn,  32);
}

TEST(CReviveDialog, DefaultConstructionHasNullButtons) {
    cReviveDialog d;
    EXPECT_EQ(d.GetPresentBtnForTest(), nullptr);
    EXPECT_EQ(d.GetLoginBtnForTest(),   nullptr);
    EXPECT_EQ(d.GetVillageBtnForTest(), nullptr);
}

TEST(CReviveDialog, SetButtonsStoresPointers) {
    cReviveDialog d;
    cButton p, l, v;
    d.SetButtonsForTest(&p, &l, &v);
    EXPECT_EQ(d.GetPresentBtnForTest(), &p);
    EXPECT_EQ(d.GetLoginBtnForTest(),   &l);
    EXPECT_EQ(d.GetVillageBtnForTest(), &v);
}


TEST(CReviveDialog, LinkingIsNoOpWithInjectedButtons) {
    Harness h;
    h.dlg.Linking();
    EXPECT_EQ(h.dlg.GetPresentBtnForTest(), &h.present);
    EXPECT_EQ(h.dlg.GetLoginBtnForTest(),   &h.login);
    EXPECT_EQ(h.dlg.GetVillageBtnForTest(), &h.village);
}


TEST(CReviveDialog, SetActiveForwardsToBase) {
    Harness h;
    h.dlg.SetActive(true);
    EXPECT_TRUE(h.dlg.isActive());
    h.dlg.SetActive(false);
    EXPECT_FALSE(h.dlg.isActive());
}

TEST(CReviveDialog, SetActiveOutsideSiegeShowsPresent) {
    Harness h;
    g_mapNum   = 5;
    g_siegeMap = 10;
    h.dlg.SetMapNumCallbackForTest(&faMapNum, nullptr);
    h.dlg.SetSiegeWarMapNumCallbackForTest(&faSiegeMap, nullptr);
    h.dlg.SetActive(true);
    EXPECT_TRUE(h.present.isActive());
    EXPECT_FALSE(h.village.isActive());
}

TEST(CReviveDialog, SetActiveInSiegeShowsVillage) {
    Harness h;
    g_mapNum   = 10;
    g_siegeMap = 10;
    h.dlg.SetMapNumCallbackForTest(&faMapNum, nullptr);
    h.dlg.SetSiegeWarMapNumCallbackForTest(&faSiegeMap, nullptr);
    h.dlg.SetActive(true);
    EXPECT_FALSE(h.present.isActive());
    EXPECT_TRUE(h.village.isActive());
}

TEST(CReviveDialog, SetActiveNoCallbacksDefaultsToNonSiege) {
    Harness h;
    // No callbacks set; default siegeMap = 0 (off).
    h.dlg.SetActive(true);
    EXPECT_TRUE(h.present.isActive());
    EXPECT_FALSE(h.village.isActive());
}

TEST(CReviveDialog, SetActiveWithoutButtonsIsSafe) {
    cReviveDialog d;
    d.SetActive(true);
    SUCCEED();
}

TEST(CReviveDialog, SetActiveFalseStillRunsSiegeBranch) {
    Harness h;
    g_mapNum   = 10;
    g_siegeMap = 10;
    h.dlg.SetMapNumCallbackForTest(&faMapNum, nullptr);
    h.dlg.SetSiegeWarMapNumCallbackForTest(&faSiegeMap, nullptr);
    h.dlg.SetActive(false);
    EXPECT_FALSE(h.dlg.isActive());
    // 1:1 with legacy: SetActive(BOOL val) always runs
    // the siege branch, regardless of val.
    EXPECT_FALSE(h.present.isActive());
    EXPECT_TRUE(h.village.isActive());
}

TEST(CReviveDialog, SetActiveToggleBetweenMaps) {
    Harness h;
    h.dlg.SetMapNumCallbackForTest(&faMapNum, nullptr);
    h.dlg.SetSiegeWarMapNumCallbackForTest(&faSiegeMap, nullptr);

    g_mapNum   = 1;
    g_siegeMap = 10;
    h.dlg.SetActive(true);
    EXPECT_TRUE(h.present.isActive());
    EXPECT_FALSE(h.village.isActive());

    g_mapNum   = 10;
    h.dlg.SetActive(true);
    EXPECT_FALSE(h.present.isActive());
    EXPECT_TRUE(h.village.isActive());

    g_mapNum   = 1;
    h.dlg.SetActive(true);
    EXPECT_TRUE(h.present.isActive());
    EXPECT_FALSE(h.village.isActive());
}

TEST(CReviveDialog, LoginButtonIsNotToggled) {
    Harness h;
    // Legacy: the login button is set up but never
    // touched in SetActive.  The modern port follows
    // the same contract: SetActive does not change the
    // login button state.
    h.present.SetActive(false);
    h.village.SetActive(false);
    h.login.SetActive(true);

    g_mapNum   = 5;
    g_siegeMap = 10;
    h.dlg.SetMapNumCallbackForTest(&faMapNum, nullptr);
    h.dlg.SetSiegeWarMapNumCallbackForTest(&faSiegeMap, nullptr);
    h.dlg.SetActive(true);
    EXPECT_TRUE(h.login.isActive());  // unchanged
}


TEST(CReviveDialog, NonCopyable) {
    static_assert(!std::is_copy_constructible<cReviveDialog>::value,
                  "cReviveDialog must not be copyable");
    static_assert(!std::is_copy_assignable<cReviveDialog>::value,
                  "cReviveDialog must not be copy-assignable");
    SUCCEED();
}
