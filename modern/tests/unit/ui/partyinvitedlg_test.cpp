// partyinvitedlg_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cPartyInviteDlg (party invitation
// dialog: 2 button + 1 cTextArea + 1 cStatic).
//
// Covers modern/src/ui/partyinvitedlg.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\PartyInviteDlg.h (812 B) and
//   墨香【源码】\[Client]MH\PartyInviteDlg.cpp.
//
// What's tested:
//   - Default construction: cPartyInviteDlg is a cDialog
//     and inherits its tree management.
//   - 4 child pointers start null (1:1 with legacy
//     default init).
//   - 4 id constants are distinct (1:1 with legacy
//     PA_INVITEDISTRIBUTE / PA_INVITER /
//     PA_INVITEOK / PA_INVITECANCEL).
//   - 4 id constants match expected local range
//     440-443 (no collision with previous Tier 2
//     dialogs 200-432).
//   - 2 option constants match legacy
//     ePartyOpt_Random / ePartyOpt_Damage (0 / 1).
//   - Linking resolves the 4 children by id.
//   - Linking without children leaves all pointers
//     null (SetMsg is safe).
//   - Linking before Init does not crash.
//   - SetMsg with kOptRandom + valid inviter updates
//     cStatic + cTextArea (1:1 with legacy sprintf).
//   - SetMsg with kOptDamage + valid inviter updates
//     cStatic + cTextArea (1:1 with legacy
//     `else if` branch).
//   - SetMsg with unknown option (neither Random
//     nor Damage) leaves cStatic empty (1:1 with
//     legacy: no `else` branch means Opt stays
//     empty).
//   - SetMsg with null inviter is safe (1:1 quirk:
//     modern port guards null; legacy would
//     crash on sprintf).
//   - SetMsg without Linking is safe.
//   - SetMsg before Init does not crash.
//
// 1:1 quirks preserved:
//   - Ctor body empty (1:1 quirk: m_type =
//     WT_PARTYINVITEDLG drop, modern cWindow
//     does not have m_type field).
//   - SetMsg uses placeholder format strings
//     "PARTY_OPT_RANDOM" / "PARTY_OPT_DAMAGE" /
//     "PARTY_INVITER_MSG_FORMAT" instead of
//     CHATMGR->GetChatMsg(640/641/305).
//   - SetMsg with null pInviter is safe (modern
//     port guards; legacy would crash).
//   - SetMsg with unknown option leaves cStatic
//     empty (1:1 with legacy: no `else` branch).
//   - kOptRandom=0 / kOptDamage=1 (1:1 with legacy
//     ePartyOpt_Random / ePartyOpt_Damage).
//   - Local id range 440-443 (distinct from
//     200-432 used by previous Tier 2 dialogs; no
//     collision).

#include "partyinvitedlg.hpp"
#include "cdialog.hpp"
#include "cbutton.hpp"
#include "cstatic.hpp"
#include "ctextarea.hpp"
#include "cwindow.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>

namespace mxh::ui::test {

// ===========================================================================
// Construction + state
// ===========================================================================

TEST(CPartyInviteDlgTest, DefaultConstructionIsValid) {
    cPartyInviteDlg dlg;
    // 1:1 quirk: ctor body is empty (legacy
    // m_type = WT_PARTYINVITEDLG drop, modern
    // cWindow does not have m_type field).
    SUCCEED();
}

TEST(CPartyInviteDlgTest, InheritsDialogTreeManagement) {
    cPartyInviteDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetAbsXY(10, 20);
    EXPECT_EQ(dlg.absX(), 10);
    EXPECT_EQ(dlg.absY(), 20);
}

TEST(CPartyInviteDlgTest, IdConstantsAreDistinct) {
    EXPECT_NE(cPartyInviteDlg::kIdDistribute,
              cPartyInviteDlg::kIdInviter);
    EXPECT_NE(cPartyInviteDlg::kIdDistribute,
              cPartyInviteDlg::kIdOk);
    EXPECT_NE(cPartyInviteDlg::kIdDistribute,
              cPartyInviteDlg::kIdCancel);
    EXPECT_NE(cPartyInviteDlg::kIdInviter,
              cPartyInviteDlg::kIdOk);
    EXPECT_NE(cPartyInviteDlg::kIdInviter,
              cPartyInviteDlg::kIdCancel);
    EXPECT_NE(cPartyInviteDlg::kIdOk,
              cPartyInviteDlg::kIdCancel);
}

TEST(CPartyInviteDlgTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cPartyInviteDlg::kIdDistribute, 440);
    EXPECT_EQ(cPartyInviteDlg::kIdInviter, 441);
    EXPECT_EQ(cPartyInviteDlg::kIdOk, 442);
    EXPECT_EQ(cPartyInviteDlg::kIdCancel, 443);
}

TEST(CPartyInviteDlgTest, OptConstantsMatchLegacy) {
    // 1:1 with legacy ePartyOpt_Random = 0 /
    // ePartyOpt_Damage = 1.
    EXPECT_EQ(cPartyInviteDlg::kOptRandom, 0);
    EXPECT_EQ(cPartyInviteDlg::kOptDamage, 1);
}

// ===========================================================================
// Linking
// ===========================================================================

namespace {

void BuildDlgWithChildren(cPartyInviteDlg& dlg,
                          cStatic** outDistribute,
                          cTextArea** outInviter,
                          cButton** outOk,
                          cButton** outCancel) {
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    auto distribute = std::make_unique<cStatic>();
    distribute->Init(0, 0, 100, 14, nullptr, cPartyInviteDlg::kIdDistribute);
    *outDistribute = distribute.get();
    dlg.Add(std::unique_ptr<cWindow>(distribute.release()));

    auto inviter = std::make_unique<cTextArea>();
    inviter->Init(0, 0, 200, 100, nullptr, cPartyInviteDlg::kIdInviter);
    inviter->InitTextArea({0, 0, 200, 100}, 256);
    *outInviter = inviter.get();
    dlg.Add(std::unique_ptr<cWindow>(inviter.release()));

    auto ok = std::make_unique<cButton>();
    ok->Init(0, 0, 30, 30, nullptr, nullptr, nullptr, nullptr, nullptr,
             cPartyInviteDlg::kIdOk);
    *outOk = ok.get();
    dlg.Add(std::unique_ptr<cWindow>(ok.release()));

    auto cancel = std::make_unique<cButton>();
    cancel->Init(0, 0, 30, 30, nullptr, nullptr, nullptr, nullptr, nullptr,
                 cPartyInviteDlg::kIdCancel);
    *outCancel = cancel.get();
    dlg.Add(std::unique_ptr<cWindow>(cancel.release()));

    dlg.Linking();
}

}  // namespace

TEST(CPartyInviteDlgTest, LinkingResolvesAllChildren) {
    cPartyInviteDlg dlg;
    cStatic* pDistribute = nullptr;
    cTextArea* pInviter = nullptr;
    cButton* pOk = nullptr;
    cButton* pCancel = nullptr;
    BuildDlgWithChildren(dlg, &pDistribute, &pInviter, &pOk, &pCancel);

    // m_pDistribute / m_pInviter / m_pOK / m_pCancel
    // are private; verified indirectly via SetMsg
    // updating the cStatic + cTextArea.
    dlg.SetMsg("Alice", cPartyInviteDlg::kOptRandom);
    EXPECT_EQ(pDistribute->GetStaticText(), "PARTY_OPT_RANDOM");
    EXPECT_NE(pInviter->GetScriptText().find("Alice"), std::string::npos);
}

TEST(CPartyInviteDlgTest, LinkingWithoutChildrenLeavesPointersNull) {
    cPartyInviteDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    // SetMsg without children must be safe.
    dlg.SetMsg("Alice", cPartyInviteDlg::kOptRandom);
    dlg.SetMsg("Alice", cPartyInviteDlg::kOptDamage);
    dlg.SetMsg(nullptr, cPartyInviteDlg::kOptRandom);
    SUCCEED();
}

TEST(CPartyInviteDlgTest, LinkingBeforeInitDoesNotCrash) {
    cPartyInviteDlg dlg;
    dlg.Linking();
    SUCCEED();
}

// ===========================================================================
// SetMsg
// ===========================================================================

TEST(CPartyInviteDlgTest, SetMsgWithRandomOptionUpdatesBothChildren) {
    cPartyInviteDlg dlg;
    cStatic* pDistribute = nullptr;
    cTextArea* pInviter = nullptr;
    cButton* pOk = nullptr;
    cButton* pCancel = nullptr;
    BuildDlgWithChildren(dlg, &pDistribute, &pInviter, &pOk, &pCancel);

    dlg.SetMsg("Alice", cPartyInviteDlg::kOptRandom);
    EXPECT_EQ(pDistribute->GetStaticText(), "PARTY_OPT_RANDOM");
    EXPECT_NE(pInviter->GetScriptText().find("Alice"), std::string::npos);
    EXPECT_NE(pInviter->GetScriptText().find("PARTY_INVITER_MSG_FORMAT"),
              std::string::npos);
}

TEST(CPartyInviteDlgTest, SetMsgWithDamageOptionUpdatesBothChildren) {
    cPartyInviteDlg dlg;
    cStatic* pDistribute = nullptr;
    cTextArea* pInviter = nullptr;
    cButton* pOk = nullptr;
    cButton* pCancel = nullptr;
    BuildDlgWithChildren(dlg, &pDistribute, &pInviter, &pOk, &pCancel);

    dlg.SetMsg("Bob", cPartyInviteDlg::kOptDamage);
    EXPECT_EQ(pDistribute->GetStaticText(), "PARTY_OPT_DAMAGE");
    EXPECT_NE(pInviter->GetScriptText().find("Bob"), std::string::npos);
}

TEST(CPartyInviteDlgTest, SetMsgWithUnknownOptionLeavesDistributeEmpty) {
    // 1:1 with legacy: no `else` branch means
    // Opt stays empty when Option != Random and
    // Option != Damage.
    cPartyInviteDlg dlg;
    cStatic* pDistribute = nullptr;
    cTextArea* pInviter = nullptr;
    cButton* pOk = nullptr;
    cButton* pCancel = nullptr;
    BuildDlgWithChildren(dlg, &pDistribute, &pInviter, &pOk, &pCancel);

    dlg.SetMsg("Carol", /*option=*/255);
    EXPECT_EQ(pDistribute->GetStaticText(), "");
    // Inviter text still set (sprintf is
    // unconditional in the legacy).
    EXPECT_NE(pInviter->GetScriptText().find("Carol"), std::string::npos);
}

TEST(CPartyInviteDlgTest, SetMsgWithNullInviterIsSafe) {
    // 1:1 quirk: modern port guards null
    // pInviter (legacy would crash on sprintf
    // with null).
    cPartyInviteDlg dlg;
    cStatic* pDistribute = nullptr;
    cTextArea* pInviter = nullptr;
    cButton* pOk = nullptr;
    cButton* pCancel = nullptr;
    BuildDlgWithChildren(dlg, &pDistribute, &pInviter, &pOk, &pCancel);

    dlg.SetMsg(nullptr, cPartyInviteDlg::kOptRandom);
    // Inviter text is empty (guarded null).
    EXPECT_EQ(pInviter->GetScriptText(), "");
    // Distribute text still set.
    EXPECT_EQ(pDistribute->GetStaticText(), "PARTY_OPT_RANDOM");
}

TEST(CPartyInviteDlgTest, SetMsgOverwritesPreviousText) {
    cPartyInviteDlg dlg;
    cStatic* pDistribute = nullptr;
    cTextArea* pInviter = nullptr;
    cButton* pOk = nullptr;
    cButton* pCancel = nullptr;
    BuildDlgWithChildren(dlg, &pDistribute, &pInviter, &pOk, &pCancel);

    dlg.SetMsg("First", cPartyInviteDlg::kOptRandom);
    EXPECT_NE(pInviter->GetScriptText().find("First"), std::string::npos);

    dlg.SetMsg("Second", cPartyInviteDlg::kOptDamage);
    // Old inviter text overwritten.
    EXPECT_EQ(pInviter->GetScriptText().find("First"), std::string::npos);
    EXPECT_NE(pInviter->GetScriptText().find("Second"), std::string::npos);
    // Distribute text updated.
    EXPECT_EQ(pDistribute->GetStaticText(), "PARTY_OPT_DAMAGE");
}

TEST(CPartyInviteDlgTest, SetMsgWithoutLinkIsSafe) {
    cPartyInviteDlg dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetMsg("Alice", cPartyInviteDlg::kOptRandom);
    dlg.SetMsg("Bob", cPartyInviteDlg::kOptDamage);
    dlg.SetMsg(nullptr, cPartyInviteDlg::kOptRandom);
    SUCCEED();
}

TEST(CPartyInviteDlgTest, SetMsgBeforeInitDoesNotCrash) {
    cPartyInviteDlg dlg;
    dlg.SetMsg("Alice", cPartyInviteDlg::kOptRandom);
    SUCCEED();
}

}  // namespace mxh::ui::test
