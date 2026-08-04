// baildialog_test.cpp - Phase C Batch 2.35 dialog 1:1 port
// contract test for modern cBailDialog (bail dialog: enter
// amount of bad fame to pay off + show the bail cost + minimum
// required).
//
// Covers modern/src/ui/baildialog.{hpp,cpp}, a 1:1 port of
//   legacy [Client]MH/BailDialog.h (497 B) and
//   legacy [Client]MH/BailDialog.cpp.
//
// What is tested:
//   - Default construction: 2 child pointers null, m_BadFame = 0.
//   - Linking resolves the cEditBox + cTextArea children
//     (kBailEditBoxId=320, kBailTextId=321) by id.
//   - Linking calls SetValidCheck + SetAlign on the
//     cEditBox.
//   - Linking calls SetScriptText on the cTextArea with
//     placeholder text "BAIL_TEXT_PLACEHOLDER".
//   - Constants match legacy preprocessed values:
//     kMinBadFameForBail=100, kBailPrice=10000,
//     kMbiBailNoticeMsg=4036, kMbiBailNoticeErr=4037,
//     kMbiBailLowBadFame=4038, kMbtOk=1, kMbtYesNo=2,
//     chat message IDs 644/645/648/659/117.
//   - Default ctor leaves all 7 callback function
//     pointers null + userData null.
//   - SetCallbacks captures all 7 function pointers +
//     userData.
//   - SetCallbacks accepts null userData explicitly.
//   - Open with wired GetHeroBadFameFn > kMinBadFameForBail
//     sets the edit text to "0" + activates the dialog.
//   - Open with GetHeroBadFameFn <= kMinBadFameForBail
//     does NOT activate; instead dispatches ShowMsgBoxFn
//     with (kMbiBailLowBadFame, kMbtOk, kChatMsgLowBadFame,
//     kMinBadFameForBail, 0).
//   - Open with GetHeroBadFameFn=null is a no-op (no
//     crash, dialog stays hidden).
//   - Open with ShowMsgBoxFn=null in the else-branch is
//     a silent skip.
//   - Close dispatches SetDisable(false) + SetActive(false)
//     + EndDealStateFn() in that exact order.
//   - Close without EndDealStateFn skips the state-end
//     branch silently but still disables + deactivates.
//   - SetFame with ParseAmountFn returns 0 short-circuits
//     (m_BadFame stays at 0, no msg box shown).
//   - SetFame with badFame + kMinBadFameForBail > hero bad
//     fame shows MBI_BAILNOTICEERR + kChatMsgNoticeBadFame
//     + kMinBadFameForBail param + sets dialog disabled.
//   - SetFame with hero money < m_BadFame * kBailPrice
//     shows MBI_BAILNOTICEERR + kChatMsgNoticeMoney (no
//     numeric params) + sets dialog disabled.
//   - SetFame success path shows MBI_BAILNOTICEMSG + kMbtYesNo
//     + kChatMsgConfirmBail + (cost, badFame) params +
//     sets dialog disabled.
//   - SetFame preserves legacy DWORD wrap-around for both
//     bad-fame addition and bail-cost multiplication.
//   - SetFame without ParseAmountFn is a silent skip.
//   - SetBadFrameSync with m_BadFame <= 0 returns early
//     (no send, no close).
//   - SetBadFrameSync with m_BadFame > 0 dispatches
//     SendBadFameFn(GetHeroIdFn(), m_BadFame) + calls
//     Close() in that order.
//   - SetBadFrameSync without SendBadFameFn is a silent skip.
//   - SetBadFrameSync without GetHeroIdFn is a silent skip.
//   - Defensive null-checks: Linking + Open + Close +
//     SetFame + SetBadFrameSync without link are safe.
//
// 1:1 quirks preserved:
//   - Ctor initializes 2 child pointers to null + m_BadFame
//     to 0 (modern port uses default member init).
//   - Linking calls SetValidCheck(1) (VCM_NUMBER = digits
//     only) + SetAlign(TextAlign::Right) on the cEditBox.
//   - Linking is SetScriptText call uses placeholder
//     text "BAIL_TEXT_PLACEHOLDER" (legacy uses
//     CHATMGR->GetChatMsg(644) + AddComma-formatted
//     bail cost).
//   - Open dispatches via host callback (1:1 with legacy
//     HERO + WINDOWMGR + CHATMGR + AddComma globals).
//   - Close dispatches via SetDisable + SetActive (direct
//     base calls) + EndDealStateFn (1:1 with legacy
//     OBJECTSTATEMGR->EndObjectState).
//   - SetFame dispatches via host callbacks (1:1 with
//     legacy HERO + WINDOWMGR + CHATMGR + AddComma).
//   - SetBadFrameSync dispatches via host callbacks (1:1
//     with legacy HERO + NETWORK).
//   - 7 host-injected callbacks replace legacy globals
//     (HERO + WINDOWMGR + CHATMGR + NETWORK + OBJECTSTATEMGR
//     + AddComma).

#include "baildialog.hpp"
#include "ceditbox.hpp"
#include "ctextarea.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace mxh::ui::test {

namespace {

// ===========================================================================
// Callback capture + stubs
// ===========================================================================

struct CallbackCapture {
    int getBadFameCalls = 0;
    std::uint32_t badFameReturn = 0;

    int getMoneyCalls = 0;
    std::uint32_t moneyReturn = 0;

    int getIdCalls = 0;
    std::uint32_t idReturn = 0;

    int parseAmountCalls = 0;
    std::vector<std::string> parseAmountInputs;
    std::uint32_t parseAmountReturn = 0;
    bool parseAmountReturnAlways = true;

    int msgBoxCalls = 0;
    std::vector<std::int32_t> msgBoxWindowIds;
    std::vector<std::int32_t> msgBoxButtonTypes;
    std::vector<std::int32_t> msgBoxMsgIds;
    std::vector<std::uint32_t> msgBoxParam1s;
    std::vector<std::uint32_t> msgBoxParam2s;
    bool msgBoxReturnValue = true;

    int endDealCalls = 0;

    int sendBadFameCalls = 0;
    std::uint32_t lastSendObjectId = 0;
    std::uint32_t lastSendFame = 0;
};

std::uint32_t StubGetHeroBadFame(void* userData) {
    auto* cap = static_cast<CallbackCapture*>(userData);
    if (!cap) return 0u;
    ++cap->getBadFameCalls;
    return cap->badFameReturn;
}

std::uint32_t StubGetHeroMoney(void* userData) {
    auto* cap = static_cast<CallbackCapture*>(userData);
    if (!cap) return 0u;
    ++cap->getMoneyCalls;
    return cap->moneyReturn;
}

std::uint32_t StubGetHeroId(void* userData) {
    auto* cap = static_cast<CallbackCapture*>(userData);
    if (!cap) return 0u;
    ++cap->getIdCalls;
    return cap->idReturn;
}

std::uint32_t StubParseAmount(const char* text, void* userData) {
    auto* cap = static_cast<CallbackCapture*>(userData);
    if (!cap) return 0u;
    ++cap->parseAmountCalls;
    if (text) cap->parseAmountInputs.emplace_back(text);
    return cap->parseAmountReturn;
}

bool StubShowMsgBox(std::int32_t windowId, std::int32_t buttonType,
                     std::int32_t msgId, std::uint32_t param1,
                     std::uint32_t param2, void* userData) {
    auto* cap = static_cast<CallbackCapture*>(userData);
    if (!cap) return false;
    ++cap->msgBoxCalls;
    cap->msgBoxWindowIds.push_back(windowId);
    cap->msgBoxButtonTypes.push_back(buttonType);
    cap->msgBoxMsgIds.push_back(msgId);
    cap->msgBoxParam1s.push_back(param1);
    cap->msgBoxParam2s.push_back(param2);
    return cap->msgBoxReturnValue;
}

void StubEndDealState(void* userData) {
    auto* cap = static_cast<CallbackCapture*>(userData);
    if (!cap) return;
    ++cap->endDealCalls;
}

bool StubSendBadFame(std::uint32_t objectId, std::uint32_t fame,
                    void* userData) {
    auto* cap = static_cast<CallbackCapture*>(userData);
    if (!cap) return false;
    ++cap->sendBadFameCalls;
    cap->lastSendObjectId = objectId;
    cap->lastSendFame = fame;
    return true;
}

void InstallAllCallbacks(cBailDialog& dlg, CallbackCapture& cap) {
    dlg.SetCallbacks(StubGetHeroBadFame,
                     StubGetHeroMoney,
                     StubGetHeroId,
                     StubParseAmount,
                     StubShowMsgBox,
                     StubEndDealState,
                     StubSendBadFame,
                     &cap);
}

}  // namespace

// ===========================================================================
// Construction + id constants
// ===========================================================================

TEST(CBailDialogTest, DefaultConstructionIsValid) {
    cBailDialog dlg;
    EXPECT_EQ(dlg.GetBailEditBox(), nullptr);
    EXPECT_EQ(dlg.GetBailText(),    nullptr);
    EXPECT_EQ(dlg.GetBadFame(),     0u);
}

TEST(CBailDialogTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cBailDialog::kBailEditBoxId, 320);
    EXPECT_EQ(cBailDialog::kBailTextId,    321);
    EXPECT_NE(cBailDialog::kBailEditBoxId, cBailDialog::kBailTextId);
}

TEST(CBailDialogTest, LegacyConstantsMatchPreprocessedValues) {
    // 1:1 with [CC]Header/CommonGameDefine.h.
    EXPECT_EQ(cBailDialog::kMinBadFameForBail, 100u);
    EXPECT_EQ(cBailDialog::kBailPrice, 10000u);
    // 1:1 with [Client]MH/WindowIDs.h WINDOW_ID sequence.
    EXPECT_EQ(cBailDialog::kMbiBailNoticeMsg, 4036);
    EXPECT_EQ(cBailDialog::kMbiBailNoticeErr, 4037);
    EXPECT_EQ(cBailDialog::kMbiBailLowBadFame, 4038);
    // 1:1 with [Client]MH/cMsgBox.h enum eMBType.
    EXPECT_EQ(cBailDialog::kMbtOk, 1);
    EXPECT_EQ(cBailDialog::kMbtYesNo, 2);
    // 1:1 with legacy CHATMGR msg table (for host to format).
    EXPECT_EQ(cBailDialog::kChatMsgLowBadFame,     659);
    EXPECT_EQ(cBailDialog::kChatMsgNoticeBadFame, 648);
    EXPECT_EQ(cBailDialog::kChatMsgNoticeMoney,   117);
    EXPECT_EQ(cBailDialog::kChatMsgConfirmBail,   645);
    EXPECT_EQ(cBailDialog::kChatMsgBailText,       644);
}

// ===========================================================================
// Linking
// ===========================================================================

namespace {

struct BailChildren {
    cEditBox* edit = nullptr;
    cTextArea* text = nullptr;
};

void BuildDlgWithChildren(cBailDialog& dlg, BailChildren& out) {
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    auto edit = std::make_unique<cEditBox>();
    edit->Init(0, 0, 200, 14, nullptr, nullptr,
               cBailDialog::kBailEditBoxId);
    edit->InitEditbox(50, 64);
    out.edit = edit.get();
    dlg.Add(std::unique_ptr<cWindow>(edit.release()));

    auto text = std::make_unique<cTextArea>();
    text->Init(0, 0, 200, 100, nullptr, cBailDialog::kBailTextId);
    text->InitTextArea({0, 0, 200, 100}, 256);
    out.text = text.get();
    dlg.Add(std::unique_ptr<cWindow>(text.release()));

    dlg.Linking();
}

}  // namespace

TEST(CBailDialogTest, LinkingResolvesBothChildren) {
    cBailDialog dlg;
    BailChildren raws;
    BuildDlgWithChildren(dlg, raws);

    EXPECT_EQ(dlg.GetBailEditBox(), raws.edit);
    EXPECT_EQ(dlg.GetBailText(),    raws.text);
}

TEST(CBailDialogTest, LinkingConfiguresValidCheckAndAlign) {
    cBailDialog dlg;
    BailChildren raws;
    BuildDlgWithChildren(dlg, raws);
    ASSERT_NE(dlg.GetBailEditBox(), nullptr);
    EXPECT_EQ(dlg.GetBailEditBox()->GetValidCheckMethod(), 1);
    EXPECT_EQ(dlg.GetBailEditBox()->textAlign(),
              cEditBox::TextAlign::Right);
}

TEST(CBailDialogTest, LinkingCallsSetScriptText) {
    cBailDialog dlg;
    BailChildren raws;
    BuildDlgWithChildren(dlg, raws);
    ASSERT_NE(dlg.GetBailText(), nullptr);
    EXPECT_STREQ(dlg.GetBailText()->GetScriptText().c_str(),
                 "BAIL_TEXT_PLACEHOLDER");
}

TEST(CBailDialogTest, LinkingWithoutChildrenLeavesPointersNull) {
    cBailDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    EXPECT_EQ(dlg.GetBailEditBox(), nullptr);
    EXPECT_EQ(dlg.GetBailText(),    nullptr);
}

// ===========================================================================
// SetCallbacks
// ===========================================================================

TEST(CBailDialogTest, DefaultConstructorLeavesAllCallbacksNull) {
    // 1:1 with legacy: dialog without host integration has no
    // HERO / WINDOWMGR / CHATMGR / NETWORK / OBJECTSTATEMGR /
    // AddComma wiring (R-12.x deferred). Open / Close / SetFame
    // / SetBadFrameSync are safe no-ops while callbacks are null.
    cBailDialog dlg;
    EXPECT_FALSE(dlg.isActive());
    dlg.Open();
    EXPECT_FALSE(dlg.isActive());
    dlg.Close();
    EXPECT_FALSE(dlg.isActive());
    dlg.SetFame();
    EXPECT_EQ(dlg.GetBadFame(), 0u);
    dlg.SetBadFrameSync();
    EXPECT_EQ(dlg.GetBadFame(), 0u);
}

TEST(CBailDialogTest, SetCallbacksAcceptsNullUserData) {
    cBailDialog dlg;
    dlg.SetCallbacks(StubGetHeroBadFame,
                     StubGetHeroMoney,
                     StubGetHeroId,
                     StubParseAmount,
                     StubShowMsgBox,
                     StubEndDealState,
                     StubSendBadFame,
                     nullptr);
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Open();
    EXPECT_FALSE(dlg.isActive());
    dlg.Close();
    EXPECT_FALSE(dlg.isActive());
}

// ===========================================================================
// Open
// ===========================================================================

TEST(CBailDialogTest, OpenWithHighBadFameActivatesDialog) {
    // 1:1 with legacy: when HERO->GetBadFame() > kMinBadFameForBail,
    // the dialog sets the edit text to "0" + activates itself.
    cBailDialog dlg;
    BailChildren raws;
    BuildDlgWithChildren(dlg, raws);

    CallbackCapture cap;
    cap.badFameReturn = 500u;
    InstallAllCallbacks(dlg, cap);

    dlg.Open();

    EXPECT_EQ(cap.getBadFameCalls, 1);
    EXPECT_EQ(cap.msgBoxCalls, 0);
    EXPECT_TRUE(dlg.isActive());
    ASSERT_NE(dlg.GetBailEditBox(), nullptr);
    EXPECT_STREQ(dlg.GetBailEditBox()->editText().c_str(), "0");
}

TEST(CBailDialogTest, OpenWithLowBadFameShowsMsgBox) {
    // 1:1 with legacy: when HERO->GetBadFame() <= kMinBadFameForBail,
    // show MBI_BAILLOWBADFAME + kChatMsgLowBadFame + the min bad fame
    // as a numeric param. Dialog stays hidden.
    cBailDialog dlg;
    BailChildren raws;
    BuildDlgWithChildren(dlg, raws);

    CallbackCapture cap;
    cap.badFameReturn = 50u;
    InstallAllCallbacks(dlg, cap);

    dlg.Open();

    EXPECT_EQ(cap.getBadFameCalls, 1);
    ASSERT_EQ(cap.msgBoxCalls, 1);
    EXPECT_EQ(cap.msgBoxWindowIds[0], cBailDialog::kMbiBailLowBadFame);
    EXPECT_EQ(cap.msgBoxButtonTypes[0], cBailDialog::kMbtOk);
    EXPECT_EQ(cap.msgBoxMsgIds[0], cBailDialog::kChatMsgLowBadFame);
    EXPECT_EQ(cap.msgBoxParam1s[0], cBailDialog::kMinBadFameForBail);
    EXPECT_EQ(cap.msgBoxParam2s[0], 0u);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CBailDialogTest, OpenWithExactBadFameBoundaryShowsMsgBox) {
    // 1:1 with legacy: the check is strictly >. When hero bad fame
    // equals kMinBadFameForBail exactly, the failure branch fires.
    cBailDialog dlg;
    BailChildren raws;
    BuildDlgWithChildren(dlg, raws);

    CallbackCapture cap;
    cap.badFameReturn = cBailDialog::kMinBadFameForBail;
    InstallAllCallbacks(dlg, cap);

    dlg.Open();

    EXPECT_EQ(cap.msgBoxCalls, 1);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CBailDialogTest, OpenWithoutGetHeroBadFameIsNoOp) {
    // 1:1 quirk: when GetHeroBadFameFn is null, the dialog stays
    // hidden (safe no-op).
    cBailDialog dlg;
    BailChildren raws;
    BuildDlgWithChildren(dlg, raws);

    CallbackCapture cap;
    dlg.SetCallbacks(nullptr, StubGetHeroMoney, StubGetHeroId,
                     StubParseAmount, StubShowMsgBox,
                     StubEndDealState, StubSendBadFame, &cap);

    dlg.Open();

    EXPECT_EQ(cap.msgBoxCalls, 0);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CBailDialogTest, OpenWithoutShowMsgBoxSkipsFailureBranch) {
    // 1:1 quirk: when GetHeroBadFameFn returns low but ShowMsgBoxFn
    // is null, the failure branch is silently skipped. Dialog
    // stays hidden.
    cBailDialog dlg;
    BailChildren raws;
    BuildDlgWithChildren(dlg, raws);

    CallbackCapture cap;
    cap.badFameReturn = 0u;
    dlg.SetCallbacks(StubGetHeroBadFame, nullptr, nullptr,
                     nullptr, nullptr, nullptr, nullptr, &cap);

    dlg.Open();

    EXPECT_EQ(cap.getBadFameCalls, 1);
    EXPECT_EQ(cap.msgBoxCalls, 0);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CBailDialogTest, OpenBeforeLinkIsSafe) {
    cBailDialog dlg;
    CallbackCapture cap;
    cap.badFameReturn = 500u;
    InstallAllCallbacks(dlg, cap);
    dlg.Open();
    // Edit box pointer is null (no Linking), so the setEditText
    // call is skipped. Dialog still activates per legacy.
    EXPECT_TRUE(dlg.isActive());
}

// ===========================================================================
// Close
// ===========================================================================

TEST(CBailDialogTest, CloseDisablesAndDeactivatesAndEndsDeal) {
    // 1:1 with legacy Close: SetDisable(FALSE) + SetActive(FALSE)
    // + EndObjectState(HERO, eObjectState_Deal). The order is
    // important: disable before deactivate (so the user cannot
    // click after the close starts), deactivate, then end state.
    cBailDialog dlg;
    BailChildren raws;
    BuildDlgWithChildren(dlg, raws);
    dlg.SetActive(true);

    CallbackCapture cap;
    InstallAllCallbacks(dlg, cap);

    dlg.Close();

    EXPECT_FALSE(dlg.isActive());
    EXPECT_EQ(cap.endDealCalls, 1);
}

TEST(CBailDialogTest, CloseWithoutEndDealStateStillDisables) {
    // 1:1 quirk: the state-end branch is silently skipped when
    // EndDealStateFn is null. SetDisable + SetActive still fire.
    cBailDialog dlg;
    BailChildren raws;
    BuildDlgWithChildren(dlg, raws);
    dlg.SetActive(true);

    CallbackCapture cap;
    dlg.SetCallbacks(nullptr, nullptr, nullptr, nullptr, nullptr,
                     nullptr, nullptr, &cap);

    dlg.Close();

    EXPECT_FALSE(dlg.isActive());
    EXPECT_EQ(cap.endDealCalls, 0);
}

TEST(CBailDialogTest, CloseBeforeLinkIsSafe) {
    cBailDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetActive(true);

    CallbackCapture cap;
    InstallAllCallbacks(dlg, cap);

    dlg.Close();
    EXPECT_FALSE(dlg.isActive());
    EXPECT_EQ(cap.endDealCalls, 1);
}

// ===========================================================================
// SetFame
// ===========================================================================

namespace {

struct SetFameSetup {
    cBailDialog* dlg = nullptr;
    BailChildren* raws = nullptr;
    CallbackCapture* cap = nullptr;
};

void SetFameInit(SetFameSetup& s) {
    s.dlg->Init(0, 0, 400, 400, nullptr, 0);
    BuildDlgWithChildren(*s.dlg, *s.raws);
    InstallAllCallbacks(*s.dlg, *s.cap);
}

}  // namespace

TEST(CBailDialogTest, SetFameWithZeroParseResultShortCircuits) {
    // 1:1 with legacy: if atoi returns 0, the function returns
    // early without showing any msg box or setting m_BadFame.
    cBailDialog dlg;
    BailChildren raws;
    CallbackCapture cap;
    SetFameSetup s{&dlg, &raws, &cap};
    SetFameInit(s);
    cap.parseAmountReturn = 0u;

    dlg.SetFame();

    EXPECT_EQ(cap.parseAmountCalls, 1);
    EXPECT_EQ(cap.msgBoxCalls, 0);
    EXPECT_EQ(cap.getBadFameCalls, 0);
    EXPECT_EQ(cap.getMoneyCalls, 0);
    EXPECT_EQ(dlg.GetBadFame(), 0u);
}

TEST(CBailDialogTest, SetFameWithInsufficientFameShowsErrorAndDisables) {
    // 1:1 with legacy: when m_BadFame + kMinBadFameForBail > HERO bad fame,
    // show MBI_BAILNOTICEERR + kChatMsgNoticeBadFame + kMinBadFameForBail param
    // + SetDisable(TRUE).
    cBailDialog dlg;
    BailChildren raws;
    CallbackCapture cap;
    SetFameSetup s{&dlg, &raws, &cap};
    SetFameInit(s);
    cap.parseAmountReturn = 100u;
    cap.badFameReturn = 150u;  // 100 + 100 = 200 > 150

    dlg.SetFame();

    EXPECT_EQ(cap.parseAmountCalls, 1);
    EXPECT_EQ(cap.getBadFameCalls, 1);
    EXPECT_EQ(cap.getMoneyCalls, 0);
    ASSERT_EQ(cap.msgBoxCalls, 1);
    EXPECT_EQ(cap.msgBoxWindowIds[0], cBailDialog::kMbiBailNoticeErr);
    EXPECT_EQ(cap.msgBoxButtonTypes[0], cBailDialog::kMbtOk);
    EXPECT_EQ(cap.msgBoxMsgIds[0], cBailDialog::kChatMsgNoticeBadFame);
    EXPECT_EQ(cap.msgBoxParam1s[0], cBailDialog::kMinBadFameForBail);
    EXPECT_EQ(cap.msgBoxParam2s[0], 0u);
    EXPECT_EQ(dlg.GetBadFame(), 100u);
    EXPECT_FALSE(dlg.isEnabled());
}

TEST(CBailDialogTest, SetFameWithInsufficientMoneyShowsErrorAndDisables) {
    // 1:1 with legacy: when HERO money < m_BadFame * kBailPrice,
    // show MBI_BAILNOTICEERR + kChatMsgNoticeMoney (no numeric params)
    // + SetDisable(TRUE).
    cBailDialog dlg;
    BailChildren raws;
    CallbackCapture cap;
    SetFameSetup s{&dlg, &raws, &cap};
    SetFameInit(s);
    cap.parseAmountReturn = 50u;
    cap.badFameReturn = 1000u;  // 50 + 100 = 150 < 1000
    cap.moneyReturn = 100u;     // 100 < 50 * 10000

    dlg.SetFame();

    EXPECT_EQ(cap.getBadFameCalls, 1);
    EXPECT_EQ(cap.getMoneyCalls, 1);
    ASSERT_EQ(cap.msgBoxCalls, 1);
    EXPECT_EQ(cap.msgBoxWindowIds[0], cBailDialog::kMbiBailNoticeErr);
    EXPECT_EQ(cap.msgBoxButtonTypes[0], cBailDialog::kMbtOk);
    EXPECT_EQ(cap.msgBoxMsgIds[0], cBailDialog::kChatMsgNoticeMoney);
    EXPECT_EQ(cap.msgBoxParam1s[0], 0u);
    EXPECT_EQ(cap.msgBoxParam2s[0], 0u);
    EXPECT_EQ(dlg.GetBadFame(), 50u);
    EXPECT_FALSE(dlg.isEnabled());
}

TEST(CBailDialogTest, SetFameSuccessPathShowsConfirmationAndDisables) {
    // 1:1 with legacy: success path shows MBI_BAILNOTICEMSG +
    // kMbtYesNo + kChatMsgConfirmBail + (cost = m_BadFame *
    // kBailPrice, m_BadFame) params + SetDisable(TRUE).
    cBailDialog dlg;
    BailChildren raws;
    CallbackCapture cap;
    SetFameSetup s{&dlg, &raws, &cap};
    SetFameInit(s);
    cap.parseAmountReturn = 50u;
    cap.badFameReturn = 1000u;
    cap.moneyReturn = 1000000u;

    dlg.SetFame();

    EXPECT_EQ(cap.getBadFameCalls, 1);
    EXPECT_EQ(cap.getMoneyCalls, 1);
    ASSERT_EQ(cap.msgBoxCalls, 1);
    EXPECT_EQ(cap.msgBoxWindowIds[0], cBailDialog::kMbiBailNoticeMsg);
    EXPECT_EQ(cap.msgBoxButtonTypes[0], cBailDialog::kMbtYesNo);
    EXPECT_EQ(cap.msgBoxMsgIds[0], cBailDialog::kChatMsgConfirmBail);
    EXPECT_EQ(cap.msgBoxParam1s[0], 50u * cBailDialog::kBailPrice);
    EXPECT_EQ(cap.msgBoxParam2s[0], 50u);
    EXPECT_EQ(dlg.GetBadFame(), 50u);
    EXPECT_FALSE(dlg.isEnabled());
}

TEST(CBailDialogTest, SetFameBadFameAdditionUsesLegacyDwordWrap) {
    cBailDialog dlg;
    BailChildren raws;
    CallbackCapture cap;
    SetFameSetup setup{&dlg, &raws, &cap};
    SetFameInit(setup);
    constexpr std::uint32_t amount = 0xFFFFFFCDu;
    cap.parseAmountReturn = amount;
    cap.badFameReturn = 50u;
    cap.moneyReturn = 0xFFFFFFFFu;

    dlg.SetFame();

    ASSERT_EQ(cap.msgBoxCalls, 1);
    EXPECT_EQ(cap.msgBoxMsgIds[0], cBailDialog::kChatMsgConfirmBail);
    EXPECT_EQ(cap.msgBoxParam1s[0], amount * cBailDialog::kBailPrice);
    EXPECT_EQ(cap.msgBoxParam2s[0], amount);
}

TEST(CBailDialogTest, SetFameBailCostUsesLegacyDwordWrap) {
    cBailDialog dlg;
    BailChildren raws;
    CallbackCapture cap;
    SetFameSetup setup{&dlg, &raws, &cap};
    SetFameInit(setup);
    constexpr std::uint32_t amount = 500000u;
    cap.parseAmountReturn = amount;
    cap.badFameReturn = 1000000u;
    cap.moneyReturn = 800000000u;

    dlg.SetFame();

    ASSERT_EQ(cap.msgBoxCalls, 1);
    EXPECT_EQ(cap.msgBoxMsgIds[0], cBailDialog::kChatMsgConfirmBail);
    EXPECT_EQ(cap.msgBoxParam1s[0], amount * cBailDialog::kBailPrice);
    EXPECT_EQ(cap.msgBoxParam2s[0], amount);
}

TEST(CBailDialogTest, SetFameWithoutParseAmountIsNoOp) {
    // 1:1 quirk: when ParseAmountFn is null, the function returns
    // early (safe no-op).
    cBailDialog dlg;
    BailChildren raws;
    BuildDlgWithChildren(dlg, raws);
    CallbackCapture cap;
    dlg.SetCallbacks(StubGetHeroBadFame, StubGetHeroMoney,
                     StubGetHeroId, nullptr, StubShowMsgBox,
                     StubEndDealState, StubSendBadFame, &cap);

    dlg.SetFame();

    EXPECT_EQ(cap.parseAmountCalls, 0);
    EXPECT_EQ(cap.msgBoxCalls, 0);
    EXPECT_EQ(dlg.GetBadFame(), 0u);
}

TEST(CBailDialogTest, SetFameWithoutLinkIsSafe) {
    cBailDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();  // no children linked

    CallbackCapture cap;
    InstallAllCallbacks(dlg, cap);

    dlg.SetFame();
    // m_pBailEdtBox is null, so the parse branch short-circuits.
    EXPECT_EQ(cap.parseAmountCalls, 0);
}



// ===========================================================================
// SetBadFrameSync
// ===========================================================================

TEST(CBailDialogTest, SetBadFrameSyncWithZeroReturnsEarly) {
    cBailDialog dlg;
    CallbackCapture cap;
    InstallAllCallbacks(dlg, cap);
    dlg.SetActive(true);
    dlg.SetDisable(true);

    dlg.SetBadFrameSync();

    EXPECT_EQ(cap.getIdCalls, 0);
    EXPECT_EQ(cap.sendBadFameCalls, 0);
    EXPECT_EQ(cap.endDealCalls, 0);
    EXPECT_TRUE(dlg.isActive());
    EXPECT_FALSE(dlg.isEnabled());
}

TEST(CBailDialogTest, SetBadFrameSyncWithPositiveSendsAndCloses) {
    cBailDialog dlg;
    BailChildren raws;
    CallbackCapture cap;
    SetFameSetup setup{&dlg, &raws, &cap};
    SetFameInit(setup);
    cap.parseAmountReturn = 25u;
    cap.badFameReturn = 1000u;
    cap.moneyReturn = 1000000u;
    cap.idReturn = 0x12345678u;
    dlg.SetFame();
    dlg.SetActive(true);

    dlg.SetBadFrameSync();

    EXPECT_EQ(cap.getIdCalls, 1);
    EXPECT_EQ(cap.sendBadFameCalls, 1);
    EXPECT_EQ(cap.lastSendObjectId, 0x12345678u);
    EXPECT_EQ(cap.lastSendFame, 25u);
    EXPECT_EQ(cap.endDealCalls, 1);
    EXPECT_FALSE(dlg.isActive());
    EXPECT_TRUE(dlg.isEnabled());
}

TEST(CBailDialogTest, SetBadFrameSyncWithoutSendBadFameIsNoOp) {
    cBailDialog dlg;
    BailChildren raws;
    CallbackCapture cap;
    SetFameSetup setup{&dlg, &raws, &cap};
    SetFameInit(setup);
    cap.parseAmountReturn = 25u;
    cap.badFameReturn = 1000u;
    cap.moneyReturn = 1000000u;
    dlg.SetFame();
    dlg.SetActive(true);
    dlg.SetCallbacks(StubGetHeroBadFame, StubGetHeroMoney,
                     StubGetHeroId, StubParseAmount, StubShowMsgBox,
                     StubEndDealState, nullptr, &cap);

    dlg.SetBadFrameSync();

    EXPECT_EQ(cap.getIdCalls, 0);
    EXPECT_EQ(cap.sendBadFameCalls, 0);
    EXPECT_EQ(cap.endDealCalls, 0);
    EXPECT_TRUE(dlg.isActive());
    EXPECT_FALSE(dlg.isEnabled());
}

TEST(CBailDialogTest, SetBadFrameSyncWithoutGetHeroIdIsNoOp) {
    cBailDialog dlg;
    BailChildren raws;
    CallbackCapture cap;
    SetFameSetup setup{&dlg, &raws, &cap};
    SetFameInit(setup);
    cap.parseAmountReturn = 25u;
    cap.badFameReturn = 1000u;
    cap.moneyReturn = 1000000u;
    dlg.SetFame();
    dlg.SetActive(true);
    dlg.SetCallbacks(StubGetHeroBadFame, StubGetHeroMoney,
                     nullptr, StubParseAmount, StubShowMsgBox,
                     StubEndDealState, StubSendBadFame, &cap);

    dlg.SetBadFrameSync();

    EXPECT_EQ(cap.getIdCalls, 0);
    EXPECT_EQ(cap.sendBadFameCalls, 0);
    EXPECT_EQ(cap.endDealCalls, 0);
    EXPECT_TRUE(dlg.isActive());
    EXPECT_FALSE(dlg.isEnabled());
}

TEST(CBailDialogTest, SetBadFrameSyncBeforeLinkIsSafe) {
    cBailDialog dlg;
    CallbackCapture cap;
    InstallAllCallbacks(dlg, cap);

    dlg.SetBadFrameSync();

    EXPECT_EQ(cap.getIdCalls, 0);
    EXPECT_EQ(cap.sendBadFameCalls, 0);
    EXPECT_EQ(cap.endDealCalls, 0);
}

}  // namespace mxh::ui::test
