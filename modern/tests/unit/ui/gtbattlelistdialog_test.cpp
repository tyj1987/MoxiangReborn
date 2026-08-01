#include "gtbattlelistdialog.hpp"

#include "mxh/ui/cListCtrl.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <type_traits>

using mxh::ui::cDialog;
using mxh::ui::cGTBattleListDialog;
using mxh::ui::cListCtrl;
using mxh::ui::GTBattleGroup;
using mxh::ui::GTBattleInfo;

namespace {

struct Controls {
    cListCtrl list;

    void Attach(cGTBattleListDialog& dialog) {
        dialog.SetControlsForTest(&list);
    }
};

struct RefreshCapture {
    int chatCalls = 0;
    struct Inner {
        int calls = 0;
        std::uint32_t lastBattleId = 0;
    } insertCap;
};

const char* StubChatFormat(std::int32_t messageId, void* userData) {
    auto* rc = static_cast<RefreshCapture*>(userData);
    if (rc) ++rc->chatCalls;
    switch (messageId) {
    case cGTBattleListDialog::kPlayOffMessageId:
        return "[P]%c";
    case cGTBattleListDialog::kGroupMessageId:
        return "[G]%c";
    case cGTBattleListDialog::kGuildPairMessageId:
        return "%s vs %s";
    default:
        return nullptr;
    }
}

void CaptureInsertRow(std::uint32_t battleId, void* userData) {
    auto* rc = static_cast<RefreshCapture*>(userData);
    if (!rc) return;
    ++rc->insertCap.calls;
    rc->insertCap.lastBattleId = battleId;
}

struct JoinCapture {
    int calls = 0;
    std::uint32_t lastBattleId = 0;
    bool returnValue = true;
};

bool CaptureObserverJoin(std::uint32_t battleId, void* userData) {
    auto* cap = static_cast<JoinCapture*>(userData);
    ++cap->calls;
    cap->lastBattleId = battleId;
    return cap->returnValue;
}

GTBattleInfo MakeBattle(std::uint32_t id, GTBattleGroup group,
                        const char* g1, const char* g2) {
    GTBattleInfo info{};
    info.battleId = id;
    info.group = group;
    std::strncpy(info.guildName1, g1, sizeof(info.guildName1) - 1);
    std::strncpy(info.guildName2, g2, sizeof(info.guildName2) - 1);
    return info;
}

} // namespace

TEST(GTBattleListDialogTest, InheritsDialogAndIsNonCopyable) {
    static_assert(std::is_base_of_v<cDialog, cGTBattleListDialog>);
    static_assert(!std::is_copy_constructible_v<cGTBattleListDialog>);
    static_assert(!std::is_copy_assignable_v<cGTBattleListDialog>);
    SUCCEED();
}

TEST(GTBattleListDialogTest, ConstantsMatchPreprocessedLegacyValues) {
    EXPECT_EQ(cGTBattleListDialog::kBattleListId, 1390);
    EXPECT_EQ(cGTBattleListDialog::kNoSelection, -1);
    EXPECT_EQ(cGTBattleListDialog::kPlayOffMessageId, 953);
    EXPECT_EQ(cGTBattleListDialog::kGroupMessageId, 954);
    EXPECT_EQ(cGTBattleListDialog::kGuildPairMessageId, 955);
    EXPECT_EQ(cGTBattleListDialog::kActionRowClick, 0x0100);
    EXPECT_EQ(cGTBattleListDialog::kSelectedRgb, 0xFFFFEA00u);
    EXPECT_EQ(cGTBattleListDialog::kDefaultRgb, 0xFFFFFFFFu);
    EXPECT_EQ(cGTBattleListDialog::kGroupCount, 4u);
    EXPECT_EQ(cGTBattleListDialog::kColumnCount, 2u);
}

TEST(GTBattleListDialogTest, ConstructorDefaultsAreCorrect) {
    cGTBattleListDialog dialog;
    EXPECT_EQ(dialog.battleCount(), 0u);
    EXPECT_FALSE(dialog.playOff());
    EXPECT_EQ(dialog.selectedIndex(), cGTBattleListDialog::kNoSelection);
    EXPECT_EQ(dialog.battleListCtrl(), nullptr);
    EXPECT_FALSE(dialog.isActive());
}

TEST(GTBattleListDialogTest, SetControlsForTestAssignsCtrl) {
    cGTBattleListDialog dialog;
    Controls controls;
    controls.Attach(dialog);
    EXPECT_EQ(dialog.battleListCtrl(), &controls.list);
}

TEST(GTBattleListDialogTest, LinkingResolvesControlAndResetsState) {
    cGTBattleListDialog dialog;
    dialog.SetPlayOff(true);
    dialog.AddBattleInfo(MakeBattle(1, GTBattleGroup::A, "G1", "G2"));

    auto child = std::make_unique<cListCtrl>();
    child->Init(0, 0, 100, 100, nullptr, cGTBattleListDialog::kBattleListId);
    cListCtrl* raw = child.get();
    dialog.Add(std::move(child));

    dialog.Linking();

    EXPECT_EQ(dialog.battleListCtrl(), raw);
    EXPECT_FALSE(dialog.playOff());
    EXPECT_EQ(dialog.battleCount(), 0u);
    EXPECT_EQ(dialog.selectedIndex(), cGTBattleListDialog::kNoSelection);
    EXPECT_FALSE(dialog.hasBattle(1));
}

TEST(GTBattleListDialogTest, AddBattleInfoIgnoresEmptyNames) {
    cGTBattleListDialog dialog;
    dialog.AddBattleInfo(MakeBattle(1, GTBattleGroup::A, "", "G2"));
    EXPECT_EQ(dialog.battleCount(), 0u);
    dialog.AddBattleInfo(MakeBattle(2, GTBattleGroup::A, "G1", ""));
    EXPECT_EQ(dialog.battleCount(), 0u);
    dialog.AddBattleInfo(MakeBattle(3, GTBattleGroup::B, "G1", "G2"));
    EXPECT_EQ(dialog.battleCount(), 1u);
    EXPECT_TRUE(dialog.hasBattle(3));
}

TEST(GTBattleListDialogTest, AddBattleInfoStoresAll) {
    cGTBattleListDialog dialog;
    dialog.AddBattleInfo(MakeBattle(10, GTBattleGroup::A, "G1", "G2"));
    dialog.AddBattleInfo(MakeBattle(20, GTBattleGroup::B, "G3", "G4"));
    EXPECT_EQ(dialog.battleCount(), 2u);
    EXPECT_TRUE(dialog.hasBattle(10));
    EXPECT_TRUE(dialog.hasBattle(20));
    EXPECT_FALSE(dialog.hasBattle(30));
}

TEST(GTBattleListDialogTest, DeleteBattleInfoRemovesAndDecrements) {
    cGTBattleListDialog dialog;
    dialog.AddBattleInfo(MakeBattle(10, GTBattleGroup::A, "G1", "G2"));
    dialog.AddBattleInfo(MakeBattle(20, GTBattleGroup::B, "G3", "G4"));
    EXPECT_TRUE(dialog.DeleteBattleInfo(10));
    EXPECT_EQ(dialog.battleCount(), 1u);
    EXPECT_FALSE(dialog.hasBattle(10));
    EXPECT_FALSE(dialog.DeleteBattleInfo(99));
    EXPECT_EQ(dialog.battleCount(), 1u);
}

TEST(GTBattleListDialogTest, DeleteAllBattleInfoClearsEverything) {
    cGTBattleListDialog dialog;
    Controls controls;
    controls.Attach(dialog);
    dialog.AddBattleInfo(MakeBattle(10, GTBattleGroup::A, "G1", "G2"));
    dialog.SetPlayOff(true);
    EXPECT_EQ(dialog.battleCount(), 1u);

    dialog.DeleteAllBattleInfo();
    EXPECT_EQ(dialog.battleCount(), 0u);
    EXPECT_FALSE(dialog.playOff());
    EXPECT_EQ(dialog.selectedIndex(), cGTBattleListDialog::kNoSelection);
    // DeleteAllBattleInfo itself doesn't deactivate (legacy mirrors:
    // SetActive(false) is what triggers the cleanup).
}

TEST(GTBattleListDialogTest, DeleteAddBattleInfoAliasClearsEverything) {
    cGTBattleListDialog dialog;
    dialog.AddBattleInfo(MakeBattle(10, GTBattleGroup::A, "G1", "G2"));
    dialog.DeleteAddBattleInfo();
    EXPECT_EQ(dialog.battleCount(), 0u);
}

TEST(GTBattleListDialogTest, SetActiveFalseClearsAll) {
    cGTBattleListDialog dialog;
    Controls controls;
    controls.Attach(dialog);
    dialog.AddBattleInfo(MakeBattle(10, GTBattleGroup::A, "G1", "G2"));
    dialog.SetActive(false);
    EXPECT_EQ(dialog.battleCount(), 0u);
}

TEST(GTBattleListDialogTest, SetActiveTrueDoesNothingElse) {
    cGTBattleListDialog dialog;
    Controls controls;
    controls.Attach(dialog);
    dialog.AddBattleInfo(MakeBattle(10, GTBattleGroup::A, "G1", "G2"));
    dialog.SetActive(true);
    EXPECT_EQ(dialog.battleCount(), 1u);
}

TEST(GTBattleListDialogTest, RefreshBattleListPopulatesCtrl) {
    cGTBattleListDialog dialog;
    Controls controls;
    controls.Attach(dialog);
    RefreshCapture cap;
    dialog.SetCallbacks(&StubChatFormat, &CaptureInsertRow, nullptr, &cap);
    dialog.AddBattleInfo(MakeBattle(10, GTBattleGroup::A, "G1", "G2"));
    dialog.AddBattleInfo(MakeBattle(20, GTBattleGroup::B, "G3", "G4"));
    dialog.RefreshBattleList();
    EXPECT_EQ(controls.list.rowCount(), 2u);
    EXPECT_GT(cap.chatCalls, 0);
    EXPECT_EQ(cap.insertCap.calls, 2);
    EXPECT_EQ(cap.insertCap.lastBattleId, 20u);
}

TEST(GTBattleListDialogTest, RefreshBattleListToleratesNullCtrl) {
    cGTBattleListDialog dialog;
    dialog.AddBattleInfo(MakeBattle(10, GTBattleGroup::A, "G1", "G2"));
    EXPECT_NO_FATAL_FAILURE(dialog.RefreshBattleList());
}

TEST(GTBattleListDialogTest, RefreshUsesTestBattlesWhenSet) {
    cGTBattleListDialog dialog;
    Controls controls;
    controls.Attach(dialog);
    dialog.SetCallbacks(&StubChatFormat, nullptr, nullptr, nullptr);
    dialog.SetBattlesForTest(nullptr, 0);

    const GTBattleInfo test[] = {
        MakeBattle(100, GTBattleGroup::C, "AA", "BB"),
        MakeBattle(101, GTBattleGroup::D, "CC", "DD"),
    };
    dialog.SetBattlesForTest(test, 2);
    dialog.UseTestBattles(true);
    dialog.RefreshBattleList();

    EXPECT_EQ(controls.list.rowCount(), 2u);
}

TEST(GTBattleListDialogTest, HandleMouseActionIgnoresInactive) {
    cGTBattleListDialog dialog;
    Controls controls;
    controls.Attach(dialog);
    controls.list.SetSelectedRowIdx(0);
    dialog.AddBattleInfo(MakeBattle(10, GTBattleGroup::A, "G1", "G2"));
    dialog.HandleMouseAction(0, 0, cGTBattleListDialog::kActionRowClick);
    EXPECT_EQ(dialog.selectedIndex(), cGTBattleListDialog::kNoSelection);
}

TEST(GTBattleListDialogTest, HandleMouseActionCapturesSelection) {
    cGTBattleListDialog dialog;
    Controls controls;
    controls.Attach(dialog);
    dialog.AddBattleInfo(MakeBattle(10, GTBattleGroup::A, "G1", "G2"));
    dialog.AddBattleInfo(MakeBattle(20, GTBattleGroup::B, "G3", "G4"));
    dialog.SetActive(true);
    controls.list.SetSelectedRowIdx(1);
    dialog.HandleMouseAction(0, 0, cGTBattleListDialog::kActionRowClick);
    EXPECT_EQ(dialog.selectedIndex(), 1);
}

TEST(GTBattleListDialogTest, HandleMouseActionIgnoresOutOfRange) {
    cGTBattleListDialog dialog;
    Controls controls;
    controls.Attach(dialog);
    dialog.AddBattleInfo(MakeBattle(10, GTBattleGroup::A, "G1", "G2"));
    dialog.SetActive(true);
    controls.list.SetSelectedRowIdx(99);
    dialog.HandleMouseAction(0, 0, cGTBattleListDialog::kActionRowClick);
    EXPECT_EQ(dialog.selectedIndex(), cGTBattleListDialog::kNoSelection);
}

TEST(GTBattleListDialogTest, HandleMouseActionIgnoresNonClickFlags) {
    cGTBattleListDialog dialog;
    Controls controls;
    controls.Attach(dialog);
    dialog.AddBattleInfo(MakeBattle(10, GTBattleGroup::A, "G1", "G2"));
    dialog.SetActive(true);
    controls.list.SetSelectedRowIdx(0);
    dialog.HandleMouseAction(0, 0, 0);
    EXPECT_EQ(dialog.selectedIndex(), cGTBattleListDialog::kNoSelection);
}

TEST(GTBattleListDialogTest, EnterBattleOnObserverUsesSelectedRow) {
    cGTBattleListDialog dialog;
    JoinCapture cap;
    dialog.SetCallbacks(nullptr, nullptr, &CaptureObserverJoin, &cap);
    dialog.AddBattleInfo(MakeBattle(42, GTBattleGroup::A, "G1", "G2"));
    EXPECT_FALSE(dialog.EnterBattleOnObserver());

    Controls controls;
    controls.Attach(dialog);
    dialog.SetActive(true);
    controls.list.SetSelectedRowIdx(0);
    dialog.HandleMouseAction(0, 0, cGTBattleListDialog::kActionRowClick);
    EXPECT_EQ(dialog.selectedIndex(), 0);

    cap.returnValue = true;
    EXPECT_TRUE(dialog.EnterBattleOnObserver());
    EXPECT_EQ(cap.calls, 1);
    EXPECT_EQ(cap.lastBattleId, 42u);

    cap.returnValue = false;
    EXPECT_FALSE(dialog.EnterBattleOnObserver());
    EXPECT_EQ(cap.calls, 2);
}

TEST(GTBattleListDialogTest, EnterBattleOnObserverToleratesNullCallback) {
    cGTBattleListDialog dialog;
    Controls controls;
    controls.Attach(dialog);
    dialog.SetCallbacks(nullptr, nullptr, nullptr, nullptr);
    dialog.AddBattleInfo(MakeBattle(10, GTBattleGroup::A, "G1", "G2"));
    dialog.SetActive(true);
    controls.list.SetSelectedRowIdx(0);
    dialog.HandleMouseAction(0, 0, cGTBattleListDialog::kActionRowClick);
    EXPECT_TRUE(dialog.EnterBattleOnObserver());
}

TEST(GTBattleListDialogTest, GroupInitialMatchesLegacyMapping) {
    EXPECT_EQ(cGTBattleListDialog::GroupInitial(GTBattleGroup::A), 'A');
    EXPECT_EQ(cGTBattleListDialog::GroupInitial(GTBattleGroup::B), 'B');
    EXPECT_EQ(cGTBattleListDialog::GroupInitial(GTBattleGroup::C), 'C');
    EXPECT_EQ(cGTBattleListDialog::GroupInitial(GTBattleGroup::D), 'D');
    EXPECT_EQ(cGTBattleListDialog::GroupInitial(GTBattleGroup::Unknown), '?');
}
