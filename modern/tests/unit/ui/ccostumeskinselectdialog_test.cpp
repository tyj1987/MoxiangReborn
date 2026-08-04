// mxh/tests/unit/ui/ccostumeskinselectdialog_test.cpp
//
// Unit tests for mxh::ui::cCostumeSkinSelectDialog (Phase C dialog port).
//
// Locks down the 1:1 surface:
//   * kSkinItemListMax == 3, kMaxItemNameBuf == 31
//   * CostumeSkinTab enum: Hat=0, Dress=1, Accessory=2, Max=3
//   * SkinSelectItemInfo field order (dwIndex, szSkinName,
//     dwLimitLevel, wEquipItem[3])
//   * Default construction zeros all state
//   * Linking pulls cListDialog + cPushupButton from test hook
//   * SetActive(true) populates the list for the current tab
//   * OnActionEvent tab btn click switches tab + repaints
//   * GetCurrentSkinInfo returns the dwSelectIdx-th entry
//   * CostumeSkinKindData is a no-op (host injects data)
//   * SetCostumTabBtnFocus flips the current tab + fires cb
//   * SetCostumeSkinDataForTest populates each tab list
//   * SetSkinDelayTime / SetSkinDelayResult store their flags

#include "mxh/ui/ccostumeskinselectdialog.hpp"
#include "../../../src/ui/legacy_window_event.hpp"
#include "mxh/ui/clistdialog.hpp"
#include "mxh/ui/cPushupButton.hpp"

#include <gtest/gtest.h>

#include <cstring>

using mxh::ui::cCostumeSkinSelectDialog;
using mxh::ui::CostumeSkinTab;
using mxh::ui::SkinSelectItemInfo;
using mxh::ui::kSkinItemListMax;
using mxh::ui::kMaxItemNameBuf;
using mxh::ui::cListDialog;
using mxh::ui::cPushupButton;

namespace test_cssd {
int g_addItemCount    = 0;
int g_removeAllCount  = 0;
int g_focusCount      = 0;
CostumeSkinTab g_lastFocused = CostumeSkinTab::Max;

void faAddItem(const SkinSelectItemInfo* /*info*/, void* user) {
    ++g_addItemCount;
    (void)user;
}
void faRemoveAll(void* user) {
    ++g_removeAllCount;
    (void)user;
}
void faFocus(CostumeSkinTab kind, void* user) {
    ++g_focusCount;
    g_lastFocused = kind;
    (void)user;
}
}

TEST(CCostumeSkinSelectDialog, ConstantsMatchLegacy) {
    EXPECT_EQ(kSkinItemListMax, 3);
    EXPECT_EQ(kMaxItemNameBuf, 31);
}

TEST(CCostumeSkinSelectDialog, CostumeSkinTabEnumIsStable) {
    EXPECT_EQ(static_cast<int>(CostumeSkinTab::Hat),       0);
    EXPECT_EQ(static_cast<int>(CostumeSkinTab::Dress),     1);
    EXPECT_EQ(static_cast<int>(CostumeSkinTab::Accessory), 2);
    EXPECT_EQ(static_cast<int>(CostumeSkinTab::Max),       3);
}

TEST(CCostumeSkinSelectDialog, SkinSelectItemInfoFieldOrder) {
    // The struct must hold 4 fields: dwIndex, szSkinName[31],
    // dwLimitLevel, wEquipItem[3].  Verify by assignment.
    SkinSelectItemInfo info{};
    info.dwIndex = 7;
    std::strncpy(info.szSkinName, "Test", sizeof(info.szSkinName) - 1);
    info.dwLimitLevel = 50;
    info.wEquipItem[0] = 100;
    info.wEquipItem[1] = 200;
    info.wEquipItem[2] = 300;
    EXPECT_EQ(info.dwIndex, 7u);
    EXPECT_STREQ(info.szSkinName, "Test");
    EXPECT_EQ(info.dwLimitLevel, 50u);
    EXPECT_EQ(info.wEquipItem[0], 100);
    EXPECT_EQ(info.wEquipItem[1], 200);
    EXPECT_EQ(info.wEquipItem[2], 300);
}

TEST(CCostumeSkinSelectDialog, DefaultConstructionIsReset) {
    cCostumeSkinSelectDialog d;
    EXPECT_EQ(d.selectIdx(), 0u);
    EXPECT_EQ(d.hatCount(), 0);
    EXPECT_EQ(d.dressCount(), 0);
    EXPECT_EQ(d.accessoryCount(), 0);
    EXPECT_EQ(d.currentTab(), CostumeSkinTab::Hat);
    EXPECT_EQ(d.skinDelayTime(), 0u);
    EXPECT_FALSE(d.skinDelayResult());
}

TEST(CCostumeSkinSelectDialog, LinkingWiresChildren) {
    cCostumeSkinSelectDialog d;
    cListDialog listDlg;
    cPushupButton tabBtns[3];
    cCostumeSkinSelectDialog::ChildWindows w{};
    w.listDlg = &listDlg;
    for (int i = 0; i < 3; ++i) w.tabBtns[i] = &tabBtns[i];
    d.SetChildWindowsForTest(w);
    d.Linking();
    SUCCEED();
}

TEST(CCostumeSkinSelectDialog, SetCostumeSkinDataPopulatesEachTab) {
    cCostumeSkinSelectDialog d;
    SkinSelectItemInfo hat{};
    hat.dwIndex = 1;
    std::strncpy(hat.szSkinName, "Hat1", sizeof(hat.szSkinName) - 1);
    SkinSelectItemInfo dress{};
    dress.dwIndex = 2;
    std::strncpy(dress.szSkinName, "Dress1", sizeof(dress.szSkinName) - 1);
    SkinSelectItemInfo acc{};
    acc.dwIndex = 3;
    std::strncpy(acc.szSkinName, "Acc1", sizeof(acc.szSkinName) - 1);
    d.SetCostumeSkinDataForTest(CostumeSkinTab::Hat,       {hat});
    d.SetCostumeSkinDataForTest(CostumeSkinTab::Dress,     {dress});
    d.SetCostumeSkinDataForTest(CostumeSkinTab::Accessory, {acc});
    EXPECT_EQ(d.hatCount(), 1);
    EXPECT_EQ(d.dressCount(), 1);
    EXPECT_EQ(d.accessoryCount(), 1);
    EXPECT_STREQ(d.skinListForTest(CostumeSkinTab::Hat).at(0).szSkinName, "Hat1");
    EXPECT_STREQ(d.skinListForTest(CostumeSkinTab::Dress).at(0).szSkinName, "Dress1");
    EXPECT_STREQ(d.skinListForTest(CostumeSkinTab::Accessory).at(0).szSkinName, "Acc1");
}

TEST(CCostumeSkinSelectDialog, CostumeSkinKindDataIsNoOp) {
    cCostumeSkinSelectDialog d;
    d.CostumeSkinKindData();
    SUCCEED();
}

TEST(CCostumeSkinSelectDialog, SetActiveTruePopulatesListForCurrentTab) {
    test_cssd::g_addItemCount = 0;
    test_cssd::g_removeAllCount = 0;
    cCostumeSkinSelectDialog d;
    SkinSelectItemInfo hat{};
    hat.dwIndex = 1;
    std::strncpy(hat.szSkinName, "Hat1", sizeof(hat.szSkinName) - 1);
    d.SetCostumeSkinDataForTest(CostumeSkinTab::Hat, {hat});
    d.SetListAddItemCallbackForTest(&test_cssd::faAddItem, nullptr);
    d.SetListRemoveAllCallbackForTest(&test_cssd::faRemoveAll, nullptr);
    d.SetActive(true);
    EXPECT_EQ(test_cssd::g_addItemCount, 1);
    EXPECT_EQ(test_cssd::g_removeAllCount, 1);
    EXPECT_TRUE(d.isActive());
}

TEST(CCostumeSkinSelectDialog, OnActionEventTabBtnClickSwitchesTab) {
    test_cssd::g_focusCount = 0;
    test_cssd::g_lastFocused = CostumeSkinTab::Max;
    cCostumeSkinSelectDialog d;
    d.SetTabBtnFocusCallbackForTest(&test_cssd::faFocus, nullptr);
    d.OnActionEvent(cCostumeSkinSelectDialog::kTabBtnDressId, nullptr, mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(d.currentTab(), CostumeSkinTab::Dress);
    EXPECT_EQ(test_cssd::g_focusCount, 1);
    EXPECT_EQ(test_cssd::g_lastFocused, CostumeSkinTab::Dress);
    d.OnActionEvent(cCostumeSkinSelectDialog::kTabBtnAccessoryId, nullptr, mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(d.currentTab(), CostumeSkinTab::Accessory);
    EXPECT_EQ(test_cssd::g_lastFocused, CostumeSkinTab::Accessory);
    d.OnActionEvent(cCostumeSkinSelectDialog::kTabBtnHatId, nullptr, mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_EQ(d.currentTab(), CostumeSkinTab::Hat);
    EXPECT_EQ(test_cssd::g_lastFocused, CostumeSkinTab::Hat);
}

TEST(CCostumeSkinSelectDialog, OnActionEventIgnoresUnknownId) {
    cCostumeSkinSelectDialog d;
    EXPECT_FALSE(d.OnActionEvent(9999, nullptr, 0x0001));
    EXPECT_EQ(d.currentTab(), CostumeSkinTab::Hat);
}

TEST(CCostumeSkinSelectDialog, OnActionEventIgnoresNonClickEvents) {
    cCostumeSkinSelectDialog d;
    EXPECT_FALSE(d.OnActionEvent(cCostumeSkinSelectDialog::kTabBtnDressId, nullptr, 0x0000));
    EXPECT_EQ(d.currentTab(), CostumeSkinTab::Hat);
}

TEST(CCostumeSkinSelectDialog, GetCurrentSkinInfoReturnsEntry) {
    cCostumeSkinSelectDialog d;
    SkinSelectItemInfo hat{};
    hat.dwIndex = 42;
    std::strncpy(hat.szSkinName, "Special", sizeof(hat.szSkinName) - 1);
    d.SetCostumeSkinDataForTest(CostumeSkinTab::Hat, {hat});
    const SkinSelectItemInfo* info = d.GetCurrentSkinInfo(0);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->dwIndex, 42u);
    EXPECT_STREQ(info->szSkinName, "Special");
}

TEST(CCostumeSkinSelectDialog, GetCurrentSkinInfoOutOfRangeIsNull) {
    cCostumeSkinSelectDialog d;
    EXPECT_EQ(d.GetCurrentSkinInfo(0), nullptr);
    SkinSelectItemInfo hat{};
    hat.dwIndex = 1;
    d.SetCostumeSkinDataForTest(CostumeSkinTab::Hat, {hat});
    EXPECT_EQ(d.GetCurrentSkinInfo(99), nullptr);
}

TEST(CCostumeSkinSelectDialog, SetCostumTabBtnFocusFlipsCurrentTab) {
    test_cssd::g_focusCount = 0;
    cCostumeSkinSelectDialog d;
    d.SetTabBtnFocusCallbackForTest(&test_cssd::faFocus, nullptr);
    d.SetCostumTabBtnFocus(CostumeSkinTab::Accessory);
    EXPECT_EQ(d.currentTab(), CostumeSkinTab::Accessory);
    EXPECT_EQ(test_cssd::g_focusCount, 1);
}

TEST(CCostumeSkinSelectDialog, SetSkinDelayTimeStoresValue) {
    cCostumeSkinSelectDialog d;
    d.SetSkinDelayTime(1000);
    EXPECT_EQ(d.skinDelayTime(), 1000u);
}

TEST(CCostumeSkinSelectDialog, SetSkinDelayResultStoresValue) {
    cCostumeSkinSelectDialog d;
    d.SetSkinDelayResult(true);
    EXPECT_TRUE(d.skinDelayResult());
    d.SetSkinDelayResult(false);
    EXPECT_FALSE(d.skinDelayResult());
}

TEST(CCostumeSkinSelectDialog, ActionEventIsNoOpStub) {
    cCostumeSkinSelectDialog d;
    EXPECT_EQ(d.ActionEvent(nullptr), 0u);
}

TEST(CCostumeSkinSelectDialog, DestructorClearsLists) {
    cCostumeSkinSelectDialog* d = new cCostumeSkinSelectDialog();
    SkinSelectItemInfo hat{};
    hat.dwIndex = 1;
    d->SetCostumeSkinDataForTest(CostumeSkinTab::Hat, {hat});
    delete d;
    SUCCEED();
}

TEST(CCostumeSkinSelectDialog, NonCopyable) {
    static_assert(!std::is_copy_constructible_v<cCostumeSkinSelectDialog>);
    static_assert(!std::is_copy_assignable_v<cCostumeSkinSelectDialog>);
}
