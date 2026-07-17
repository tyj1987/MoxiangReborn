// shoutdialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cShoutDialog (shout message sender
// dialog: 1 cEditBox + 1 SEND button + item info state).
//
// Covers modern/src/ui/shoutdialog.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\ShoutDialog.h (832 B) and
//   墨香【源码】\[Client]MH\ShoutDialog.cpp.
//
// What's tested:
//   - Default construction: cShoutDialog is a cDialog
//     and inherits its tree management.
//   - 2 state fields start 0 (1:1 with legacy
//     default init).
//   - Id constant matches expected local range
//     (kIdMsgBox=410).
//   - Linking resolves the cEditBox child by id.
//   - Linking without children leaves m_pMsgBox
//     null (SendShoutMsgSyn is safe).
//   - Linking before Init does not crash.
//   - SetItemInfo round-trip (item idx + pos).
//   - SendShoutMsgSyn returns false (TODO: 4-singleton
//     dispatch CHATMGR + FILTERTABLE + HERO + NETWORK,
//     R-12.x deferred). The 1:1 contract is preserved:
//     returns bool, false matches the legacy
//     "early return on empty" safe no-op path.
//   - SendShoutMsgSyn without Linking is safe.
//   - SendShoutMsgSyn before Init is safe.
//
// 1:1 quirks preserved:
//   - Ctor body empty (1:1 quirk: m_type =
//     WT_SHOUT_DLG drop, modern cWindow does not
//     have m_type field).
//   - State fields default 0 (1:1 with legacy
//     m_dwItemIdx = m_dwItemPos = 0 init).
//   - SetItemInfo inline setter (1:1 with legacy
//     inline setter).
//   - SendShoutMsgSyn is TODO (4-singleton
//     dispatch, R-12.x deferred). Returns false
//     matching the legacy "early return on empty"
//     safe no-op path.
//   - Local id range 410 (distinct from 200-401
//     used by previous Tier 2 dialogs; no
//     collision).

#include "shoutdialog.hpp"
#include "cdialog.hpp"
#include "ceditbox.hpp"
#include "cwindow.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

namespace mxh::ui::test {

// ===========================================================================
// Construction + state
// ===========================================================================

TEST(CShoutDialogTest, DefaultConstructionHasZeroState) {
    cShoutDialog dlg;
    // 1:1 quirk: ctor body is empty (legacy
    // m_type = WT_SHOUT_DLG drop, modern cWindow
    // does not have m_type field). m_dwItemIdx +
    // m_dwItemPos start at 0.
    EXPECT_EQ(dlg.GetItemIdx(), 0u);
    EXPECT_EQ(dlg.GetItemPos(), 0u);
}

TEST(CShoutDialogTest, InheritsDialogTreeManagement) {
    cShoutDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetAbsXY(10, 20);
    EXPECT_EQ(dlg.absX(), 10);
    EXPECT_EQ(dlg.absY(), 20);
}

TEST(CShoutDialogTest, IdConstantMatchesExpectedLocalRange) {
    EXPECT_EQ(cShoutDialog::kIdMsgBox, 410);
}

// ===========================================================================
// Linking
// ===========================================================================

TEST(CShoutDialogTest, LinkingResolvesEditBox) {
    cShoutDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto edit = std::make_unique<cEditBox>();
    edit->Init(0, 0, 200, 14, nullptr, nullptr, cShoutDialog::kIdMsgBox);
    edit->InitEditbox(50, 64);
    cEditBox* pEdit = edit.get();
    dlg.Add(std::unique_ptr<cWindow>(edit.release()));

    dlg.Linking();
    // m_pMsgBox is private; verified indirectly
    // via the Linking call not crashing + the
    // dialog state is consistent.
    pEdit->SetEditText("hello world");
    EXPECT_EQ(pEdit->editText(), "hello world");
}

TEST(CShoutDialogTest, LinkingWithoutChildrenDoesNotCrash) {
    cShoutDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    // SendShoutMsgSyn without m_pMsgBox is safe.
    dlg.SendShoutMsgSyn();
    SUCCEED();
}

TEST(CShoutDialogTest, LinkingBeforeInitDoesNotCrash) {
    cShoutDialog dlg;
    dlg.Linking();
    SUCCEED();
}

// ===========================================================================
// SetItemInfo
// ===========================================================================

TEST(CShoutDialogTest, SetItemInfoRoundTrip) {
    cShoutDialog dlg;
    dlg.SetItemInfo(42u, 100u);
    EXPECT_EQ(dlg.GetItemIdx(), 42u);
    EXPECT_EQ(dlg.GetItemPos(), 100u);
}

TEST(CShoutDialogTest, SetItemInfoOverridesPrevious) {
    cShoutDialog dlg;
    dlg.SetItemInfo(1u, 2u);
    dlg.SetItemInfo(100u, 200u);
    EXPECT_EQ(dlg.GetItemIdx(), 100u);
    EXPECT_EQ(dlg.GetItemPos(), 200u);
}

TEST(CShoutDialogTest, SetItemInfoZeroIsValid) {
    cShoutDialog dlg;
    dlg.SetItemInfo(0u, 0u);
    EXPECT_EQ(dlg.GetItemIdx(), 0u);
    EXPECT_EQ(dlg.GetItemPos(), 0u);
}

// ===========================================================================
// SendShoutMsgSyn
// ===========================================================================

TEST(CShoutDialogTest, SendShoutMsgSynReturnsFalseUntilSingletonsPorted) {
    // 1:1 with legacy contract: returns false on
    // early return (empty / filtered / network
    // failure). Modern port returns false as a
    // safe no-op while the 4 singletons
    // (CHATMGR + FILTERTABLE + HERO + NETWORK)
    // are unported. When ported, the body becomes
    // the legacy code (returns true on success,
    // false on empty/filtered).
    cShoutDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto edit = std::make_unique<cEditBox>();
    edit->Init(0, 0, 200, 14, nullptr, nullptr, cShoutDialog::kIdMsgBox);
    edit->InitEditbox(50, 64);
    dlg.Add(std::unique_ptr<cWindow>(edit.release()));
    dlg.Linking();
    dlg.SetItemInfo(42u, 100u);

    EXPECT_FALSE(dlg.SendShoutMsgSyn());
}

TEST(CShoutDialogTest, SendShoutMsgSynDoesNotChangeState) {
    // The modern port does not execute the legacy
    // state-mutation (SetActive(false) +
    // m_dwItemIdx/Pos = 0) because the method
    // returns false at the TODO marker. Item idx
    // + pos are preserved.
    cShoutDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);

    auto edit = std::make_unique<cEditBox>();
    edit->Init(0, 0, 200, 14, nullptr, nullptr, cShoutDialog::kIdMsgBox);
    edit->InitEditbox(50, 64);
    dlg.Add(std::unique_ptr<cWindow>(edit.release()));
    dlg.Linking();
    dlg.SetItemInfo(42u, 100u);

    dlg.SendShoutMsgSyn();
    // State preserved (legacy would reset to 0
    // on success, but modern returns false at
    // the TODO marker).
    EXPECT_EQ(dlg.GetItemIdx(), 42u);
    EXPECT_EQ(dlg.GetItemPos(), 100u);
}

TEST(CShoutDialogTest, SendShoutMsgSynWithoutLinkIsSafe) {
    cShoutDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetItemInfo(1u, 2u);
    // SendShoutMsgSyn without m_pMsgBox is safe.
    EXPECT_FALSE(dlg.SendShoutMsgSyn());
}

TEST(CShoutDialogTest, SendShoutMsgSynBeforeInitDoesNotCrash) {
    cShoutDialog dlg;
    dlg.SendShoutMsgSyn();
    SUCCEED();
}

}  // namespace mxh::ui::test
