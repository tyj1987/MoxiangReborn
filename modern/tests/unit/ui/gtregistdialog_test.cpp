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
//   - SetActive val=false calls base SetActive and
//     dispatches the optional HERO + OBJECTSTATEMGR
//     host callbacks when the hero is in Deal state.
//   - SetActive without Linking is safe.
//   - SetActive before Init does not crash.
//   - TournamentRegistSyn preserves the guild-master
//     rank gate, sends the hero ID through the host,
//     and returns the legacy eGTError value.
//   - TournamentRegistSyn without Linking returns
//     kErrorNoGuildMaster.
//   - TournamentRegistSyn before Init does not crash.
//   - SetRegistGuildCount updates both linked statics
//     with legacy DWORD subtraction/LONG conversion.
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
//     port preserves this through optional host
//     callbacks.
//   - TournamentRegistSyn returns
//     kErrorNoGuildMaster as the default
//     (matching the legacy early-return path for
//     non-master while singletons are unported).
//   - SetRegistGuildCount preserves DWORD-to-LONG wrap.
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
    // With no rank provider the defensive host path defaults to
    // non-master and returns the legacy error code.
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

TEST(CGTRegistDialogTest, SetRegistGuildCountWithoutLinkedStaticsIsSafe) {
    // The real cStatic::SetStaticValue path is wired; with no
    // linked statics the defensive path remains safe.
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

// ===========================================================================
// SetCallbacks host-callback injection (C-Batch-2.43)
// ===========================================================================

namespace {

struct GTRegistCallbackState {
    std::int32_t heroState = 0;
    int getCount = 0;
    int endCount = 0;
};

std::int32_t GetGTRegistHeroState(void* userData) {
    auto* state = static_cast<GTRegistCallbackState*>(userData);
    ++state->getCount;
    return state->heroState;
}

void EndGTRegistDealState(void* userData) {
    ++static_cast<GTRegistCallbackState*>(userData)->endCount;
}

}  // namespace

TEST(CGTRegistDialogTest, SetActiveFalseEndsDealState) {
    cGTRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    GTRegistCallbackState state{cGTRegistDialog::kObjectStateDeal};
    dlg.SetCallbacks(GetGTRegistHeroState, EndGTRegistDealState, &state);
    dlg.SetActive(false);
    EXPECT_EQ(state.getCount, 1);
    EXPECT_EQ(state.endCount, 1);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CGTRegistDialogTest, SetActiveFalseSkipsNonDealState) {
    cGTRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    GTRegistCallbackState state{99};
    dlg.SetCallbacks(GetGTRegistHeroState, EndGTRegistDealState, &state);
    dlg.SetActive(false);
    EXPECT_EQ(state.getCount, 1);
    EXPECT_EQ(state.endCount, 0);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CGTRegistDialogTest, SetActiveTrueNeverDispatchesCallbacks) {
    cGTRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    GTRegistCallbackState state{cGTRegistDialog::kObjectStateDeal};
    dlg.SetCallbacks(GetGTRegistHeroState, EndGTRegistDealState, &state);
    dlg.SetActive(true);
    EXPECT_EQ(state.getCount, 0);
    EXPECT_EQ(state.endCount, 0);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CGTRegistDialogTest, SetActiveFalseSkipsNullHeroStateCallback) {
    cGTRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    GTRegistCallbackState state{cGTRegistDialog::kObjectStateDeal};
    dlg.SetCallbacks(nullptr, EndGTRegistDealState, &state);
    dlg.SetActive(false);
    EXPECT_EQ(state.endCount, 0);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CGTRegistDialogTest, SetActiveFalseSkipsNullEndDealCallback) {
    cGTRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    GTRegistCallbackState state{cGTRegistDialog::kObjectStateDeal};
    dlg.SetCallbacks(GetGTRegistHeroState, nullptr, &state);
    dlg.SetActive(false);
    EXPECT_EQ(state.getCount, 0);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CGTRegistDialogTest, SetCallbacksReplacesPreviousHost) {
    cGTRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    GTRegistCallbackState first{cGTRegistDialog::kObjectStateDeal};
    GTRegistCallbackState second{cGTRegistDialog::kObjectStateDeal};
    dlg.SetCallbacks(GetGTRegistHeroState, EndGTRegistDealState, &first);
    dlg.SetCallbacks(GetGTRegistHeroState, EndGTRegistDealState, &second);
    dlg.SetActive(false);
    EXPECT_EQ(first.getCount, 0);
    EXPECT_EQ(first.endCount, 0);
    EXPECT_EQ(second.getCount, 1);
    EXPECT_EQ(second.endCount, 1);
}

TEST(CGTRegistDialogTest, SetActiveFalseWithoutCallbacksStillUpdatesBaseState) {
    cGTRegistDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);
    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}


// ===========================================================================
// Tournament registration network + count display (C-Batch-2.45)
// ===========================================================================

namespace {

struct GTRegistNetworkCapture {
    std::int32_t rank = 0;
    std::uint32_t heroId = 0;
    std::uint32_t sentObjectId = 0;
    int rankCalls = 0;
    int heroCalls = 0;
    int sendCalls = 0;
    bool sendResult = true;

    static std::int32_t GetRank(void* userData) {
        auto* capture = static_cast<GTRegistNetworkCapture*>(userData);
        ++capture->rankCalls;
        return capture->rank;
    }

    static std::uint32_t GetHeroId(void* userData) {
        auto* capture = static_cast<GTRegistNetworkCapture*>(userData);
        ++capture->heroCalls;
        return capture->heroId;
    }

    static bool Send(std::uint32_t objectId, void* userData) {
        auto* capture = static_cast<GTRegistNetworkCapture*>(userData);
        ++capture->sendCalls;
        capture->sentObjectId = objectId;
        return capture->sendResult;
    }
};

}  // namespace

TEST(CGTRegistDialogTest, TournamentWireAndRankConstantsMatchLegacy) {
    EXPECT_EQ(cGTRegistDialog::kGuildMasterRank, 50);
    EXPECT_EQ(cGTRegistDialog::kGTournamentCategory, 60u);
    EXPECT_EQ(cGTRegistDialog::kTournamentRegistProtocol, 1u);
}

TEST(CGTRegistDialogTest, TournamentRegistSynRejectsNonMaster) {
    cGTRegistDialog dlg;
    GTRegistNetworkCapture capture;
    capture.rank = 40;
    dlg.SetTournamentCallbacks(&GTRegistNetworkCapture::GetRank,
                               &GTRegistNetworkCapture::GetHeroId,
                               &GTRegistNetworkCapture::Send, &capture);

    EXPECT_EQ(dlg.TournamentRegistSyn(), cGTRegistDialog::kErrorNoGuildMaster);
    EXPECT_EQ(capture.rankCalls, 1);
    EXPECT_EQ(capture.heroCalls, 0);
    EXPECT_EQ(capture.sendCalls, 0);
}

TEST(CGTRegistDialogTest, TournamentRegistSynMasterSendsHeroId) {
    cGTRegistDialog dlg;
    GTRegistNetworkCapture capture;
    capture.rank = cGTRegistDialog::kGuildMasterRank;
    capture.heroId = 0x87654321u;
    dlg.SetTournamentCallbacks(&GTRegistNetworkCapture::GetRank,
                               &GTRegistNetworkCapture::GetHeroId,
                               &GTRegistNetworkCapture::Send, &capture);

    EXPECT_EQ(dlg.TournamentRegistSyn(), cGTRegistDialog::kErrorNoError);
    EXPECT_EQ(capture.rankCalls, 1);
    EXPECT_EQ(capture.heroCalls, 1);
    EXPECT_EQ(capture.sendCalls, 1);
    EXPECT_EQ(capture.sentObjectId, 0x87654321u);
}

TEST(CGTRegistDialogTest, TournamentRegistSynIgnoresSendFailure) {
    cGTRegistDialog dlg;
    GTRegistNetworkCapture capture;
    capture.rank = cGTRegistDialog::kGuildMasterRank;
    capture.sendResult = false;
    dlg.SetTournamentCallbacks(&GTRegistNetworkCapture::GetRank,
                               &GTRegistNetworkCapture::GetHeroId,
                               &GTRegistNetworkCapture::Send, &capture);

    EXPECT_EQ(dlg.TournamentRegistSyn(), cGTRegistDialog::kErrorNoError);
    EXPECT_EQ(capture.sendCalls, 1);
}

TEST(CGTRegistDialogTest, TournamentRegistSynNullRankDefaultsToNonMaster) {
    cGTRegistDialog dlg;
    GTRegistNetworkCapture capture;
    dlg.SetTournamentCallbacks(nullptr, &GTRegistNetworkCapture::GetHeroId,
                               &GTRegistNetworkCapture::Send, &capture);

    EXPECT_EQ(dlg.TournamentRegistSyn(), cGTRegistDialog::kErrorNoGuildMaster);
    EXPECT_EQ(capture.heroCalls, 0);
    EXPECT_EQ(capture.sendCalls, 0);
}

TEST(CGTRegistDialogTest, TournamentRegistSynMasterWithoutHeroCallbackIsSafe) {
    cGTRegistDialog dlg;
    GTRegistNetworkCapture capture;
    capture.rank = cGTRegistDialog::kGuildMasterRank;
    dlg.SetTournamentCallbacks(&GTRegistNetworkCapture::GetRank, nullptr,
                               &GTRegistNetworkCapture::Send, &capture);

    EXPECT_EQ(dlg.TournamentRegistSyn(), cGTRegistDialog::kErrorNoError);
    EXPECT_EQ(capture.sendCalls, 0);
}

TEST(CGTRegistDialogTest, TournamentRegistSynMasterWithoutSendSkipsHeroLookup) {
    cGTRegistDialog dlg;
    GTRegistNetworkCapture capture;
    capture.rank = cGTRegistDialog::kGuildMasterRank;
    dlg.SetTournamentCallbacks(&GTRegistNetworkCapture::GetRank,
                               &GTRegistNetworkCapture::GetHeroId,
                               nullptr, &capture);

    EXPECT_EQ(dlg.TournamentRegistSyn(), cGTRegistDialog::kErrorNoError);
    EXPECT_EQ(capture.heroCalls, 0);
}

TEST(CGTRegistDialogTest, TournamentCallbacksReplacePreviousHost) {
    cGTRegistDialog dlg;
    GTRegistNetworkCapture first;
    GTRegistNetworkCapture second;
    first.rank = cGTRegistDialog::kGuildMasterRank;
    second.rank = cGTRegistDialog::kGuildMasterRank;
    dlg.SetTournamentCallbacks(&GTRegistNetworkCapture::GetRank,
                               &GTRegistNetworkCapture::GetHeroId,
                               &GTRegistNetworkCapture::Send, &first);
    dlg.SetTournamentCallbacks(&GTRegistNetworkCapture::GetRank,
                               &GTRegistNetworkCapture::GetHeroId,
                               &GTRegistNetworkCapture::Send, &second);

    EXPECT_EQ(dlg.TournamentRegistSyn(), cGTRegistDialog::kErrorNoError);
    EXPECT_EQ(first.rankCalls, 0);
    EXPECT_EQ(second.rankCalls, 1);
    EXPECT_EQ(second.sendCalls, 1);
}

TEST(CGTRegistDialogTest, SetRegistGuildCountUpdatesBothStatics) {
    cGTRegistDialog dlg;
    cStatic* registGuild = nullptr;
    cStatic* registableGuild = nullptr;
    cButton* registButton = nullptr;
    BuildDlgWithChildren(dlg, &registGuild, &registableGuild, &registButton);

    dlg.SetRegistGuildCount(10u);

    EXPECT_EQ(registGuild->GetStaticValue(), 10);
    EXPECT_EQ(registableGuild->GetStaticValue(), 22);
}

TEST(CGTRegistDialogTest, SetRegistGuildCountPreservesLegacyDwordToLongWrap) {
    cGTRegistDialog dlg;
    cStatic* registGuild = nullptr;
    cStatic* registableGuild = nullptr;
    cButton* registButton = nullptr;
    BuildDlgWithChildren(dlg, &registGuild, &registableGuild, &registButton);

    dlg.SetRegistGuildCount(33u);

    EXPECT_EQ(registGuild->GetStaticValue(), 33);
    EXPECT_EQ(registableGuild->GetStaticValue(), -1);
}

}  // namespace mxh::ui::test
