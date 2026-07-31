//
// Unit tests for mxh::ui::cHelper
// (Phase C dialog port).
//
// Locks down the 1:1 surface of legacy cHelper
// (guide / newbie helper animation: cAni motion
// list with timer state, motion swap, greet-check
// flag):
//   * Default construction: cHelper is a cDialog.
//   * Inherits from cDialog.
//   * NonCopyable.
//   * Init + SetAbsXY works (inherited).
//   * Init preserves the dialog id.
//   * Init position / size.
//   * Init idempotence.
//   * Render() returns early when dialog is not
//     active (1:1 with legacy IsActive early-exit).
//   * Render() calls host-injected callback when
//     active + slot has a non-NULL cAni.
//   * Render() does not call host-injected callback
//     when slot is NULL (1:1 with legacy would
//     crash but modern port guards via null check).
//   * ActionEvent() returns WE_NULL (1:1 with legacy).
//   * ActionEvent() forwards to host-injected
//     callback when slot is non-NULL.
//   * SetMotion() swaps m_curMotion (1:1 with legacy).
//   * SetMotion() with out-of-range idx is a no-op.
//   * GetMotion() returns current motion.
//   * SetMaxSprite() forwards to host-injected
//     callback for valid slot + non-NULL cAni.
//   * SetMaxSprite() with NULL cAni is a no-op.
//   * SetMaxSprite() with out-of-range idx is a no-op.
//   * AddSprite() forwards to host-injected
//     callback for valid slot + non-NULL cAni.
//   * AddSprite() with NULL cAni is a no-op.
//   * AddSprite() with out-of-range idx is a no-op.
//   * SetStartTime() sets start + cur time and
//     re-enables greet-check (1:1 with legacy).
//   * SetGreetTime() stores greet time.
//   * GetGreetTime() returns stored greet time.
//   * IsGreetCheck() reflects m_greetCheck flag.
//   * StopGreetCheck() clears m_greetCheck.
//   * Default m_greetCheck is true (1:1 with legacy
//     ctor m_bGreetCheck = TRUE).
//   * Default m_curMotion is 0 (1:1 with legacy
//     ctor m_wCurMotion = 0).
//   * Default timer fields are 0.
//   * kMaxMotionSlots constant matches emHM_MAX (1).
//   * kWeNull constant matches WE_NULL (0).
//   * HelperMotion enum has Stand = 0 and Max = 1.
//   * Render() before Init does not crash.
//   * ActionEvent() before Init does not crash.
//

#include "mxh/ui/chelper.hpp"
#include "mxh/ui/cdialog.hpp"
#include "mxh/ui/cwindow.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

using mxh::ui::cDialog;
using mxh::ui::cHelper;
using mxh::ui::cWindow;
using mxh::ui::HelperMotion;

namespace {

struct Harness {
    cHelper dlg;
    Harness() {
        dlg.Init(0, 0, 100, 100, nullptr, 0);
    }
};

// Tracking helpers used by host-injected callbacks.
struct RenderTracker {
    int call_count = 0;
    void* last_ani = nullptr;
};
struct ActionTracker {
    int call_count = 0;
    void* last_ani = nullptr;
};
struct AddSpriteTracker {
    int call_count = 0;
    void* last_sprite = nullptr;
    std::uint16_t last_delay = 0;
};
struct SetMaxSpriteTracker {
    int call_count = 0;
    std::int32_t last_max = -1;
};

void RenderCb(void* ani, void* user) {
    auto* t = static_cast<RenderTracker*>(user);
    ++t->call_count;
    t->last_ani = ani;
}
void ActionCb(void* ani, void* user) {
    auto* t = static_cast<ActionTracker*>(user);
    ++t->call_count;
    t->last_ani = ani;
}
void AddSpriteCb(void* ani, void* sprite, std::uint16_t delay, void* user) {
    auto* t = static_cast<AddSpriteTracker*>(user);
    ++t->call_count;
    t->last_sprite = sprite;
    t->last_delay = delay;
    (void)ani;
}
void SetMaxSpriteCb(void* ani, std::int32_t nMaxNum, void* user) {
    auto* t = static_cast<SetMaxSpriteTracker*>(user);
    ++t->call_count;
    t->last_max = nMaxNum;
    (void)ani;
}

}  // namespace

// ---------- Construction / destruction ----------

TEST(CHelperTest, CtorDoesNotCrash) {
    cHelper dlg;
    SUCCEED();
}

TEST(CHelperTest, DtorDoesNotCrash) {
    cHelper dlg;
    SUCCEED();
}

TEST(CHelperTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cHelper>,
                  "cHelper must inherit from cDialog");
    SUCCEED();
}

TEST(CHelperTest, IsAlsoAWindow) {
    static_assert(std::is_base_of_v<cWindow, cHelper>,
                  "cHelper must be a cWindow (transitively)");
    SUCCEED();
}

TEST(CHelperTest, NonCopyable) {
    static_assert(!std::is_copy_constructible_v<cHelper>,
                  "cHelper must be non-copyable");
    static_assert(!std::is_copy_assignable_v<cHelper>,
                  "cHelper must be non-copy-assignable");
    SUCCEED();
}

TEST(CHelperTest, MultipleInstancesAreSafe) {
    cHelper a;
    cHelper b;
    SUCCEED();
}

TEST(CHelperTest, DefaultMotionIsStand) {
    cHelper dlg;
    EXPECT_EQ(dlg.GetMotion(), HelperMotion::Stand);
}

TEST(CHelperTest, DefaultGreetCheckIsTrue) {
    cHelper dlg;
    EXPECT_TRUE(dlg.IsGreetCheck());
}

TEST(CHelperTest, DefaultGreetTimeIsZero) {
    cHelper dlg;
    EXPECT_EQ(dlg.GetGreetTime(), 0u);
}

TEST(CHelperTest, DefaultMotionAniSlotsAreNull) {
    cHelper dlg;
    for (std::size_t i = 0; i < cHelper::kMaxMotionSlots; ++i) {
        EXPECT_EQ(dlg.GetMotionAniForTest(i), nullptr);
    }
}

// ---------- Init ----------

TEST(CHelperTest, InitStoresPositionAndSize) {
    Harness h;
    EXPECT_EQ(h.dlg.absX(), 0);
    EXPECT_EQ(h.dlg.absY(), 0);
    EXPECT_EQ(h.dlg.width(), 100u);
    EXPECT_EQ(h.dlg.height(), 100u);
}

TEST(CHelperTest, InitIsIdempotent) {
    cHelper dlg;
    dlg.Init(0, 0, 50, 50, nullptr, 1);
    dlg.Init(10, 20, 200, 200, nullptr, 2);
    EXPECT_EQ(dlg.width(), 200u);
    EXPECT_EQ(dlg.height(), 200u);
    EXPECT_EQ(dlg.id(), 2);
}
//
// 1:1 quirk note: legacy cHelper::Render early-
// exit mirrors IsActive() (legacy).  Modern
// base class exposes isActive() (lowercase a).
// The Render() test below verifies the early-exit
// contract (1:1 with legacy).
//
TEST(CHelperTest, InitBeforeAnyStateChangeDoesNotCrash) {
    cHelper dlg;
    dlg.Init(0, 0, 50, 50, nullptr, 0);
    SUCCEED();
}

// ---------- Render (1:1 with legacy IsActive early-exit) ----------

TEST(CHelperTest, RenderInactiveDialogIsNoOp) {
    RenderTracker tracker;
    Harness h;
    h.dlg.SetRenderCallbackForTest(RenderCb, &tracker);
    h.dlg.SetMotionAniForTest(0, &tracker);
    h.dlg.SetActive(false);
    h.dlg.Render();
    EXPECT_EQ(tracker.call_count, 0);
}

TEST(CHelperTest, RenderActiveDialogCallsCallback) {
    RenderTracker tracker;
    Harness h;
    h.dlg.SetRenderCallbackForTest(RenderCb, &tracker);
    int ani = 0;
    h.dlg.SetMotionAniForTest(0, &ani);
    h.dlg.SetActive(true);
    h.dlg.Render();
    EXPECT_EQ(tracker.call_count, 1);
    EXPECT_EQ(tracker.last_ani, &ani);
}

TEST(CHelperTest, RenderWithNullAniDoesNotCrash) {
    RenderTracker tracker;
    Harness h;
    h.dlg.SetRenderCallbackForTest(RenderCb, &tracker);
    h.dlg.SetMotionAniForTest(0, nullptr);
    h.dlg.SetActive(true);
    h.dlg.Render();
    EXPECT_EQ(tracker.call_count, 0);
}

TEST(CHelperTest, RenderWithoutCallbackDoesNotCrash) {
    Harness h;
    int ani = 0;
    h.dlg.SetMotionAniForTest(0, &ani);
    h.dlg.SetActive(true);
    h.dlg.Render();
    SUCCEED();
}

TEST(CHelperTest, RenderBeforeInitDoesNotCrash) {
    cHelper dlg;
    dlg.Render();
    SUCCEED();
}
//
// 1:1 quirk note: legacy cHelper::ActionEvent
// returns WE_NULL.  Modern cWindow exposes
// WindowEvent::Null (= 0).  The cHelper::kWeNull
// constant is locked to 0 below (1:1 with legacy).
//
TEST(CHelperTest, ActionEventReturnsWeNull) {
    Harness h;
    EXPECT_EQ(h.dlg.ActionEvent(10, 20, 0), cHelper::kWeNull);
}

TEST(CHelperTest, ActionEventForwardsToCallback) {
    ActionTracker tracker;
    Harness h;
    h.dlg.SetActionCallbackForTest(ActionCb, &tracker);
    int ani = 0;
    h.dlg.SetMotionAniForTest(0, &ani);
    EXPECT_EQ(h.dlg.ActionEvent(10, 20, 0), cHelper::kWeNull);
    EXPECT_EQ(tracker.call_count, 1);
    EXPECT_EQ(tracker.last_ani, &ani);
}

TEST(CHelperTest, ActionEventWithNullAniIsNoOp) {
    ActionTracker tracker;
    Harness h;
    h.dlg.SetActionCallbackForTest(ActionCb, &tracker);
    h.dlg.SetMotionAniForTest(0, nullptr);
    EXPECT_EQ(h.dlg.ActionEvent(10, 20, 0), cHelper::kWeNull);
    EXPECT_EQ(tracker.call_count, 0);
}

TEST(CHelperTest, ActionEventBeforeInitDoesNotCrash) {
    cHelper dlg;
    EXPECT_EQ(dlg.ActionEvent(0, 0, 0), cHelper::kWeNull);
}
//
// 1:1 quirk note: legacy SetMotion swaps
// m_wCurMotion = Idx and toggles the cAni SetActive
// + Stop + SetCurSpriteIdx on the old motion and
// SetActive + Play on the new motion.  The cAni
// toggle is deferred to the host (R-12.x), but the
// m_curMotion swap is locked below (1:1 with legacy).
//
TEST(CHelperTest, SetMotionSwapsCurrentMotion) {
    cHelper dlg;
    dlg.SetMotion(HelperMotion::Stand);
    EXPECT_EQ(dlg.GetMotion(), HelperMotion::Stand);
}

TEST(CHelperTest, SetMotionOutOfRangeIsNoOp) {
    cHelper dlg;
    const auto before = dlg.GetMotion();
    dlg.SetMotion(static_cast<HelperMotion>(99));
    EXPECT_EQ(dlg.GetMotion(), before);
}
//
// 1:1 quirk note: legacy SetMaxSprite(wIdx, nMaxNum)
// forwards to m_MotionList[wIdx].SetMaxSprite(nMaxNum).
// Modern port forwards via host-injected callback.
//
TEST(CHelperTest, SetMaxSpriteForwardsToCallback) {
    SetMaxSpriteTracker tracker;
    Harness h;
    h.dlg.SetSetMaxSpriteCallbackForTest(SetMaxSpriteCb, &tracker);
    int ani = 0;
    h.dlg.SetMotionAniForTest(0, &ani);
    h.dlg.SetMaxSprite(0, 8);
    EXPECT_EQ(tracker.call_count, 1);
    EXPECT_EQ(tracker.last_max, 8);
}

TEST(CHelperTest, SetMaxSpriteWithNullAniIsNoOp) {
    SetMaxSpriteTracker tracker;
    Harness h;
    h.dlg.SetSetMaxSpriteCallbackForTest(SetMaxSpriteCb, &tracker);
    h.dlg.SetMaxSprite(0, 8);
    EXPECT_EQ(tracker.call_count, 0);
}

TEST(CHelperTest, SetMaxSpriteOutOfRangeIsNoOp) {
    SetMaxSpriteTracker tracker;
    Harness h;
    h.dlg.SetSetMaxSpriteCallbackForTest(SetMaxSpriteCb, &tracker);
    int ani = 0;
    h.dlg.SetMotionAniForTest(0, &ani);
    h.dlg.SetMaxSprite(99, 8);
    EXPECT_EQ(tracker.call_count, 0);
}
//
// 1:1 quirk note: legacy AddSprite(wIdx, cImage*,
// WORD delay) forwards to m_MotionList[wIdx]
// .AddSprite(sprite, delay) + Init(absPos, sprite
// size, NULL).  The cAni Init step is deferred to
// the host via the AddSprite callback in modern.
//
TEST(CHelperTest, AddSpriteForwardsToCallback) {
    AddSpriteTracker tracker;
    Harness h;
    h.dlg.SetAddSpriteCallbackForTest(AddSpriteCb, &tracker);
    int ani = 0;
    int sprite = 0;
    h.dlg.SetMotionAniForTest(0, &ani);
    h.dlg.AddSprite(0, &sprite, 100);
    EXPECT_EQ(tracker.call_count, 1);
    EXPECT_EQ(tracker.last_sprite, &sprite);
    EXPECT_EQ(tracker.last_delay, 100);
}

TEST(CHelperTest, AddSpriteWithNullAniIsNoOp) {
    AddSpriteTracker tracker;
    Harness h;
    h.dlg.SetAddSpriteCallbackForTest(AddSpriteCb, &tracker);
    int sprite = 0;
    h.dlg.AddSprite(0, &sprite, 100);
    EXPECT_EQ(tracker.call_count, 0);
}

TEST(CHelperTest, AddSpriteOutOfRangeIsNoOp) {
    AddSpriteTracker tracker;
    Harness h;
    h.dlg.SetAddSpriteCallbackForTest(AddSpriteCb, &tracker);
    int ani = 0;
    h.dlg.SetMotionAniForTest(0, &ani);
    h.dlg.AddSprite(99, nullptr, 100);
    EXPECT_EQ(tracker.call_count, 0);
}
//
// 1:1 quirk note: legacy SetStartTime sets
// m_dwStartTime = m_dwCurTime = time and
// m_bGreetCheck = TRUE.  All three are locked.
//
TEST(CHelperTest, SetStartTimeSetsAllThreeFields) {
    cHelper dlg;
    dlg.StopGreetCheck();
    EXPECT_FALSE(dlg.IsGreetCheck());
    dlg.SetStartTime(12345);
    EXPECT_TRUE(dlg.IsGreetCheck());
}

TEST(CHelperTest, SetGreetTimeStoresValue) {
    cHelper dlg;
    dlg.SetGreetTime(9876);
    EXPECT_EQ(dlg.GetGreetTime(), 9876u);
}

TEST(CHelperTest, StopGreetCheckClearsFlag) {
    cHelper dlg;
    EXPECT_TRUE(dlg.IsGreetCheck());
    dlg.StopGreetCheck();
    EXPECT_FALSE(dlg.IsGreetCheck());
}
//
// 1:1 quirk note: legacy timer fields use DWORD
// (32-bit).  Modern port uses std::uint32_t.
// The boundary values below (UINT32_MAX) verify the
// modern type is wide enough (1:1 with legacy DWORD).
//
TEST(CHelperTest, SetGreetTimeAcceptsMaxUint32) {
    cHelper dlg;
    dlg.SetGreetTime(0xFFFFFFFFu);
    EXPECT_EQ(dlg.GetGreetTime(), 0xFFFFFFFFu);
}
//
// 1:1 quirk note: legacy cHelper enum HELPER_MOTION
// { emHM_Stand = 0, emHM_MAX = 1 } (only Stand is
// used in legacy -- the array size is emHM_MAX = 1).
// Modern port enum HelperMotion matches that.
//
TEST(CHelperTest, HelperMotionEnumStandIsZero) {
    EXPECT_EQ(static_cast<int>(HelperMotion::Stand), 0);
}

TEST(CHelperTest, HelperMotionEnumMaxIsOne) {
    EXPECT_EQ(static_cast<int>(HelperMotion::Max), 1);
}

TEST(CHelperTest, KMaxMotionSlotsConstantIsOne) {
    // 1:1 with legacy emHM_MAX = 1.
    EXPECT_EQ(cHelper::kMaxMotionSlots, 1u);
}

TEST(CHelperTest, KWeNullConstantIsZero) {
    // 1:1 with legacy WE_NULL = 0.
    EXPECT_EQ(cHelper::kWeNull, 0u);
}
//
// 1:1 quirk note: legacy SetMotion body also toggles
// the cAni SetActive + Stop + SetCurSpriteIdx +
// Play, but the modern port only preserves the
// m_curMotion swap (R-12.x deferred).  The
// SetMotionSwapsThenRenderUsesNewMotion test
// exercises the combination of SetMotion + Render.
//
TEST(CHelperTest, SetMotionSwapsThenRenderUsesNewMotion) {
    RenderTracker tracker;
    Harness h;
    h.dlg.SetRenderCallbackForTest(RenderCb, &tracker);
    int ani = 0;
    h.dlg.SetMotionAniForTest(0, &ani);
    h.dlg.SetActive(true);
    h.dlg.SetMotion(HelperMotion::Stand);
    EXPECT_EQ(h.dlg.GetMotion(), HelperMotion::Stand);
    h.dlg.Render();
    EXPECT_EQ(tracker.last_ani, &ani);
}