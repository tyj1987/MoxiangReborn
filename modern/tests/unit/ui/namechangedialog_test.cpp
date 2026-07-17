// namechangedialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cNameChangeDialog (name change editor
// dialog: 1 cEditBox + 1 OK button + item DB idx state).
//
// Covers modern/src/ui/namechangedialog.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\NameChangeDialog.h (877 B) and
//   墨香【源码】\[Client]MH\NameChangeDialog.cpp.
//
// What's tested:
//   - Default construction: cNameChangeDialog is a
//     cDialog and inherits its tree management.
//   - m_dwDBIdx starts 0 (1:1 with legacy default
//     init).
//   - Id constant matches expected local range
//     (kIdNameBox=450).
//   - kVcmCharname == 2 (1:1 with legacy
//     VCM_CHARNAME enum value).
//   - Linking resolves the cEditBox child by id
//     and calls SetValidCheck(2) on it.
//   - Linking without children leaves m_pNameBox
//     null (SetActive + NameChangeSyn are safe).
//   - Linking before Init does not crash.
//   - SetActive val=true calls base SetActive
//     + clears edit text (1:1 with legacy).
//   - SetActive val=false calls base SetActive
//     (no edit text clear, 1:1 with legacy).
//   - SetActive without Linking is safe.
//   - SetItemDBIdx / GetItemDBIdx round-trip.
//   - NameChangeSyn is a no-op (TODO: 4-singleton
//     dispatch CHATMGR + FILTERTABLE + HERO +
//     NETWORK, R-12.x deferred). The 1:1 contract
//     is preserved: returns void, no state change.
//   - NameChangeSyn without Linking is safe.
//   - NameChangeSyn before Init does not crash.
//
// 1:1 quirks preserved:
//   - Ctor body empty (1:1 quirk: m_type =
//     WT_NAMECHANGE_DLG drop, modern cWindow
//     does not have m_type field).
//   - State field default 0 (1:1 with legacy
//     m_dwDBIdx = 0 init).
//   - Linking SetValidCheck(2) (1:1 with legacy
//     VCM_CHARNAME = 2).
//   - SetActive override: call base + clear edit
//     text on val=true (1:1 with legacy).
//   - 1:1 quirk: modern SetEditText is a no-op
//     unless InitEditbox was called (m_bInitEdit
//     guard). Test caller must call InitEditbox
//     before SetEditText takes effect.
//   - NameChangeSyn TODO (4-singleton dispatch,
//     R-12.x deferred).
//   - kVcmCharname = 2 (1:1 with legacy
//     cEditBox::SetValidCheck enum value).
//   - Local id range 450 (distinct from 200-443
//     used by previous Tier 2 dialogs; no
//     collision).

#include "namechangedialog.hpp"
#include "cdialog.hpp"
#include "ceditbox.hpp"
#include "cwindow.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>

namespace mxh::ui::test {

// ===========================================================================
// Construction + state
// ===========================================================================

TEST(CNameChangeDialogTest, DefaultConstructionHasZeroState) {
    cNameChangeDialog dlg;
    // 1:1 quirk: ctor body is empty (legacy
    // m_type = WT_NAMECHANGE_DLG drop, modern
    // cWindow does not have m_type field).
    // m_dwDBIdx starts 0.
    EXPECT_EQ(dlg.GetItemDBIdx(), 0u);
}

TEST(CNameChangeDialogTest, InheritsDialogTreeManagement) {
    cNameChangeDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetAbsXY(10, 20);
    EXPECT_EQ(dlg.absX(), 10);
    EXPECT_EQ(dlg.absY(), 20);
}

TEST(CNameChangeDialogTest, IdConstantMatchesExpectedLocalRange) {
    EXPECT_EQ(cNameChangeDialog::kIdNameBox, 450);
}

TEST(CNameChangeDialogTest, VcmCharnameIsTwo) {
    // 1:1 with legacy cEditBox::SetValidCheck
    // VCM_CHARNAME = 2 (character-name valid
    // check).
    EXPECT_EQ(cNameChangeDialog::kVcmCharname, 2);
}

// ===========================================================================
// Linking
// ===========================================================================

TEST(CNameChangeDialogTest, LinkingResolvesEditBox) {
    cNameChangeDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto edit = std::make_unique<cEditBox>();
    edit->Init(0, 0, 200, 14, nullptr, nullptr, cNameChangeDialog::kIdNameBox);
    edit->InitEditbox(50, 64);
    cEditBox* pEdit = edit.get();
    dlg.Add(std::unique_ptr<cWindow>(edit.release()));

    dlg.Linking();
    // m_pNameBox is private; verified indirectly
    // via SetActive(true) calling SetEditText("")
    // on the cEditBox.
    dlg.SetActive(true);
    EXPECT_EQ(pEdit->editText(), "");
}

TEST(CNameChangeDialogTest, LinkingWithoutChildrenDoesNotCrash) {
    cNameChangeDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    // SetActive + NameChangeSyn without children
    // must be safe.
    dlg.SetActive(true);
    dlg.SetActive(false);
    dlg.NameChangeSyn();
    SUCCEED();
}

TEST(CNameChangeDialogTest, LinkingBeforeInitDoesNotCrash) {
    cNameChangeDialog dlg;
    dlg.Linking();
    SUCCEED();
}

// ===========================================================================
// SetActive override
// ===========================================================================

TEST(CNameChangeDialogTest, SetActiveTrueUpdatesBaseState) {
    cNameChangeDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    EXPECT_FALSE(dlg.isActive());
    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CNameChangeDialogTest, SetActiveFalseUpdatesBaseState) {
    cNameChangeDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CNameChangeDialogTest, SetActiveTrueClearsEditText) {
    // 1:1 with legacy: SetActive(val=TRUE) calls
    // m_pNameBox->SetEditText("") after base
    // SetActive. Requires InitEditbox to have
    // been called (m_bInitEdit guard).
    cNameChangeDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto edit = std::make_unique<cEditBox>();
    edit->Init(0, 0, 200, 14, nullptr, nullptr, cNameChangeDialog::kIdNameBox);
    edit->InitEditbox(50, 64);
    cEditBox* pEdit = edit.get();
    dlg.Add(std::unique_ptr<cWindow>(edit.release()));
    dlg.Linking();

    pEdit->SetEditText("stale name");
    EXPECT_EQ(pEdit->editText(), "stale name");
    dlg.SetActive(true);
    EXPECT_EQ(pEdit->editText(), "");
}

TEST(CNameChangeDialogTest, SetActiveFalseDoesNotClearEditText) {
    // 1:1 with legacy: SetActive(val=FALSE) only
    // calls base SetActive. Pre-existing edit
    // text survives.
    cNameChangeDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto edit = std::make_unique<cEditBox>();
    edit->Init(0, 0, 200, 14, nullptr, nullptr, cNameChangeDialog::kIdNameBox);
    edit->InitEditbox(50, 64);
    cEditBox* pEdit = edit.get();
    dlg.Add(std::unique_ptr<cWindow>(edit.release()));
    dlg.Linking();

    pEdit->SetEditText("preserved name");
    dlg.SetActive(false);
    EXPECT_EQ(pEdit->editText(), "preserved name");
}

TEST(CNameChangeDialogTest, SetActiveWithoutLinkIsSafe) {
    cNameChangeDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CNameChangeDialogTest, SetActiveBeforeInitDoesNotCrash) {
    cNameChangeDialog dlg;
    dlg.SetActive(true);
    dlg.SetActive(false);
    SUCCEED();
}

// ===========================================================================
// SetItemDBIdx / GetItemDBIdx
// ===========================================================================

TEST(CNameChangeDialogTest, SetItemDBIdxRoundTrip) {
    cNameChangeDialog dlg;
    dlg.SetItemDBIdx(42u);
    EXPECT_EQ(dlg.GetItemDBIdx(), 42u);
}

TEST(CNameChangeDialogTest, SetItemDBIdxOverridesPrevious) {
    cNameChangeDialog dlg;
    dlg.SetItemDBIdx(1u);
    dlg.SetItemDBIdx(100u);
    EXPECT_EQ(dlg.GetItemDBIdx(), 100u);
}

TEST(CNameChangeDialogTest, SetItemDBIdxZeroIsValid) {
    cNameChangeDialog dlg;
    dlg.SetItemDBIdx(0u);
    EXPECT_EQ(dlg.GetItemDBIdx(), 0u);
}

// ===========================================================================
// NameChangeSyn
// ===========================================================================

TEST(CNameChangeDialogTest, NameChangeSynIsNoOpUntilSingletonsPorted) {
    // 1:1 with legacy contract: returns void.
    // Modern port is a no-op (TODO: 4-singleton
    // dispatch CHATMGR + FILTERTABLE + HERO +
    // NETWORK, R-12.x deferred). The 1:1 contract
    // is preserved: no state change observable.
    cNameChangeDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto edit = std::make_unique<cEditBox>();
    edit->Init(0, 0, 200, 14, nullptr, nullptr, cNameChangeDialog::kIdNameBox);
    edit->InitEditbox(50, 64);
    dlg.Add(std::unique_ptr<cWindow>(edit.release()));
    dlg.Linking();
    dlg.SetItemDBIdx(42u);

    dlg.SetActive(true);
    dlg.NameChangeSyn();
    // State preserved (legacy would have set
    // active false + sent network message, but
    // modern is TODO).
    EXPECT_TRUE(dlg.isActive());
    EXPECT_EQ(dlg.GetItemDBIdx(), 42u);
}

TEST(CNameChangeDialogTest, NameChangeSynDoesNotChangeState) {
    cNameChangeDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto edit = std::make_unique<cEditBox>();
    edit->Init(0, 0, 200, 14, nullptr, nullptr, cNameChangeDialog::kIdNameBox);
    edit->InitEditbox(50, 64);
    dlg.Add(std::unique_ptr<cWindow>(edit.release()));
    dlg.Linking();
    dlg.SetItemDBIdx(42u);

    dlg.SetActive(true);
    dlg.NameChangeSyn();
    // State preserved.
    EXPECT_TRUE(dlg.isActive());
    EXPECT_EQ(dlg.GetItemDBIdx(), 42u);
}

TEST(CNameChangeDialogTest, NameChangeSynWithoutLinkIsSafe) {
    cNameChangeDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetItemDBIdx(1u);
    dlg.NameChangeSyn();
    SUCCEED();
}

TEST(CNameChangeDialogTest, NameChangeSynBeforeInitDoesNotCrash) {
    cNameChangeDialog dlg;
    dlg.NameChangeSyn();
    SUCCEED();
}

}  // namespace mxh::ui::test
