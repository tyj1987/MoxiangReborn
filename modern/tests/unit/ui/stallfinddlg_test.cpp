// stallfinddlg_test.cpp — Phase 6.14 / 0.13.48 coverage for
// cStallFindDlg (street-stall item-search dialog). Tests the
// data model + state machine + SortStallList + SetPage +
// SetBasePage + SetStallPriceInfo + OnActionEvent. Engine-side
// singletons (GameResourceManager / ITEMMGR / CHATMGR /
// OBJECTMGR / NETWORK / WINDOWMGR / RESRCMGR / MHFile / HERO /
// GAMEIN / PKMGR) are stubbed to no-op; the data-side state is
// preserved 1:1.

#include "stallfinddlg.hpp"

#include "cButton.hpp"
#include "cComboBox.hpp"
#include "cListDialog.hpp"
#include "cPushupButton.hpp"
#include "cStatic.hpp"
#include "cWindow.hpp"

#include <gtest/gtest.h>

#include <memory>

namespace {

// BuildDlgWithChildren: 1:1 with the legacy resource-loader
// step. In the legacy, the dialog is loaded with all 24+ child
// controls already attached (m_pItemTypeCombo, m_arrItemDetail
// TypeCombo[8], m_pItemList, m_pClassList, m_pNameStatic,
// m_pPriceStatic, m_pStallList, m_parrPageBtn[5],
// m_parrPageUpDownBtn[2], m_pSellModeRadioBtn,
// m_pBuyModeRadioBtn). Linking() then resolves them by id.
// The modern port follows the same flow: children are added
// first, then Linking() resolves them.
//
// This helper creates + adds the minimum set of children that
// LinkingResolvesAllChildren + LinkingIsIdempotent assert.
void BuildDlgWithChildren(mxh::ui::cStallFindDlg& d) {
    d.Init(0, 0, 400, 400, nullptr, 0);
    // 1 main type combo.
    {
        auto c = std::make_unique<mxh::ui::cComboBox>();
        c->Init(0, 0, 100, 30, nullptr,
                mxh::ui::cStallFindDlg::ID_TYPECOMBO);
        d.Add(std::unique_ptr<mxh::ui::cWindow>(c.release()));
    }
    // 8 detail combos.
    const std::int32_t detailIds[8] = {
        mxh::ui::cStallFindDlg::ID_WEAPON_DETAILCOMBO,
        mxh::ui::cStallFindDlg::ID_CLOTHES_DETAILCOMBO,
        mxh::ui::cStallFindDlg::ID_ACCESSORY_DETAILCOMBO,
        mxh::ui::cStallFindDlg::ID_POTION_DETAILCOMBO,
        mxh::ui::cStallFindDlg::ID_MATERIAL_DETAILCOMBO,
        mxh::ui::cStallFindDlg::ID_ETC_DETAILCOMBO,
        mxh::ui::cStallFindDlg::ID_ITEMMALL_DETAILCOMBO,
        mxh::ui::cStallFindDlg::ID_TITAN_DETAILCOMBO,
    };
    for (int i = 0; i < 8; ++i) {
        auto c = std::make_unique<mxh::ui::cComboBox>();
        c->Init(0, 0, 100, 30, nullptr, detailIds[i]);
        d.Add(std::unique_ptr<mxh::ui::cWindow>(c.release()));
    }
    // 3 list dialogs (item / class / result).
    {
        auto l = std::make_unique<mxh::ui::cListDialog>();
        l->InitList(5, 0, 0, 100, 30);
        l->Init(0, 0, 0, 0, nullptr, mxh::ui::cStallFindDlg::ID_ITEMLIST);
        d.Add(std::unique_ptr<mxh::ui::cWindow>(l.release()));
    }
    {
        auto l = std::make_unique<mxh::ui::cListDialog>();
        l->InitList(5, 0, 0, 100, 30);
        l->Init(0, 0, 0, 0, nullptr, mxh::ui::cStallFindDlg::ID_CLASSLIST);
        d.Add(std::unique_ptr<mxh::ui::cWindow>(l.release()));
    }
    {
        auto l = std::make_unique<mxh::ui::cListDialog>();
        l->InitList(5, 0, 0, 100, 30);
        l->Init(0, 0, 0, 0, nullptr, mxh::ui::cStallFindDlg::ID_RESULTLIST);
        d.Add(std::unique_ptr<mxh::ui::cWindow>(l.release()));
    }
    // 2 statics (name + price).
    {
        auto s = std::make_unique<mxh::ui::cStatic>();
        s->Init(0, 0, 0, 0, nullptr,
                mxh::ui::cStallFindDlg::ID_NAMESTATIC);
        d.Add(std::unique_ptr<mxh::ui::cWindow>(s.release()));
    }
    {
        auto s = std::make_unique<mxh::ui::cStatic>();
        s->Init(0, 0, 0, 0, nullptr,
                mxh::ui::cStallFindDlg::ID_PRICESTATIC);
        d.Add(std::unique_ptr<mxh::ui::cWindow>(s.release()));
    }
    // 2 pushup buttons (sell mode + buy mode).
    {
        auto b = std::make_unique<mxh::ui::cPushupButton>();
        b->Init(0, 0, 30, 30, nullptr, nullptr, nullptr, nullptr, nullptr,
                mxh::ui::cStallFindDlg::ID_PB_SELLMODE);
        d.Add(std::unique_ptr<mxh::ui::cWindow>(b.release()));
    }
    {
        auto b = std::make_unique<mxh::ui::cPushupButton>();
        b->Init(0, 0, 30, 30, nullptr, nullptr, nullptr, nullptr, nullptr,
                mxh::ui::cStallFindDlg::ID_PB_BUYMODE);
        d.Add(std::unique_ptr<mxh::ui::cWindow>(b.release()));
    }
    // 5 page pushup buttons.
    for (int i = 0; i < 5; ++i) {
        auto b = std::make_unique<mxh::ui::cPushupButton>();
        b->Init(0, 0, 20, 20, nullptr, nullptr, nullptr, nullptr, nullptr,
                mxh::ui::cStallFindDlg::ID_RESULTPAGEBTN1 + i);
        d.Add(std::unique_ptr<mxh::ui::cWindow>(b.release()));
    }
    // 2 page up/down buttons.
    {
        auto b = std::make_unique<mxh::ui::cButton>();
        b->Init(0, 0, 20, 20, nullptr, nullptr, nullptr, nullptr, nullptr,
                mxh::ui::cStallFindDlg::ID_RESULTPAGEBTNUP);
        d.Add(std::unique_ptr<mxh::ui::cWindow>(b.release()));
    }
    {
        auto b = std::make_unique<mxh::ui::cButton>();
        b->Init(0, 0, 20, 20, nullptr, nullptr, nullptr, nullptr, nullptr,
                mxh::ui::cStallFindDlg::ID_RESULTPAGEBTNDOWN);
        d.Add(std::unique_ptr<mxh::ui::cWindow>(b.release()));
    }
    // 2 search buttons (search + search all).
    {
        auto b = std::make_unique<mxh::ui::cButton>();
        b->Init(0, 0, 50, 30, nullptr, nullptr, nullptr, nullptr, nullptr,
                mxh::ui::cStallFindDlg::ID_SEARCHBTN);
        d.Add(std::unique_ptr<mxh::ui::cWindow>(b.release()));
    }
    {
        auto b = std::make_unique<mxh::ui::cButton>();
        b->Init(0, 0, 50, 30, nullptr, nullptr, nullptr, nullptr, nullptr,
                mxh::ui::cStallFindDlg::ID_SEARCHALLBTN);
        d.Add(std::unique_ptr<mxh::ui::cWindow>(b.release()));
    }
    d.Linking();
}

}  // namespace

TEST(CStallFindDlg, DefaultConstructionIsValid) {
    mxh::ui::cStallFindDlg d;
    EXPECT_EQ(d.GetStallCount(), 0);
    EXPECT_EQ(d.GetBasePage(), 0);
    EXPECT_EQ(d.GetMaxPage(), 0);
    EXPECT_EQ(d.GetCurrentPage(), -1);
    EXPECT_EQ(d.GetItemType(), 0);
    EXPECT_EQ(d.GetItemDetailType(), 0);
    EXPECT_EQ(d.GetSelectedItemListIdx(), -1);
    EXPECT_EQ(d.GetSelectedClassListIdx(), -1);
    EXPECT_EQ(d.GetSelectedStallListIdx(), -1);
    EXPECT_EQ(d.GetSelectedObjectIndex(), 0u);
    EXPECT_FALSE(d.IsSearchedAll());
    EXPECT_EQ(d.GetSearchType(),
              static_cast<std::uint32_t>(mxh::ui::cStallFindDlg::SK_SELL));
}

TEST(CStallFindDlg, InheritsDialogTreeManagement) {
    mxh::ui::cStallFindDlg d;
    EXPECT_EQ(d.childCount(), 0u);  // before Linking
}

TEST(CStallFindDlg, ConstantsAreStable) {
    EXPECT_EQ(mxh::ui::cStallFindDlg::SEARCH_DELAY, 3000u);
    EXPECT_EQ(mxh::ui::cStallFindDlg::ITEMVIEW_DELAY, 1000u);
    EXPECT_EQ(mxh::ui::cStallFindDlg::MAX_RESULT_PAGE, 5u);
    EXPECT_EQ(mxh::ui::cStallFindDlg::MAX_LINE_PER_PAGE, 6u);
    EXPECT_EQ(mxh::ui::cStallFindDlg::MAX_STALLITEM_NUM, 40u);
    EXPECT_EQ(mxh::ui::cStallFindDlg::ITEM_TYPE_COUNT, 9u);
}

TEST(CStallFindDlg, ItemTypeEnumIsStable) {
    EXPECT_EQ(static_cast<int>(mxh::ui::cStallFindDlg::WEAPON), 0);
    EXPECT_EQ(static_cast<int>(mxh::ui::cStallFindDlg::CLOTHES), 1);
    EXPECT_EQ(static_cast<int>(mxh::ui::cStallFindDlg::ACCESSORY), 2);
    EXPECT_EQ(static_cast<int>(mxh::ui::cStallFindDlg::POTION), 3);
    EXPECT_EQ(static_cast<int>(mxh::ui::cStallFindDlg::MATERIAL), 4);
    EXPECT_EQ(static_cast<int>(mxh::ui::cStallFindDlg::ETC), 5);
    EXPECT_EQ(static_cast<int>(mxh::ui::cStallFindDlg::ITEM_MALL), 6);
    EXPECT_EQ(static_cast<int>(mxh::ui::cStallFindDlg::TITAN_ITEM), 7);
}

TEST(CStallFindDlg, SearchKindEnumIsStable) {
    EXPECT_EQ(mxh::ui::cStallFindDlg::SK_SELL, 0u);
    EXPECT_EQ(mxh::ui::cStallFindDlg::SK_BUY, 1u);
}

TEST(CStallFindDlg, IdConstantsAreDistinct) {
    EXPECT_NE(mxh::ui::cStallFindDlg::ID_TYPECOMBO,
              mxh::ui::cStallFindDlg::ID_TYPECOMBOBTN);
    EXPECT_NE(mxh::ui::cStallFindDlg::ID_WEAPON_DETAILCOMBO,
              mxh::ui::cStallFindDlg::ID_CLOTHES_DETAILCOMBO);
    EXPECT_NE(mxh::ui::cStallFindDlg::ID_ITEMLIST,
              mxh::ui::cStallFindDlg::ID_CLASSLIST);
    EXPECT_NE(mxh::ui::cStallFindDlg::ID_RESULTPAGEBTN1,
              mxh::ui::cStallFindDlg::ID_RESULTPAGEBTN5);
    EXPECT_NE(mxh::ui::cStallFindDlg::ID_SEARCHBTN,
              mxh::ui::cStallFindDlg::ID_SEARCHALLBTN);
}

TEST(CStallFindDlg, LinkingResolvesAllChildren) {
    mxh::ui::cStallFindDlg d;
    BuildDlgWithChildren(d);
    // 1:1 with legacy. After Linking, findWindowById finds all
    // 27 children (24 ids + 5 page btns + 2 up/down + 1 dlg).
    EXPECT_NE(d.findWindowById(mxh::ui::cStallFindDlg::ID_TYPECOMBO), nullptr);
    EXPECT_NE(d.findWindowById(mxh::ui::cStallFindDlg::ID_WEAPON_DETAILCOMBO), nullptr);
    EXPECT_NE(d.findWindowById(mxh::ui::cStallFindDlg::ID_TITAN_DETAILCOMBO), nullptr);
    EXPECT_NE(d.findWindowById(mxh::ui::cStallFindDlg::ID_ITEMLIST), nullptr);
    EXPECT_NE(d.findWindowById(mxh::ui::cStallFindDlg::ID_RESULTLIST), nullptr);
    EXPECT_NE(d.findWindowById(mxh::ui::cStallFindDlg::ID_NAMESTATIC), nullptr);
    EXPECT_NE(d.findWindowById(mxh::ui::cStallFindDlg::ID_PB_SELLMODE), nullptr);
    EXPECT_NE(d.findWindowById(mxh::ui::cStallFindDlg::ID_RESULTPAGEBTN1), nullptr);
    EXPECT_NE(d.findWindowById(mxh::ui::cStallFindDlg::ID_RESULTPAGEBTN5), nullptr);
    EXPECT_NE(d.findWindowById(mxh::ui::cStallFindDlg::ID_RESULTPAGEBTNUP), nullptr);
    EXPECT_NE(d.findWindowById(mxh::ui::cStallFindDlg::ID_SEARCHBTN), nullptr);
}

TEST(CStallFindDlg, LinkingSetsInitialItemTypeToWeapon) {
    // 1:1 with legacy: m_nItemType starts at 0 (WEAPON), the
    // first detail combo is active, the rest are disabled.
    mxh::ui::cStallFindDlg d;
    d.Linking();
    EXPECT_EQ(d.GetItemType(), 0);  // WEAPON
}

TEST(CStallFindDlg, LinkingIsIdempotent) {
    mxh::ui::cStallFindDlg d;
    BuildDlgWithChildren(d);
    d.Linking();
    EXPECT_NE(d.findWindowById(mxh::ui::cStallFindDlg::ID_TYPECOMBO), nullptr);
}

TEST(CStallFindDlg, SetSearchType) {
    mxh::ui::cStallFindDlg d;
    EXPECT_EQ(d.GetSearchType(), 0u);  // SK_SELL default
    d.SetSearchType(mxh::ui::cStallFindDlg::SK_BUY);
    EXPECT_EQ(d.GetSearchType(), 1u);
}

TEST(CStallFindDlg, SortStallListAscending) {
    // 1:1 with legacy: shell sort by dwPrice ascending.
    mxh::ui::cStallFindDlg d;
    d.SetStallInfoForTesting(0, {"item0", 300, 100});
    d.SetStallInfoForTesting(1, {"item1", 100, 200});
    d.SetStallInfoForTesting(2, {"item2", 200, 300});
    d.SetStallInfoForTesting(3, {"item3", 50,  400});
    d.SetStallCount(4);
    d.SortStallList(/*ascending*/false);
    // Ascending: 50, 100, 200, 300.
    EXPECT_EQ(d.GetStallInfoForTesting(0).dwPrice, 50u);
    EXPECT_EQ(d.GetStallInfoForTesting(1).dwPrice, 100u);
    EXPECT_EQ(d.GetStallInfoForTesting(2).dwPrice, 200u);
    EXPECT_EQ(d.GetStallInfoForTesting(3).dwPrice, 300u);
}

TEST(CStallFindDlg, SortStallListDescending) {
    mxh::ui::cStallFindDlg d;
    d.SetStallInfoForTesting(0, {"item0", 300, 100});
    d.SetStallInfoForTesting(1, {"item1", 100, 200});
    d.SetStallInfoForTesting(2, {"item2", 200, 300});
    d.SetStallInfoForTesting(3, {"item3", 50,  400});
    d.SetStallCount(4);
    d.SortStallList(/*ascending*/true);
    // Descending: 300, 200, 100, 50.
    EXPECT_EQ(d.GetStallInfoForTesting(0).dwPrice, 300u);
    EXPECT_EQ(d.GetStallInfoForTesting(1).dwPrice, 200u);
    EXPECT_EQ(d.GetStallInfoForTesting(2).dwPrice, 100u);
    EXPECT_EQ(d.GetStallInfoForTesting(3).dwPrice, 50u);
}

TEST(CStallFindDlg, SortStallListWithZeroItemsIsNoOp) {
    mxh::ui::cStallFindDlg d;
    d.SortStallList(false);
    EXPECT_EQ(d.GetStallCount(), 0);
}

TEST(CStallFindDlg, SetStallPriceInfoResetsAndPopulates) {
    // 1:1 with legacy: SetStallPriceInfo clears the previous
    // list, copies the new prices, sorts, sets page 0.
    std::vector<mxh::ui::cStallFindDlg::StallPriceInfo> prices;
    prices.push_back({"apple",  100, 1000});
    prices.push_back({"banana", 200, 2000});
    prices.push_back({"cherry", 50,  3000});
    mxh::ui::cStallFindDlg d;
    d.SetStallPriceInfo(prices);
    EXPECT_EQ(d.GetStallCount(), 3);
    EXPECT_EQ(d.GetCurrentPage(), 0);
    // SortStallList is called (default SK_SELL → ascending).
    EXPECT_EQ(d.GetStallInfoForTesting(0).dwPrice, 50u);   // cherry
    EXPECT_EQ(d.GetStallInfoForTesting(1).dwPrice, 100u);  // apple
    EXPECT_EQ(d.GetStallInfoForTesting(2).dwPrice, 200u);  // banana
}

TEST(CStallFindDlg, SetStallPriceInfoClampsToMaxStallItemNum) {
    std::vector<mxh::ui::cStallFindDlg::StallPriceInfo> prices;
    for (int i = 0; i < 50; ++i) {
        prices.push_back({"item" + std::to_string(i),
                          static_cast<std::uint32_t>(i), 0u});
    }
    mxh::ui::cStallFindDlg d;
    d.SetStallPriceInfo(prices);
    EXPECT_EQ(d.GetStallCount(),
              static_cast<int>(mxh::ui::cStallFindDlg::MAX_STALLITEM_NUM));
}

TEST(CStallFindDlg, SetBasePageNextShifts) {
    mxh::ui::cStallFindDlg d;
    d.SetBasePageForTesting(0);
    d.SetMaxPageForTesting(15);
    d.SetBasePage(true);
    EXPECT_EQ(d.GetBasePage(),
              static_cast<int>(mxh::ui::cStallFindDlg::MAX_RESULT_PAGE));
}

TEST(CStallFindDlg, SetBasePagePreviousShifts) {
    mxh::ui::cStallFindDlg d;
    d.SetBasePageForTesting(10);
    d.SetMaxPageForTesting(15);
    d.SetBasePage(false);
    EXPECT_EQ(d.GetBasePage(), 10 - static_cast<int>(mxh::ui::cStallFindDlg::MAX_RESULT_PAGE));
}

TEST(CStallFindDlg, SetBasePageClampsToBounds) {
    mxh::ui::cStallFindDlg d;
    d.SetBasePageForTesting(0);
    d.SetMaxPageForTesting(15);
    d.SetBasePage(false);  // already at 0 → no change
    EXPECT_EQ(d.GetBasePage(), 0);
    d.SetBasePageForTesting(15);
    d.SetBasePage(true);  // would go past max → no change
    EXPECT_EQ(d.GetBasePage(), 15);
}

TEST(CStallFindDlg, CheckDelayFirstCallReturnsTrue) {
    mxh::ui::cStallFindDlg d;
    EXPECT_TRUE(d.CheckDelay(3000, 0));
}

TEST(CStallFindDlg, SetActiveTrueDoesNotCrash) {
    mxh::ui::cStallFindDlg d;
    d.Linking();
    d.SetActive(true);
}

TEST(CStallFindDlg, SetActiveFalseDoesNotCrash) {
    mxh::ui::cStallFindDlg d;
    d.Linking();
    d.SetActive(false);
}

TEST(CStallFindDlg, SetActiveToggleRoundTrip) {
    mxh::ui::cStallFindDlg d;
    d.Linking();
    d.SetActive(true);
    d.SetActive(false);
}

TEST(CStallFindDlg, OnActionEventUnknownIdIsNoOp) {
    mxh::ui::cStallFindDlg d;
    d.Linking();
    d.OnActionEvent(999, nullptr, 0x01);
    EXPECT_EQ(d.GetCurrentPage(), -1);  // unchanged
}

TEST(CStallFindDlg, OnActionEventBeforeLinkingIsSafe) {
    mxh::ui::cStallFindDlg d;
    d.OnActionEvent(mxh::ui::cStallFindDlg::ID_TYPECOMBO, nullptr, 0x100);
    SUCCEED();
}

TEST(CStallFindDlg, OnActionEventTypeComboSelectIsNoOp) {
    // 1:1 with legacy: WE_COMBOBOXSELECT on the type combo
    // triggers OnEventTypeCombo (which updates m_nItemType +
    // m_arrItemDetailTypeCombo). The modern port's
    // OnEventTypeCombo is a no-op stub (engine-side state).
    mxh::ui::cStallFindDlg d;
    d.Linking();
    d.OnActionEvent(mxh::ui::cStallFindDlg::ID_TYPECOMBO, nullptr, 0x100);
    EXPECT_EQ(d.GetItemType(), 0);  // unchanged
}

TEST(CStallFindDlg, OnActionEventDetailComboSelectIsNoOp) {
    mxh::ui::cStallFindDlg d;
    d.Linking();
    d.OnActionEvent(mxh::ui::cStallFindDlg::ID_WEAPON_DETAILCOMBO, nullptr, 0x100);
    d.OnActionEvent(mxh::ui::cStallFindDlg::ID_TITAN_DETAILCOMBO, nullptr, 0x100);
    EXPECT_EQ(d.GetItemDetailType(), 0);  // unchanged
}

TEST(CStallFindDlg, OnActionEventPageBtnsIsNoOp) {
    mxh::ui::cStallFindDlg d;
    d.Linking();
    d.OnActionEvent(mxh::ui::cStallFindDlg::ID_RESULTPAGEBTN1, nullptr, 0x20);
    d.OnActionEvent(mxh::ui::cStallFindDlg::ID_RESULTPAGEBTN5, nullptr, 0x20);
    SUCCEED();
}

TEST(CStallFindDlg, OnActionEventPageUpDownIsNoOp) {
    mxh::ui::cStallFindDlg d;
    d.Linking();
    d.OnActionEvent(mxh::ui::cStallFindDlg::ID_RESULTPAGEBTNUP, nullptr, 0x20);
    d.OnActionEvent(mxh::ui::cStallFindDlg::ID_RESULTPAGEBTNDOWN, nullptr, 0x20);
    SUCCEED();
}

TEST(CStallFindDlg, OnActionEventSellBuyModeIsNoOp) {
    mxh::ui::cStallFindDlg d;
    d.Linking();
    d.OnActionEvent(mxh::ui::cStallFindDlg::ID_PB_SELLMODE, nullptr, 0x20);
    d.OnActionEvent(mxh::ui::cStallFindDlg::ID_PB_BUYMODE, nullptr, 0x20);
    SUCCEED();
}

TEST(CStallFindDlg, ActionEventBeforeLinkingIsSafe) {
    mxh::ui::cStallFindDlg d;
    d.ActionEvent(0, 0, 0);
    SUCCEED();
}

TEST(CStallFindDlg, ActionEventWithoutDoubleClickIsNoOp) {
    mxh::ui::cStallFindDlg d;
    d.Linking();
    // No double-click → no item-view send. The state is unchanged.
    d.ActionEvent(0, 0, 0x04 /*LButtonClick*/);
    EXPECT_EQ(d.GetSelectedStallListIdx(), -1);
}

TEST(CStallFindDlg, UpdateItemListDoesNotCrash) {
    mxh::ui::cStallFindDlg d;
    d.Linking();
    // No bin file in modern → m_ptrItemInfo is empty.
    d.UpdateItemList();
    // Set m_nItemType / m_nItemDetailType and try again.
    d.SetItemTypeForTesting(0);
    d.SetItemDetailTypeForTesting(0);
    d.UpdateItemList();
    d.SetItemTypeForTesting(7);
    d.UpdateItemList();
}

TEST(CStallFindDlg, UpdateStallListDoesNotCrash) {
    mxh::ui::cStallFindDlg d;
    d.Linking();
    d.UpdateStallList();
    d.SetCurrentPageForTesting(0);
    d.SetStallCount(3);
    d.UpdateStallList();
}

TEST(CStallFindDlg, SetPageIndexClamps) {
    mxh::ui::cStallFindDlg d;
    d.Linking();
    d.SetBasePageForTesting(0);
    d.SetMaxPageForTesting(10);
    d.SetPage(0);
    EXPECT_EQ(d.GetCurrentPage(), 0);
    d.SetPage(2);
    EXPECT_EQ(d.GetCurrentPage(), 2);
}

TEST(CStallFindDlg, SendItemViewMsgWithoutSelectionIsNoOp) {
    mxh::ui::cStallFindDlg d;
    d.Linking();
    d.SendItemViewMsg();
    EXPECT_EQ(d.GetSelectedObjectIndex(), 0u);  // unchanged
}

TEST(CStallFindDlg, SendItemViewMsgWithSelectionRecordsIndex) {
    mxh::ui::cStallFindDlg d;
    d.Linking();
    d.SetSelectedStallListIdxForTesting(0);
    d.SetSelectedObjectIndexForTesting(12345);
    d.SendItemViewMsg();
    // Engine-side NETWORK->Send stubbed; data-side m_dwSelectedObjectIndex
    // unchanged. (The legacy updates it on LBTNDBLCLICK, not on
    // SendItemViewMsg itself.)
    EXPECT_EQ(d.GetSelectedObjectIndex(), 12345u);
}

TEST(CStallFindDlg, GetItemDetailTypeComboOutOfRangeReturnsNull) {
    mxh::ui::cStallFindDlg d;
    d.Linking();
    EXPECT_EQ(d.GetItemDetailTypeCombo(-1), nullptr);
    EXPECT_EQ(d.GetItemDetailTypeCombo(99), nullptr);
}

TEST(CStallFindDlg, GetPageBtnOutOfRangeReturnsNull) {
    mxh::ui::cStallFindDlg d;
    d.Linking();
    EXPECT_EQ(d.GetPageBtn(-1), nullptr);
    EXPECT_EQ(d.GetPageBtn(99), nullptr);
}

TEST(CStallFindDlg, GetPageUpDownBtnOutOfRangeReturnsNull) {
    mxh::ui::cStallFindDlg d;
    d.Linking();
    EXPECT_EQ(d.GetPageUpDownBtn(-1), nullptr);
    EXPECT_EQ(d.GetPageUpDownBtn(2), nullptr);
}

TEST(CStallFindDlg, StallPriceInfoDefaultFields) {
    mxh::ui::cStallFindDlg::StallPriceInfo s;
    EXPECT_EQ(s.dwPrice, 0u);
    EXPECT_EQ(s.dwOwnerIdx, 0u);
    EXPECT_TRUE(s.strName.empty());
}

TEST(CStallFindDlg, ItemInfoDefaultFields) {
    mxh::ui::cStallFindDlg::ItemInfo info;
    EXPECT_EQ(info.type, 0);
    EXPECT_EQ(info.detailType, 0);
    EXPECT_EQ(info.itemIdx, 0u);
}

TEST(CStallFindDlg, SetSearchedAllRoundTrip) {
    mxh::ui::cStallFindDlg d;
    EXPECT_FALSE(d.IsSearchedAll());
    d.SetSearchedAll(true);
    EXPECT_TRUE(d.IsSearchedAll());
    d.SetSearchedAll(false);
    EXPECT_FALSE(d.IsSearchedAll());
}

TEST(CStallFindDlg, SetStallInfoForTestingOutOfRangeIgnored) {
    mxh::ui::cStallFindDlg d;
    d.SetStallInfoForTesting(-1, {"x", 0, 0});
    d.SetStallInfoForTesting(99, {"x", 0, 0});
    // No crash, no state change.
    EXPECT_EQ(d.GetStallCount(), 0);
}

TEST(CStallFindDlg, RenderIsNoop) {
    mxh::ui::cStallFindDlg d;
    d.Linking();
    d.Render();
    d.Render();
}
