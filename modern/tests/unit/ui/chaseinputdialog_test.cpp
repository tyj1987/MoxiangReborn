// chaseinputdialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cChaseInputDialog (chase input dialog:
// enter target player name for wanted chase item).
//
// Covers modern/src/ui/chaseinputdialog.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\ChaseinputDialog.h (497 B) and
//   `墨香【源码】\[Client]MH\ChaseinputDialog.cpp`.
//
// What's tested:
//   - Default construction: 1 child pointer null, m_dwItemIdx
//     = 0, m_LastChktime = 0.
//   - Linking resolves the cEditBox child (kEditNameId=300)
//     by id + SetValidCheck(VCM_CHARNAME alias = 2).
//   - SetActive override calls base SetActive + clears
//     edit text + resets m_dwItemIdx when val=true.
//   - SetActive false does not modify edit text or item
//     idx (1:1 quirk: clear only on val=true).
//   - SetItemIdx updates m_dwItemIdx.
//   - SetItemIdx default + edge values.
//   - WantedChaseSyn preserves the complete legacy validation,
//     filter, wanted-list, send, and rate-limit flow.
//   - Accessors return the linked child pointer + item
//     idx.
//   - VcmCharnameAlias is 2 (1:1 quirk).
//   - Defensive null-checks: Linking + SetActive +
//     WantedChaseSyn without link are safe.
//
// 1:1 quirks preserved:
//   - Ctor drops m_type = WT_CHASEINPUT_DLG (legacy
//     cWindow type tag removed in Phase 6).
//   - Linking calls SetValidCheck(VCM_CHARNAME alias = 2)
//     — closest modern equivalent for the legacy
//     cIMEex character-name validator (same as
//     cMiniFriendDialog).
//   - SetActive matches base noexcept (R-12 polymorphic
//     virtual required).
//   - SetActive only clears edit text + resets m_dwItemIdx
//     when val=true (1:1 with legacy).
//   - WantedChaseSyn uses seven host callbacks for the six
//     legacy globals while preserving branch order.

#include "chaseinputdialog.hpp"
#include "mxh/proto/protocol.hpp"
#include "ceditbox.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace mxh::ui::test {

// ===========================================================================
// Construction
// ===========================================================================

TEST(CChaseInputDialogTest, DefaultConstructionHasNullAndZero) {
    cChaseInputDialog dlg;
    EXPECT_EQ(dlg.GetEditName(), nullptr);
    EXPECT_EQ(dlg.GetItemIdx(),  0u);
    EXPECT_EQ(dlg.GetLastChktime(), 0u);
}

TEST(CChaseInputDialogTest, IdConstantMatchesExpectedLocalRange) {
    // 1:1 quirk: pick 300 to avoid collisions with other
    // Tier 2 dialog id ranges (cCharMakeDlg 200-203,
    // cGuildJoinDialog 210-212, cCharStateDialog 220-224,
    // cSOSDialog 230-231, cMiniFriendDialog 240-243,
    // cReviveDialog 250-252, cMPNoticeDialog 260-261,
    // cEventNotifyDialog 270-271, cGuildCreateDialog
    // 280-284, cGuildUnionCreateDialog 290-292).
    EXPECT_EQ(cChaseInputDialog::kEditNameId, 300);
}

TEST(CChaseInputDialogTest, VcmCharnameAliasIsTwo) {
    // 1:1 quirk: VCM_CHARNAME alias = 2 (alpha only,
    // closest modern equivalent to legacy's cIMEex
    // character-name validator).
    EXPECT_EQ(cChaseInputDialog::kVcmCharnameAlias, 2);
}

// ===========================================================================
// Linking
// ===========================================================================

namespace {

// Build a cChaseInputDialog with 1 cEditBox child wired
// in the modern id range (300). Returns the raw pointer
// via the out param; ownership lives in the dlg (child
// is added via cWindow::Add).
void BuildDlgWithEdit(cChaseInputDialog& dlg, cEditBox*& out_edit) {
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    auto edit = std::make_unique<cEditBox>();
    edit->Init(0, 0, 200, 14, nullptr, nullptr,
               cChaseInputDialog::kEditNameId);
    // InitEditbox(50, 64) enables SetEditText
    // (cEditBox::m_maxBytes > 0).
    edit->InitEditbox(50, 64);
    out_edit = edit.get();
    dlg.Add(std::unique_ptr<cWindow>(edit.release()));
    dlg.Linking();
}

}  // namespace

namespace {

struct ChaseCapture {
    int currentTimeCalls = 0;
    std::uint32_t currentTime = 30000u;

    int systemMessageCalls = 0;
    std::vector<std::int32_t> systemMessageIds;

    int heroIdCalls = 0;
    std::uint32_t heroId = 0x12345678u;
    int heroNameCalls = 0;
    std::string heroName = std::string({'H', 'e', 'r', 'o'});

    int filterCalls = 0;
    std::string filteredInput;
    bool filterResult = false;

    int wantedCalls = 0;
    std::string wantedInput;
    bool wantedResult = true;

    int sendCalls = 0;
    std::uint32_t sentObjectId = 0;
    std::uint32_t sentItemIdx = 0;
    std::string sentWantedName;
    bool sendResult = true;
};

std::uint32_t StubGetCurrentTime(void* userData) {
    auto* capture = static_cast<ChaseCapture*>(userData);
    if (!capture) return 0u;
    ++capture->currentTimeCalls;
    return capture->currentTime;
}

void StubAddSystemMessage(std::int32_t chatMsgId, void* userData) {
    auto* capture = static_cast<ChaseCapture*>(userData);
    if (!capture) return;
    ++capture->systemMessageCalls;
    capture->systemMessageIds.push_back(chatMsgId);
}

std::uint32_t StubGetHeroObjectId(void* userData) {
    auto* capture = static_cast<ChaseCapture*>(userData);
    if (!capture) return 0u;
    ++capture->heroIdCalls;
    return capture->heroId;
}

const char* StubGetHeroObjectName(void* userData) {
    auto* capture = static_cast<ChaseCapture*>(userData);
    if (!capture) return nullptr;
    ++capture->heroNameCalls;
    return capture->heroName.c_str();
}

bool StubFilterWord(const char* uppercaseName, void* userData) {
    auto* capture = static_cast<ChaseCapture*>(userData);
    if (!capture) return false;
    ++capture->filterCalls;
    capture->filteredInput = uppercaseName ? uppercaseName : std::string();
    return capture->filterResult;
}

bool StubIsWantedName(const char* wantedName, void* userData) {
    auto* capture = static_cast<ChaseCapture*>(userData);
    if (!capture) return false;
    ++capture->wantedCalls;
    capture->wantedInput = wantedName ? wantedName : std::string();
    return capture->wantedResult;
}

bool StubSendChaseSyn(std::uint32_t objectId, const char* wantedName,
                      std::uint32_t itemIdx, void* userData) {
    auto* capture = static_cast<ChaseCapture*>(userData);
    if (!capture) return false;
    ++capture->sendCalls;
    capture->sentObjectId = objectId;
    capture->sentItemIdx = itemIdx;
    capture->sentWantedName = wantedName ? wantedName : std::string();
    return capture->sendResult;
}

void InstallAllCallbacks(cChaseInputDialog& dlg, ChaseCapture& capture) {
    dlg.SetCallbacks(StubGetCurrentTime, StubAddSystemMessage,
                     StubGetHeroObjectId, StubGetHeroObjectName,
                     StubFilterWord, StubIsWantedName, StubSendChaseSyn,
                     &capture);
}

void SetTargetName(cChaseInputDialog& dlg, const char* targetName) {
    ASSERT_NE(dlg.GetEditName(), nullptr);
    dlg.GetEditName()->SetEditText(targetName);
}

}  // namespace

TEST(CChaseInputDialogTest, LinkingResolvesEditBox) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);

    EXPECT_EQ(dlg.GetEditName(), raw_edit);
}

TEST(CChaseInputDialogTest, LinkingConfiguresValidCheck) {
    // 1:1 quirk: legacy calls
    // m_pEditName->SetValidCheck(VCM_CHARNAME = 2).
    // Modern port uses kVcmCharnameAlias = 2.
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    ASSERT_NE(dlg.GetEditName(), nullptr);
    EXPECT_EQ(dlg.GetEditName()->GetValidCheckMethod(), 2);
}

TEST(CChaseInputDialogTest, LinkingWithoutChildLeavesPointerNull) {
    cChaseInputDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    EXPECT_EQ(dlg.GetEditName(), nullptr);
}

TEST(CChaseInputDialogTest, LinkingBeforeInitDoesNotCrash) {
    cChaseInputDialog dlg;
    dlg.Linking();
    EXPECT_EQ(dlg.GetEditName(), nullptr);
}

// ===========================================================================
// SetActive (1:1 override, base + clear on val=true)
// ===========================================================================

TEST(CChaseInputDialogTest, SetActiveTrueUpdatesBaseState) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    EXPECT_FALSE(dlg.isActive());

    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CChaseInputDialogTest, SetActiveFalseUpdatesBaseState) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    dlg.SetActive(true);
    ASSERT_TRUE(dlg.isActive());

    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CChaseInputDialogTest, SetActiveTrueClearsEditText) {
    // 1:1 with legacy: SetActive(true) clears the
    // edit text + resets m_dwItemIdx.
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    ASSERT_NE(dlg.GetEditName(), nullptr);

    // Set some text + item idx before activating.
    dlg.GetEditName()->SetEditText("TargetPlayer");
    dlg.SetItemIdx(42);
    ASSERT_EQ(dlg.GetItemIdx(), 42u);

    dlg.SetActive(true);

    EXPECT_EQ(dlg.GetEditName()->editText(), "");
    EXPECT_EQ(dlg.GetItemIdx(), 0u);
}

TEST(CChaseInputDialogTest, SetActiveFalseDoesNotClearState) {
    // 1:1 quirk: clear only happens on val=true.
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);

    dlg.GetEditName()->SetEditText("KeepMe");
    dlg.SetItemIdx(99);
    dlg.SetActive(false);

    EXPECT_EQ(dlg.GetEditName()->editText(), "KeepMe");
    EXPECT_EQ(dlg.GetItemIdx(), 99u);
}

TEST(CChaseInputDialogTest, SetActiveWithoutLinkIsSafe) {
    cChaseInputDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    dlg.SetItemIdx(100u);
    dlg.SetActive(true);
    EXPECT_EQ(dlg.GetItemIdx(), 0u);
    dlg.SetActive(false);
}

// ===========================================================================
// SetItemIdx
// ===========================================================================

TEST(CChaseInputDialogTest, SetItemIdxUpdatesValue) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    dlg.SetItemIdx(42);
    EXPECT_EQ(dlg.GetItemIdx(), 42u);
    dlg.SetItemIdx(0);
    EXPECT_EQ(dlg.GetItemIdx(), 0u);
    dlg.SetItemIdx(0xFFFFFFFFu);
    EXPECT_EQ(dlg.GetItemIdx(), 0xFFFFFFFFu);
}

TEST(CChaseInputDialogTest, SetItemIdxBeforeInitDoesNotCrash) {
    cChaseInputDialog dlg;
    dlg.SetItemIdx(100);
    EXPECT_EQ(dlg.GetItemIdx(), 100u);
}

// ===========================================================================
// WantedChaseSyn
// ===========================================================================

TEST(CChaseInputDialogTest, LegacyConstantsMatchWireValues) {
    EXPECT_EQ(cChaseInputDialog::kRateLimitMilliseconds, 30000u);
    EXPECT_EQ(cChaseInputDialog::kTrackingJinItemIdx, 55387u);
    EXPECT_EQ(cChaseInputDialog::kChatMsgRateLimited, 909);
    EXPECT_EQ(cChaseInputDialog::kChatMsgSelfTarget, 911);
    EXPECT_EQ(cChaseInputDialog::kChatMsgFilteredTarget, 919);
    EXPECT_EQ(cChaseInputDialog::kItemCategory, 5u);
    EXPECT_EQ(cChaseInputDialog::kShopItemChaseSynProtocol, 154u);
    EXPECT_EQ(static_cast<std::uint8_t>(mxh::proto::ItemProtocol::ShopItemChaseSyn), 154u);
    EXPECT_EQ(static_cast<std::uint8_t>(mxh::proto::ItemProtocol::ShopItemChaseAck), 155u);
    EXPECT_EQ(static_cast<std::uint8_t>(mxh::proto::ItemProtocol::ShopItemChaseNack), 156u);
}

TEST(CChaseInputDialogTest, WantedChaseSynRateLimitsRecentRequest) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    SetTargetName(dlg, "Target");
    ChaseCapture capture;
    capture.currentTime = 1000u;
    InstallAllCallbacks(dlg, capture);
    dlg.SetActive(true);
    SetTargetName(dlg, "Target");

    dlg.WantedChaseSyn();

    ASSERT_EQ(capture.systemMessageCalls, 1);
    EXPECT_EQ(capture.systemMessageIds[0], cChaseInputDialog::kChatMsgRateLimited);
    EXPECT_EQ(capture.sendCalls, 0);
    EXPECT_EQ(dlg.GetLastChktime(), 0u);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CChaseInputDialogTest, WantedChaseSynAtRateLimitBoundarySends) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    SetTargetName(dlg, "Target");
    ChaseCapture capture;
    capture.currentTime = 30000u;
    InstallAllCallbacks(dlg, capture);

    dlg.WantedChaseSyn();

    EXPECT_EQ(capture.systemMessageCalls, 0);
    EXPECT_EQ(capture.sendCalls, 1);
    EXPECT_EQ(dlg.GetLastChktime(), 30000u);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CChaseInputDialogTest, WantedChaseSynWithEmptyNameReturnsEarly) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    SetTargetName(dlg, "");
    ChaseCapture capture;
    InstallAllCallbacks(dlg, capture);

    dlg.WantedChaseSyn();

    EXPECT_EQ(capture.sendCalls, 0);
    EXPECT_EQ(capture.systemMessageCalls, 0);
    EXPECT_EQ(dlg.GetLastChktime(), 0u);
}

TEST(CChaseInputDialogTest, WantedChaseSynRejectsExactHeroName) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    SetTargetName(dlg, "Hero");
    ChaseCapture capture;
    InstallAllCallbacks(dlg, capture);

    dlg.WantedChaseSyn();

    ASSERT_EQ(capture.systemMessageCalls, 1);
    EXPECT_EQ(capture.systemMessageIds[0], cChaseInputDialog::kChatMsgSelfTarget);
    EXPECT_EQ(capture.sendCalls, 0);
}

TEST(CChaseInputDialogTest, WantedChaseSynFiltersUppercaseCopy) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    SetTargetName(dlg, "aBc");
    ChaseCapture capture;
    capture.filterResult = true;
    InstallAllCallbacks(dlg, capture);

    dlg.WantedChaseSyn();

    EXPECT_EQ(capture.filteredInput, "ABC");
    EXPECT_EQ(capture.systemMessageIds[0], cChaseInputDialog::kChatMsgFilteredTarget);
    EXPECT_EQ(capture.sendCalls, 0);
}

TEST(CChaseInputDialogTest, WantedChaseSynSendsOriginalCaseName) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    SetTargetName(dlg, "aBc");
    ChaseCapture capture;
    InstallAllCallbacks(dlg, capture);

    dlg.WantedChaseSyn();

    EXPECT_EQ(capture.filteredInput, "ABC");
    EXPECT_EQ(capture.sentWantedName, "aBc");
    EXPECT_EQ(capture.sentObjectId, capture.heroId);
    EXPECT_EQ(capture.sendCalls, 1);
}

TEST(CChaseInputDialogTest, TrackingJinRequiresWantedListMembership) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    SetTargetName(dlg, "Target");
    dlg.SetItemIdx(cChaseInputDialog::kTrackingJinItemIdx);
    ChaseCapture capture;
    capture.wantedResult = false;
    InstallAllCallbacks(dlg, capture);

    dlg.WantedChaseSyn();

    EXPECT_EQ(capture.wantedCalls, 1);
    EXPECT_EQ(capture.wantedInput, "Target");
    EXPECT_EQ(capture.sendCalls, 0);
    EXPECT_EQ(dlg.GetLastChktime(), 0u);
}

TEST(CChaseInputDialogTest, TrackingJinSendsWantedMember) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    SetTargetName(dlg, "Target");
    dlg.SetItemIdx(cChaseInputDialog::kTrackingJinItemIdx);
    ChaseCapture capture;
    capture.wantedResult = true;
    InstallAllCallbacks(dlg, capture);

    dlg.WantedChaseSyn();

    EXPECT_EQ(capture.wantedCalls, 1);
    EXPECT_EQ(capture.sendCalls, 1);
    EXPECT_EQ(capture.sentItemIdx, cChaseInputDialog::kTrackingJinItemIdx);
}

TEST(CChaseInputDialogTest, NonTrackingItemSkipsWantedListCheck) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    SetTargetName(dlg, "Target");
    dlg.SetItemIdx(123u);
    ChaseCapture capture;
    capture.wantedResult = false;
    InstallAllCallbacks(dlg, capture);

    dlg.WantedChaseSyn();

    EXPECT_EQ(capture.wantedCalls, 0);
    EXPECT_EQ(capture.sendCalls, 1);
}

TEST(CChaseInputDialogTest, WantedChaseSynSendsAndUpdatesState) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    SetTargetName(dlg, "Target");
    dlg.SetActive(true);
    SetTargetName(dlg, "Target");
    dlg.SetItemIdx(42u);
    ChaseCapture capture;
    capture.currentTime = 45000u;
    InstallAllCallbacks(dlg, capture);

    dlg.WantedChaseSyn();

    EXPECT_EQ(capture.currentTimeCalls, 1);
    EXPECT_EQ(capture.heroNameCalls, 1);
    EXPECT_EQ(capture.filterCalls, 1);
    EXPECT_EQ(capture.heroIdCalls, 1);
    EXPECT_EQ(capture.sendCalls, 1);
    EXPECT_EQ(capture.sentObjectId, 0x12345678u);
    EXPECT_EQ(capture.sentWantedName, "Target");
    EXPECT_EQ(capture.sentItemIdx, 42u);
    EXPECT_FALSE(dlg.isActive());
    EXPECT_EQ(dlg.GetLastChktime(), 45000u);
}

TEST(CChaseInputDialogTest, WantedChaseSynIgnoresSendResult) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    SetTargetName(dlg, "Target");
    ChaseCapture capture;
    capture.sendResult = false;
    InstallAllCallbacks(dlg, capture);

    dlg.WantedChaseSyn();

    EXPECT_EQ(capture.sendCalls, 1);
    EXPECT_FALSE(dlg.isActive());
    EXPECT_EQ(dlg.GetLastChktime(), capture.currentTime);
}

TEST(CChaseInputDialogTest, MissingCurrentTimeIsSafeNoOp) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    SetTargetName(dlg, "Target");
    ChaseCapture capture;
    dlg.SetCallbacks(nullptr, StubAddSystemMessage, StubGetHeroObjectId,
                     StubGetHeroObjectName, StubFilterWord,
                     StubIsWantedName, StubSendChaseSyn, &capture);

    dlg.WantedChaseSyn();

    EXPECT_EQ(capture.sendCalls, 0);
}

TEST(CChaseInputDialogTest, MissingEditBoxIsSafeNoOp) {
    cChaseInputDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    ChaseCapture capture;
    InstallAllCallbacks(dlg, capture);

    dlg.WantedChaseSyn();

    EXPECT_EQ(capture.currentTimeCalls, 1);
    EXPECT_EQ(capture.sendCalls, 0);
}

TEST(CChaseInputDialogTest, MissingHeroIdSkipsSendAndStateChange) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    SetTargetName(dlg, "Target");
    dlg.SetActive(true);
    SetTargetName(dlg, "Target");
    ChaseCapture capture;
    dlg.SetCallbacks(StubGetCurrentTime, StubAddSystemMessage, nullptr,
                     StubGetHeroObjectName, StubFilterWord,
                     StubIsWantedName, StubSendChaseSyn, &capture);

    dlg.WantedChaseSyn();

    EXPECT_EQ(capture.sendCalls, 0);
    EXPECT_TRUE(dlg.isActive());
    EXPECT_EQ(dlg.GetLastChktime(), 0u);
}

TEST(CChaseInputDialogTest, MissingSendCallbackSkipsSendAndStateChange) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    SetTargetName(dlg, "Target");
    dlg.SetActive(true);
    SetTargetName(dlg, "Target");
    ChaseCapture capture;
    dlg.SetCallbacks(StubGetCurrentTime, StubAddSystemMessage,
                     StubGetHeroObjectId, StubGetHeroObjectName,
                     StubFilterWord, StubIsWantedName, nullptr, &capture);

    dlg.WantedChaseSyn();

    EXPECT_EQ(capture.heroIdCalls, 0);
    EXPECT_EQ(capture.sendCalls, 0);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CChaseInputDialogTest, MissingHeroNameSkipsSelfCheck) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    SetTargetName(dlg, "Hero");
    ChaseCapture capture;
    dlg.SetCallbacks(StubGetCurrentTime, StubAddSystemMessage,
                     StubGetHeroObjectId, nullptr, StubFilterWord,
                     StubIsWantedName, StubSendChaseSyn, &capture);

    dlg.WantedChaseSyn();

    EXPECT_EQ(capture.systemMessageCalls, 0);
    EXPECT_EQ(capture.sendCalls, 1);
}

TEST(CChaseInputDialogTest, MissingFilterCallbackSkipsFilterBranch) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    SetTargetName(dlg, "Target");
    ChaseCapture capture;
    dlg.SetCallbacks(StubGetCurrentTime, StubAddSystemMessage,
                     StubGetHeroObjectId, StubGetHeroObjectName,
                     nullptr, StubIsWantedName, StubSendChaseSyn, &capture);

    dlg.WantedChaseSyn();

    EXPECT_EQ(capture.sendCalls, 1);
}

TEST(CChaseInputDialogTest, MissingWantedCallbackSkipsTrackingGate) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    SetTargetName(dlg, "Target");
    dlg.SetItemIdx(cChaseInputDialog::kTrackingJinItemIdx);
    ChaseCapture capture;
    dlg.SetCallbacks(StubGetCurrentTime, StubAddSystemMessage,
                     StubGetHeroObjectId, StubGetHeroObjectName,
                     StubFilterWord, nullptr, StubSendChaseSyn, &capture);

    dlg.WantedChaseSyn();

    EXPECT_EQ(capture.sendCalls, 1);
}

TEST(CChaseInputDialogTest, CurrentTimeWrapAroundStillRateLimits) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    ChaseCapture capture;
    capture.currentTime = 0xFFFFFFF0u;
    InstallAllCallbacks(dlg, capture);
    SetTargetName(dlg, "First");
    dlg.WantedChaseSyn();
    ASSERT_EQ(capture.sendCalls, 1);

    dlg.SetActive(true);
    SetTargetName(dlg, "Second");
    capture.currentTime = 100u;
    dlg.WantedChaseSyn();

    EXPECT_EQ(capture.sendCalls, 1);
    ASSERT_EQ(capture.systemMessageCalls, 1);
    EXPECT_EQ(capture.systemMessageIds[0], cChaseInputDialog::kChatMsgRateLimited);
}

TEST(CChaseInputDialogTest, WantedNameIsLimitedToLegacySixteenBytes) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    SetTargetName(dlg, "ABCDEFGHIJKLMNOPQRST");
    ChaseCapture capture;
    InstallAllCallbacks(dlg, capture);

    dlg.WantedChaseSyn();

    EXPECT_EQ(capture.sentWantedName, "ABCDEFGHIJKLMNOP");
}

TEST(CChaseInputDialogTest, CaseSensitiveHeroNameComparisonMatchesLegacy) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    SetTargetName(dlg, "hero");
    ChaseCapture capture;
    InstallAllCallbacks(dlg, capture);

    dlg.WantedChaseSyn();

    EXPECT_EQ(capture.systemMessageCalls, 0);
    EXPECT_EQ(capture.sendCalls, 1);
}

// ===========================================================================
// WantedChaseSyn default-callback safety
// ===========================================================================

TEST(CChaseInputDialogTest, WantedChaseSynWithDefaultCallbacksIsSafeNoOp) {
    // No host runtime is wired yet, so the method returns safely.
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    dlg.WantedChaseSyn();
    // No observable state change (the body is a
    // no-op).
    SUCCEED();
}

TEST(CChaseInputDialogTest, WantedChaseSynWithoutLinkIsSafe) {
    cChaseInputDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    dlg.WantedChaseSyn();
    SUCCEED();
}

}  // namespace mxh::ui::test
