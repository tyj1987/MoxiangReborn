//
// Unit tests for mxh::ui::cMousePointer
// (Phase C dialog port).
//
// Locks down the 1:1 surface of legacy
// CMousePointer (cursor / monster-target pointer:
// empty ctor with NULL cAni members, empty dtor,
// empty Linking, and 3 Monster* entry points all
// with empty bodies -- the legacy source has all
// four function bodies commented out):
//   * Default construction: cMousePointer is a cDialog.
//   * Inherits from cDialog.
//   * NonCopyable.
//   * Init + SetAbsXY works (inherited).
//   * Init preserves the dialog id.
//   * Init position / size.
//   * Init idempotence.
//   * Linking() runs safely with no children.
//   * Linking() can be called before Init.
//   * Linking() can be called multiple times.
//   * Linking() does not crash with NULL children
//     (1:1 with legacy commented-out body).
//   * MonsterAttack() / MonsterMouseOver() /
//     MonsterLeave() all run safely (empty bodies).
//   * MonsterAttack before Init does not crash.
//   * MonsterMouseOver before Linking does not crash.
//   * MonsterLeave before any state does not crash.
//   * Default cAni members are NULL (1:1 with legacy
//     ctor NULL initialisation).
//   * Test hooks can inject non-NULL cAni pointers.
//   * Test hooks can reset cAni pointers to NULL.
//   * kIdMouseBasic constant matches WindowIDs.h.
//   * kIdMouseClick constant matches WindowIDs.h.
//   * Monster entry points can be called in any
//     order without state corruption.
//

#include "mxh/ui/cmousepointer.hpp"
#include "mxh/ui/cdialog.hpp"
#include "mxh/ui/cwindow.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

using mxh::ui::cDialog;
using mxh::ui::cMousePointer;
using mxh::ui::cWindow;

namespace {

struct Harness {
    cMousePointer dlg;
    Harness() {
        dlg.Init(0, 0, 32, 32, nullptr, 0);
    }
};

}  // namespace

// ---------- Construction / destruction ----------

TEST(CMousePointerTest, CtorDoesNotCrash) {
    cMousePointer dlg;
    SUCCEED();
}

TEST(CMousePointerTest, DtorDoesNotCrash) {
    cMousePointer dlg;
    SUCCEED();
}

TEST(CMousePointerTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cMousePointer>,
                  "cMousePointer must inherit from cDialog");
    SUCCEED();
}

TEST(CMousePointerTest, IsAlsoAWindow) {
    static_assert(std::is_base_of_v<cWindow, cMousePointer>,
                  "cMousePointer must be a cWindow (transitively)");
    SUCCEED();
}

TEST(CMousePointerTest, NonCopyable) {
    static_assert(!std::is_copy_constructible_v<cMousePointer>,
                  "cMousePointer must be non-copyable");
    static_assert(!std::is_copy_assignable_v<cMousePointer>,
                  "cMousePointer must be non-copy-assignable");
    SUCCEED();
}

TEST(CMousePointerTest, MultipleInstancesAreSafe) {
    cMousePointer a;
    cMousePointer b;
    cMousePointer c;
    SUCCEED();
}

TEST(CMousePointerTest, DefaultAniMembersAreNull) {
    cMousePointer dlg;
    EXPECT_EQ(dlg.GetAniBasicForTest(), nullptr);
    EXPECT_EQ(dlg.GetAniClickForTest(), nullptr);
}

// ---------- Init ----------

TEST(CMousePointerTest, InitStoresPositionAndSize) {
    Harness h;
    EXPECT_EQ(h.dlg.absX(), 0);
    EXPECT_EQ(h.dlg.absY(), 0);
    EXPECT_EQ(h.dlg.width(), 32u);
    EXPECT_EQ(h.dlg.height(), 32u);
}

TEST(CMousePointerTest, InitIsIdempotent) {
    cMousePointer dlg;
    dlg.Init(0, 0, 16, 16, nullptr, 1);
    dlg.Init(10, 20, 32, 32, nullptr, 2);
    EXPECT_EQ(dlg.width(), 32u);
    EXPECT_EQ(dlg.height(), 32u);
    EXPECT_EQ(dlg.id(), 2);
}

TEST(CMousePointerTest, InitBeforeAnyStateChangeDoesNotCrash) {
    cMousePointer dlg;
    dlg.Init(0, 0, 16, 16, nullptr, 0);
    SUCCEED();
}

// ---------- Linking (1:1 quirk: empty body) ----------

TEST(CMousePointerTest, LinkingAfterInitDoesNotCrash) {
    Harness h;
    h.dlg.Linking();
    SUCCEED();
}

TEST(CMousePointerTest, LinkingBeforeInitDoesNotCrash) {
    cMousePointer dlg;
    dlg.Linking();
    SUCCEED();
}

TEST(CMousePointerTest, LinkingMultipleTimesIsSafe) {
    Harness h;
    h.dlg.Linking();
    h.dlg.Linking();
    h.dlg.Linking();
    SUCCEED();
}

TEST(CMousePointerTest, LinkingWithNullAniMembersIsSafe) {
    Harness h;
    EXPECT_EQ(h.dlg.GetAniBasicForTest(), nullptr);
    EXPECT_EQ(h.dlg.GetAniClickForTest(), nullptr);
    h.dlg.Linking();
    EXPECT_EQ(h.dlg.GetAniBasicForTest(), nullptr);
    EXPECT_EQ(h.dlg.GetAniClickForTest(), nullptr);
}

// ---------- Monster entry points (1:1 quirk: empty bodies) ----------

TEST(CMousePointerTest, MonsterAttackDoesNotCrash) {
    Harness h;
    h.dlg.MonsterAttack();
    SUCCEED();
}

TEST(CMousePointerTest, MonsterMouseOverDoesNotCrash) {
    Harness h;
    h.dlg.MonsterMouseOver();
    SUCCEED();
}

TEST(CMousePointerTest, MonsterLeaveDoesNotCrash) {
    Harness h;
    h.dlg.MonsterLeave();
    SUCCEED();
}

TEST(CMousePointerTest, MonsterAttackBeforeInitIsSafe) {
    cMousePointer dlg;
    dlg.MonsterAttack();
    SUCCEED();
}

TEST(CMousePointerTest, MonsterMouseOverBeforeLinkingIsSafe) {
    cMousePointer dlg;
    dlg.MonsterMouseOver();
    SUCCEED();
}

TEST(CMousePointerTest, MonsterLeaveBeforeAnyStateIsSafe) {
    cMousePointer dlg;
    dlg.MonsterLeave();
    SUCCEED();
}

TEST(CMousePointerTest, MonsterEntryPointsInAnyOrder) {
    Harness h;
    h.dlg.MonsterLeave();
    h.dlg.MonsterAttack();
    h.dlg.MonsterMouseOver();
    h.dlg.MonsterAttack();
    h.dlg.MonsterLeave();
    SUCCEED();
}

// ---------- Test hooks ----------

TEST(CMousePointerTest, SetAniBasicForTestStoresPointer) {
    Harness h;
    int dummy = 0;
    h.dlg.SetAniBasicForTest(&dummy);
    EXPECT_EQ(h.dlg.GetAniBasicForTest(), &dummy);
}

TEST(CMousePointerTest, SetAniClickForTestStoresPointer) {
    Harness h;
    int dummy = 0;
    h.dlg.SetAniClickForTest(&dummy);
    EXPECT_EQ(h.dlg.GetAniClickForTest(), &dummy);
}

TEST(CMousePointerTest, SetAniForTestToNullResets) {
    Harness h;
    int dummy = 0;
    h.dlg.SetAniBasicForTest(&dummy);
    h.dlg.SetAniBasicForTest(nullptr);
    EXPECT_EQ(h.dlg.GetAniBasicForTest(), nullptr);
    h.dlg.SetAniClickForTest(&dummy);
    h.dlg.SetAniClickForTest(nullptr);
    EXPECT_EQ(h.dlg.GetAniClickForTest(), nullptr);
}

TEST(CMousePointerTest, MonsterEntryPointsSafeWithInjectedAnis) {
    Harness h;
    int dummy_basic = 0;
    int dummy_click = 0;
    h.dlg.SetAniBasicForTest(&dummy_basic);
    h.dlg.SetAniClickForTest(&dummy_click);
    h.dlg.MonsterAttack();
    h.dlg.MonsterMouseOver();
    h.dlg.MonsterLeave();
    EXPECT_EQ(h.dlg.GetAniBasicForTest(), &dummy_basic);
    EXPECT_EQ(h.dlg.GetAniClickForTest(), &dummy_click);
}

// ---------- Child window id constants (legacy WindowIDs.h) ----------

TEST(CMousePointerTest, MouseBasicIdConstantMatchesLegacyEnum) {
    EXPECT_EQ(cMousePointer::kIdMouseBasic, 1310);
}

TEST(CMousePointerTest, MouseClickIdConstantMatchesLegacyEnum) {
    EXPECT_EQ(cMousePointer::kIdMouseClick, 1311);
}

TEST(CMousePointerTest, ChildIdsAreDistinct) {
    EXPECT_NE(cMousePointer::kIdMouseBasic, cMousePointer::kIdMouseClick);
}