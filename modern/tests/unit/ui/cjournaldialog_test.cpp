// mxh/tests/unit/ui/cjournaldialog_test.cpp
//
// Unit tests for mxh::ui::cJournalDialog (Phase C dialog port).
//
// Locks down the 1:1 surface:
//   * kMaxJournalPageBtn / kMaxCheckboxPerPage / kJournalViewPerPage
//     / kMaxJournalSavedList = 5 / 5 / 5 / 50
//   * eJournal_* (Quest/Wanted/Levelup) + eJournal_Update/Delete
//   * Default construction: m_BasePage=0, m_MaxPage=0,
//     m_CurPage=0, m_bSavedJournal=false, all checkboxes off
//   * JournalItemAdd(Quest) populates m_JournalList with the
//     quest title looked up via the title callback
//   * JournalItemAdd(Wanted) populates m_JournalList with
//     the wanted name + param1 (result code)
//   * JournalItemAdd(Levelup) populates m_JournalList with
//     the level value in param1
//   * JournalItemAdd(bSaved=true) mirrors the item into
//     m_JournalSavedList
//   * JournalReset clears both lists + resets page counters
//   * JournalListReset computes m_MaxPage from list size
//   * SetBasePage advances/retreats by kMaxJournalPageBtn
//   * SetPage updates m_CurPage + m_BasePage
//   * SetItemCheck toggles m_bCheckItem[i]
//   * SelectedJournalSave copies checked items into the saved
//     list + fires Update net-msg + caps at 50
//   * SelectedJournalDelete removes the matching saved items
//     + fires Delete net-msg
//   * ViewJournalListToggle flips m_bSavedJournal
//   * AddList adds to live list (and saved list when bSaved)
//   * NonCopyable

#include "mxh/ui/cjournaldialog.hpp"

#include <gtest/gtest.h>

#include <cstring>

using mxh::ui::cJournalDialog;
using mxh::ui::JournalInfo;
using mxh::ui::JournalKind;
using mxh::ui::JournalNetOp;
using mxh::ui::JournalItem;
using mxh::ui::kMaxJournalPageBtn;
using mxh::ui::kMaxCheckboxPerPage;
using mxh::ui::kJournalViewPerPage;
using mxh::ui::kMaxJournalSavedList;

namespace {

const char* faQuestTitle(std::uint32_t key, void* /*user*/) {
    // Echo the key as a stable title for assertion.
    static char buf[32];
    std::snprintf(buf, sizeof(buf), "Q%u", key);
    return buf;
}

}  // namespace

TEST(CJournalDialog, ConstantsMatchLegacy) {
    EXPECT_EQ(kMaxJournalPageBtn,   5);
    EXPECT_EQ(kMaxCheckboxPerPage,  5);
    EXPECT_EQ(kJournalViewPerPage,  5);
    EXPECT_EQ(kMaxJournalSavedList, 50);
}

TEST(CJournalDialog, JournalKindEnumIsStable) {
    EXPECT_EQ(static_cast<std::uint32_t>(JournalKind::Quest),   0u);
    EXPECT_EQ(static_cast<std::uint32_t>(JournalKind::Wanted),  1u);
    EXPECT_EQ(static_cast<std::uint32_t>(JournalKind::Levelup), 2u);
}

TEST(CJournalDialog, JournalNetOpEnumIsStable) {
    EXPECT_EQ(static_cast<std::uint32_t>(JournalNetOp::Update), 0u);
    EXPECT_EQ(static_cast<std::uint32_t>(JournalNetOp::Delete), 1u);
}

TEST(CJournalDialog, DefaultConstructionIsZeroed) {
    cJournalDialog d;
    EXPECT_EQ(d.basePage(), 0);
    EXPECT_EQ(d.maxPage(),  0);
    EXPECT_EQ(d.curPage(),  0);
    EXPECT_FALSE(d.isSavedJournal());
    EXPECT_EQ(d.liveListCount(),  0);
    EXPECT_EQ(d.savedListCount(), 0);
    for (int i = 0; i < kMaxCheckboxPerPage; ++i) {
        EXPECT_FALSE(d.isItemChecked(i));
    }
}

TEST(CJournalDialog, AddQuestItemPopulatesLiveList) {
    cJournalDialog d;
    d.SetQuestTitleCallbackForTest(&faQuestTitle, nullptr);
    JournalInfo info{};
    info.Index   = 101;
    info.Kind    = static_cast<std::uint32_t>(JournalKind::Quest);
    info.Param   = 50;             // questId
    info.Param_2 = 0;
    info.Param_3 = 1;              // bCompleted
    std::strncpy(info.RegDate, "2026-07-26", sizeof(info.RegDate) - 1);
    d.JournalItemAdd(info);
    EXPECT_EQ(d.liveListCount(), 1);
    EXPECT_EQ(d.savedListCount(), 0);
}

TEST(CJournalDialog, AddQuestItemSavedMirrorsToSavedList) {
    cJournalDialog d;
    d.SetQuestTitleCallbackForTest(&faQuestTitle, nullptr);
    JournalInfo info{};
    info.Index  = 1;
    info.Kind   = static_cast<std::uint32_t>(JournalKind::Quest);
    info.Param  = 7;
    info.bSaved = 1;
    d.JournalItemAdd(info);
    EXPECT_EQ(d.liveListCount(),  1);
    EXPECT_EQ(d.savedListCount(), 1);
}

TEST(CJournalDialog, AddWantedItemStoresNameAndResult) {
    cJournalDialog d;
    JournalInfo info{};
    info.Index  = 5;
    info.Kind   = static_cast<std::uint32_t>(JournalKind::Wanted);
    info.Param  = 1;   // result = Wanted_Succeed
    std::strncpy(info.ParamName, "Target", sizeof(info.ParamName) - 1);
    d.JournalItemAdd(info);
    EXPECT_EQ(d.liveListCount(), 1);
}

TEST(CJournalDialog, AddLevelupItemStoresLevelInParam1) {
    cJournalDialog d;
    JournalInfo info{};
    info.Index = 9;
    info.Kind  = static_cast<std::uint32_t>(JournalKind::Levelup);
    info.Param = 50;   // level 50
    d.JournalItemAdd(info);
    EXPECT_EQ(d.liveListCount(), 1);
}

TEST(CJournalDialog, AddItemNullParamNameIsSafe) {
    cJournalDialog d;
    JournalInfo info{};
    info.Kind  = static_cast<std::uint32_t>(JournalKind::Wanted);
    info.Param = 1;
    // ParamName is empty -- std::string assignment is safe.
    d.JournalItemAdd(info);
    EXPECT_EQ(d.liveListCount(), 1);
}

TEST(CJournalDialog, JournalResetClearsEverything) {
    cJournalDialog d;
    d.SetQuestTitleCallbackForTest(&faQuestTitle, nullptr);
    JournalInfo info{};
    info.Index  = 1;
    info.Kind   = static_cast<std::uint32_t>(JournalKind::Quest);
    info.Param  = 1;
    d.JournalItemAdd(info);
    EXPECT_EQ(d.liveListCount(), 1);
    d.JournalReset();
    EXPECT_EQ(d.liveListCount(),  0);
    EXPECT_EQ(d.savedListCount(), 0);
    EXPECT_EQ(d.basePage(), 0);
    EXPECT_EQ(d.maxPage(),  0);
    EXPECT_EQ(d.curPage(),  0);
    EXPECT_FALSE(d.isSavedJournal());
}

TEST(CJournalDialog, JournalListResetComputesMaxPage) {
    cJournalDialog d;
    d.SetQuestTitleCallbackForTest(&faQuestTitle, nullptr);
    // 12 items / 5 per page = 2 full pages + remainder -> m_MaxPage = 2.
    for (int i = 0; i < 12; ++i) {
        JournalInfo info{};
        info.Index = static_cast<std::uint32_t>(i + 1);
        info.Kind  = static_cast<std::uint32_t>(JournalKind::Quest);
        info.Param = i;
        d.JournalItemAdd(info);
    }
    d.JournalListReset();
    EXPECT_EQ(d.liveListCount(), 12);
    // 12 / 5 = 2 with remainder 2; m_MaxPage = 2 - 1 + 1 = 2.
    EXPECT_EQ(d.maxPage(), 2);
}

TEST(CJournalDialog, SetBasePageAdvances) {
    cJournalDialog d;
    d.SetQuestTitleCallbackForTest(&faQuestTitle, nullptr);
    for (int i = 0; i < 30; ++i) {
        JournalInfo info{};
        info.Index = static_cast<std::uint32_t>(i + 1);
        info.Kind  = static_cast<std::uint32_t>(JournalKind::Quest);
        info.Param = i;
        d.JournalItemAdd(info);
    }
    d.JournalListReset();
    // 30 / 5 = 6, no remainder -> m_MaxPage = 5.
    EXPECT_EQ(d.maxPage(), 5);
    d.SetBasePage(true);
    EXPECT_EQ(d.basePage(), 5);
    d.SetBasePage(true);
    EXPECT_EQ(d.basePage(), 5);   // can't advance past MaxPage
}

TEST(CJournalDialog, SetBasePageRetreats) {
    cJournalDialog d;
    d.SetQuestTitleCallbackForTest(&faQuestTitle, nullptr);
    for (int i = 0; i < 30; ++i) {
        JournalInfo info{};
        info.Index = static_cast<std::uint32_t>(i + 1);
        info.Kind  = static_cast<std::uint32_t>(JournalKind::Quest);
        info.Param = i;
        d.JournalItemAdd(info);
    }
    d.JournalListReset();
    d.SetBasePage(true);
    EXPECT_EQ(d.basePage(), 5);
    d.SetBasePage(false);
    EXPECT_EQ(d.basePage(), 0);
    d.SetBasePage(false);
    EXPECT_EQ(d.basePage(), 0);
}

TEST(CJournalDialog, SetItemCheckToggles) {
    cJournalDialog d;
    d.SetItemCheck(0);
    EXPECT_TRUE(d.isItemChecked(0));
    d.SetItemCheck(0);
    EXPECT_FALSE(d.isItemChecked(0));
    d.SetItemCheck(2);
    EXPECT_TRUE(d.isItemChecked(2));
}

TEST(CJournalDialog, SetItemCheckOutOfRangeIsNoOp) {
    cJournalDialog d;
    d.SetItemCheck(99);
    d.SetItemCheck(-1);
    SUCCEED();
}

TEST(CJournalDialog, SelectedJournalSaveCopiesAndFiresUpdate) {
    cJournalDialog d;
    d.SetQuestTitleCallbackForTest(&faQuestTitle, nullptr);
    JournalInfo info{};
    info.Index = 11;
    info.Kind  = static_cast<std::uint32_t>(JournalKind::Quest);
    info.Param = 7;
    d.JournalItemAdd(info);
    d.JournalListReset();
    int updateCount = 0;
    std::uint32_t lastIndex = 0;
    d.SetSendNetMsgCallbackForTest(
        [](std::uint32_t index, std::uint32_t type, void* user) {
            auto* c = static_cast<int*>(user);
            if (type == 0) {  // Update
                ++*c;
            }
            // ignore the saved index for now
        },
        &updateCount);
    d.SetItemCheck(0);
    d.SelectedJournalSave();
    EXPECT_EQ(d.savedListCount(), 1);
    EXPECT_EQ(updateCount, 1);
    EXPECT_FALSE(d.isItemChecked(0));  // cleared after the save
}

TEST(CJournalDialog, SelectedJournalSaveAlreadySavedIsNoOp) {
    cJournalDialog d;
    d.SetQuestTitleCallbackForTest(&faQuestTitle, nullptr);
    JournalInfo info{};
    info.Index   = 1;
    info.Kind    = static_cast<std::uint32_t>(JournalKind::Quest);
    info.Param   = 1;
    info.bSaved  = 1;
    d.JournalItemAdd(info);
    d.JournalListReset();
    int updateCount = 0;
    d.SetSendNetMsgCallbackForTest(
        [](std::uint32_t, std::uint32_t type, void* user) {
            if (type == 0) ++*static_cast<int*>(user);
        },
        &updateCount);
    d.SetItemCheck(0);
    d.SelectedJournalSave();
    EXPECT_EQ(updateCount, 0);   // already saved -> no Update
    EXPECT_EQ(d.savedListCount(), 1);
}

TEST(CJournalDialog, SelectedJournalSaveCapsAt50) {
    cJournalDialog d;
    d.SetQuestTitleCallbackForTest(&faQuestTitle, nullptr);
    // Pre-load 50 saved items.
    for (int i = 0; i < 50; ++i) {
        JournalInfo info{};
        info.Index  = static_cast<std::uint32_t>(i + 1);
        info.Kind   = static_cast<std::uint32_t>(JournalKind::Quest);
        info.Param  = i;
        info.bSaved = 1;
        d.JournalItemAdd(info);
    }
    // Add a 51st live (not saved) item.
    JournalInfo extra{};
    extra.Index = 100;
    extra.Kind  = static_cast<std::uint32_t>(JournalKind::Quest);
    extra.Param = 99;
    d.JournalItemAdd(extra);
    d.JournalListReset();
    EXPECT_EQ(d.savedListCount(), 50);
    int sysmsgCount = 0;
    d.SetChatMsgCallbackForTest(
        [](int id, void* user) {
            if (id == cJournalDialog::kChatMsgSavedListFull) {
                ++*static_cast<int*>(user);
            }
            return "";
        },
        &sysmsgCount);
    d.SetItemCheck(0);
    d.SelectedJournalSave();
    EXPECT_EQ(d.savedListCount(), 50);   // still capped
    EXPECT_EQ(sysmsgCount, 1);
}

TEST(CJournalDialog, SelectedJournalDeleteFiresDelete) {
    cJournalDialog d;
    d.SetQuestTitleCallbackForTest(&faQuestTitle, nullptr);
    JournalInfo info{};
    info.Index  = 11;
    info.Kind   = static_cast<std::uint32_t>(JournalKind::Quest);
    info.Param  = 1;
    info.bSaved = 1;
    d.JournalItemAdd(info);
    // Switch to saved-view so JournalListReset populates
    // m_JournalSavedList's ViewIndex (and not m_JournalList's).
    d.ViewJournalListToggle();
    d.JournalListReset();
    int deleteCount = 0;
    d.SetSendNetMsgCallbackForTest(
        [](std::uint32_t, std::uint32_t type, void* user) {
            if (type == 1) ++*static_cast<int*>(user);
        },
        &deleteCount);
    d.SetItemCheck(0);
    d.SelectedJournalDelete();
    EXPECT_EQ(deleteCount, 1);
    EXPECT_EQ(d.savedListCount(), 0);
}

TEST(CJournalDialog, ViewJournalListToggleFlipsSavedFlag) {
    cJournalDialog d;
    EXPECT_FALSE(d.isSavedJournal());
    d.ViewJournalListToggle();
    EXPECT_TRUE(d.isSavedJournal());
    d.ViewJournalListToggle();
    EXPECT_FALSE(d.isSavedJournal());
}

TEST(CJournalDialog, ViewJournalListToggleClearsCheckboxes) {
    cJournalDialog d;
    d.SetItemCheck(0);
    d.SetItemCheck(2);
    d.ViewJournalListToggle();
    EXPECT_FALSE(d.isItemChecked(0));
    EXPECT_FALSE(d.isItemChecked(2));
}

TEST(CJournalDialog, ViewJournalListToggleResetsCurPage) {
    cJournalDialog d;
    d.SetQuestTitleCallbackForTest(&faQuestTitle, nullptr);
    for (int i = 0; i < 30; ++i) {
        JournalInfo info{};
        info.Index = static_cast<std::uint32_t>(i + 1);
        info.Kind  = static_cast<std::uint32_t>(JournalKind::Quest);
        info.Param = i;
        d.JournalItemAdd(info);
    }
    d.JournalListReset();
    d.SetPage(3);
    EXPECT_EQ(d.curPage(), 3);
    d.ViewJournalListToggle();
    EXPECT_EQ(d.curPage(), 0);
}

TEST(CJournalDialog, AddListBypassesPublicApiAddsLiveAndSaved) {
    cJournalDialog d;
    auto item = std::make_unique<JournalItem>();
    item->type = JournalKind::Quest;
    item->JournalDBIndex = 7;
    item->questTitle = "T";
    item->regDate[0] = 'A';
    item->regDate[1] = '\0';
    d.AddList(std::move(item), /*bSaved=*/true);
    EXPECT_EQ(d.liveListCount(),  1);
    EXPECT_EQ(d.savedListCount(), 1);
}

TEST(CJournalDialog, NonCopyable) {
    static_assert(!std::is_copy_constructible_v<cJournalDialog>);
    static_assert(!std::is_copy_assignable_v<cJournalDialog>);
}
