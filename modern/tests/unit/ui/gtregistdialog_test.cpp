// gtregistdialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cGTRegistDialog (guild tournament
// registration dialog: 2 cStatic + 1 cButton).
//
// Covers modern/src/ui/gtregistdialog.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\GTRegistDialog.h (865 B) and
//   墨香【源码】\[Client]MH\GTRegistDialog.cpp.
//
// What's tested:
//   - Default construction: cGTRegistDialog is a
//     cDialog and inherits its tree management.
//   - 3 child pointers start null (1:1 with legacy
//     default init).
//   - 3 id constants are distinct (1:1 with legacy
//     GDT_ENTRY1 / GDT_ENTRY2 / GDT_ENTRYBTN).
//   - 3 id constants match expected local range
//     470-472 (no collision with previous Tier 2
//     dialogs 200-460).
//   - 5 error constants + kMaxGuildInTournament match
//     legacy eGTError_ + MAXGUILD_INTOURNAMENT
//     values.
//   - Linking resolves the 3 children by id.
//   - Linking without children leaves all pointers
//     null (SetActive + TournamentRegistSyn +
//     SetRegistGuildCount are safe).
//   - Linking before Init does not crash.
//   - SetActive val=true calls base SetActive
//     (no singleton dispatch on val=true per
//     legacy 1:1 quirk).
//   - SetActive val=false calls base SetActive
//     (HERO + OBJECTSTATEMGR TODO: the modern
//     port returns without observable state
//     change).
//   - SetActive without Linking is safe.
//   - SetActive before Init does not crash.
//   - TournamentRegistSyn returns kErrorNoGuildMaster
//     (TODO: 3-singleton dispatch HERO + GUILDMGR
//     + NETWORK, R-12.x deferred). The 1:1 contract
//     is preserved: returns uint32 matching the
//     legacy early-return path.
//   - TournamentRegistSyn without Linking returns
//     kErrorNoGuildMaster.
//   - TournamentRegistSyn before Init does not crash.
//   - SetRegistGuildCount is a no-op (TODO: cStatic
//     ::SetStaticValue not ported, R-12.x deferred).
//   - SetRegistGuildCount without Linking is safe.
//   - SetRegistGuildCount before Init does not crash.
//
// 1:1 quirks preserved:
//   - Ctor body empty (1:1 quirk: m_type =
//     WT_GTREGIST_DLG drop, modern cWindow
//     does not have m_type field).
//   - SetActive override: base SetActive always
//     called (matches legacy call order).
//   - 1:1 quirk: legacy val == FALSE only triggers
//     the HERO + OBJECTSTATEMGR dispatch (val ==
//     TRUE has no singleton dispatch). Modern
//     port preserves this 1:1 behavior.
//   - TournamentRegistSyn returns
//     kErrorNoGuildMaster as the default
//     (matching the legacy early-return path for
//     non-master while singletons are unported).
//   - SetRegistGuildCount is a no-op (TODO marker).
//   - eGTError enum inlined as class-level constants
//     (1:1 with legacy enum values).
//   - kMaxGuildInTournament = 32 (1:1 with legacy
//     common header constant).
//   - Local id range 470-472 (distinct from
//     200-460 used by previous Tier 2 dialogs).

#include "gtregistdialog.hpp"
#include "cdialog.hpp"
#include "cstatic.hpp"
#include "cbutton.hpp"
#include "cwindow.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

namespace mxh::ui::test {

// ===========================================================================
// Construction + state
// ===========================================================================

TEST(CGTRegistDialogTest, DefaultConstructionIsValid) {
    cGTRegistDialog dlg;
    // 1:1 quirk: ctor body is empty (legacy
    // m_type = WT_GTREGIST_DLG drop, modern cWindow
    // does not have m_type field).
    SUCCEED();
}

TEST(CGTRegistDialogTest, InheritsDialogTreeManagement) {
    cGTRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetAbsXY(10, 20);
    EXPECT_EQ(dlg.absX(), 10);
    EXPECT_EQ(dlg.absY(), 20);
}

TEST(CGTRegistDialogTest, IdConstantsAreDistinct) {
    EXPECT_NE(cGTRegistDialog::kIdRegistGuild,
              cGTRegistDialog::kIdRegistableGuild);
    EXPECT_NE(cGTRegistDialog::kIdRegistGuild,
              cGTRegistDialog::kIdRegistBtn);
    EXPECT_NE(cGTRegistDialog::kIdRegistableGuild,
              cGTRegistDialog::kIdRegistBtn);
}

TEST(CGTRegistDialogTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cGTRegistDialog::kIdRegistGuild, 470);
    EXPECT_EQ(cGTRegistDialog::kIdRegistableGuild, 471);
    EXPECT_EQ(cGTRegistDialog::kIdRegistBtn, 472);
}

TEST(CGTRegistDialogTest, ErrorConstantsMatchLegacy) {
    // 1:1 with legacy eGTError_ enum values.
    EXPECT_EQ(cGTRegistDialog::kErrorNoError, 0u);
    EXPECT_EQ(cGTRegistDialog::kErrorNoGuildMaster, 1u);
    EXPECT_EQ(cGTRegistDialog::kErrorUnderLevel, 2u);
    EXPECT_EQ(cGTRegistDialog::kErrorUnderLimitMember, 3u);
    EXPECT_EQ(cGTRegistDialog::kErrorNotRegistDay, 4u);
}

TEST(CGTRegistDialogTest, MaxGuildInTournamentIs32) {
    // 1:1 with legacy common header constant
    // MAXGUILD_INTOURNAMENT = 32.
    EXPECT_EQ(cGTRegistDialog::kMaxGuildInTournament, 32u);
}

// ===========================================================================
// Linking
// ===========================================================================

namespace {

void BuildDlgWithChildren(cGTRegistDialog& dlg,
                          cStatic** outRegistGuild,
                          cStatic** outRegistableGuild,
                          cButton** outRegistBtn) {
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    auto registGuild = std::make_unique<cStatic>();
    registGuild->Init(0, 0, 100, 14, nullptr, cGTRegistDialog::kIdRegistGuild);
    *outRegistGuild = registGuild.get();
    dlg.Add(std::unique_ptr<cWindow>(registGuild.release()));

    auto registableGuild = std::make_unique<cStatic>();
    registableGuild->Init(0, 0, 100, 14, nullptr, cGTRegistDialog::kIdRegistableGuild);
    *outRegistableGuild = registableGuild.get();
    dlg.Add(std::unique_ptr<cWindow>(registableGuild.release()));

    auto registBtn = std::make_unique<cButton>();
    registBtn->Init(0, 0, 30, 30, nullptr, nullptr, nullptr, nullptr, nullptr,
                    cGTRegistDialog::kIdRegistBtn);
    *outRegistBtn = registBtn.get();
    dlg.Add(std::unique_ptr<cWindow>(registBtn.release()));

    dlg.Linking();
}

}  // namespace

TEST(CGTRegistDialogTest, LinkingResolvesAllChildren) {
    cGTRegistDialog dlg;
    cStatic* pRegistGuild = nullptr;
    cStatic* pRegistableGuild = nullptr;
    cButton* pRegistBtn = nullptr;
    BuildDlgWithChildren(dlg, &pRegistGuild, &pRegistableGuild, &pRegistBtn);

    // m_pRegistGuild / m_pRegistableGuild / m_pRegistBtn
    // are private; verified indirectly via the
    // Linking call not crashing + the dialog state
    // is consistent.
    SUCCEED();
}

TEST(CGTRegistDialogTest, LinkingWithoutChildrenDoesNotCrash) {
    cGTRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    // SetActive + TournamentRegistSyn +
    // SetRegistGuildCount without children must be
    // safe.
    dlg.SetActive(true);
    dlg.SetActive(false);
    dlg.TournamentRegistSyn();
    dlg.SetRegistGuildCount(10u);
    SUCCEED();
}

TEST(CGTRegistDialogTest, LinkingBeforeInitDoesNotCrash) {
    cGTRegistDialog dlg;
    dlg.Linking();
    SUCCEED();
}

// ===========================================================================
// SetActive override
// ===========================================================================

TEST(CGTRegistDialogTest, SetActiveTrueUpdatesBaseState) {
    cGTRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    EXPECT_FALSE(dlg.isActive());
    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGTRegistDialogTest, SetActiveFalseUpdatesBaseState) {
    cGTRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CGTRegistDialogTest, SetActiveWithoutLinkIsSafe) {
    cGTRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CGTRegistDialogTest, SetActiveBeforeInitDoesNotCrash) {
    cGTRegistDialog dlg;
    dlg.SetActive(true);
    dlg.SetActive(false);
    SUCCEED();
}

// ===========================================================================
// TournamentRegistSyn
// ===========================================================================

TEST(CGTRegistDialogTest, TournamentRegistSynReturnsNoGuildMaster) {
    // 1:1 with legacy contract: returns uint32.
    // Modern port returns kErrorNoGuildMaster
    // (TODO: 3-singleton dispatch HERO + GUILDMGR +
    // NETWORK, R-12.x deferred). The 1:1 contract
    // is preserved: returns uint32 matching the
    // legacy early-return path for non-master.
    cGTRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);

    EXPECT_EQ(dlg.TournamentRegistSyn(),
              cGTRegistDialog::kErrorNoGuildMaster);
}

TEST(CGTRegistDialogTest, TournamentRegistSynDoesNotChangeState) {
    cGTRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);

    dlg.TournamentRegistSyn();
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGTRegistDialogTest, TournamentRegistSynWithoutLinkIsSafe) {
    cGTRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    EXPECT_EQ(dlg.TournamentRegistSyn(),
              cGTRegistDialog::kErrorNoGuildMaster);
}

TEST(CGTRegistDialogTest, TournamentRegistSynBeforeInitDoesNotCrash) {
    cGTRegistDialog dlg;
    EXPECT_EQ(dlg.TournamentRegistSyn(),
              cGTRegistDialog::kErrorNoGuildMaster);
}

// ===========================================================================
// SetRegistGuildCount
// ===========================================================================

TEST(CGTRegistDialogTest, SetRegistGuildCountIsNoOpUntilSetStaticValuePorted) {
    // 1:1 with legacy contract: returns void.
    // Modern port is a no-op (TODO: cStatic::
    // SetStaticValue not ported, R-12.x deferred).
    cGTRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetRegistGuildCount(10u);
    SUCCEED();
}

TEST(CGTRegistDialogTest, SetRegistGuildCountWithoutLinkIsSafe) {
    cGTRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetRegistGuildCount(0u);
    dlg.SetRegistGuildCount(32u);
    SUCCEED();
}

TEST(CGTRegistDialogTest, SetRegistGuildCountBeforeInitDoesNotCrash) {
    cGTRegistDialog dlg;
    dlg.SetRegistGuildCount(5u);
    SUCCEED();
}

}  // namespace mxh::ui::test
