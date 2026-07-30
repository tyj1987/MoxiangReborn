//
// Unit tests for mxh::ui::cMiniFriendDialog (Phase C dialog port).
//
// Locks down the 1:1 surface:
//  * Constants: kIdName=700, kIdNameEdit=701,
//    kIdAddOkBtn=702, kIdAddCancelBtn=703,
//    kValidCheckCharName=1
//  * Default construction: all 4 children null, not disabled
//  * SetChildrenForTest stores pointers
//  * SetName("X") sets the edit box text
//  * SetName("") is allowed
//  * SetName(nullptr) is a safe no-op (text remains)
//  * SetActive(true) clears the edit box text
//  * SetActive(false) preserves the edit box text
//  * SetDisabled(true) blocks SetActive toggle
//  * SetDisabled(false) restores normal SetActive behaviour
//  * Linking is a no-op (host injects children first)
//  * NonCopyable
//

#include "mxh/ui/cminifrienddialog.hpp"
#include "mxh/ui/cbutton.hpp"
#include "mxh/ui/ceditbox.hpp"
#include "mxh/ui/cstatic.hpp"

#include <gtest/gtest.h>

#include <type_traits>

using mxh::ui::cButton;
using mxh::ui::cEditBox;
using mxh::ui::cMiniFriendDialog;
using mxh::ui::cStatic;

namespace {

struct Harness {
    cMiniFriendDialog dlg;
    cStatic  name;
    cEditBox nameEdit;
    cButton  addOk;
    cButton  addCancel;

    Harness() {
        dlg.SetChildrenForTest(&name, &nameEdit, &addOk, &addCancel);
        // cEditBox::SetEditText refuses to mutate the buffer before
        // InitEditbox is called.  Configure a 64-byte buffer so the
        // harness can use SetEditText like a real edit box.
        nameEdit.InitEditbox(0, 64);
    }
};

}  // namespace


TEST(CMiniFriendDialog, ConstantsMatchLegacy) {
    EXPECT_EQ(cMiniFriendDialog::kIdName,         700);
    EXPECT_EQ(cMiniFriendDialog::kIdNameEdit,     701);
    EXPECT_EQ(cMiniFriendDialog::kIdAddOkBtn,     702);
    EXPECT_EQ(cMiniFriendDialog::kIdAddCancelBtn, 703);
    EXPECT_EQ(cMiniFriendDialog::kValidCheckCharName, 1);
}

TEST(CMiniFriendDialog, DefaultConstructionHasNullChildren) {
    cMiniFriendDialog d;
    EXPECT_EQ(d.GetNameForTest(),         nullptr);
    EXPECT_EQ(d.GetNameEditForTest(),     nullptr);
    EXPECT_EQ(d.GetAddOkBtnForTest(),     nullptr);
    EXPECT_EQ(d.GetAddCancelBtnForTest(), nullptr);
    EXPECT_FALSE(d.IsDisabled());
}

TEST(CMiniFriendDialog, SetChildrenStoresPointers) {
    cMiniFriendDialog d;
    cStatic  n; cEditBox ne; cButton ok, cancel;
    d.SetChildrenForTest(&n, &ne, &ok, &cancel);
    EXPECT_EQ(d.GetNameForTest(),         &n);
    EXPECT_EQ(d.GetNameEditForTest(),     &ne);
    EXPECT_EQ(d.GetAddOkBtnForTest(),     &ok);
    EXPECT_EQ(d.GetAddCancelBtnForTest(), &cancel);
}


TEST(CMiniFriendDialog, LinkingIsNoOpWithInjectedChildren) {
    Harness h;
    h.dlg.Linking();
    EXPECT_EQ(h.dlg.GetNameForTest(),         &h.name);
    EXPECT_EQ(h.dlg.GetNameEditForTest(),     &h.nameEdit);
    EXPECT_EQ(h.dlg.GetAddOkBtnForTest(),     &h.addOk);
    EXPECT_EQ(h.dlg.GetAddCancelBtnForTest(), &h.addCancel);
}


TEST(CMiniFriendDialog, SetNamePopulatesEditText) {
    Harness h;
    h.dlg.SetName("Alice");
    EXPECT_EQ(h.nameEdit.editText(), "Alice");
}

TEST(CMiniFriendDialog, SetNameWithEmptyStringClearsText) {
    Harness h;
    h.dlg.SetName("Alice");
    h.dlg.SetName("");
    EXPECT_EQ(h.nameEdit.editText(), "");
}

TEST(CMiniFriendDialog, SetNameWithNullLeavesTextUnchanged) {
    Harness h;
    h.dlg.SetName("Alice");
    h.dlg.SetName(nullptr);
    EXPECT_EQ(h.nameEdit.editText(), "Alice");
}


TEST(CMiniFriendDialog, SetActiveTrueClearsEditText) {
    Harness h;
    h.nameEdit.SetEditText("pre-filled");
    h.dlg.SetActive(true);
    EXPECT_TRUE(h.dlg.isActive());
    EXPECT_EQ(h.nameEdit.editText(), "");
}

TEST(CMiniFriendDialog, SetActiveFalsePreservesEditText) {
    Harness h;
    h.dlg.SetActive(true);
    h.nameEdit.SetEditText("Alice");
    h.dlg.SetActive(false);
    EXPECT_FALSE(h.dlg.isActive());
    EXPECT_EQ(h.nameEdit.editText(), "Alice");
}

TEST(CMiniFriendDialog, SetActiveTrueTwiceKeepsTextEmpty) {
    Harness h;
    h.dlg.SetActive(true);
    h.dlg.SetActive(true);
    EXPECT_TRUE(h.dlg.isActive());
    EXPECT_EQ(h.nameEdit.editText(), "");
}


TEST(CMiniFriendDialog, SetDisabledBlocksSetActive) {
    Harness h;
    h.dlg.SetDisabled(true);
    h.dlg.SetActive(true);
    EXPECT_FALSE(h.dlg.isActive());
    h.dlg.SetActive(false);
    EXPECT_FALSE(h.dlg.isActive());
}

TEST(CMiniFriendDialog, SetDisabledFalseRestoresSetActive) {
    Harness h;
    h.dlg.SetDisabled(true);
    h.dlg.SetDisabled(false);
    h.dlg.SetActive(true);
    EXPECT_TRUE(h.dlg.isActive());
}


TEST(CMiniFriendDialog, NonCopyable) {
    static_assert(!std::is_copy_constructible<cMiniFriendDialog>::value,
                  "cMiniFriendDialog must not be copyable");
    static_assert(!std::is_copy_assignable<cMiniFriendDialog>::value,
                  "cMiniFriendDialog must not be copy-assignable");
    SUCCEED();
}
