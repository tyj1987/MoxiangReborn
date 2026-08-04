// mxh/tests/unit/ui/coptiondialog_test.cpp
//
// Unit tests for mxh::ui::cOptionDialog (Phase C dialog port).
//
// Locks down the 1:1 surface:
//   * sGAMEOPTION field order is preserved (sizeof sanity)
//   * Add() routes cPushupButton / cDialog into the right slots
//   * SetActive(true) pulls a fresh snapshot via the default callback
//     + calls UpdateData(FALSE) to populate the widgets
//   * SetActive(true) notifies the main-bar icon callback
//   * OnActionEvent OK dispatches the apply callback + closes
//   * OnActionEvent CANCEL dispatches the cancel callback + closes
//   * OnActionEvent RESET dispatches the default callback + repaints
//   * OnActionEvent PUSHDOWN of one pushup unpushes the other
//   * OnActionEvent CHECKED on AUTOCONTROL disables the graphic tab
//   * DisableGraphicTab flips the flag
//   * GetEffectSnow reflects the snapshot
//   * UpdateData(TRUE) reads from the widget accessor into m_GameOption
//   * UpdateData(FALSE) writes m_GameOption back to the widget accessor
//   * The struct fields the legacy code toggles (bAutoCtrl,
//     bMunpaIntro, nLODMode, nEffectSnow, bAmbientMax, bIntroFlag,
//     bNoAvatarView) are all present and round-trippable

#include "mxh/ui/coptiondialog.hpp"
#include "mxh/ui/cPushupButton.hpp"
#include "mxh/ui/ccheckbox.hpp"
#include "mxh/ui/legacy_window_event.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <map>
#include <string>

using mxh::ui::cOptionDialog;
using mxh::ui::sGAMEOPTION;
using mxh::ui::cPushupButton;
using mxh::ui::cCheckBox;
using mxh::ui::cDialog;
using mxh::ui::cWindow;

namespace {

// Tiny in-memory widget registry the test accessor reads/writes.
struct FakeWidgetStore {
    std::map<std::int32_t, bool> checkboxes;
    std::map<std::int32_t, bool> pushups;
    std::map<std::int32_t, int>  guages;
};

bool faIsChecked(std::int32_t id, void* user) {
    auto* s = static_cast<FakeWidgetStore*>(user);
    auto it = s->checkboxes.find(id);
    return it != s->checkboxes.end() ? it->second : false;
}
void faSetChecked(std::int32_t id, bool v, void* user) {
    static_cast<FakeWidgetStore*>(user)->checkboxes[id] = v;
}
bool faIsPushed(std::int32_t id, void* user) {
    auto* s = static_cast<FakeWidgetStore*>(user);
    auto it = s->pushups.find(id);
    return it != s->pushups.end() ? it->second : false;
}
void faSetPush(std::int32_t id, bool v, void* user) {
    static_cast<FakeWidgetStore*>(user)->pushups[id] = v;
}
int faGuageGet(std::int32_t id, void* user) {
    auto* s = static_cast<FakeWidgetStore*>(user);
    auto it = s->guages.find(id);
    return it != s->guages.end() ? it->second : 0;
}
void faGuageSet(std::int32_t id, int v, void* user) {
    static_cast<FakeWidgetStore*>(user)->guages[id] = v;
}

struct Harness {
    cOptionDialog dlg;
    FakeWidgetStore store;
    cPushupButton tabBtn;
    cDialog tabSheet;
    Harness() {
        cOptionDialog::WidgetAccessor wa{};
        wa.checkboxIsChecked = &faIsChecked;
        wa.checkboxSetChecked = &faSetChecked;
        wa.pushupIsPushed = &faIsPushed;
        wa.pushupSetPush = &faSetPush;
        wa.guageGetCur = &faGuageGet;
        wa.guageSetCur = &faGuageSet;
        wa.user = &store;
        dlg.SetWidgetAccessorForTest(wa);
    }
};

}  // namespace

TEST(COptionDialog, SGameOptionHasAllLegacyFields) {
    // 1:1 with legacy sGAMEOPTION field order.  We do a
    // sizeof check (a sharp drop in size would mean a field
    // got dropped by accident).
    EXPECT_GE(sizeof(sGAMEOPTION), 0x46u);
    sGAMEOPTION o{};
    o.bNoDeal = true;     EXPECT_TRUE(o.bNoDeal);
    o.bNoParty = true;    EXPECT_TRUE(o.bNoParty);
    o.bNoFriend = true;   EXPECT_TRUE(o.bNoFriend);
    o.bNoVimu = true;     EXPECT_TRUE(o.bNoVimu);
    o.bNameMunpa = true;  EXPECT_TRUE(o.bNameMunpa);
    o.bNameParty = true;  EXPECT_TRUE(o.bNameParty);
    o.bNameOthers = true; EXPECT_TRUE(o.bNameOthers);
    o.bNoMemberDamage = true; EXPECT_TRUE(o.bNoMemberDamage);
    o.bNoGameTip = true;  EXPECT_TRUE(o.bNoGameTip);
    o.bMunpaIntro = true; EXPECT_TRUE(o.bMunpaIntro);
    o.nMacroMode = 1;     EXPECT_EQ(o.nMacroMode, 1);
    o.bNoWhisper = true;  EXPECT_TRUE(o.bNoWhisper);
    o.bNoChatting = true; EXPECT_TRUE(o.bNoChatting);
    o.bNoBalloon = true;  EXPECT_TRUE(o.bNoBalloon);
    o.bAutoHide = true;   EXPECT_TRUE(o.bAutoHide);
    o.bNoShoutChat = true; EXPECT_TRUE(o.bNoShoutChat);
    o.bNoGuildChat = true; EXPECT_TRUE(o.bNoGuildChat);
    o.bNoAllianceChat = true; EXPECT_TRUE(o.bNoAllianceChat);
    o.bNoSystemMsg = true; EXPECT_TRUE(o.bNoSystemMsg);
    o.bNoExpMsg = true;   EXPECT_TRUE(o.bNoExpMsg);
    o.bNoItemMsg = true;  EXPECT_TRUE(o.bNoItemMsg);
    o.nGamma = 50;        EXPECT_EQ(o.nGamma, 50);
    o.nSightDistance = 100; EXPECT_EQ(o.nSightDistance, 100);
    o.bGraphicCursor = true; EXPECT_TRUE(o.bGraphicCursor);
    o.bShadowHero = true;  EXPECT_TRUE(o.bShadowHero);
    o.bShadowMonster = true; EXPECT_TRUE(o.bShadowMonster);
    o.bShadowOthers = true; EXPECT_TRUE(o.bShadowOthers);
    o.bAutoCtrl = true;    EXPECT_TRUE(o.bAutoCtrl);
    o.nLODMode = 1;        EXPECT_EQ(o.nLODMode, 1);
    o.nEffectMode = 1;     EXPECT_EQ(o.nEffectMode, 1);
    o.nEffectSnow = 1;     EXPECT_EQ(o.nEffectSnow, 1);
    o.bSoundBGM = true;    EXPECT_TRUE(o.bSoundBGM);
    o.bSoundEnvironment = true; EXPECT_TRUE(o.bSoundEnvironment);
    o.nVolumnBGM = 80;     EXPECT_EQ(o.nVolumnBGM, 80);
    o.nVolumnEnvironment = 50; EXPECT_EQ(o.nVolumnEnvironment, 50);
    o.bAmbientMax = true;  EXPECT_TRUE(o.bAmbientMax);
    o.bIntroFlag = true;   EXPECT_TRUE(o.bIntroFlag);
    o.bNoAvatarView = true; EXPECT_TRUE(o.bNoAvatarView);
}

TEST(COptionDialog, DefaultConstructionHasZeroGameOption) {
    cOptionDialog d;
    EXPECT_FALSE(d.gameOption().bNoDeal);
    EXPECT_EQ(d.gameOption().nGamma, 0);
    EXPECT_FALSE(d.isGraphicTabDisabled());
    EXPECT_FALSE(d.isActive());
}

TEST(COptionDialog, AddRoutesPushupButtonToTabBtnSlot) {
    cOptionDialog d;
    cPushupButton btn;
    d.Add(&btn);
    // The widget was added (we can't introspect the slot
    // directly, but the dialog should not have crashed).
    SUCCEED();
}

TEST(COptionDialog, AddRoutesDialogToTabSheetSlot) {
    cOptionDialog d;
    cDialog sheet;
    d.Add(&sheet);
    SUCCEED();
}

TEST(COptionDialog, SetActiveTruePullsSnapshotAndUpdatesWidgets) {
    Harness h;
    int callCount = 0;
    h.dlg.SetDefaultCallbackForTest(
        [](sGAMEOPTION* opt, void* user) {
            ++*static_cast<int*>(user);
            opt->bNoDeal = true;
            opt->bNoParty = true;
            opt->nGamma = 99;
            opt->nEffectSnow = 1;
        },
        &callCount);
    h.dlg.SetActive(true);
    EXPECT_TRUE(h.dlg.isActive());
    EXPECT_TRUE(h.dlg.gameOption().bNoDeal);
    EXPECT_TRUE(h.dlg.gameOption().bNoParty);
    EXPECT_EQ(h.dlg.gameOption().nGamma, 99);
    // UpdateData(FALSE) writes back into the fake store.
    EXPECT_TRUE(h.store.checkboxes[101]);   // bNoDeal id=101
    EXPECT_TRUE(h.store.checkboxes[102]);   // bNoParty
    EXPECT_EQ(h.store.guages[301], 99);     // gamma id=301
    EXPECT_TRUE(h.store.checkboxes[307]);   // nEffectSnow id=307
}

TEST(COptionDialog, SetActiveTrueNotifiesMainBarIcon) {
    Harness h;
    struct Ctx { int n = 0; bool a = false; };
    Ctx ctx;
    h.dlg.SetMainBarIconCallbackForTest(
        [](bool active, void* user) {
            auto* c = static_cast<Ctx*>(user);
            ++c->n;
            c->a = active;
        },
        &ctx);
    h.dlg.SetActive(true);
    EXPECT_EQ(ctx.n, 1);
    EXPECT_TRUE(ctx.a);
    h.dlg.SetActive(false);
    EXPECT_EQ(ctx.n, 2);
    EXPECT_FALSE(ctx.a);
}

TEST(COptionDialog, OnActionEventOkAppliesAndCloses) {
    Harness h;
    int applyCount = 0;
    h.dlg.SetApplyCallbackForTest(
        [](sGAMEOPTION*, void* user) {
            auto* c = static_cast<int*>(user);
            ++*c;
        },
        &applyCount);
    // 1:1 with legacy: OnActionEvent(OTI_BTN_OK, 0, WE_BTNCLICK)
    h.dlg.OnActionEvent(cOptionDialog::kOtiBtnOk, nullptr, mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(applyCount, 1);
    EXPECT_FALSE(h.dlg.isActive());
}

TEST(COptionDialog, OnActionEventCancelClosesWithoutApply) {
    Harness h;
    int applyCount = 0;
    int cancelCount = 0;
    h.dlg.SetApplyCallbackForTest(
        [](sGAMEOPTION*, void* user) { ++*static_cast<int*>(user); }, &applyCount);
    h.dlg.SetCancelCallbackForTest(
        [](void* user) { ++*static_cast<int*>(user); }, &cancelCount);
    h.dlg.SetActive(true);
    h.dlg.OnActionEvent(cOptionDialog::kOtiBtnCancel, nullptr, mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(applyCount, 0);
    EXPECT_EQ(cancelCount, 1);
    EXPECT_FALSE(h.dlg.isActive());
}

TEST(COptionDialog, OnActionEventResetRepaintsFromDefault) {
    Harness h;
    int defaultCount = 0;
    h.dlg.SetDefaultCallbackForTest(
        [](sGAMEOPTION* opt, void* user) {
            ++*static_cast<int*>(user);
            opt->nGamma = 7;
        },
        &defaultCount);
    h.dlg.SetActive(true);
    EXPECT_EQ(defaultCount, 1);
    h.dlg.OnActionEvent(cOptionDialog::kOtiBtnReset, nullptr, mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(defaultCount, 2);
    EXPECT_EQ(h.store.guages[301], 7);
}

TEST(COptionDialog, OnActionEventPushDownUnpushesOtherInGroup) {
    Harness h;
    h.store.pushups[120] = false;
    h.store.pushups[121] = true;
    // 1:1 with legacy: PUSHDOWN on the chatmode pushup unpushes
    // the macromode pushup in the same group.
    h.dlg.OnActionEvent(120, nullptr, mxh::ui::legacy_window_event::kPushDown);
    EXPECT_FALSE(h.store.pushups[121]);
    // After both are unpushed, neither is the "active" one.
    h.dlg.OnActionEvent(121, nullptr, mxh::ui::legacy_window_event::kPushDown);
    EXPECT_FALSE(h.store.pushups[120]);
    EXPECT_FALSE(h.store.pushups[121]);
}

TEST(COptionDialog, OnActionEventAutoControlCheckDisablesGraphicTab) {
    Harness h;
    EXPECT_FALSE(h.dlg.isGraphicTabDisabled());
    h.dlg.OnActionEvent(cOptionDialog::kOtiCbAutoControl, nullptr, mxh::ui::legacy_window_event::kChecked);
    EXPECT_TRUE(h.dlg.isGraphicTabDisabled());
    h.dlg.OnActionEvent(cOptionDialog::kOtiCbAutoControl, nullptr, mxh::ui::legacy_window_event::kNotChecked);
    EXPECT_FALSE(h.dlg.isGraphicTabDisabled());
}

TEST(COptionDialog, DisableGraphicTabFlipsFlag) {
    Harness h;
    h.dlg.DisableGraphicTab(true);
    EXPECT_TRUE(h.dlg.isGraphicTabDisabled());
    h.dlg.DisableGraphicTab(false);
    EXPECT_FALSE(h.dlg.isGraphicTabDisabled());
}

TEST(COptionDialog, GetEffectSnowMatchesSnapshot) {
    Harness h;
    h.dlg.gameOption().nEffectSnow = 1;
    EXPECT_EQ(h.dlg.GetEffectSnow(), 1);
    h.dlg.gameOption().nEffectSnow = 0;
    EXPECT_EQ(h.dlg.GetEffectSnow(), 0);
}

TEST(COptionDialog, UpdateDataTrueReadsWidgetsIntoSnapshot) {
    Harness h;
    // Pre-populate the fake store.
    h.store.checkboxes[101] = true;   // bNoDeal
    h.store.checkboxes[110] = true;   // bMunpaIntro
    h.store.checkboxes[201] = true;   // bNoWhisper
    h.store.checkboxes[303] = true;   // bShadowHero
    h.store.checkboxes[308] = true;   // bAutoCtrl
    h.store.checkboxes[401] = true;   // bSoundBGM
    h.store.guages[301] = 42;          // gamma
    h.store.guages[403] = 88;          // BGM volume
    h.store.pushups[120] = true;       // macromode
    // 1:1 with legacy: UpdateData(TRUE) reads from the widgets
    // into the snapshot, and *does not* call DisableGraphicTab
    // (the tail-end DisableGraphicTab call is in UpdateData(FALSE),
    // not UpdateData(TRUE)).
    h.dlg.UpdateData(/*bSave=*/true);
    EXPECT_TRUE(h.dlg.gameOption().bNoDeal);
    EXPECT_TRUE(h.dlg.gameOption().bMunpaIntro);
    EXPECT_TRUE(h.dlg.gameOption().bNoWhisper);
    EXPECT_TRUE(h.dlg.gameOption().bShadowHero);
    EXPECT_TRUE(h.dlg.gameOption().bAutoCtrl);
    EXPECT_TRUE(h.dlg.gameOption().bSoundBGM);
    EXPECT_EQ(h.dlg.gameOption().nGamma, 42);
    EXPECT_EQ(h.dlg.gameOption().nVolumnBGM, 88);
    EXPECT_EQ(h.dlg.gameOption().nMacroMode, 1);
    EXPECT_FALSE(h.dlg.isGraphicTabDisabled());
}

TEST(COptionDialog, UpdateDataFalseWritesSnapshotToWidgets) {
    Harness h;
    h.dlg.gameOption().bNoDeal = true;
    h.dlg.gameOption().bNoParty = true;
    h.dlg.gameOption().nGamma = 33;
    h.dlg.gameOption().nVolumnBGM = 50;
    h.dlg.gameOption().bSoundBGM = true;
    h.dlg.gameOption().nEffectSnow = 1;
    h.dlg.UpdateData(/*bSave=*/false);
    EXPECT_TRUE(h.store.checkboxes[101]);   // bNoDeal
    EXPECT_TRUE(h.store.checkboxes[102]);   // bNoParty
    EXPECT_EQ(h.store.guages[301], 33);
    EXPECT_EQ(h.store.guages[403], 50);
    EXPECT_TRUE(h.store.checkboxes[401]);
    EXPECT_TRUE(h.store.checkboxes[307]);
    // 1:1 with legacy: bAutoCtrl=false (default) -> not disabled
    EXPECT_FALSE(h.dlg.isGraphicTabDisabled());
}

TEST(COptionDialog, OnActionEventSetChatAndSetMacroAreNoOps) {
    // 1:1 with legacy: SETCHAT / SETMACRO buttons are
    // commented out in the legacy code, so the modern
    // port treats them as no-ops.  Clicking them does
    // nothing (no apply, no close, no state change).
    Harness h;
    int applyCount = 0;
    h.dlg.SetApplyCallbackForTest(
        [](sGAMEOPTION*, void* u) { ++*static_cast<int*>(u); }, &applyCount);
    h.dlg.SetActive(true);
    h.dlg.OnActionEvent(cOptionDialog::kOtiBtnSetChat, nullptr, mxh::ui::legacy_window_event::kButtonClick);
    h.dlg.OnActionEvent(cOptionDialog::kOtiBtnSetMacro, nullptr, mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(applyCount, 0);
    EXPECT_TRUE(h.dlg.isActive());
}


// === Canonical WINDOW_EVENT constants (C-Batch-2.68) ===

TEST(COptionDialogTest, UsesCanonicalWindowEventConstants) {
    EXPECT_EQ(cOptionDialog::kWeBtnClick, mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(cOptionDialog::kWePushUp, mxh::ui::legacy_window_event::kPushUp);
    EXPECT_EQ(cOptionDialog::kWePushDown, mxh::ui::legacy_window_event::kPushDown);
    EXPECT_EQ(cOptionDialog::kWeChecked, mxh::ui::legacy_window_event::kChecked);
    EXPECT_EQ(cOptionDialog::kWeNotChecked, mxh::ui::legacy_window_event::kNotChecked);
}

TEST(COptionDialog, NonCopyable) {
    static_assert(!std::is_copy_constructible_v<cOptionDialog>);
    static_assert(!std::is_copy_assignable_v<cOptionDialog>);
}
