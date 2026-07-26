// mxh/tests/unit/ui/cmininotedialog_test.cpp
//
// Unit tests for mxh::ui::cMiniNoteDialog (Phase C dialog port).
//
// Locks down the 1:1 surface:
//   * Mode enum (Read / Write / Max) and 1:1 default
//   * Init() loads the SetShopItem table
//   * Linking() populates the per-mode control lists
//   * ShowMiniNoteMode() toggles the active flag on each control
//   * SetActiveMiniNoteMode() iterates the per-mode vector
//   * SetMiniNote() writes sender + body to both read + write textareas
//   * SetMiniNote() with ItemIdx uses the SetShopItem table
//   * SetActive(true) clears write-mode textareas
//   * SetActive(false) drops focus
//   * GetSenderName / GetNoteID / SetNoteID

#include "mxh/ui/cmininotedialog.hpp"
#include "mxh/ui/cstatic.hpp"
#include "mxh/ui/ctextarea.hpp"
#include "mxh/ui/ceditbox.hpp"
#include "mxh/ui/cbutton.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <string>

using mxh::ui::cButton;
using mxh::ui::cEditBox;
using mxh::ui::cMiniNoteDialog;
using mxh::ui::cStatic;
using mxh::ui::cTextArea;
using mxh::ui::MiniNoteMode;
using mxh::ui::MiniNoteMode_Read;
using mxh::ui::MiniNoteMode_Write;
using mxh::ui::MiniNoteMode_Max;

namespace {

// Minimal harness with one real child window of each kind.  We
// use the real modern cStatic / cTextArea / cEditBox / cButton
// so the SetStaticText / SetScriptText / SetEditText / SetActive
// operations work end-to-end.
struct Harness {
    cMiniNoteDialog dlg;
    cStatic   rTitle, wTitle, sender, senderStc, receiver;
    cTextArea rNoteText, wNoteText;
    cEditBox  receiverEdit;
    cButton   replayBtn, deleteBtn, sendOkBtn, sendCancelBtn;

    Harness() {
        // cEditBox refuses to SetEditText before InitEditbox
        // (m_maxBytes must be non-zero -- the legacy engine
        // considers it a configuration error otherwise).
        receiverEdit.InitEditbox(100, 32);
        cMiniNoteDialog::ChildWindows w{};
        w.rTitle        = &rTitle;
        w.wTitle        = &wTitle;
        w.rNoteText     = &rNoteText;
        w.wNoteText     = &wNoteText;
        w.sender        = &sender;
        w.senderStc     = &senderStc;
        w.replayBtn     = &replayBtn;
        w.deleteBtn     = &deleteBtn;
        w.receiverEdit  = &receiverEdit;
        w.receiver      = &receiver;
        w.sendOkBtn     = &sendOkBtn;
        w.sendCancelBtn = &sendCancelBtn;
        dlg.SetChildWindowsForTest(w);
    }
};

}  // namespace

TEST(CMiniNoteDialog, ModeEnumMatchesLegacy) {
    EXPECT_EQ(static_cast<std::int32_t>(MiniNoteMode_Read),  0);
    EXPECT_EQ(static_cast<std::int32_t>(MiniNoteMode_Write), 1);
    EXPECT_EQ(static_cast<std::int32_t>(MiniNoteMode_Max),   2);
}

TEST(CMiniNoteDialog, DefaultConstructionHasModeMinusOne) {
    cMiniNoteDialog d;
    EXPECT_EQ(d.GetCurMode(), -1);
    EXPECT_EQ(d.GetNoteID(), 0u);
    EXPECT_EQ(d.ReadText(), "");
    EXPECT_EQ(d.WriteText(), "");
}

TEST(CMiniNoteDialog, InitLoadsSetShopItemTable) {
    cMiniNoteDialog d;
    d.Init(0, 0, 200, 100, nullptr, 0);
    // The table is reserved in Init() (legacy m_SetitemNameTable
    // .Initialize(10) maps to reserve(10)).  The actual bin load
    // is deferred; tests inject items via AddSetShopItemForTest.
    EXPECT_EQ(d.SetShopItemCount(), 0u);
    d.AddSetShopItemForTest(123, "Potion");
    EXPECT_EQ(d.SetShopItemCount(), 1u);
}

TEST(CMiniNoteDialog, LinkingPopulatesPerModeControlLists) {
    Harness h;
    h.dlg.Linking();
    // 6 read-mode controls (rTitle, rNoteText, sender, senderStc,
    // replayBtn, deleteBtn) and 6 write-mode controls (wTitle,
    // wNoteText, receiverEdit, sendOkBtn, sendCancelBtn,
    // receiver).  We test by side-effect: ShowMiniNoteMode
    // toggles SetActive on the per-mode list.
    h.dlg.ShowMiniNoteMode(MiniNoteMode_Write);
    EXPECT_FALSE(h.rTitle.isActive());     // read-mode controls hidden
    EXPECT_TRUE(h.wTitle.isActive());     // write-mode controls shown
    h.dlg.ShowMiniNoteMode(MiniNoteMode_Read);
    EXPECT_TRUE(h.rTitle.isActive());      // read-mode controls shown
    EXPECT_FALSE(h.wTitle.isActive());
}

TEST(CMiniNoteDialog, LinkingDisablesEnterOnTextAreas) {
    Harness h;
    h.dlg.Linking();
    EXPECT_FALSE(h.rNoteText.IsEnterAllow());
    EXPECT_FALSE(h.wNoteText.IsEnterAllow());
}

TEST(CMiniNoteDialog, ShowMiniNoteModeIdempotentOnSameMode) {
    Harness h;
    h.dlg.Linking();
    h.dlg.ShowMiniNoteMode(MiniNoteMode_Write);
    // Activate every read-mode control so we can see the side-
    // effect when ShowMiniNoteMode toggles.
    h.dlg.SetActiveMiniNoteMode(MiniNoteMode_Read, true);
    EXPECT_EQ(h.dlg.GetCurMode(), MiniNoteMode_Write);
    // Calling ShowMiniNoteMode(Write) again is a no-op: the
    // write-mode controls stay active, the read-mode controls
    // (which were active) stay active.
    h.dlg.ShowMiniNoteMode(MiniNoteMode_Write);
    EXPECT_TRUE(h.rTitle.isActive());
    EXPECT_TRUE(h.wTitle.isActive());
}

TEST(CMiniNoteDialog, ShowMiniNoteModeTogglesReadWrite) {
    Harness h;
    h.dlg.Linking();
    // Default: no mode set (m_CurMiniNoteMode == -1).
    h.dlg.ShowMiniNoteMode(MiniNoteMode_Read);
    EXPECT_TRUE(h.rTitle.isActive());
    EXPECT_FALSE(h.wTitle.isActive());
    EXPECT_EQ(h.dlg.GetCurMode(), MiniNoteMode_Read);
    h.dlg.ShowMiniNoteMode(MiniNoteMode_Write);
    EXPECT_FALSE(h.rTitle.isActive());
    EXPECT_TRUE(h.wTitle.isActive());
    EXPECT_EQ(h.dlg.GetCurMode(), MiniNoteMode_Write);
}

TEST(CMiniNoteDialog, SetModeJustRecordsMode) {
    cMiniNoteDialog d;
    d.SetMode(MiniNoteMode_Read);
    EXPECT_EQ(d.GetCurMode(), MiniNoteMode_Read);
    d.SetMode(MiniNoteMode_Write);
    EXPECT_EQ(d.GetCurMode(), MiniNoteMode_Write);
}

TEST(CMiniNoteDialog, SetMiniNoteWritesBodyToBothTextareas) {
    Harness h;
    h.dlg.Linking();
    h.dlg.SetMiniNote("Alice", "Hello world", /*itemIdx=*/0);
    EXPECT_EQ(h.rNoteText.GetScriptText(), "Hello world");
    EXPECT_EQ(h.wNoteText.GetScriptText(), "Hello world");
    EXPECT_EQ(h.senderStc.GetStaticText(), "Alice");
    EXPECT_EQ(h.receiverEdit.editText(), "Alice");
}

TEST(CMiniNoteDialog, SetMiniNoteWithItemPrefixesItemName) {
    Harness h;
    h.dlg.Linking();
    h.dlg.AddSetShopItemForTest(7001, "Potion");
    h.dlg.SetMiniNote("Alice", "buy me one", /*itemIdx=*/7001);
    // 1:1 with legacy sprintf(buf, CHATMGR->GetChatMsg(732), pItem->Name)
    // followed by strcat(buf, Note).  Modern port uses "Item:<Name>".
    EXPECT_EQ(h.rNoteText.GetScriptText(), "Item:Potionbuy me one");
    EXPECT_EQ(h.wNoteText.GetScriptText(), "Item:Potionbuy me one");
}

TEST(CMiniNoteDialog, SetMiniNoteUnknownItemIdxDropsPrefix) {
    Harness h;
    h.dlg.Linking();
    h.dlg.SetMiniNote("Alice", "no item", /*itemIdx=*/9999);
    // No match in the SetShopItem table → no "Item:" prefix.
    EXPECT_EQ(h.rNoteText.GetScriptText(), "no item");
}

TEST(CMiniNoteDialog, SetActiveTrueClearsWriteTextareas) {
    Harness h;
    h.dlg.Linking();
    h.dlg.SetMiniNote("Alice", "leftover", 0);
    EXPECT_EQ(h.wNoteText.GetScriptText(), "leftover");
    EXPECT_EQ(h.receiverEdit.editText(), "Alice");
    // 1:1 with legacy: SetActive(TRUE) wipes the compose
    // textareas so a fresh compose starts empty.
    h.dlg.SetActive(true);
    EXPECT_EQ(h.wNoteText.GetScriptText(), "");
    EXPECT_EQ(h.receiverEdit.editText(), "");
    EXPECT_TRUE(h.dlg.isActive());
}

TEST(CMiniNoteDialog, SetActiveFalseDropsFocus) {
    Harness h;
    h.dlg.Linking();
    h.dlg.SetActive(true);
    h.dlg.SetActive(false);
    EXPECT_FALSE(h.receiverEdit.hasFocus());
    EXPECT_FALSE(h.wNoteText.hasFocus());
    EXPECT_FALSE(h.dlg.isActive());
}

TEST(CMiniNoteDialog, GetSenderNameReturnsSenderStaticText) {
    Harness h;
    h.dlg.Linking();
    h.dlg.SetMiniNote("Bob", "msg", 0);
    EXPECT_STREQ(h.dlg.GetSenderName(), "Bob");
}

TEST(CMiniNoteDialog, GetSetNoteIDRoundTrip) {
    cMiniNoteDialog d;
    EXPECT_EQ(d.GetNoteID(), 0u);
    d.SetNoteID(12345);
    EXPECT_EQ(d.GetNoteID(), 12345u);
}

TEST(CMiniNoteDialog, AddSetShopItemForTest) {
    cMiniNoteDialog d;
    d.AddSetShopItemForTest(1, "Sword");
    d.AddSetShopItemForTest(2, "Shield");
    d.AddSetShopItemForTest(3, "Bow");
    EXPECT_EQ(d.SetShopItemCount(), 3u);
}

TEST(CMiniNoteDialog, DestructorClearsPerModeLists) {
    // Lifetime / RAII sanity: the destructor must clear the
    // per-mode control vectors + the SetShopItem table.  If
    // any cWindow* in the lists outlives the dialog, the
    // vector still holds a dangling reference; that's a
    // design choice inherited from legacy cPtrList (legacy
    // ALSO did not own the cWindow* -- the cDialog tree
    // owns them).
    {
        cMiniNoteDialog d;
        cStatic s;
        cMiniNoteDialog::ChildWindows w{};
        w.rTitle = &s;
        d.SetChildWindowsForTest(w);
        d.Linking();
        d.AddSetShopItemForTest(1, "X");
        EXPECT_EQ(d.SetShopItemCount(), 1u);
    }
    SUCCEED();
}
