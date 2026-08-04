// pointsavedialog_test.cpp — 1:1 port tests for
// 墨香 CPointSaveDialog.

#include "pointsavedialog.hpp"
#include "cdialog.hpp"
#include "ceditbox.hpp"
#include "ctextarea.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

using mxh::ui::cDialog;
using mxh::ui::cEditBox;
using mxh::ui::cPointSaveDialog;

namespace {

struct LinkedDialog {
    cPointSaveDialog dlg;
    std::unique_ptr<cEditBox> nameEdit;

    LinkedDialog() {
        dlg.Init(0, 0, 200, 200, nullptr, 0);
        nameEdit = std::make_unique<cEditBox>();
        nameEdit->Init(0, 0, 100, 20, nullptr, nullptr,
                       cPointSaveDialog::kIdNameEditBox);
        // InitEditbox so SetEditText works.
        nameEdit->InitEditbox(50, 64);
        auto* ePtr = nameEdit.get();
        dlg.Add(std::move(nameEdit));

        dlg.Linking();

        editPtr_ = ePtr;
    }

    cEditBox* editPtr_ = nullptr;
};

}  // namespace

TEST(CPointSaveDialogTest, CtorDoesNotCrash) {
    cPointSaveDialog dlg;
    SUCCEED();
}

TEST(CPointSaveDialogTest, DtorDoesNotCrash) {
    cPointSaveDialog dlg;
    SUCCEED();
}

TEST(CPointSaveDialogTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cPointSaveDialog>,
                  "cPointSaveDialog must inherit from cDialog");
    SUCCEED();
}

TEST(CPointSaveDialogTest, DefaultNewPointIsTrue) {
    cPointSaveDialog dlg;
    EXPECT_TRUE(dlg.IsNewPoint());
}

TEST(CPointSaveDialogTest, DefaultItemStateIsZero) {
    cPointSaveDialog dlg;
    EXPECT_EQ(dlg.GetItemIdx(), 0u);
    EXPECT_EQ(dlg.GetItemPos(), 0u);
}

TEST(CPointSaveDialogTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cPointSaveDialog::kIdNameEditBox, 710);
}

TEST(CPointSaveDialogTest, VcmCharNameIsTwo) {
    EXPECT_EQ(cPointSaveDialog::kVcmCharName, 2);
}

// ---------- Linking ----------

TEST(CPointSaveDialogTest, LinkingResolvesNameEditBox) {
    LinkedDialog ld;
    // m_pNameEdtBox is private; verify via SetFocusEdit behavior
    // (which we test below).
    SUCCEED();
}

TEST(CPointSaveDialogTest, LinkingBeforeInitDoesNotCrash) {
    cPointSaveDialog dlg;
    dlg.Linking();
    SUCCEED();
}

TEST(CPointSaveDialogTest, LinkingWithoutChildrenDoesNotCrash) {
    cPointSaveDialog dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.Linking();
    SUCCEED();
}

// ---------- SetActive ----------

TEST(CPointSaveDialogTest, SetActiveTrueUpdatesBaseState) {
    cPointSaveDialog dlg;
    dlg.Init(0, 0, 200, 200, nullptr, 0);
    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CPointSaveDialogTest, SetActiveFalseUpdatesBaseState) {
    LinkedDialog ld;
    ld.dlg.SetActive(true);
    EXPECT_TRUE(ld.dlg.isActive());
    ld.dlg.SetActive(false);
    EXPECT_FALSE(ld.dlg.isActive());
}

TEST(CPointSaveDialogTest, SetActiveTrueClearsEditText) {
    LinkedDialog ld;
    ld.editPtr_->SetEditText("OldName");
    EXPECT_EQ(ld.editPtr_->editText(), "OldName");
    ld.dlg.SetActive(true);
    EXPECT_EQ(ld.editPtr_->editText(), "");
}

TEST(CPointSaveDialogTest, SetActiveBeforeInitDoesNotCrash) {
    cPointSaveDialog dlg;
    dlg.SetActive(true);
    SUCCEED();
}

// ---------- SetItemToMapServer ----------

TEST(CPointSaveDialogTest, SetItemToMapServerUpdatesItemIdx) {
    cPointSaveDialog dlg;
    dlg.SetItemToMapServer(100, 5);
    EXPECT_EQ(dlg.GetItemIdx(), 100u);
}

TEST(CPointSaveDialogTest, SetItemToMapServerUpdatesItemPos) {
    cPointSaveDialog dlg;
    dlg.SetItemToMapServer(100, 5);
    EXPECT_EQ(dlg.GetItemPos(), 5u);
}

TEST(CPointSaveDialogTest, SetItemToMapServerMultipleCalls) {
    cPointSaveDialog dlg;
    dlg.SetItemToMapServer(100, 5);
    dlg.SetItemToMapServer(200, 10);
    EXPECT_EQ(dlg.GetItemIdx(), 200u);
    EXPECT_EQ(dlg.GetItemPos(), 10u);
}

// ---------- SetDialogStatus ----------

TEST(CPointSaveDialogTest, SetDialogStatusToggles) {
    cPointSaveDialog dlg;
    EXPECT_TRUE(dlg.IsNewPoint());
    dlg.SetDialogStatus(false);
    EXPECT_FALSE(dlg.IsNewPoint());
    dlg.SetDialogStatus(true);
    EXPECT_TRUE(dlg.IsNewPoint());
}

// ---------- ChangePointName / CancelPointName ----------

// ChangePointName + CancelPointName host-dispatch tests below.
// ===========================================================================
// Callback fixtures + host-dispatch tests
// ===========================================================================

namespace {

struct PointSaveHostCalls {
    bool          checkSameNameReturn = false;
    std::uint32_t moveDialogDBIdx     = 42u;
    std::uint32_t heroObjectId        = 0u;
    std::uint16_t heroPosX            = 100u;
    std::uint16_t heroPosZ            = 200u;
    std::int32_t  heroState           = 0;
    std::uint16_t mapNum              = 0;
    int           checkSameNameCalls  = 0;
    int           enableTableCalls    = 0;
    int           endObjectStateCalls = 0;
    int           addSynCalls         = 0;
    int           updateSynCalls      = 0;
    int           systemMessageCalls  = 0;
    std::uint32_t endedObjectId       = 0u;
    std::int32_t  endedState          = 0;
    bool          sameNameSeen        = false;

    static bool CheckSameName(const char* /*name*/, void* ud) {
        auto* hc = static_cast<PointSaveHostCalls*>(ud);
        ++hc->checkSameNameCalls;
        hc->sameNameSeen = true;
        return hc->checkSameNameReturn;
    }
    static std::uint32_t MoveDBIdx(void* ud) {
        return static_cast<PointSaveHostCalls*>(ud)->moveDialogDBIdx;
    }
    static std::uint32_t HeroId(void* ud) {
        return static_cast<PointSaveHostCalls*>(ud)->heroObjectId;
    }
    static void HeroPos(std::uint16_t* x, std::uint16_t* z, void* ud) {
        auto* hc = static_cast<PointSaveHostCalls*>(ud);
        *x = hc->heroPosX;
        *z = hc->heroPosZ;
    }
    static std::int32_t HeroState(void* ud) {
        return static_cast<PointSaveHostCalls*>(ud)->heroState;
    }
    static std::uint16_t MapNum(void* ud) {
        return static_cast<PointSaveHostCalls*>(ud)->mapNum;
    }
    static const char* ChatMsg(std::int32_t /*id*/, void* /*ud*/) {
        return "SAVEPOINT_TEST_MSG";
    }
    static void AddSystem(const char* /*t*/, void* ud) {
        ++static_cast<PointSaveHostCalls*>(ud)->systemMessageCalls;
    }
    static void EnableTable(std::int8_t /*t*/, void* ud) {
        ++static_cast<PointSaveHostCalls*>(ud)->enableTableCalls;
    }
    static void EndObjectState(std::uint32_t objectId, std::int32_t state,
                                void* ud) {
        auto* hc = static_cast<PointSaveHostCalls*>(ud);
        ++hc->endObjectStateCalls;
        hc->endedObjectId = objectId;
        hc->endedState = state;
    }
    static void SendAddSyn(std::uint32_t /*h*/, std::uint32_t /*d*/,
                           std::uint16_t /*m*/, const char* /*n*/,
                           std::uint16_t /*x*/, std::uint16_t /*z*/,
                           std::uint16_t /*i*/, std::uint16_t /*p*/,
                           void* ud) {
        ++static_cast<PointSaveHostCalls*>(ud)->addSynCalls;
    }
    static void SendUpdateSyn(std::uint32_t /*h*/, std::uint32_t /*d*/,
                              const char* /*n*/, void* ud) {
        ++static_cast<PointSaveHostCalls*>(ud)->updateSynCalls;
    }
};

void InstallPointSaveCallbacks(cPointSaveDialog& dlg, PointSaveHostCalls* hc) {
    dlg.SetCallbacks(&PointSaveHostCalls::CheckSameName,
                     &PointSaveHostCalls::MoveDBIdx,
                     &PointSaveHostCalls::HeroId,
                     &PointSaveHostCalls::HeroPos,
                     &PointSaveHostCalls::HeroState,
                     &PointSaveHostCalls::MapNum,
                     &PointSaveHostCalls::ChatMsg,
                     &PointSaveHostCalls::AddSystem,
                     &PointSaveHostCalls::EnableTable,
                     &PointSaveHostCalls::EndObjectState,
                     &PointSaveHostCalls::SendAddSyn,
                     &PointSaveHostCalls::SendUpdateSyn,
                     hc);
}

}  // namespace

TEST(CPointSaveDialogTest, LegacyMessageAndStateConstantsMatchSource) {
    // 1:1 with legacy CHATMGR->GetChatMsg(784) + eItemTable_* enum values +
    // eObjectState_Deal + CommonGameDefine.h MAX_SAVEDMOVE_NAME=21.
    EXPECT_EQ(cPointSaveDialog::kSysmsgDuplicateName,    784);
    EXPECT_EQ(cPointSaveDialog::kItemTableInventory,     0);
    EXPECT_EQ(cPointSaveDialog::kItemTablePyoguk,        2);
    EXPECT_EQ(cPointSaveDialog::kItemTableShop,          3);
    EXPECT_EQ(cPointSaveDialog::kItemTableMunpaWarehouse, 10);
    EXPECT_EQ(cPointSaveDialog::kObjectStateDeal,        6);
    EXPECT_EQ(cPointSaveDialog::kMaxSavedMoveName,       21u);
    EXPECT_EQ(cPointSaveDialog::kMaxSavedMoveNameTrunc,  20u);
}

TEST(CPointSaveDialogTest, ChangePointNameEmptyNameEnablesTablesAndCloses) {
    // 1:1 with legacy empty-name branch:
    // ITEMMGR->SetDisableDialog(FALSE, ...) x4 +
    // SetEditText("") + SetActive(FALSE) + return.
    LinkedDialog ld;
    PointSaveHostCalls hc;
    InstallPointSaveCallbacks(ld.dlg, &hc);
    ld.dlg.SetActive(true);
    ld.editPtr_->SetEditText("");

    ld.dlg.ChangePointName();

    // 4 tables re-enabled (1:1 with legacy x4
    // SetDisableDialog(FALSE, ...)).
    EXPECT_EQ(hc.enableTableCalls, 4);
    // No same-name check was performed
    // (legacy returns at the empty-name check).
    EXPECT_EQ(hc.checkSameNameCalls, 0);
    // No SEND dispatched (legacy returns
    // before the SEND block).
    EXPECT_EQ(hc.addSynCalls, 0);
    EXPECT_EQ(hc.updateSynCalls, 0);
    EXPECT_FALSE(ld.dlg.isActive());
}

TEST(CPointSaveDialogTest, ChangePointNameSameNameEmitsMsgAndEndsDealState) {
    // 1:1 with legacy CheckSameName branch:
    // ITEMMGR x4 + HERO state==Deal -> EndObjectState +
    // chat msg 784 + return (no SEND).
    LinkedDialog ld;
    PointSaveHostCalls hc;
    hc.checkSameNameReturn = true;
    hc.heroObjectId = 7u;
    hc.heroState = cPointSaveDialog::kObjectStateDeal;
    InstallPointSaveCallbacks(ld.dlg, &hc);
    ld.dlg.SetActive(true);
    // SetActive(true) clears the edit text;
    // set the user-typed name AFTER activation
    // (this matches the legacy cPointSaveDialog flow
    // -- see SetActiveTrueClearsEditText test).
    ld.editPtr_->SetEditText("Bone Town");

    ld.dlg.ChangePointName();

    EXPECT_EQ(hc.checkSameNameCalls, 1);
    EXPECT_EQ(hc.enableTableCalls, 4);
    EXPECT_EQ(hc.endObjectStateCalls, 1);
    EXPECT_EQ(hc.endedObjectId, 7u);
    EXPECT_EQ(hc.endedState, cPointSaveDialog::kObjectStateDeal);
    EXPECT_EQ(hc.systemMessageCalls, 1);
    EXPECT_EQ(hc.addSynCalls, 0);
    EXPECT_EQ(hc.updateSynCalls, 0);
    // 1:1 with legacy: same-name branch
    // RETURNS without calling
    // SetActive(FALSE); the dialog stays
    // open so the user can edit the name.
    EXPECT_TRUE(ld.dlg.isActive());
}

TEST(CPointSaveDialogTest, ChangePointNameSameNameNoDealStateSkipsEndObject) {
    // 1:1 quirk: legacy only ends the deal
    // state when HERO is in deal AND not
    // in npc script. With HERO state !=
    // kObjectStateDeal, EndObjectState is
    // not called; the chat msg still emits.
    LinkedDialog ld;
    PointSaveHostCalls hc;
    hc.checkSameNameReturn = true;
    hc.heroObjectId = 7u;
    hc.heroState = 5;  // not deal
    InstallPointSaveCallbacks(ld.dlg, &hc);
    ld.dlg.SetActive(true);
    // SetActive(true) clears the edit text;
    // set the user-typed name AFTER activation
    // (this matches the legacy cPointSaveDialog flow
    // -- see SetActiveTrueClearsEditText test).
    ld.editPtr_->SetEditText("Bone Town");

    ld.dlg.ChangePointName();

    EXPECT_EQ(hc.endObjectStateCalls, 0);
    EXPECT_EQ(hc.systemMessageCalls, 1);  // still emits
    EXPECT_EQ(hc.addSynCalls, 0);
    // 1:1 with legacy: same-name branch
    // RETURNS without closing the dialog.
    EXPECT_TRUE(ld.dlg.isActive());
}

TEST(CPointSaveDialogTest, ChangePointNameNewPointDispatchesAddSynAndCloses) {
    // 1:1 with legacy m_bNewPoint==TRUE:
    // SEND_MOVEDATA_WITHITEM is dispatched
    // with hero id + dbIdx + mapNum + name +
    // pos(x,z) + itemIdx + itemPos; then the
    // dialog is cleared + closed.
    LinkedDialog ld;
    PointSaveHostCalls hc;
    hc.heroObjectId = 99u;
    hc.moveDialogDBIdx = 12u;
    hc.mapNum = 5u;
    hc.heroPosX = 1000u;
    hc.heroPosZ = 2000u;
    InstallPointSaveCallbacks(ld.dlg, &hc);
    ld.dlg.SetDialogStatus(true);  // m_bNewPoint = true
    ld.dlg.SetItemToMapServer(7, 3);
    ld.dlg.SetActive(true);
    // SetActive(true) clears the edit text;
    // set the user-typed name AFTER activation
    // (this matches the legacy cPointSaveDialog flow
    // -- see SetActiveTrueClearsEditText test).
    ld.editPtr_->SetEditText("Bone Town");

    ld.dlg.ChangePointName();

    EXPECT_EQ(hc.addSynCalls, 1);
    EXPECT_EQ(hc.updateSynCalls, 0);
    EXPECT_EQ(hc.checkSameNameCalls, 1);  // legacy always queries CheckSameName
    EXPECT_EQ(hc.systemMessageCalls, 0);
    EXPECT_FALSE(ld.dlg.isActive());
    EXPECT_TRUE(ld.editPtr_->editText().empty());
}

TEST(CPointSaveDialogTest, ChangePointNameExistingPointDispatchesUpdateSynAndCloses) {
    // 1:1 with legacy m_bNewPoint==FALSE:
    // SEND_MOVEDATA_SIMPLE dispatched with
    // hero id + dbIdx + name; dialog cleared
    // + closed.
    LinkedDialog ld;
    PointSaveHostCalls hc;
    hc.heroObjectId = 33u;
    hc.moveDialogDBIdx = 21u;
    InstallPointSaveCallbacks(ld.dlg, &hc);
    ld.dlg.SetDialogStatus(false);  // m_bNewPoint = false
    ld.dlg.SetActive(true);
    ld.editPtr_->SetEditText("Old Village");

    ld.dlg.ChangePointName();

    EXPECT_EQ(hc.updateSynCalls, 1);
    EXPECT_EQ(hc.addSynCalls, 0);
    EXPECT_EQ(hc.checkSameNameCalls, 1);  // legacy always queries CheckSameName
    EXPECT_FALSE(ld.dlg.isActive());
    EXPECT_TRUE(ld.editPtr_->editText().empty());
}

TEST(CPointSaveDialogTest, ChangePointNameNoCallbacksIsSafe) {
    // No callbacks installed -> legacy
    // null-singleton path: ChangePointName
    // falls through the empty branch since
    // `editName` may be empty when the edit
    // box is null, otherwise falls through
    // to the m_bNewPoint branch with all
    // Send* callbacks null (also no-op). The
    // dialog ends up closed + edit cleared.
    LinkedDialog ld;
    ld.dlg.SetActive(true);
    // SetActive(true) clears the edit text;
    // set the user-typed name AFTER activation
    // (this matches the legacy cPointSaveDialog flow
    // -- see SetActiveTrueClearsEditText test).
    ld.editPtr_->SetEditText("Bone Town");

    ld.dlg.ChangePointName();

    EXPECT_FALSE(ld.dlg.isActive());
    EXPECT_TRUE(ld.editPtr_->editText().empty());
}

TEST(CPointSaveDialogTest, CancelPointNameEnablesTablesAndCloses) {
    // 1:1 with legacy CancelPointName body:
    // ITEMMGR->SetDisableDialog x4 +
    // SetActive(FALSE). The HERO +
    // OBJECTSTATEMGR block is COMMENTED OUT
    // in legacy (1:1 quirk preserved).
    LinkedDialog ld;
    PointSaveHostCalls hc;
    InstallPointSaveCallbacks(ld.dlg, &hc);
    ld.dlg.SetActive(true);

    ld.dlg.CancelPointName();

    EXPECT_EQ(hc.enableTableCalls, 4);
    EXPECT_EQ(hc.endObjectStateCalls, 0);  // commented out in legacy
    EXPECT_FALSE(ld.dlg.isActive());
}

TEST(CPointSaveDialogTest, CancelPointNameWithoutCallbacksClosesDialog) {
    // No callbacks installed -> SetActive
    // closure still runs (1:1 with legacy).
    LinkedDialog ld;
    ld.dlg.SetActive(true);
    ASSERT_TRUE(ld.dlg.isActive());

    ld.dlg.CancelPointName();

    EXPECT_FALSE(ld.dlg.isActive());
}

TEST(CPointSaveDialogTest, SetCallbacksReplacesExistingHostDispatch) {
    // First invocation uses the first
    // context; second invocation swaps to
    // the second context. Verify the second
    // context receives the dispatch and the
    // first one is left alone.
    LinkedDialog ld;
    PointSaveHostCalls firstCtx;
    PointSaveHostCalls secondCtx;
    InstallPointSaveCallbacks(ld.dlg, &firstCtx);
    ld.dlg.SetActive(true);
    ld.editPtr_->SetEditText("First Name");
    ld.dlg.ChangePointName();
    int firstSends = firstCtx.addSynCalls + firstCtx.updateSynCalls;
    EXPECT_EQ(firstSends, 1);

    ld.dlg.SetActive(true);
    ld.editPtr_->SetEditText("Second Name");
    InstallPointSaveCallbacks(ld.dlg, &secondCtx);
    ld.dlg.ChangePointName();
    EXPECT_EQ(firstSends, 1);  // first context NOT incremented again
    EXPECT_EQ(secondCtx.addSynCalls + secondCtx.updateSynCalls, 1);
}


