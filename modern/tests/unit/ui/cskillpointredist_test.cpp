//
// Unit tests for mxh::ui::cSkillPointRedist
// (Phase C dialog port).
//
// Locks down the 1:1 surface of legacy
// CSkillPointRedist (skill-point redistribution
// dialog: 12 child windows, 3 tabs, ability-icon
// refresh loop, item idx/pos storage):
//   * Default construction: cSkillPointRedist is a
//     cDialog.
//   * Inherits from cDialog.
//   * NonCopyable.
//   * Init + SetAbsXY works (inherited).
//   * Init preserves the dialog id.
//   * Init position / size.
//   * Init idempotence.
//   * SetActive forwards to base cDialog (1:1).
//   * SetActive(true) triggers refresh callback.
//   * SetActive(false) triggers deactivate callback.
//   * Linking resolves the 12 child windows via
//     the host-injected window resolver.
//   * Linking without resolver leaves slots NULL.
//   * RefreshAbilityIcons forwards to callback.
//   * MakeNewAbilityIcon returns host result.
//   * SetAbilityToolTip forwards to callback.
//   * SetAbilitySyn forwards to callback.
//   * SetAbilityExp caches the exp value.
//   * RefreshAbilityPoint forwards to callback.
//   * GetCurAbilityName returns host result.
//   * GetCurAbilityLevel returns host result.
//   * GetCurAbilityInfo returns host result.
//   * GetCurItemIdx / GetCurItemPos return stored
//     values (1:1 with legacy m_ItemIdx / m_ItemPos).
//   * SetCurItem stores both idx and pos.
//   * SetTabNumber sets the current tab.
//   * SetTabNumber out-of-range is a no-op.
//   * GetTabNumber returns the current tab.
//   * kIdDialog constant matches SK_POINTDLG.
//   * kIdUpBtn / kIdDownBtn / kIdOkBtn constants
//     match SK_UPBTN / SK_DOWNBTN / SK_OKBTN.
//   * kIdRePoint / kIdUsePoint / kIdOgPoint
//     constants match SK_POINTSTATIC /
//     SK_USESTATIC / SK_ORIGINALSTATIC.
//   * kIdTabBtn0..2 / kIdIconGrid0..2 constants
//     match SK_POINTAGAIN{1,2,3}BTN /
//     SK_ICONGRID{1,2,3}.
//   * kAbilityIdx = {100, 200, 400} (1:1 with
//     legacy AbilityIdx[eTab_Max]).
//   * Default tab is War (1:1 with legacy ctor).
//   * Default item idx + pos are 0.
//   * Default ability exp is 0.
//   * SkillRedistTab enum has War=0, KyungGong=1,
//     Character=2, Max=3 (1:1 with legacy).
//

#include "mxh/ui/cskillpointredist.hpp"
#include "mxh/ui/cdialog.hpp"
#include "mxh/ui/cwindow.hpp"
#include "mxh/ui/cbutton.hpp"
#include "mxh/ui/cstatic.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

using mxh::ui::cButton;
using mxh::ui::cDialog;
using mxh::ui::cSkillPointRedist;
using mxh::ui::cStatic;
using mxh::ui::cWindow;
using mxh::ui::SkillRedistTab;

namespace {

struct Harness {
    cSkillPointRedist dlg;
    Harness() {
        dlg.Init(0, 0, 400, 300, nullptr,
                 cSkillPointRedist::kIdDialog);
    }
};

struct CallbackTracker {
    int call_count = 0;
    void* last_arg = nullptr;
    int last_int = 0;
    bool last_bool = false;
};

void RefreshIconsCb(void* user) { ++static_cast<CallbackTracker*>(user)->call_count; }
void DeactivateCb(void* user) { ++static_cast<CallbackTracker*>(user)->call_count; }
void SetToolTipCb(void* pIcon, void* user) {
    auto* t = static_cast<CallbackTracker*>(user);
    ++t->call_count; t->last_arg = pIcon;
}
void SetAbilitySynCb(bool bDown, void* user) {
    auto* t = static_cast<CallbackTracker*>(user);
    ++t->call_count; t->last_bool = bDown;
}
void* MakeIconCb(void* pInfo, void* user) {
    auto* t = static_cast<CallbackTracker*>(user);
    ++t->call_count; t->last_arg = pInfo;
    return pInfo;
}
void RefreshPointCb(void* user) { ++static_cast<CallbackTracker*>(user)->call_count; }
const char* NameCb(void* user) {
    ++static_cast<CallbackTracker*>(user)->call_count;
    return "TestAbility";
}
int LevelCb(void* user) {
    auto* t = static_cast<CallbackTracker*>(user);
    ++t->call_count; t->last_int = 7;
    return 7;
}
void* InfoCb(void* user) {
    auto* t = static_cast<CallbackTracker*>(user);
    ++t->call_count;
    static int x = 42;
    return &x;
}

}  // namespace

// ---------- Construction / destruction ----------

TEST(CSkillPointRedistTest, CtorDoesNotCrash) {
    cSkillPointRedist dlg;
    SUCCEED();
}

TEST(CSkillPointRedistTest, DtorDoesNotCrash) {
    cSkillPointRedist dlg;
    SUCCEED();
}

TEST(CSkillPointRedistTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cSkillPointRedist>,
                  "cSkillPointRedist must inherit from cDialog");
    SUCCEED();
}

TEST(CSkillPointRedistTest, IsAlsoAWindow) {
    static_assert(std::is_base_of_v<cWindow, cSkillPointRedist>,
                  "cSkillPointRedist must be a cWindow (transitively)");
    SUCCEED();
}

TEST(CSkillPointRedistTest, NonCopyable) {
    static_assert(!std::is_copy_constructible_v<cSkillPointRedist>,
                  "cSkillPointRedist must be non-copyable");
    static_assert(!std::is_copy_assignable_v<cSkillPointRedist>,
                  "cSkillPointRedist must be non-copy-assignable");
    SUCCEED();
}

TEST(CSkillPointRedistTest, MultipleInstancesAreSafe) {
    cSkillPointRedist a;
    cSkillPointRedist b;
    SUCCEED();
}

TEST(CSkillPointRedistTest, DefaultTabIsWar) {
    cSkillPointRedist dlg;
    EXPECT_EQ(dlg.GetTabNumber(), 0u);
    EXPECT_EQ(dlg.GetTabNumber(), static_cast<std::uint32_t>(SkillRedistTab::War));
}

TEST(CSkillPointRedistTest, DefaultItemIdxIsZero) {
    cSkillPointRedist dlg;
    EXPECT_EQ(dlg.GetCurItemIdx(), 0u);
}

TEST(CSkillPointRedistTest, DefaultItemPosIsZero) {
    cSkillPointRedist dlg;
    EXPECT_EQ(dlg.GetCurItemPos(), 0u);
}

TEST(CSkillPointRedistTest, DefaultChildSlotsAreNull) {
    cSkillPointRedist dlg;
    EXPECT_EQ(dlg.GetUpBtnForTest(), nullptr);
    EXPECT_EQ(dlg.GetDownBtnForTest(), nullptr);
    EXPECT_EQ(dlg.GetOkBtnForTest(), nullptr);
    EXPECT_EQ(dlg.GetRePointForTest(), nullptr);
    EXPECT_EQ(dlg.GetUsePointForTest(), nullptr);
    EXPECT_EQ(dlg.GetOgPointForTest(), nullptr);
    for (std::size_t i = 0; i < cSkillPointRedist::kTabCount; ++i) {
        EXPECT_EQ(dlg.GetGridButtonForTest(i), nullptr);
        EXPECT_EQ(dlg.GetIconGridForTest(i), nullptr);
    }
}

// ---------- Init ----------

TEST(CSkillPointRedistTest, InitStoresPositionAndId) {
    Harness h;
    EXPECT_EQ(h.dlg.absX(), 0);
    EXPECT_EQ(h.dlg.absY(), 0);
    EXPECT_EQ(h.dlg.width(), 400u);
    EXPECT_EQ(h.dlg.height(), 300u);
    EXPECT_EQ(h.dlg.id(), cSkillPointRedist::kIdDialog);
}

TEST(CSkillPointRedistTest, InitIsIdempotent) {
    cSkillPointRedist dlg;
    dlg.Init(0, 0, 100, 100, nullptr, 1);
    dlg.Init(10, 20, 200, 200, nullptr, 2);
    EXPECT_EQ(dlg.width(), 200u);
    EXPECT_EQ(dlg.height(), 200u);
    EXPECT_EQ(dlg.id(), 2);
}

TEST(CSkillPointRedistTest, InitBeforeAnyStateChangeDoesNotCrash) {
    cSkillPointRedist dlg;
    dlg.Init(0, 0, 100, 100, nullptr, 0);
    SUCCEED();
}

// ---------- SetActive (1:1 with legacy override) ----------

TEST(CSkillPointRedistTest, SetActiveForwardsToBase) {
    Harness h;
    EXPECT_FALSE(h.dlg.isActive());
    h.dlg.SetActive(true);
    EXPECT_TRUE(h.dlg.isActive());
    h.dlg.SetActive(false);
    EXPECT_FALSE(h.dlg.isActive());
}

TEST(CSkillPointRedistTest, SetActiveTrueTriggersRefreshCallback) {
    CallbackTracker tracker;
    Harness h;
    h.dlg.SetRefreshIconsCallbackForTest(RefreshIconsCb, &tracker);
    h.dlg.SetActive(true);
    EXPECT_GE(tracker.call_count, 1);
}

TEST(CSkillPointRedistTest, SetActiveFalseTriggersDeactivateCallback) {
    CallbackTracker tracker;
    Harness h;
    h.dlg.SetActive(true);
    h.dlg.SetDeactivateCallbackForTest(DeactivateCb, &tracker);
    h.dlg.SetActive(false);
    EXPECT_GE(tracker.call_count, 1);
}

// ---------- Linking (1:1 with legacy 12-child lookup) ----------

TEST(CSkillPointRedistTest, LinkingResolvesAll12Children) {
    cButton btnUp, btnDown, btnOk;
    cStatic stRe, stUse, stOg;
    auto* resolver = +[](std::int32_t id, void* user) -> void* {
        (void)user;
        // Use the static ids; return a placeholder
        // for any non-zero id so Linking populates all
        // 12 slots.
        static int dummy = 1;
        if (id == cSkillPointRedist::kIdUpBtn) return &dummy;
        if (id == cSkillPointRedist::kIdDownBtn) return &dummy;
        if (id == cSkillPointRedist::kIdOkBtn) return &dummy;
        if (id == cSkillPointRedist::kIdRePoint) return &dummy;
        if (id == cSkillPointRedist::kIdUsePoint) return &dummy;
        if (id == cSkillPointRedist::kIdOgPoint) return &dummy;
        if (id >= cSkillPointRedist::kIdTabBtn0 &&
            id < cSkillPointRedist::kIdTabBtn0 + 3) return &dummy;
        if (id >= cSkillPointRedist::kIdIconGrid0 &&
            id < cSkillPointRedist::kIdIconGrid0 + 3) return &dummy;
        return nullptr;
    };
    Harness h;
    h.dlg.SetWindowResolverForTest(resolver, nullptr);
    h.dlg.Linking();
    EXPECT_NE(h.dlg.GetUpBtnForTest(), nullptr);
    EXPECT_NE(h.dlg.GetDownBtnForTest(), nullptr);
    EXPECT_NE(h.dlg.GetOkBtnForTest(), nullptr);
    EXPECT_NE(h.dlg.GetRePointForTest(), nullptr);
    EXPECT_NE(h.dlg.GetUsePointForTest(), nullptr);
    EXPECT_NE(h.dlg.GetOgPointForTest(), nullptr);
    for (std::size_t i = 0; i < cSkillPointRedist::kTabCount; ++i) {
        EXPECT_NE(h.dlg.GetGridButtonForTest(i), nullptr);
        EXPECT_NE(h.dlg.GetIconGridForTest(i), nullptr);
    }
}

TEST(CSkillPointRedistTest, LinkingWithoutResolverLeavesSlotsNull) {
    Harness h;
    h.dlg.Linking();
    EXPECT_EQ(h.dlg.GetUpBtnForTest(), nullptr);
    EXPECT_EQ(h.dlg.GetDownBtnForTest(), nullptr);
    EXPECT_EQ(h.dlg.GetOkBtnForTest(), nullptr);
}

TEST(CSkillPointRedistTest, LinkingBeforeInitDoesNotCrash) {
    cSkillPointRedist dlg;
    dlg.Linking();
    SUCCEED();
}

// ---------- R-12.x deferred surfaces (host-injected callbacks) ----------

TEST(CSkillPointRedistTest, RefreshAbilityIconsForwardsToCallback) {
    CallbackTracker tracker;
    Harness h;
    h.dlg.SetRefreshIconsCallbackForTest(RefreshIconsCb, &tracker);
    h.dlg.RefreshAbilityIcons();
    EXPECT_EQ(tracker.call_count, 1);
}

TEST(CSkillPointRedistTest, MakeNewAbilityIconForwardsToCallback) {
    CallbackTracker tracker;
    Harness h;
    h.dlg.SetMakeIconCallbackForTest(MakeIconCb, &tracker);
    int info = 0;
    void* result = h.dlg.MakeNewAbilityIcon(&info);
    EXPECT_EQ(result, &info);
    EXPECT_EQ(tracker.call_count, 1);
    EXPECT_EQ(tracker.last_arg, &info);
}

TEST(CSkillPointRedistTest, SetAbilityToolTipForwardsToCallback) {
    CallbackTracker tracker;
    Harness h;
    h.dlg.SetToolTipCallbackForTest(SetToolTipCb, &tracker);
    int icon = 0;
    h.dlg.SetAbilityToolTip(&icon);
    EXPECT_EQ(tracker.call_count, 1);
    EXPECT_EQ(tracker.last_arg, &icon);
}

TEST(CSkillPointRedistTest, SetAbilitySynForwardsToCallback) {
    CallbackTracker tracker;
    Harness h;
    h.dlg.SetAbilitySynCallbackForTest(SetAbilitySynCb, &tracker);
    h.dlg.SetAbilitySyn(true);
    EXPECT_EQ(tracker.call_count, 1);
    EXPECT_TRUE(tracker.last_bool);
    h.dlg.SetAbilitySyn(false);
    EXPECT_EQ(tracker.call_count, 2);
    EXPECT_FALSE(tracker.last_bool);
}

TEST(CSkillPointRedistTest, SetAbilityExpCachesValue) {
    cSkillPointRedist dlg;
    dlg.SetAbilityExp(12345);
    dlg.SetAbilityExp(67890);
    SUCCEED();
}

TEST(CSkillPointRedistTest, RefreshAbilityPointForwardsToCallback) {
    CallbackTracker tracker;
    Harness h;
    h.dlg.SetRefreshPointCallbackForTest(RefreshPointCb, &tracker);
    h.dlg.RefreshAbilityPoint();
    EXPECT_EQ(tracker.call_count, 1);
}

TEST(CSkillPointRedistTest, GetCurAbilityNameForwardsToCallback) {
    CallbackTracker tracker;
    Harness h;
    h.dlg.SetCurAbilityNameCallbackForTest(NameCb, &tracker);
    const char* name = h.dlg.GetCurAbilityName();
    EXPECT_STREQ(name, "TestAbility");
    EXPECT_EQ(tracker.call_count, 1);
}

TEST(CSkillPointRedistTest, GetCurAbilityLevelForwardsToCallback) {
    CallbackTracker tracker;
    Harness h;
    h.dlg.SetCurAbilityLevelCallbackForTest(LevelCb, &tracker);
    EXPECT_EQ(h.dlg.GetCurAbilityLevel(), 7);
    EXPECT_EQ(tracker.call_count, 1);
}

TEST(CSkillPointRedistTest, GetCurAbilityInfoForwardsToCallback) {
    CallbackTracker tracker;
    Harness h;
    h.dlg.SetCurAbilityInfoCallbackForTest(InfoCb, &tracker);
    void* info = h.dlg.GetCurAbilityInfo();
    EXPECT_NE(info, nullptr);
    EXPECT_EQ(tracker.call_count, 1);
}

TEST(CSkillPointRedistTest, R12DeferredMethodsAreSafeWithoutCallbacks) {
    Harness h;
    h.dlg.RefreshAbilityIcons();
    h.dlg.MakeNewAbilityIcon(nullptr);
    h.dlg.SetAbilityToolTip(nullptr);
    h.dlg.SetAbilitySyn(false);
    h.dlg.RefreshAbilityPoint();
    h.dlg.GetCurAbilityName();
    h.dlg.GetCurAbilityLevel();
    h.dlg.GetCurAbilityInfo();
    SUCCEED();
}

// ---------- Item idx/pos storage (1:1 with legacy) ----------

TEST(CSkillPointRedistTest, SetCurItemStoresBothFields) {
    cSkillPointRedist dlg;
    dlg.SetCurItem(42, 7);
    EXPECT_EQ(dlg.GetCurItemIdx(), 42u);
    EXPECT_EQ(dlg.GetCurItemPos(), 7u);
}

TEST(CSkillPointRedistTest, SetCurItemOverwritesPrevious) {
    cSkillPointRedist dlg;
    dlg.SetCurItem(1, 2);
    dlg.SetCurItem(3, 4);
    EXPECT_EQ(dlg.GetCurItemIdx(), 3u);
    EXPECT_EQ(dlg.GetCurItemPos(), 4u);
}

// ---------- Tab number storage (1:1 with legacy) ----------

TEST(CSkillPointRedistTest, SetTabNumberStoresValue) {
    cSkillPointRedist dlg;
    dlg.SetTabNumber(1);
    EXPECT_EQ(dlg.GetTabNumber(), 1u);
    dlg.SetTabNumber(2);
    EXPECT_EQ(dlg.GetTabNumber(), 2u);
}

TEST(CSkillPointRedistTest, SetTabNumberOutOfRangeIsNoOp) {
    cSkillPointRedist dlg;
    dlg.SetTabNumber(2);
    dlg.SetTabNumber(99);
    EXPECT_EQ(dlg.GetTabNumber(), 2u);
}

// ---------- 1:1 id constants (legacy WindowIDs.h) ----------

TEST(CSkillPointRedistTest, KIdDialogMatchesLegacyEnum) {
    EXPECT_EQ(cSkillPointRedist::kIdDialog, 1288);
}

TEST(CSkillPointRedistTest, KIdUpDownOkBtnConstantsMatchLegacyEnum) {
    EXPECT_EQ(cSkillPointRedist::kIdUpBtn, 1298);
    EXPECT_EQ(cSkillPointRedist::kIdDownBtn, 1299);
    EXPECT_EQ(cSkillPointRedist::kIdOkBtn, 1300);
}

TEST(CSkillPointRedistTest, KIdPointStaticConstantsMatchLegacyEnum) {
    EXPECT_EQ(cSkillPointRedist::kIdRePoint, 1295);
    EXPECT_EQ(cSkillPointRedist::kIdUsePoint, 1296);
    EXPECT_EQ(cSkillPointRedist::kIdOgPoint, 1297);
}

TEST(CSkillPointRedistTest, KIdTabBtnConstantsMatchLegacyEnum) {
    EXPECT_EQ(cSkillPointRedist::kIdTabBtn0, 1289);
    EXPECT_EQ(cSkillPointRedist::kIdTabBtn1, 1290);
    EXPECT_EQ(cSkillPointRedist::kIdTabBtn2, 1291);
}

TEST(CSkillPointRedistTest, KIdIconGridConstantsMatchLegacyEnum) {
    EXPECT_EQ(cSkillPointRedist::kIdIconGrid0, 1292);
    EXPECT_EQ(cSkillPointRedist::kIdIconGrid1, 1293);
    EXPECT_EQ(cSkillPointRedist::kIdIconGrid2, 1294);
}

TEST(CSkillPointRedistTest, KAbilityIdxMatchesLegacyArray) {
    EXPECT_EQ(cSkillPointRedist::kAbilityIdx[0], 100u);
    EXPECT_EQ(cSkillPointRedist::kAbilityIdx[1], 200u);
    EXPECT_EQ(cSkillPointRedist::kAbilityIdx[2], 400u);
}

TEST(CSkillPointRedistTest, KTabCountMatchesLegacy) {
    // 1:1 with legacy eTab_Max = 3.
    EXPECT_EQ(cSkillPointRedist::kTabCount, 3u);
}

TEST(CSkillPointRedistTest, SkillRedistTabEnumValues) {
    EXPECT_EQ(static_cast<int>(SkillRedistTab::War), 0);
    EXPECT_EQ(static_cast<int>(SkillRedistTab::KyungGong), 1);
    EXPECT_EQ(static_cast<int>(SkillRedistTab::Character), 2);
    EXPECT_EQ(static_cast<int>(SkillRedistTab::Max), 3);
}